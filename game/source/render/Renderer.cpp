#include "render/Renderer.h"

#include "core/Vec3.h"
#include "gameplay/PlatformerCamera.h"
#include "gameplay/Player.h"
#include "platform/RuntimePaths.h"
#include "world/GreyboxWorld.h"
#include "world/Slope.h"

#include "raylib.h"
#include "rlgl.h"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace render
{
namespace
{
constexpr Color kBackgroundColor{32, 36, 48, 255};
constexpr Color kGroundColor{78, 84, 96, 255};
constexpr Color kPlatformColor{110, 118, 132, 255};
constexpr Color kPlatformAccentColor{96, 104, 118, 255};
constexpr Color kPlayerColor{216, 96, 72, 255};
constexpr Color kMovingPlatformColor{168, 132, 72, 255};
constexpr Color kWalkableSlopeColor{132, 148, 92, 255};
constexpr Color kSteepSlopeColor{148, 92, 84, 255};
constexpr Color kPhysicsTestBoxColor{64, 176, 196, 255};
constexpr Color kWireColor{24, 26, 32, 255};
constexpr Color kMissingTextureFallbackColor{220, 48, 160, 255};
constexpr Color kTestModelTint{255, 255, 255, 255};
constexpr Color kMissingModelFallbackColor{235, 115, 46, 255};
constexpr Color kMissingAuthoredModelFallbackColor{72, 148, 108, 255};

constexpr int kGridSlices = 20;
constexpr float kGridSpacing = 1.0f;

// Visual-only cooked-texture probe. Not in GreyboxWorld and not a physics body.
constexpr core::Vec3 kTestTextureQuadCenter{0.0f, 1.5f, 2.5f};
constexpr float kTestTextureQuadWidth = 2.0f;
constexpr float kTestTextureQuadHeight = 2.0f;
constexpr const char* kTestTextureRuntimeRelativePath = "assets/textures/test_checker.png";

// Visual-only cooked GLB probes. Not in GreyboxWorld and not physics bodies.
constexpr core::Vec3 kTestModelPosition{2.5f, 1.0f, 2.5f};
constexpr float kTestModelScale = 1.0f;
constexpr core::Vec3 kTestModelFallbackSize{1.1f, 1.2f, 1.1f};
constexpr core::Vec3 kAuthoredModelPosition{-2.5f, 1.0f, 2.5f};
constexpr float kAuthoredModelScale = 1.0f;

Vector3 ToRaylib(core::Vec3 value)
{
    return Vector3{value.x, value.y, value.z};
}

void DrawGreyboxBox(core::Vec3 center, core::Vec3 size, Color fill)
{
    const Vector3 position = ToRaylib(center);
    DrawCube(position, size.x, size.y, size.z, fill);
    DrawCubeWires(position, size.x, size.y, size.z, kWireColor);
}

void DrawOrientedGreyboxBox(const world::SlopeSpec& slope, Color fill)
{
    rlPushMatrix();
    rlTranslatef(slope.center.x, slope.center.y, slope.center.z);
    rlRotatef(slope.rotationZDegrees, 0.0f, 0.0f, 1.0f);
    DrawCube(Vector3{0.0f, 0.0f, 0.0f}, slope.size.x, slope.size.y, slope.size.z, fill);
    DrawCubeWires(Vector3{0.0f, 0.0f, 0.0f}, slope.size.x, slope.size.y, slope.size.z, kWireColor);
    rlPopMatrix();
}

Camera3D MakeCamera(const gameplay::PlatformerCamera& camera)
{
    const core::Vec3 target = camera.Target();
    Camera3D view{};
    view.position = ToRaylib(target + camera.offset);
    view.target = ToRaylib(target);
    view.up = Vector3{0.0f, 1.0f, 0.0f};
    view.fovy = camera.fieldOfViewY;
    view.projection = CAMERA_PERSPECTIVE;
    return view;
}

void DrawTestTextureQuad(const Texture2D& texture)
{
    const float halfWidth = kTestTextureQuadWidth * 0.5f;
    const float halfHeight = kTestTextureQuadHeight * 0.5f;
    const float x = kTestTextureQuadCenter.x;
    const float y = kTestTextureQuadCenter.y;
    const float z = kTestTextureQuadCenter.z;

    rlSetTexture(texture.id);
    rlBegin(RL_QUADS);
    rlColor4ub(255, 255, 255, 255);
    rlNormal3f(0.0f, 0.0f, 1.0f);
    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(x - halfWidth, y - halfHeight, z);
    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(x + halfWidth, y - halfHeight, z);
    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(x + halfWidth, y + halfHeight, z);
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(x - halfWidth, y + halfHeight, z);
    rlEnd();
    rlSetTexture(0);
}

void DrawMissingTextureFallback()
{
    DrawCube(
        ToRaylib(kTestTextureQuadCenter),
        kTestTextureQuadWidth,
        kTestTextureQuadHeight,
        0.05f,
        kMissingTextureFallbackColor);
}

void DrawMissingModelFallback(core::Vec3 center, Color fill)
{
    DrawCube(
        ToRaylib(center),
        kTestModelFallbackSize.x,
        kTestModelFallbackSize.y,
        kTestModelFallbackSize.z,
        fill);
    DrawCubeWires(
        ToRaylib(center),
        kTestModelFallbackSize.x,
        kTestModelFallbackSize.y,
        kTestModelFallbackSize.z,
        kWireColor);
}

bool TryResolveRuntimeAsset(
    std::string_view logicalId,
    const char* kind,
    std::filesystem::path& runtimePath,
    std::string& runtimePathString)
{
    runtimePath = platform::RuntimeAssetPath(logicalId);
    runtimePathString = runtimePath.lexically_normal().make_preferred().string();
    if (runtimePath.empty() || !runtimePath.is_absolute())
    {
        TraceLog(
            LOG_ERROR,
            "Refusing to load cooked %s '%s': runtime path is not absolute. "
            "Asset lookup uses the executable directory, never the process CWD.",
            kind,
            std::string(logicalId).c_str());
        return false;
    }

    if (!std::filesystem::exists(runtimePath) || !std::filesystem::is_regular_file(runtimePath))
    {
        TraceLog(
            LOG_ERROR,
            "Failed to load cooked %s '%s' from runtime path '%s' (file not found). "
            "Using a visual fallback. Cook assets with: python tools/cook_assets.py",
            kind,
            std::string(logicalId).c_str(),
            runtimePathString.c_str());
        return false;
    }

    return true;
}
}

struct Renderer::GpuTexture
{
    Texture2D texture{};
};

struct Renderer::GpuModel
{
    Model model{};
};

Renderer::Renderer() = default;

Renderer::~Renderer() = default;

void Renderer::LoadRuntimeAssets()
{
    UnloadRuntimeAssets();
    LoadTestCheckerTexture();
    LoadTestStaticModel();
    LoadTestAuthoredModel();
}

void Renderer::LoadTestCheckerTexture()
{
    std::filesystem::path runtimePath;
    std::string runtimePathString;
    if (!TryResolveRuntimeAsset(
            platform::kTestCheckerLogicalId,
            "texture",
            runtimePath,
            runtimePathString))
    {
        testTexture.reset();
        testTextureLoaded = false;
        testTextureFallbackActive = true;
        return;
    }

    testTexture = std::make_unique<GpuTexture>();
    testTexture->texture = LoadTexture(runtimePathString.c_str());
    if (!IsTextureValid(testTexture->texture))
    {
        TraceLog(
            LOG_ERROR,
            "Failed to load cooked texture '%s' from runtime path '%s'. "
            "Using a magenta fallback quad. Cook assets with: python tools/cook_assets.py",
            TestTextureLogicalId(),
            runtimePathString.c_str());
        testTexture.reset();
        testTextureLoaded = false;
        testTextureFallbackActive = true;
        return;
    }

    SetTextureFilter(testTexture->texture, TEXTURE_FILTER_POINT);
    testTextureLoaded = true;
    testTextureFallbackActive = false;
}

void Renderer::LoadTestStaticModel()
{
    std::filesystem::path runtimePath;
    std::string runtimePathString;
    if (!TryResolveRuntimeAsset(
            platform::kTestStaticModelLogicalId,
            "model",
            runtimePath,
            runtimePathString))
    {
        testModel.reset();
        testModelLoaded = false;
        testModelFallbackActive = true;
        return;
    }

    testModel = std::make_unique<GpuModel>();
    testModel->model = LoadModel(runtimePathString.c_str());
    if (!IsModelValid(testModel->model))
    {
        TraceLog(
            LOG_ERROR,
            "Failed to load cooked model '%s' from runtime path '%s'. "
            "Using an orange fallback cube. Cook assets with: python tools/cook_assets.py",
            TestModelLogicalId(),
            runtimePathString.c_str());
        testModel.reset();
        testModelLoaded = false;
        testModelFallbackActive = true;
        return;
    }

    testModelLoaded = true;
    testModelFallbackActive = false;
}

void Renderer::LoadTestAuthoredModel()
{
    std::filesystem::path runtimePath;
    std::string runtimePathString;
    if (!TryResolveRuntimeAsset(
            platform::kTestAuthoredModelLogicalId,
            "model",
            runtimePath,
            runtimePathString))
    {
        authoredModel.reset();
        authoredModelLoaded = false;
        authoredModelFallbackActive = true;
        return;
    }

    authoredModel = std::make_unique<GpuModel>();
    authoredModel->model = LoadModel(runtimePathString.c_str());
    if (!IsModelValid(authoredModel->model))
    {
        TraceLog(
            LOG_ERROR,
            "Failed to load cooked model '%s' from runtime path '%s'. "
            "Using a visual fallback cube. Cook assets with: python tools/cook_assets.py",
            AuthoredModelLogicalId(),
            runtimePathString.c_str());
        authoredModel.reset();
        authoredModelLoaded = false;
        authoredModelFallbackActive = true;
        return;
    }

    authoredModelLoaded = true;
    authoredModelFallbackActive = false;
}

void Renderer::UnloadRuntimeAssets()
{
    if (testTexture != nullptr && testTextureLoaded)
    {
        UnloadTexture(testTexture->texture);
    }
    testTexture.reset();
    testTextureLoaded = false;
    testTextureFallbackActive = false;

    if (testModel != nullptr && testModelLoaded)
    {
        UnloadModel(testModel->model);
    }
    testModel.reset();
    testModelLoaded = false;
    testModelFallbackActive = false;

    if (authoredModel != nullptr && authoredModelLoaded)
    {
        UnloadModel(authoredModel->model);
    }
    authoredModel.reset();
    authoredModelLoaded = false;
    authoredModelFallbackActive = false;
}

bool Renderer::IsTestTextureLoaded() const
{
    return testTextureLoaded;
}

bool Renderer::IsTestTextureFallbackActive() const
{
    return testTextureFallbackActive;
}

const char* Renderer::TestTextureLogicalId() const
{
    return platform::kTestCheckerLogicalId.data();
}

const char* Renderer::TestTextureRuntimeRelativePath() const
{
    return kTestTextureRuntimeRelativePath;
}

bool Renderer::IsTestModelLoaded() const
{
    return testModelLoaded;
}

bool Renderer::IsTestModelFallbackActive() const
{
    return testModelFallbackActive;
}

const char* Renderer::TestModelLogicalId() const
{
    return platform::kTestStaticModelLogicalId.data();
}

bool Renderer::IsAuthoredModelLoaded() const
{
    return authoredModelLoaded;
}

bool Renderer::IsAuthoredModelFallbackActive() const
{
    return authoredModelFallbackActive;
}

const char* Renderer::AuthoredModelLogicalId() const
{
    return platform::kTestAuthoredModelLogicalId.data();
}

void Renderer::BeginFrame()
{
    BeginDrawing();
    ClearBackground(kBackgroundColor);
}

void Renderer::DrawWorld(
    const gameplay::Player& player,
    const gameplay::PlatformerCamera& camera,
    core::Vec3 physicsTestBoxPosition,
    core::Vec3 physicsTestBoxSize,
    core::Vec3 movingPlatformPosition,
    core::Vec3 movingPlatformSize)
{
    const Camera3D view = MakeCamera(camera);
    BeginMode3D(view);

    DrawGrid(kGridSlices, kGridSpacing);
    DrawGreyboxBox(world::kGround.center, world::kGround.size, kGroundColor);

    const Color platformColors[] = {kPlatformColor, kPlatformAccentColor};
    int platformIndex = 0;
    for (const world::Box& platform : world::kElevatedPlatforms)
    {
        DrawGreyboxBox(
            platform.center,
            platform.size,
            platformColors[platformIndex % 2]);
        ++platformIndex;
    }

    DrawGreyboxBox(movingPlatformPosition, movingPlatformSize, kMovingPlatformColor);
    DrawOrientedGreyboxBox(world::kWalkableSlope, kWalkableSlopeColor);
    DrawOrientedGreyboxBox(world::kSteepSlope, kSteepSlopeColor);
    DrawGreyboxBox(player.Position(), player.Size(), kPlayerColor);
    DrawGreyboxBox(physicsTestBoxPosition, physicsTestBoxSize, kPhysicsTestBoxColor);

    if (testTextureLoaded && testTexture != nullptr)
    {
        DrawTestTextureQuad(testTexture->texture);
    }
    else
    {
        DrawMissingTextureFallback();
    }

    if (testModelLoaded && testModel != nullptr)
    {
        DrawModel(testModel->model, ToRaylib(kTestModelPosition), kTestModelScale, kTestModelTint);
    }
    else
    {
        DrawMissingModelFallback(kTestModelPosition, kMissingModelFallbackColor);
    }

    if (authoredModelLoaded && authoredModel != nullptr)
    {
        DrawModel(
            authoredModel->model,
            ToRaylib(kAuthoredModelPosition),
            kAuthoredModelScale,
            kTestModelTint);
    }
    else
    {
        DrawMissingModelFallback(kAuthoredModelPosition, kMissingAuthoredModelFallbackColor);
    }

    EndMode3D();
}

void Renderer::EndFrame()
{
    EndDrawing();
}
}