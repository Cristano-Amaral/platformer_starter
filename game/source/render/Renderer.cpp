#include "render/Renderer.h"

#include "core/Vec3.h"
#include "gameplay/PlatformerCamera.h"
#include "gameplay/Player.h"
#include "platform/RuntimePaths.h"
#include "world/GreyboxWorld.h"
#include "world/HazardWorld.h"
#include "world/LevelGoal.h"
#include "world/RespawnWorld.h"
#include "world/Slope.h"

#include "raylib.h"
#include "rlgl.h"

#include <array>
#include <cstddef>
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
constexpr Color kMissingTexturedModelFallbackColor{92, 72, 196, 255};
constexpr Color kCheckpointFuturePost{86, 94, 112, 255};
constexpr Color kCheckpointFutureBeacon{140, 148, 168, 255};
constexpr Color kCheckpointCurrentPost{48, 140, 88, 255};
constexpr Color kCheckpointCurrentBeacon{88, 220, 124, 255};
constexpr Color kCheckpointPreviousPost{36, 88, 56, 255};
constexpr Color kCheckpointPreviousBeacon{64, 148, 88, 255};
constexpr Color kGoalIncompletePost{156, 116, 52, 255};
constexpr Color kGoalIncompleteBar{188, 148, 64, 255};
constexpr Color kGoalCompletedPost{212, 168, 48, 255};
constexpr Color kGoalCompletedBar{244, 212, 84, 255};
constexpr Color kHazardBarColor{196, 48, 36, 255};
constexpr Color kHazardToothColor{232, 96, 40, 255};
constexpr Color kLevelCompleteText{244, 212, 84, 255};
constexpr int kLevelCompleteFontSize = 42;
constexpr int kRestartHintFontSize = 22;
constexpr int kRestartHintGap = 16;

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
constexpr core::Vec3 kTexturedModelPosition{4.0f, 1.0f, 2.5f};
constexpr float kTexturedModelScale = 1.0f;

bool ModelHasAlbedoTexture(const Model& model)
{
    if (model.materialCount <= 0 || model.materials == nullptr)
    {
        return false;
    }

    for (int index = 0; index < model.materialCount; ++index)
    {
        const MaterialMap* maps = model.materials[index].maps;
        if (maps == nullptr)
        {
            continue;
        }
        if (IsTextureValid(maps[MATERIAL_MAP_ALBEDO].texture))
        {
            return true;
        }
    }

    return false;
}

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

