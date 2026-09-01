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

constexpr int kGridSlices = 20;
constexpr float kGridSpacing = 1.0f;

// Visual-only cooked-texture probe. Not in GreyboxWorld and not a physics body.
constexpr core::Vec3 kTestTextureQuadCenter{0.0f, 1.5f, 2.5f};
constexpr float kTestTextureQuadWidth = 2.0f;
constexpr float kTestTextureQuadHeight = 2.0f;
constexpr const char* kTestTextureRuntimeRelativePath = "assets/textures/test_checker.png";

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
}

struct Renderer::GpuTexture
{
    Texture2D texture{};
};

Renderer::Renderer() = default;

Renderer::~Renderer() = default;

void Renderer::LoadRuntimeAssets()
{
    UnloadRuntimeAssets();

    const std::filesystem::path runtimePath =
        platform::RuntimeAssetPath(platform::kTestCheckerLogicalId);
    const std::string runtimePathString = runtimePath.lexically_normal().make_preferred().string();

    if (runtimePath.empty() || !runtimePath.is_absolute())
    {
        TraceLog(
            LOG_ERROR,
            "Refusing to load cooked texture '%s': runtime path is not absolute. "
            "Asset lookup uses the executable directory, never the process CWD.",
            TestTextureLogicalId());
        testTexture.reset();
        testTextureLoaded = false;
        testTextureFallbackActive = true;
        return;
    }

    if (!std::filesystem::exists(runtimePath) || !std::filesystem::is_regular_file(runtimePath))
    {
        TraceLog(
            LOG_ERROR,
            "Failed to load cooked texture '%s' from runtime path '%s' (file not found). "
            "Using a magenta fallback quad. Cook assets with: python tools/cook_assets.py",
            TestTextureLogicalId(),
            runtimePathString.c_str());
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

void Renderer::UnloadRuntimeAssets()
{
    if (testTexture != nullptr && testTextureLoaded)
    {
        UnloadTexture(testTexture->texture);
    }
    testTexture.reset();
    testTextureLoaded = false;
    testTextureFallbackActive = false;
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

    EndMode3D();
}

void Renderer::EndFrame()
{
    EndDrawing();
}
}