// Visual-only: lethal AABB is the bar. Three teeth sit on the top face, inside
// the XZ footprint, so the drawn volume is slightly taller than the lethal box.
void DrawHazard(const world::HazardSpec& spec)
{
    DrawGreyboxBox(spec.center, spec.size, kHazardBarColor);

    constexpr int kToothCount = 3;
    constexpr float kToothHeight = 0.35f;
    const float toothSizeX = spec.size.x * 0.22f;
    const float toothSizeZ = spec.size.z * 0.40f;
    const float toothCenterY = spec.center.y + spec.size.y * 0.5f + kToothHeight * 0.5f;
    const float xSpan = spec.size.x * 0.32f;
    const core::Vec3 toothSize{toothSizeX, kToothHeight, toothSizeZ};
    for (int toothIndex = 0; toothIndex < kToothCount; ++toothIndex)
    {
        const float xOffset = -xSpan + static_cast<float>(toothIndex) * xSpan;
        DrawGreyboxBox(
            {spec.center.x + xOffset, toothCenterY, spec.center.z},
            toothSize,
            kHazardToothColor);
    }
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

void DrawCheckpointMarker(
    const world::CheckpointSpec& spec,
    world::CheckpointVisualState visualState)
{
    constexpr float postWidth = 0.18f;
    constexpr float postHeight = 1.6f;
    constexpr float beaconSize = 0.36f;
    constexpr float zOffset = -0.95f;

    Color postColor = kCheckpointFuturePost;
    Color beaconColor = kCheckpointFutureBeacon;
    switch (visualState)
    {
    case world::CheckpointVisualState::Current:
        postColor = kCheckpointCurrentPost;
        beaconColor = kCheckpointCurrentBeacon;
        break;
    case world::CheckpointVisualState::PreviouslyActivated:
        postColor = kCheckpointPreviousPost;
        beaconColor = kCheckpointPreviousBeacon;
        break;
    default:
        break;
    }

    const float supportTopY = spec.respawnPosition.y - world::kPlayerVisualSize.y * 0.5f;
    const core::Vec3 postCenter{
        spec.center.x,
        supportTopY + postHeight * 0.5f,
        spec.center.z + zOffset};
    const core::Vec3 postSize{postWidth, postHeight, postWidth};
    const core::Vec3 beaconCenter{
        postCenter.x,
        postCenter.y + postHeight * 0.5f + beaconSize * 0.5f,
        postCenter.z};
    const core::Vec3 beaconSizeVec{beaconSize, beaconSize, beaconSize};

    DrawGreyboxBox(postCenter, postSize, postColor);
    DrawGreyboxBox(beaconCenter, beaconSizeVec, beaconColor);
}

void DrawLevelGoalMarker(bool levelCompleted)
{
    constexpr float postWidth = 0.16f;
    constexpr float postHeight = 1.6f;
    constexpr float barHeight = 0.16f;
    constexpr float barDepth = 0.16f;
    constexpr float postSpread = 0.70f;
    constexpr float zOffset = -0.90f;

    const float platformTopY =
        world::kLevelGoal.center.y - world::kPlayerVisualSize.y * 0.5f;
    const float postCenterY = platformTopY + postHeight * 0.5f;
    const float z = world::kLevelGoal.center.z + zOffset;
    const core::Vec3 postSize{postWidth, postHeight, postWidth};
    const core::Vec3 leftPost{
        world::kLevelGoal.center.x - postSpread,
        postCenterY,
        z};
    const core::Vec3 rightPost{
        world::kLevelGoal.center.x + postSpread,
        postCenterY,
        z};
    const core::Vec3 barCenter{
        world::kLevelGoal.center.x,
        platformTopY + postHeight + barHeight * 0.5f,
        z};
    const core::Vec3 barSize{postSpread * 2.0f + postWidth, barHeight, barDepth};

    const Color postColor = levelCompleted ? kGoalCompletedPost : kGoalIncompletePost;
    const Color barColor = levelCompleted ? kGoalCompletedBar : kGoalIncompleteBar;
    DrawGreyboxBox(leftPost, postSize, postColor);
    DrawGreyboxBox(rightPost, postSize, postColor);
    DrawGreyboxBox(barCenter, barSize, barColor);
}

void DrawLevelCompleteMessage()
{
    const char* completeText = "LEVEL COMPLETE";
    const int completeWidth = MeasureText(completeText, kLevelCompleteFontSize);
    const int completeX = (GetScreenWidth() - completeWidth) / 2;
    const int completeY = GetScreenHeight() / 10;
    DrawText(completeText, completeX, completeY, kLevelCompleteFontSize, kLevelCompleteText);

    const char* hintText = "PRESS ENTER TO RESTART";
    const int hintWidth = MeasureText(hintText, kRestartHintFontSize);
    const int hintX = (GetScreenWidth() - hintWidth) / 2;
    const int hintY = completeY + kLevelCompleteFontSize + kRestartHintGap;
    DrawText(hintText, hintX, hintY, kRestartHintFontSize, kLevelCompleteText);
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
    LoadTestTexturedModel();
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

void Renderer::LoadTestTexturedModel()
{
    texturedModelMaterialCount = 0;
    texturedModelHasAlbedoTexture = false;

    std::filesystem::path runtimePath;
    std::string runtimePathString;
    if (!TryResolveRuntimeAsset(
            platform::kTestTexturedModelLogicalId,
            "model",
            runtimePath,
            runtimePathString))
    {
        texturedModel.reset();
        texturedModelLoaded = false;
        texturedModelFallbackActive = true;
        return;
    }

    texturedModel = std::make_unique<GpuModel>();
    texturedModel->model = LoadModel(runtimePathString.c_str());
    if (!IsModelValid(texturedModel->model))
    {
        TraceLog(
            LOG_ERROR,
            "Failed to load cooked model '%s' from runtime path '%s'. "
            "Using a visual fallback cube. Cook assets with: python tools/cook_assets.py",
            TexturedModelLogicalId(),
            runtimePathString.c_str());
        texturedModel.reset();
        texturedModelLoaded = false;
        texturedModelFallbackActive = true;
        return;
    }

    texturedModelLoaded = true;
    texturedModelFallbackActive = false;
    texturedModelMaterialCount = texturedModel->model.materialCount;
    texturedModelHasAlbedoTexture = ModelHasAlbedoTexture(texturedModel->model);
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

    if (texturedModel != nullptr && texturedModelLoaded)
    {
        UnloadModel(texturedModel->model);
    }
    texturedModel.reset();
    texturedModelLoaded = false;
    texturedModelFallbackActive = false;
    texturedModelMaterialCount = 0;
    texturedModelHasAlbedoTexture = false;
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

bool Renderer::IsTexturedModelLoaded() const
{
    return texturedModelLoaded;
}

bool Renderer::IsTexturedModelFallbackActive() const
{
    return texturedModelFallbackActive;
}

const char* Renderer::TexturedModelLogicalId() const
{
    return platform::kTestTexturedModelLogicalId.data();
}

int Renderer::TexturedModelMaterialCount() const
{
    return texturedModelMaterialCount;
}

bool Renderer::TexturedModelHasAlbedoTexture() const
{
    return texturedModelHasAlbedoTexture;
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
        core::Vec3 movingPlatformSize,
        std::array<world::CheckpointVisualState, world::kCheckpointCount> checkpointVisuals,
        bool levelCompleted)
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
    for (const world::HazardSpec& hazard : world::kHazards)
    {
        DrawHazard(hazard);
    }
    DrawGreyboxBox(player.Position(), player.Size(), kPlayerColor);
    DrawGreyboxBox(physicsTestBoxPosition, physicsTestBoxSize, kPhysicsTestBoxColor);
    for (int checkpointIndex = 0; checkpointIndex < world::kCheckpointCount; ++checkpointIndex)
    {
        DrawCheckpointMarker(
            world::kCheckpoints[static_cast<std::size_t>(checkpointIndex)],
            checkpointVisuals[static_cast<std::size_t>(checkpointIndex)]);
    }
    DrawLevelGoalMarker(levelCompleted);

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

    if (texturedModelLoaded && texturedModel != nullptr)
    {
        DrawModel(
            texturedModel->model,
            ToRaylib(kTexturedModelPosition),
            kTexturedModelScale,
            kTestModelTint);
    }
    else
    {
        DrawMissingModelFallback(kTexturedModelPosition, kMissingTexturedModelFallbackColor);
    }

    EndMode3D();

    if (levelCompleted)
    {
        DrawLevelCompleteMessage();
    }
}

void Renderer::EndFrame()
{
    EndDrawing();
}
}