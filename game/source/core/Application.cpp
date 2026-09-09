#include "core/Application.h"

#include "core/RunTimeFormat.h"
#include "gameplay/PlatformerCamera.h"
#include "gameplay/CollectibleRunState.h"
#include "gameplay/RunTimerState.h"
#include "gameplay/SessionBestTimeState.h"
#include "input/Input.h"
#include "persistence/BestTimeSave.h"
#include "platform/RuntimePaths.h"
#include "platform/Time.h"
#include "render/CameraView.h"
#include "world/CollectibleWorld.h"
#include "world/HazardWorld.h"
#include "world/LevelDefinition.h"
#include "world/LevelFile.h"
#include "world/LevelGoal.h"
#include "world/RespawnWorld.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

static_assert(!gameplay::SessionBestTimeState{}.hasBestTime);
static_assert(core::RunTimePartsEqual(core::RunTimePartsFromSeconds(0.0), 0, 0, 0));

#if defined(PLATFORMER_ENABLE_DEBUG_UI)
#include "assets/StaticGlbImport.h"
#include "assets/StaticModelDelete.h"
#include "editor/AuthoringPaths.h"
#include "editor/ContentBrowser.h"
#include "editor/ContentBrowserView.h"
#include "editor/CookStageReloadWorkflow.h"
#include "editor/StaticModelThumbnailCache.h"
#include "editor/EditorCamera.h"
#include "editor/EditorInput.h"
#include "editor/EditorNudge.h"
#include "editor/EditorOrientation.h"
#include "editor/EditorPicking.h"
#include "editor/EditorPlacement.h"
#include "editor/StaticPropPlacement.h"
#include "editor/StaticPropTransform.h"
#include "editor/EditorToolCommands.h"
#include "editor/EditorWorkspace.h"
#include "editor/LevelEditor.h"
#include "editor/AuthoredLifecycleCommands.h"
#include "editor/RuntimeLevelReload.h"
#include "platform/OpenFileDialog.h"
#include "render/StaticModelThumbnail.h"
#include "render/StaticModelPreview.h"
#include "render/StaticModelScene.h"
#include "ui/debug/DebugMetrics.h"
#include "world/LevelWriter.h"

#include <string>
#endif

namespace core
{
namespace
{
std::vector<world::CheckpointVisualState> MakeCheckpointVisuals(
    int activeCheckpointIndex,
    std::size_t checkpointCount)
{
    std::vector<world::CheckpointVisualState> visuals(checkpointCount);
    const int count = static_cast<int>(checkpointCount);
    for (std::size_t index = 0; index < checkpointCount; ++index)
    {
        visuals[index] = world::CheckpointVisualStateForIndex(
            static_cast<int>(index), activeCheckpointIndex, count);
    }
    return visuals;
}

render::CameraView MakeGameplayCameraView(const gameplay::PlatformerCamera& camera)
{
    const core::Vec3 target = camera.Target();
    render::CameraView view{};
    view.position = target + camera.offset;
    view.target = target;
    view.up = {0.0f, 1.0f, 0.0f};
    view.fieldOfViewY = camera.fieldOfViewY;
    return view;
}

std::vector<render::DynamicBoxDrawState> MakeDynamicBoxDrawStates(
    const std::vector<physics::DynamicBoxRuntimeState>& boxes,
    const physics::DynamicBoxGrabState& grab)
{
    std::vector<render::DynamicBoxDrawState> draw;
    draw.reserve(boxes.size());
    for (std::size_t index = 0; index < boxes.size(); ++index)
    {
        const physics::DynamicBoxRuntimeState& box = boxes[index];
        render::DynamicBoxDrawState item{};
        item.center = box.center;
        item.size = box.size;
        item.rotationX = box.rotationX;
        item.rotationY = box.rotationY;
        item.rotationZ = box.rotationZ;
        item.rotationW = box.rotationW;
        if (grab.carrying && grab.carriedIndex == static_cast<int>(index))
        {
            item.feedback = render::DynamicBoxDrawFeedback::Carried;
        }
        else if (grab.hasTarget && grab.targetIndex == static_cast<int>(index))
        {
            item.feedback = render::DynamicBoxDrawFeedback::Targeted;
        }
        draw.push_back(item);
    }
    return draw;
}

std::vector<render::PressurePlateDrawState> MakePressurePlateDrawStates(
    const std::vector<physics::PressurePlateRuntimeState>& plates)
{
    std::vector<render::PressurePlateDrawState> draw;
    draw.reserve(plates.size());
    for (const physics::PressurePlateRuntimeState& plate : plates)
    {
        render::PressurePlateDrawState item{};
        item.center = plate.center;
        item.size = plate.size;
        item.active = plate.active;
        draw.push_back(item);
    }
    return draw;
}

std::vector<render::DoorDrawState> MakeDoorDrawStates(
    const std::vector<physics::DoorRuntimeState>& doors)
{
    std::vector<render::DoorDrawState> draw;
    draw.reserve(doors.size());
    for (const physics::DoorRuntimeState& door : doors)
    {
        render::DoorDrawState item{};
        item.center = door.center;
        item.size = door.size;
        draw.push_back(item);
    }
    return draw;
}

#if defined(PLATFORMER_ENABLE_DEBUG_UI)
editor::EditorPickingWorldState MakeRuntimePickingWorldState(
    const physics::MovingPlatformState& movingPlatform,
    const std::vector<physics::DynamicBoxRuntimeState>& boxes,
    const std::vector<physics::DoorRuntimeState>& doors)
{
    editor::EditorPickingWorldState state{};
    state.movingPlatformCenter = movingPlatform.position;
    state.movingPlatformSize = movingPlatform.size;
    state.dynamicBoxCenters.reserve(boxes.size());
    state.dynamicBoxSizes.reserve(boxes.size());
    for (const physics::DynamicBoxRuntimeState& box : boxes)
    {
        state.dynamicBoxCenters.push_back(box.center);
        state.dynamicBoxSizes.push_back(box.size);
    }
    state.doorCenters.reserve(doors.size());
    state.doorSizes.reserve(doors.size());
    for (const physics::DoorRuntimeState& door : doors)
    {
        state.doorCenters.push_back(door.valid ? door.center : door.closedCenter);
        state.doorSizes.push_back(door.size);
    }
    return state;
}
#endif

bool RunTimeFormatScaffoldingOk()
{
    char buffer[32]{};
    const struct
    {
        long long totalMilliseconds;
        const char* expected;
    } cases[] = {
        {0, "00:00.000"},
        {1, "00:00.001"},
        {5200, "00:05.200"},
        {59999, "00:59.999"},
        {60000, "01:00.000"},
        {65432, "01:05.432"},
        {754567, "12:34.567"},
    };

    for (const auto& testCase : cases)
    {
        core::FormatRunTimeParts(
            buffer,
            sizeof(buffer),
            core::RunTimePartsFromTotalMilliseconds(testCase.totalMilliseconds));
        if (std::strcmp(buffer, testCase.expected) != 0)
        {
            return false;
        }
    }

    core::FormatRunTime(buffer, sizeof(buffer), 0.0);
    if (std::strcmp(buffer, "00:00.000") != 0)
    {
        return false;
    }

    core::FormatRunTime(buffer, sizeof(buffer), 60.0);
    if (std::strcmp(buffer, "01:00.000") != 0)
    {
        return false;
    }

    core::FormatSessionBestTime(buffer, sizeof(buffer), false, 0.0);
    if (std::strcmp(buffer, core::kNoSessionBestPlaceholder) != 0)
    {
        return false;
    }

    core::FormatSessionBestTime(buffer, sizeof(buffer), true, 60.0);
    return std::strcmp(buffer, "01:00.000") == 0;
}

bool BestTimeSaveFormatScaffoldingOk()
{
    using persistence::LoadBestTimeStatus;
    using persistence::ParseBestTimeV1;
    using persistence::SerializeBestTimeV1;

    const double roundTripValues[] = {1.0, 2.5, 40.5, 40.500123456789, 0.001};
    for (const double original : roundTripValues)
    {
        const persistence::LoadBestTimeResult parsed = ParseBestTimeV1(SerializeBestTimeV1(original));
        if (parsed.status != LoadBestTimeStatus::Loaded || parsed.bestSeconds != original)
        {
            return false;
        }
    }

    const struct
    {
        const char* text;
        LoadBestTimeStatus status;
    } invalidCases[] = {
        {"", LoadBestTimeStatus::Invalid},
        {"NOT_A_SAVE 1\nbest_seconds 1.0\n", LoadBestTimeStatus::Invalid},
        {"PLATFORMER_SAVE\nbest_seconds 1.0\n", LoadBestTimeStatus::Invalid},
        {"PLATFORMER_SAVE 2\nbest_seconds 1.0\n", LoadBestTimeStatus::UnsupportedVersion},
        {"PLATFORMER_SAVE 1\n", LoadBestTimeStatus::Invalid},
        {"PLATFORMER_SAVE 1\nrecord_seconds 1.0\n", LoadBestTimeStatus::Invalid},
        {"PLATFORMER_SAVE 1\nbest_seconds 0\n", LoadBestTimeStatus::Invalid},
        {"PLATFORMER_SAVE 1\nbest_seconds -1\n", LoadBestTimeStatus::Invalid},
        {"PLATFORMER_SAVE 1\nbest_seconds nan\n", LoadBestTimeStatus::Invalid},
        {"PLATFORMER_SAVE 1\nbest_seconds inf\n", LoadBestTimeStatus::Invalid},
        {"PLATFORMER_SAVE 1\nbest_seconds 1.0 extra\n", LoadBestTimeStatus::Invalid},
        {"PLATFORMER_SAVE 1\nbest_seconds abc\n", LoadBestTimeStatus::Invalid},
        {"PLATFORMER_SAVE 1\nbest_seconds 1.0\nextra\n", LoadBestTimeStatus::Invalid},
    };

    for (const auto& testCase : invalidCases)
    {
        if (ParseBestTimeV1(testCase.text).status != testCase.status)
        {
            return false;
        }
    }

    const persistence::LoadBestTimeResult loaded = ParseBestTimeV1("PLATFORMER_SAVE 1\nbest_seconds 1.5\n");
    return loaded.status == LoadBestTimeStatus::Loaded && loaded.bestSeconds == 1.5;
}

void ReportRequiredLevelFailure(
    const std::filesystem::path& path,
    const world::ParseLevelFileResult& result,
    const char* extra)
{
    std::fprintf(stderr, "Required Level 01 failed to load.\n");
    if (path.empty())
    {
        std::fprintf(stderr, "  path: (unavailable)\n");
    }
    else
    {
        std::fprintf(stderr, "  path: %s\n", path.string().c_str());
    }
    std::fprintf(stderr, "  status: %s\n", world::LoadLevelFileStatusName(result.status));
    if (result.formatVersion != 0)
    {
        std::fprintf(stderr, "  format version: %d\n", result.formatVersion);
    }
    if (result.errorLine > 0)
    {
        std::fprintf(stderr, "  line: %d\n", result.errorLine);
    }
    if (!result.error.empty())
    {
        std::fprintf(stderr, "  error: %s\n", result.error.c_str());
    }
    if (!result.level.id.empty())
    {
        std::fprintf(stderr, "  loaded id: %s\n", result.level.id.c_str());
    }
    if (extra != nullptr)
    {
        std::fprintf(stderr, "  %s\n", extra);
    }
}

void CopyBounded(char* destination, std::size_t destinationSize, std::string_view source)
{
    if (destination == nullptr || destinationSize == 0)
    {
        return;
    }
    const std::size_t length =
        source.size() < destinationSize - 1 ? source.size() : destinationSize - 1;
    if (length > 0 && source.data() != nullptr)
    {
        std::memcpy(destination, source.data(), length);
    }
    destination[length] = '\0';
}

#if defined(PLATFORMER_ENABLE_DEBUG_UI)
bool EditorQuickToolbarVisible(const editor::LevelEditorState& state)
{
    return state.active && state.workspace.showQuickToolbar
        && editor::IsLevelAuthoringAvailable();
}

editor::EditorContentViewport MakeLiveEditorContentViewport(
    float windowWidth,
    float windowHeight,
    const editor::LevelEditorState& state)
{
    return editor::MakeEditorContentViewport(
        windowWidth,
        windowHeight,
        editor::LiveEditorChromeHeight(
            state.active,
            state.menuBarHeight,
            state.toolbarHeight,
            EditorQuickToolbarVisible(state)));
}

render::WorldViewRect MakeWorldViewRect(const editor::EditorContentViewport& viewport)
{
    render::WorldViewRect rect{};
    rect.x = static_cast<int>(viewport.x);
    rect.y = static_cast<int>(viewport.y);
    rect.width = static_cast<int>(viewport.width);
    rect.height = static_cast<int>(viewport.height);
    if (rect.width < 1)
    {
        rect.width = 1;
    }
    if (rect.height < 1)
    {
        rect.height = 1;
    }
    return rect;
}
#endif
}

#if defined(GAME_DEVELOPMENT_TOOLS)
void ReportBestTimeLoadDiagnostic(persistence::LoadBestTimeStatus status)
{
    if (status == persistence::LoadBestTimeStatus::Missing
        || status == persistence::LoadBestTimeStatus::Loaded)
    {
        return;
    }

    std::fprintf(
        stderr,
        "Best time save: load status %s.\n",
        persistence::LoadBestTimeStatusName(status));
}

void ReportBestTimeSaveDiagnostic(persistence::SaveBestTimeStatus status)
{
    if (status != persistence::SaveBestTimeStatus::Error)
    {
        return;
    }

    std::fprintf(stderr, "Best time save: save status Error.\n");
}
#endif

#if defined(PLATFORMER_ENABLE_DEBUG_UI)
namespace
{
bool IsFiniteVec3(core::Vec3 value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

const char* SupportClassificationName(physics::PlayerGroundSupport state)
{
    switch (state)
    {
    case physics::PlayerGroundSupport::OnGround:
        return "Walkable";
    case physics::PlayerGroundSupport::OnSteepGround:
        return "Steep";
    default:
        return "Unsupported";
    }
}

const char* GroundSupportName(physics::PlayerGroundSupport state)
{
    switch (state)
    {
    case physics::PlayerGroundSupport::OnGround:
        return "OnGround";
    case physics::PlayerGroundSupport::OnSteepGround:
        return "OnSteepGround";
    case physics::PlayerGroundSupport::NotSupported:
        return "NotSupported";
    case physics::PlayerGroundSupport::InAir:
        return "InAir";
    default:
        return "Unknown";
    }
}

const char* RespawnReasonName(gameplay::RespawnReason reason)
{
    switch (reason)
    {
    case gameplay::RespawnReason::Fall:
        return "Fall";
    case gameplay::RespawnReason::Manual:
        return "Manual";
    case gameplay::RespawnReason::Hazard:
        return "Hazard";
    default:
        return "None";
    }
}

const char* ActiveCheckpointLabel(int activeCheckpointIndex)
{
    static char buffer[16];
    if (activeCheckpointIndex < 0)
    {
        return "None";
    }
    std::snprintf(buffer, sizeof(buffer), "%d", activeCheckpointIndex + 1);
    return buffer;
}

const char* CheckpointVisualStateName(world::CheckpointVisualState state)
{
    switch (state)
    {
    case world::CheckpointVisualState::Current:
        return "Current";
    case world::CheckpointVisualState::PreviouslyActivated:
        return "PreviouslyActivated";
    default:
        return "Future";
    }
}

const char* HazardIndexLabel(int hazardIndex)
{
    static char buffer[16];
    if (hazardIndex < 0)
    {
        return "None";
    }
    std::snprintf(buffer, sizeof(buffer), "%d", hazardIndex + 1);
    return buffer;
}

const char* CollectibleIndexLabel(int collectibleIndex)
{
    static char buffer[16];
    if (collectibleIndex < 0)
    {
        return "None";
    }
    std::snprintf(buffer, sizeof(buffer), "%d", collectibleIndex + 1);
    return buffer;
}
}

ui::DebugMetricsSnapshot MakeDebugMetricsSnapshot(
    const gameplay::Player& player,
    const gameplay::PlatformerCamera& camera,
    const physics::PhysicsWorld& physicsWorld,
    const input::InputState& inputState,
    const render::Renderer& renderer,
    const gameplay::RespawnState& respawnState,
    const gameplay::LevelCompletionState& levelCompletionState,
    const gameplay::CollectibleRunState& collectibleRunState,
    const gameplay::RunTimerState& runTimerState,
    const gameplay::SessionBestTimeState& sessionBestTimeState,
    const world::LevelDefinition& level,
    persistence::LoadBestTimeStatus bestTimeLoadStatus,
    persistence::SaveBestTimeStatus bestTimeSaveStatus,
    const char* bestTimeSavePath,
    const char* runtimeLevelPath,
    world::LoadLevelFileStatus levelLoadStatus,
    int levelFormatVersion,
    bool restartedThisFrame,
    bool hazardContactThisFrame,
    int collectedThisFrameIndex,
    float deltaSeconds)
{
    ui::DebugMetricsSnapshot snapshot;
    snapshot.fps = static_cast<float>(platform::FramesPerSecond());
    snapshot.deltaSeconds = deltaSeconds;

    snapshot.playerPosition = player.Position();
    snapshot.horizontalVelocity = player.HorizontalVelocity();
    snapshot.verticalVelocity = player.VerticalVelocity();
    snapshot.grounded = player.IsGrounded();

    snapshot.moveX = inputState.moveX;
    snapshot.jumpPressed = inputState.jumpPressed;
    snapshot.respawnPressed = inputState.respawnPressed;
    snapshot.restartPressed = inputState.restartPressed;
    snapshot.grabDropPressed = inputState.grabDropPressed;

    snapshot.coyoteElapsed = player.TimeSinceGrounded();
    snapshot.coyoteAvailable = player.IsCoyoteAvailable();
    snapshot.jumpBufferRemaining = player.JumpBufferRemaining();

    snapshot.maxMoveSpeed = gameplay::Player::kMaxMoveSpeed;
    snapshot.acceleration = gameplay::Player::kAcceleration;
    snapshot.deceleration = gameplay::Player::kDeceleration;
    snapshot.jumpSpeed = gameplay::Player::kJumpSpeed;
    snapshot.gravity = gameplay::Player::kGravity;
    snapshot.coyoteDuration = gameplay::Player::kCoyoteTime;
    snapshot.jumpBufferDuration = gameplay::Player::kJumpBufferTime;

    snapshot.desiredTarget = camera.DesiredTarget();
    snapshot.smoothedTarget = camera.Target();
    snapshot.horizontalDeadZone = gameplay::PlatformerCamera::kHorizontalDeadZone;
    snapshot.verticalDeadZone = gameplay::PlatformerCamera::kVerticalDeadZone;
    snapshot.followSharpness = gameplay::PlatformerCamera::kFollowSharpness;

    const std::vector<physics::DynamicBoxRuntimeState> dynamicBoxes = physicsWorld.GetDynamicBoxes();
    snapshot.physicsInitialized = physicsWorld.IsInitialized();
    snapshot.physicsDynamicBoxCount = physicsWorld.DynamicBodyCount();
    if (!dynamicBoxes.empty())
    {
        snapshot.physicsTestBoxPosition = dynamicBoxes[0].center;
        snapshot.physicsTestBoxLinearVelocity = dynamicBoxes[0].linearVelocity;
        snapshot.physicsTestBoxActive = dynamicBoxes[0].active;
        snapshot.dynamicTestBodyValid = dynamicBoxes[0].valid;
    }
    const physics::DynamicBoxGrabState grab = physicsWorld.GetGrabState();
    snapshot.grabHasTarget = grab.hasTarget;
    snapshot.grabTargetIndex = grab.targetIndex;
    snapshot.grabCarrying = grab.carrying;
    snapshot.grabCarriedIndex = grab.carriedIndex;
    snapshot.staticBodyCount = physicsWorld.StaticBodyCount();
    const std::vector<physics::PressurePlateRuntimeState> pressurePlates =
        physicsWorld.GetPressurePlates();
    snapshot.physicsPressurePlateCount = static_cast<int>(pressurePlates.size());
    snapshot.physicsActivePressurePlateCount = 0;
    for (const physics::PressurePlateRuntimeState& plate : pressurePlates)
    {
        if (plate.active)
        {
            ++snapshot.physicsActivePressurePlateCount;
        }
    }
    const std::vector<physics::DoorRuntimeState> doors = physicsWorld.GetDoors();
    snapshot.physicsDoorCount = static_cast<int>(doors.size());
    snapshot.physicsDoorBodyCount = physicsWorld.DoorBodyCount();
    snapshot.physicsDesiredOpenDoorCount = 0;
    for (const physics::DoorRuntimeState& door : doors)
    {
        if (door.desiredOpen)
        {
            ++snapshot.physicsDesiredOpenDoorCount;
        }
    }

    snapshot.characterVirtualInitialized = player.CharacterVirtualInitialized();
    snapshot.playerGroundSupport = GroundSupportName(player.GroundSupport());
    snapshot.playerContactCount = player.PhysicsContactCount();
    snapshot.playerPositionFinite = IsFiniteVec3(player.Position());
    snapshot.playerVelocityFinite =
        std::isfinite(player.HorizontalVelocity()) && std::isfinite(player.VerticalVelocity());
    snapshot.groundVelocity = player.GroundVelocity();
    snapshot.supportingGroundMoving = player.IsSupportingGroundMoving();
    snapshot.groundNormal = player.GroundNormal();
    snapshot.groundSlopeAngleDegrees = player.GroundSlopeAngleDegrees();
    snapshot.currentSupportWalkable = player.IsCurrentSupportWalkable();
    snapshot.supportClassification = SupportClassificationName(player.GroundSupport());

    const physics::PlayerPhysicsState playerPhysics = physicsWorld.GetPlayerPhysicsState();
    snapshot.supportBodyKind = playerPhysics.supportBodyKind;
    snapshot.dynamicContact = playerPhysics.dynamicContact;
    snapshot.playerWorldVelocity = playerPhysics.worldVelocity;
    snapshot.characterInnerBodyActive = playerPhysics.characterInnerBodyActive;

    const physics::MovingPlatformState movingPlatform = physicsWorld.GetMovingPlatform();
    snapshot.movingPlatformValid = movingPlatform.valid;
    snapshot.movingPlatformPosition = movingPlatform.position;
    snapshot.movingPlatformVelocity = movingPlatform.velocity;
    snapshot.movingPlatformDirection = movingPlatform.direction;
    snapshot.movingPlatformPathMinX = movingPlatform.pathMinX;
    snapshot.movingPlatformPathMaxX = movingPlatform.pathMaxX;
    snapshot.movingPlatformSpeed = movingPlatform.speed;

    snapshot.testTextureLoaded = renderer.IsTestTextureLoaded();
    snapshot.testTextureFallbackActive = renderer.IsTestTextureFallbackActive();
    snapshot.testTextureLogicalId = renderer.TestTextureLogicalId();
    snapshot.testTextureRuntimeRelativePath = renderer.TestTextureRuntimeRelativePath();

    snapshot.testModelLoaded = renderer.IsTestModelLoaded();
    snapshot.testModelFallbackActive = renderer.IsTestModelFallbackActive();
    snapshot.testModelLogicalId = renderer.TestModelLogicalId();

    snapshot.authoredModelLoaded = renderer.IsAuthoredModelLoaded();
    snapshot.authoredModelFallbackActive = renderer.IsAuthoredModelFallbackActive();
    snapshot.authoredModelLogicalId = renderer.AuthoredModelLogicalId();

    snapshot.texturedModelLoaded = renderer.IsTexturedModelLoaded();
    snapshot.texturedModelFallbackActive = renderer.IsTexturedModelFallbackActive();
    snapshot.texturedModelLogicalId = renderer.TexturedModelLogicalId();
    snapshot.texturedModelMaterialCount = renderer.TexturedModelMaterialCount();
    snapshot.texturedModelHasAlbedoTexture = renderer.TexturedModelHasAlbedoTexture();

    snapshot.respawnPosition = respawnState.respawnPosition;
    snapshot.killPlaneY = level.killPlaneY;
    snapshot.deathCount = respawnState.deathCount;
    snapshot.lastRespawnReason = RespawnReasonName(respawnState.lastRespawnReason);
    snapshot.activeCheckpointLabel = ActiveCheckpointLabel(respawnState.activeCheckpointIndex);
    snapshot.checkpoint1Inside = false;
    snapshot.checkpoint1VisualState = "Future";
    snapshot.checkpoint2Inside = false;
    snapshot.checkpoint2VisualState = "Future";
    if (!level.checkpoints.empty())
    {
        snapshot.checkpoint1Inside =
            world::PointInsideCheckpoint(level.checkpoints[0], player.Position());
        snapshot.checkpoint1VisualState = CheckpointVisualStateName(
            world::CheckpointVisualStateForIndex(
                0,
                respawnState.activeCheckpointIndex,
                static_cast<int>(level.checkpoints.size())));
    }
    if (level.checkpoints.size() > 1)
    {
        snapshot.checkpoint2Inside =
            world::PointInsideCheckpoint(level.checkpoints[1], player.Position());
        snapshot.checkpoint2VisualState = CheckpointVisualStateName(
            world::CheckpointVisualStateForIndex(
                1,
                respawnState.activeCheckpointIndex,
                static_cast<int>(level.checkpoints.size())));
    }

    snapshot.insideHazardLabel = HazardIndexLabel(
        world::FindHazardIndexContaining(player.Position(), level.hazards));
    snapshot.hazardContactThisFrame = hazardContactThisFrame;

    snapshot.collectedCount = gameplay::CollectedCount(collectibleRunState);
    snapshot.collectedThisFrameLabel = CollectibleIndexLabel(collectedThisFrameIndex);
    snapshot.collectibleCollected = collectibleRunState.collected;
    snapshot.collectibleInside.assign(level.collectibles.size(), 0);
    for (std::size_t index = 0; index < level.collectibles.size(); ++index)
    {
        snapshot.collectibleInside[index] =
            world::PointInsideCollectible(level.collectibles[index], player.Position()) ? 1 : 0;
    }

    snapshot.runTimeSeconds = runTimerState.elapsedSeconds;
    snapshot.runTimerFrozen = runTimerState.frozen;
    snapshot.hasSessionBest = sessionBestTimeState.hasBestTime;
    snapshot.sessionBestSeconds = sessionBestTimeState.bestSeconds;
    snapshot.bestTimeSavePath = bestTimeSavePath != nullptr ? bestTimeSavePath : "";
    snapshot.bestTimeLoadStatus = persistence::LoadBestTimeStatusName(bestTimeLoadStatus);
    snapshot.bestTimeSaveStatus = persistence::SaveBestTimeStatusName(bestTimeSaveStatus);

    snapshot.levelCompleted = levelCompletionState.completed;
    snapshot.goalCenter = level.goal.center;
    snapshot.goalSize = level.goal.size;
    snapshot.playerInsideGoal =
        world::PointInsideGoal(level.goal, player.Position());
    snapshot.restartAvailable = levelCompletionState.completed;
    snapshot.restartedThisFrame = restartedThisFrame;

    CopyBounded(snapshot.levelId, sizeof(snapshot.levelId), level.id);
    CopyBounded(
        snapshot.runtimeLevelPath,
        sizeof(snapshot.runtimeLevelPath),
        runtimeLevelPath != nullptr ? runtimeLevelPath : "");
    snapshot.levelLoadStatus = world::LoadLevelFileStatusName(levelLoadStatus);
    snapshot.levelFormatVersion = levelFormatVersion;
    snapshot.levelInitialSpawn = level.initialSpawnVisualCenter;
    snapshot.levelKillPlaneY = level.killPlaneY;
    snapshot.levelElevatedPlatformCount = static_cast<int>(level.elevatedPlatforms.size());
    snapshot.levelSlopeCount = static_cast<int>(level.slopes.size());
    snapshot.levelCheckpointCount = static_cast<int>(level.checkpoints.size());
    snapshot.levelHazardCount = static_cast<int>(level.hazards.size());
    snapshot.levelCollectibleCount = static_cast<int>(level.collectibles.size());
    snapshot.levelStaticBoxCount = 1 + snapshot.levelElevatedPlatformCount;
    snapshot.levelHasGoal = level.goal.size.x > 0.0f && level.goal.size.y > 0.0f
        && level.goal.size.z > 0.0f;
    snapshot.levelHasMovingPlatform = level.movingPlatform.size.x > 0.0f
        && level.movingPlatform.size.y > 0.0f && level.movingPlatform.size.z > 0.0f
        && level.movingPlatform.speed > 0.0f;
    snapshot.levelDynamicBoxCount = static_cast<int>(level.dynamicBoxes.size());
    snapshot.levelPressurePlateCount = static_cast<int>(level.pressurePlates.size());
    snapshot.levelDoorCount = static_cast<int>(level.doors.size());
    snapshot.levelCameraOffset = level.camera.offset;
    snapshot.levelCameraFieldOfViewY = level.camera.fieldOfViewY;
    return snapshot;
}
#endif

int Application::Run()
{
    Initialize();
    if (!initialized)
    {
        return 1;
    }

    while (!window.ShouldClose() && !fatalError)
    {
        const float deltaSeconds = platform::DeltaSeconds();
        const input::InputState inputState = input::Poll();

#if defined(PLATFORMER_ENABLE_DEBUG_UI)
        // Ignore the toggle while an ImGui field owns the keyboard so typing a
        // value cannot close the editor.
        if (inputState.toggleLevelEditorPressed && !debugUi.WantsKeyboardCapture())
        {
            SetLevelEditorActive(!levelEditorState.active);
        }
        const bool simulationPaused = levelEditorState.active;
#else
        constexpr bool simulationPaused = false;
#endif

        bool hazardContactThisFrame = false;
        int collectedThisFrameIndex = world::kNoCollectibleIndex;
        bool restartedThisFrame = false;

        // Single simulation guard. Everything inside keeps the exact M31 order
        // and content; the editor pauses it wholesale rather than scaling time.
        if (!simulationPaused)
        {
            const bool restartAvailableAtFrameStart = levelCompletionState.completed;
            if (!runTimerState.frozen)
            {
                runTimerState.elapsedSeconds += static_cast<double>(deltaSeconds);
            }
            physicsWorld.UpdateMovingPlatform(deltaSeconds);
            player.Update(inputState, deltaSeconds, physicsWorld);

            hazardContactThisFrame =
                world::FindHazardIndexContaining(player.Position(), levelDefinition.hazards)
                != world::kNoHazardIndex;

            bool respawnedThisFrame = false;
            if (player.Position().y < levelDefinition.killPlaneY)
            {
                PerformRespawn(gameplay::RespawnReason::Fall);
                respawnedThisFrame = true;
            }
            else if (hazardContactThisFrame)
            {
                PerformRespawn(gameplay::RespawnReason::Hazard);
                respawnedThisFrame = true;
            }
            else if (inputState.respawnPressed)
            {
                PerformRespawn(gameplay::RespawnReason::Manual);
                respawnedThisFrame = true;
            }
            else
            {
                const int checkpointCount = static_cast<int>(levelDefinition.checkpoints.size());
                const int expectedIndex =
                    world::NextExpectedCheckpointIndex(respawnState.activeCheckpointIndex);
                if (world::IsValidCheckpointIndex(expectedIndex, checkpointCount)
                    && world::PointInsideCheckpoint(
                           levelDefinition.checkpoints[static_cast<std::size_t>(expectedIndex)],
                           player.Position()))
                {
                    respawnState.activeCheckpointIndex = expectedIndex;
                    respawnState.respawnPosition =
                        levelDefinition.checkpoints[static_cast<std::size_t>(expectedIndex)]
                            .respawnPosition;
                }

                if (!levelCompletionState.completed
                    && world::PointInsideGoal(levelDefinition.goal, player.Position()))
                {
                    levelCompletionState.completed = true;
                    runTimerState.frozen = true;
                    if (gameplay::IsBetterSessionCompletion(
                            sessionBestTimeState,
                            runTimerState.elapsedSeconds))
                    {
                        sessionBestTimeState.hasBestTime = true;
                        sessionBestTimeState.bestSeconds = runTimerState.elapsedSeconds;
                        bestTimeSaveStatus =
                            persistence::SaveBestTime(sessionBestTimeState.bestSeconds);
#if defined(GAME_DEVELOPMENT_TOOLS)
                        ReportBestTimeSaveDiagnostic(bestTimeSaveStatus);
#endif
                    }
                }

                if (!(restartAvailableAtFrameStart && inputState.restartPressed))
                {
                    const int collectibleIndex =
                        gameplay::FindAvailableCollectibleIndexContaining(
                            player.Position(),
                            collectibleRunState,
                            levelDefinition.collectibles);
                    if (collectibleIndex != world::kNoCollectibleIndex)
                    {
                        collectibleRunState.collected[static_cast<std::size_t>(collectibleIndex)] =
                            true;
                        collectedThisFrameIndex = collectibleIndex;
                    }
                }
            }

            if (!respawnedThisFrame && restartAvailableAtFrameStart && inputState.restartPressed)
            {
                RestartRun();
                restartedThisFrame = true;
            }

            physicsWorld.SetGrabAim(player.Position(), player.FacingX());
            if (!respawnedThisFrame && !restartedThisFrame && inputState.grabDropPressed)
            {
                physicsWorld.HandleGrabDrop();
            }

            physicsWorld.Update(deltaSeconds);
            if (!respawnedThisFrame && !restartedThisFrame)
            {
                camera.Update(player.Position(), deltaSeconds);
            }
        }

        const std::vector<physics::DynamicBoxRuntimeState> dynamicBoxes =
            physicsWorld.GetDynamicBoxes();
        const physics::DynamicBoxGrabState grabState = physicsWorld.GetGrabState();
        const std::vector<render::DynamicBoxDrawState> dynamicDraw =
            MakeDynamicBoxDrawStates(dynamicBoxes, grabState);
        const std::vector<render::PressurePlateDrawState> pressurePlateDraw =
            MakePressurePlateDrawStates(physicsWorld.GetPressurePlates());
        const std::vector<physics::DoorRuntimeState> runtimeDoors = physicsWorld.GetDoors();
        const std::vector<render::DoorDrawState> doorDraw = MakeDoorDrawStates(runtimeDoors);
        const physics::MovingPlatformState movingPlatform = physicsWorld.GetMovingPlatform();
        render::CameraView cameraView = MakeGameplayCameraView(camera);
#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
        const render::CameraView gameplayCameraView = cameraView;
#endif
        render::DebugWorldOverlay overlay{};
        render::WorldViewRect worldViewRect{};
#if defined(PLATFORMER_ENABLE_DEBUG_UI)
        editor::EditorInputState editorInput{};
        editor::EditorContentViewport editorViewport{};
        if (levelEditorState.active)
        {
            editorInput = editor::PollEditorInput();
            window.SetEscapeClosesWindow(
                !editor::IsLevelAuthoringAvailable()
                || !editor::EditorViewportPlacementIsActive(
                    levelEditorState.placementMode, levelEditorState.staticPropPlacement));
            editorViewport = MakeLiveEditorContentViewport(
                static_cast<float>(window.Width()),
                static_cast<float>(window.Height()),
                levelEditorState);
            worldViewRect = MakeWorldViewRect(editorViewport);
            const bool applyLook =
                !debugUi.WantsMouseCapture() && !levelEditorState.gizmo.dragging
                && editor::PointInEditorContentViewport(
                    editorInput.mouseX, editorInput.mouseY, editorViewport);
            editor::UpdateEditorCamera(
                levelEditorState.editorCamera,
                editorInput,
                deltaSeconds,
                applyLook,
                false,
                false);
            input::SetMouseLookActive(applyLook && editorInput.lookHeld);

            cameraView = editor::MakeCameraView(levelEditorState.editorCamera);
            overlay.drawSpawnMarker = true;
            overlay.spawnCenter = levelDefinition.initialSpawnVisualCenter;
            overlay.spawnSize = world::kPlayerVisualSize;

            const editor::EditorPickingWorldState pickingWorld =
                MakeRuntimePickingWorldState(movingPlatform, dynamicBoxes, runtimeDoors);
            const editor::EditorHighlightRequest highlight = editor::MakeHighlightRequest(
                editor::HighlightSelectionFromWorking(
                    levelEditorState.selection, levelEditorState.structuralMap),
                editor::BuildPickingSet(levelDefinition, pickingWorld));
            overlay.drawHighlight = highlight.visible;
            overlay.highlightCenter = highlight.center;
            overlay.highlightSize = highlight.size;
            overlay.highlightRotationZDegrees = highlight.rotationZDegrees;

            overlay.collectedAuthoredCollectibleCenters.resize(
                levelDefinition.collectibles.size());
            const int collectedAuthoredCount = editor::CollectEditorAuthoredCollectibleCenters(
                levelDefinition,
                collectibleRunState.collected.empty() ? nullptr
                                                      : collectibleRunState.collected.data(),
                collectibleRunState.collected.size(),
                overlay.collectedAuthoredCollectibleCenters.empty()
                    ? nullptr
                    : overlay.collectedAuthoredCollectibleCenters.data(),
                static_cast<int>(overlay.collectedAuthoredCollectibleCenters.size()),
                &levelEditorState.structuralMap);
            overlay.collectedAuthoredCollectibleCenters.resize(
                collectedAuthoredCount < 0 ? 0 : static_cast<std::size_t>(collectedAuthoredCount));

            const std::vector<editor::PendingAuthoringVisual> pendingAuthoring =
                editor::CollectPendingAuthoringVisuals(
                    levelDefinition,
                    levelEditorState.workingCopy,
                    levelEditorState.structuralMap,
                    levelEditorState.selection);
            overlay.pendingAuthoring.clear();
            overlay.pendingAuthoring.reserve(pendingAuthoring.size());
            for (const editor::PendingAuthoringVisual& visual : pendingAuthoring)
            {
                render::DebugWorldOverlay::PendingAuthoringOverlayItem item{};
                switch (visual.kind)
                {
                case editor::EditorObjectKind::ElevatedPlatform:
                    item.kind = 0;
                    break;
                case editor::EditorObjectKind::Checkpoint:
                    item.kind = 1;
                    break;
                case editor::EditorObjectKind::Hazard:
                    item.kind = 2;
                    break;
                case editor::EditorObjectKind::Collectible:
                    item.kind = 3;
                    break;
                case editor::EditorObjectKind::DynamicBox:
                    item.kind = 4;
                    break;
                case editor::EditorObjectKind::StaticProp:
                    item.kind = 5;
                    break;
                case editor::EditorObjectKind::PressurePlate:
                    item.kind = 6;
                    break;
                case editor::EditorObjectKind::Door:
                    item.kind = 7;
                    break;
                default:
                    continue;
                }
                item.selected = visual.selected;
                item.boundsCenter = visual.boundsCenter;
                item.boundsSize = visual.boundsSize;
#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
                item.drawObjectVisual = visual.hasObjectVisual;
#else
                item.drawObjectVisual = false;
#endif
                item.checkpoint = visual.checkpoint;
                item.hazard = visual.hazard;
                item.collectible = visual.collectible;
                overlay.pendingAuthoring.push_back(item);
            }

#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
            {
                const std::string_view previewIdentity =
                    editor::StaticPropPlacementIsActive(levelEditorState.staticPropPlacement)
                    ? std::string_view(levelEditorState.staticPropPlacement.modelIdentity)
                    : std::string_view{};
                renderer.SyncStaticPropModels(levelDefinition, previewIdentity);
            }
            if (editor::StaticPropPlacementIsActive(levelEditorState.staticPropPlacement))
            {
                const editor::Ray3 previewRay = editor::ScreenToWorldRayFromWindow(
                    cameraView,
                    editorInput.mouseX,
                    editorInput.mouseY,
                    editorViewport);
                core::Vec3 localMin = editor::kStaticPropDefaultLocalMin;
                core::Vec3 localMax = editor::kStaticPropDefaultLocalMax;
                if (renderer.StaticPropModels() != nullptr)
                {
                    (void)renderer.StaticPropModels()->TryGetLoadedLocalBounds(
                        levelEditorState.staticPropPlacement.modelIdentity, localMin, localMax);
                }
                const editor::StaticPropPlacementPreview preview =
                    editor::ResolveStaticPropPlacementPreview(
                        levelEditorState.staticPropPlacement,
                        previewRay,
                        editor::BuildPickingSet(levelDefinition, pickingWorld),
                        localMin,
                        localMax);
                overlay.drawStaticPropPlacementPreview =
                    editor::StaticPropPlacementPreviewShouldDraw(preview);
                if (overlay.drawStaticPropPlacementPreview)
                {
                    overlay.staticPropPlacementPreview =
                        editor::MakeStaticPropSpecFromPreview(preview);
                    editor::StaticPropWorldAabb(
                        overlay.staticPropPlacementPreview,
                        localMin,
                        localMax,
                        overlay.staticPropPlacementBoundsCenter,
                        overlay.staticPropPlacementBoundsSize);
                }
            }
            else if (editor::PlacementModeIsActive(levelEditorState.placementMode))
            {
                const editor::Ray3 previewRay = editor::ScreenToWorldRayFromWindow(
                    cameraView,
                    editorInput.mouseX,
                    editorInput.mouseY,
                    editorViewport);
                const editor::PlacementCandidate candidate = editor::ResolvePlacementCandidate(
                    levelEditorState.placementMode,
                    previewRay,
                    editor::BuildPickingSet(levelDefinition, pickingWorld),
                    editor::EditorAddPlacementAnchor(levelEditorState.editorCamera));
                overlay.drawPlacementCandidate = candidate.visible;
                overlay.placementCandidateFallback =
                    candidate.source == editor::PlacementCandidateSource::CameraFallback;
                switch (candidate.kind)
                {
                case editor::EditorObjectKind::ElevatedPlatform:
                    overlay.placementCandidateKind = 0;
                    break;
                case editor::EditorObjectKind::Checkpoint:
                    overlay.placementCandidateKind = 1;
                    break;
                case editor::EditorObjectKind::Hazard:
                    overlay.placementCandidateKind = 2;
                    break;
                case editor::EditorObjectKind::Collectible:
                    overlay.placementCandidateKind = 3;
                    break;
                case editor::EditorObjectKind::DynamicBox:
                    overlay.placementCandidateKind = 4;
                    break;
                case editor::EditorObjectKind::PressurePlate:
                    overlay.placementCandidateKind = 6;
                    break;
                case editor::EditorObjectKind::Door:
                    overlay.placementCandidateKind = 7;
                    break;
                default:
                    overlay.placementCandidateKind = 0;
                    break;
                }
                overlay.placementCandidateCenter = candidate.center;
                overlay.placementCandidateSize = candidate.size;
                overlay.placementCandidateCheckpoint = candidate.checkpoint;
                overlay.placementCandidateHazard = candidate.hazard;
                overlay.placementCandidateCollectible = candidate.collectible;
            }
#endif

#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
            const editor::PendingDeleteVisuals pendingDelete =
                editor::MakePendingDeleteVisuals(
                    levelDefinition, levelEditorState.structuralMap);
            overlay.pendingDeletePlatformIndices = pendingDelete.platformIndices;
            overlay.pendingDeletePlatformCenters.clear();
            overlay.pendingDeletePlatformSizes.clear();
            overlay.pendingDeletePlatformCenters.reserve(pendingDelete.platforms.size());
            overlay.pendingDeletePlatformSizes.reserve(pendingDelete.platforms.size());
            for (const world::Box& platform : pendingDelete.platforms)
            {
                overlay.pendingDeletePlatformCenters.push_back(platform.center);
                overlay.pendingDeletePlatformSizes.push_back(platform.size);
            }
            overlay.pendingDeleteCheckpointIndices = pendingDelete.checkpointIndices;
            overlay.pendingDeleteCheckpoints = pendingDelete.checkpoints;
            overlay.pendingDeleteHazardIndices = pendingDelete.hazardIndices;
            overlay.pendingDeleteHazards = pendingDelete.hazards;
            overlay.pendingDeleteCollectibleIndices = pendingDelete.collectibleIndices;
            overlay.pendingDeleteCollectibleCenters = pendingDelete.collectibleCenters;
            overlay.pendingDeleteDynamicBoxIndices = pendingDelete.dynamicBoxIndices;
            overlay.pendingDeleteDynamicBoxCenters.clear();
            overlay.pendingDeleteDynamicBoxSizes.clear();
            overlay.pendingDeleteDynamicBoxCenters.reserve(pendingDelete.dynamicBoxes.size());
            overlay.pendingDeleteDynamicBoxSizes.reserve(pendingDelete.dynamicBoxes.size());
            for (std::size_t index = 0; index < pendingDelete.dynamicBoxIndices.size(); ++index)
            {
                const int activeIndex = pendingDelete.dynamicBoxIndices[index];
                if (activeIndex >= 0
                    && static_cast<std::size_t>(activeIndex) < dynamicBoxes.size())
                {
                    overlay.pendingDeleteDynamicBoxCenters.push_back(
                        dynamicBoxes[static_cast<std::size_t>(activeIndex)].center);
                    overlay.pendingDeleteDynamicBoxSizes.push_back(
                        dynamicBoxes[static_cast<std::size_t>(activeIndex)].size);
                    continue;
                }
                if (index < pendingDelete.dynamicBoxes.size())
                {
                    overlay.pendingDeleteDynamicBoxCenters.push_back(
                        pendingDelete.dynamicBoxes[index].center);
                    overlay.pendingDeleteDynamicBoxSizes.push_back(
                        pendingDelete.dynamicBoxes[index].size);
                }
            }
            overlay.pendingDeletePressurePlateIndices = pendingDelete.pressurePlateIndices;
            overlay.pendingDeletePressurePlateCenters.clear();
            overlay.pendingDeletePressurePlateSizes.clear();
            overlay.pendingDeletePressurePlateCenters.reserve(pendingDelete.pressurePlates.size());
            overlay.pendingDeletePressurePlateSizes.reserve(pendingDelete.pressurePlates.size());
            for (const world::PressurePlateSpec& plate : pendingDelete.pressurePlates)
            {
                overlay.pendingDeletePressurePlateCenters.push_back(plate.center);
                overlay.pendingDeletePressurePlateSizes.push_back(plate.size);
            }
            overlay.pendingDeleteDoorIndices = pendingDelete.doorIndices;
            overlay.pendingDeleteDoorCenters.clear();
            overlay.pendingDeleteDoorSizes.clear();
            overlay.pendingDeleteDoorCenters.reserve(pendingDelete.doors.size());
            overlay.pendingDeleteDoorSizes.reserve(pendingDelete.doors.size());
            for (std::size_t index = 0; index < pendingDelete.doorIndices.size(); ++index)
            {
                const int activeIndex = pendingDelete.doorIndices[index];
                if (activeIndex >= 0
                    && static_cast<std::size_t>(activeIndex) < runtimeDoors.size())
                {
                    overlay.pendingDeleteDoorCenters.push_back(
                        runtimeDoors[static_cast<std::size_t>(activeIndex)].center);
                    overlay.pendingDeleteDoorSizes.push_back(
                        runtimeDoors[static_cast<std::size_t>(activeIndex)].size);
                    continue;
                }
                if (index < pendingDelete.doors.size())
                {
                    overlay.pendingDeleteDoorCenters.push_back(pendingDelete.doors[index].center);
                    overlay.pendingDeleteDoorSizes.push_back(pendingDelete.doors[index].size);
                }
            }
            overlay.pendingDeleteStaticPropIndices = pendingDelete.staticPropIndices;
            overlay.pendingDeleteStaticPropCenters.clear();
            overlay.pendingDeleteStaticPropSizes.clear();
            overlay.pendingDeleteStaticPropCenters.reserve(pendingDelete.staticProps.size());
            overlay.pendingDeleteStaticPropSizes.reserve(pendingDelete.staticProps.size());
            for (const world::StaticPropSpec& prop : pendingDelete.staticProps)
            {
                core::Vec3 center{};
                core::Vec3 size{};
                editor::StaticPropWorldAabb(
                    prop,
                    editor::kStaticPropDefaultLocalMin,
                    editor::kStaticPropDefaultLocalMax,
                    center,
                    size);
                overlay.pendingDeleteStaticPropCenters.push_back(center);
                overlay.pendingDeleteStaticPropSizes.push_back(size);
            }
#endif

            const editor::CheckpointEditorOverlay checkpointOverlay =
                editor::MakeCheckpointEditorOverlay(
                    levelEditorState.selection, levelEditorState.workingCopy);
            overlay.drawCheckpointRespawnMarker = checkpointOverlay.visible;
            overlay.checkpointRespawnMarker = checkpointOverlay.respawnPosition;
            overlay.checkpointTriggerCenter = checkpointOverlay.triggerCenter;
            if (checkpointOverlay.visible)
            {
                const float dx =
                    checkpointOverlay.respawnPosition.x - checkpointOverlay.triggerCenter.x;
                const float dy =
                    checkpointOverlay.respawnPosition.y - checkpointOverlay.triggerCenter.y;
                const float dz =
                    checkpointOverlay.respawnPosition.z - checkpointOverlay.triggerCenter.z;
                overlay.drawCheckpointRespawnConnector = (dx * dx + dy * dy + dz * dz) > 1.0e-6f;
            }

            editor::GizmoDrawRequest gizmo{};
            if (levelEditorState.transformMode == editor::EditorTransformMode::Resize)
            {
                gizmo = editor::MakeResizeGizmoDrawRequest(
                    levelEditorState.selection,
                    levelEditorState.workingCopy,
                    cameraView,
                    levelEditorState.gizmo);
            }
            else if (levelEditorState.transformMode == editor::EditorTransformMode::Scale)
            {
                gizmo = editor::MakeScaleGizmoDrawRequest(
                    levelEditorState.selection,
                    levelEditorState.workingCopy,
                    cameraView,
                    levelEditorState.gizmo);
            }
            else
            {
                gizmo = editor::MakeGizmoDrawRequest(
                    levelEditorState.selection,
                    levelEditorState.workingCopy,
                    cameraView,
                    levelEditorState.gizmo);
            }
            overlay.drawTranslationGizmo =
                gizmo.visible
                && levelEditorState.transformMode == editor::EditorTransformMode::Translate;
            overlay.drawResizeGizmo =
                gizmo.visible
                && levelEditorState.transformMode == editor::EditorTransformMode::Resize;
            overlay.drawScaleGizmo =
                gizmo.visible
                && levelEditorState.transformMode == editor::EditorTransformMode::Scale;
            overlay.gizmoOrigin = gizmo.origin;
            overlay.gizmoAxisLength = gizmo.axisLength;
            overlay.gizmoHoveredAxis = static_cast<int>(gizmo.hovered);
            overlay.gizmoActiveAxis = static_cast<int>(gizmo.active);
            overlay.gizmoHoveredSign = gizmo.hoveredSign;
            overlay.gizmoActiveSign = gizmo.activeSign;
        }
        else
        {
            input::SetMouseLookActive(false);
            window.SetEscapeClosesWindow(true);
        }
#endif
        renderer.BeginFrame();
#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
        renderer.SyncStaticPropModels(
            levelDefinition,
            editor::StaticPropPlacementIsActive(levelEditorState.staticPropPlacement)
                ? std::string_view(levelEditorState.staticPropPlacement.modelIdentity)
                : std::string_view{});
#else
        renderer.SyncStaticPropModels(levelDefinition);
#endif
        renderer.DrawWorld(
            player,
            cameraView,
            levelDefinition,
            dynamicDraw,
            pressurePlateDraw,
            doorDraw,
            movingPlatform.position,
            movingPlatform.size,
            MakeCheckpointVisuals(
                respawnState.activeCheckpointIndex, levelDefinition.checkpoints.size()),
            levelCompletionState.completed,
            collectibleRunState.collected,
            gameplay::CollectedCount(collectibleRunState),
            runTimerState.elapsedSeconds,
            sessionBestTimeState.hasBestTime,
            sessionBestTimeState.bestSeconds,
            overlay,
            worldViewRect
        );
#if defined(PLATFORMER_ENABLE_DEBUG_UI)
        if (levelEditorState.active)
        {
            const editor::OrientationWidgetLayout widgetLayout =
                editor::MakeOrientationWidgetLayout(
                    static_cast<float>(window.Width()),
                    static_cast<float>(window.Height()),
                    editor::OrientationWidgetLiveExtraTopInset(
                        levelEditorState.active,
                        levelEditorState.menuBarHeight,
                        levelEditorState.toolbarHeight,
                        EditorQuickToolbarVisible(levelEditorState)));
            const editor::OrientationWidgetAxes widgetAxes =
                editor::ProjectOrientationWidgetAxes(levelEditorState.editorCamera);
            renderer.DrawOrientationWidget({
                true,
                widgetLayout.originX,
                widgetLayout.originY,
                widgetLayout.radius,
                widgetAxes.x,
                widgetAxes.y,
                widgetAxes.z});
            if (editor::IsLevelAuthoringAvailable()
                && editor::EditorViewportPlacementIsActive(
                    levelEditorState.placementMode, levelEditorState.staticPropPlacement))
            {
                const bool staticPropPlacing =
                    editor::StaticPropPlacementIsActive(levelEditorState.staticPropPlacement);
                renderer.DrawEditorPlacementHud(
                    true,
                    staticPropPlacing
                        ? editor::StaticPropPlacementHudName()
                        : editor::PlacementModeName(levelEditorState.placementMode),
                    staticPropPlacing
                        ? !overlay.drawStaticPropPlacementPreview
                        : overlay.placementCandidateFallback,
                    editor::OrientationWidgetLiveExtraTopInset(
                        levelEditorState.active,
                        levelEditorState.menuBarHeight,
                        levelEditorState.toolbarHeight,
                        EditorQuickToolbarVisible(levelEditorState)));
            }
        }
        editor::LevelEditorViewContext levelEditorView{
            runtimeLevelPathDisplay.c_str(),
            movingPlatform.position,
            static_cast<float>(window.Width()),
            static_cast<float>(window.Height()),
            false};
#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
        thumbnailStore.BeginFrame();
        levelEditorView.thumbnails = &thumbnailStore;
        {
            const std::string& identity = levelEditorState.contentBrowser.selectedIdentity;
            const std::filesystem::path sourceRoot = editor::AuthoringSourceRoot();
            const std::filesystem::path sourcePath =
                identity.empty() || sourceRoot.empty()
                ? std::filesystem::path{}
                : sourceRoot / identity;
            modelPreview.Sync(identity, sourcePath);
            if (identity.empty())
            {
                levelEditorState.modelPreviewFramedIdentity.clear();
            }
        }
        levelEditorView.modelPreview = &modelPreview;
        levelEditorView.staticPropModels = renderer.StaticPropModels();
        levelEditorView.gameplayCameraPosition = gameplayCameraView.position;
        levelEditorView.gameplayCameraTarget = gameplayCameraView.target;
#endif
        // Cook, Stage & Reload observation lives in Application::Run, not in
        // ImGui Draw, so F2 hide and panel visibility cannot cancel it.
        editorToolRunner.Poll();
        {
            const editor::CookStageReloadStatus statusBeforeObserve =
                cookStageReload.LastStatus();
            const editor::EditorToolJobSnapshot toolSnapshot = editorToolRunner.Snapshot();
            cookStageReload.Observe(toolSnapshot.kind, toolSnapshot.state);
            if (statusBeforeObserve == editor::CookStageReloadStatus::WaitingForCookAndStage
                && cookStageReload.LastStatus() == editor::CookStageReloadStatus::ExternalFailed)
            {
                editor::ResetLevelActionStatuses(levelEditorState);
                levelEditorState.lastMessage =
                    "Cook, Stage & Reload failed before Reload. "
                    "See Tool Output for Cook/Stage details.";
            }
        }
        editor::LevelEditorRequest editorRequest = debugUi.Draw(
            MakeDebugMetricsSnapshot(
                player,
                camera,
                physicsWorld,
                inputState,
                renderer,
                respawnState,
                levelCompletionState,
                collectibleRunState,
                runTimerState,
                sessionBestTimeState,
                levelDefinition,
                bestTimeLoadStatus,
                bestTimeSaveStatus,
                bestTimeSavePathDisplay.c_str(),
                runtimeLevelPathDisplay.c_str(),
                levelLoadStatus,
                levelFormatVersion,
                restartedThisFrame,
                hazardContactThisFrame,
                collectedThisFrameIndex,
                deltaSeconds),
            levelEditorState,
            levelDefinition,
            levelEditorView,
            editorToolRunner,
            cookStageReload.IsPending());
        if (levelEditorState.active)
        {
            // Keyboard move, wheel and world pick use this frame's ImGui capture
            // so typing into Inspector and clicking/scrolling panels cannot
            // drive the viewport behind them.
            const bool imguiWantsMouse = debugUi.WantsMouseCapture();
            editorViewport = MakeLiveEditorContentViewport(
                static_cast<float>(window.Width()),
                static_cast<float>(window.Height()),
                levelEditorState);
            const bool mouseCaptured = editor::EditorViewportPointerBlocked(
                editorInput.mouseX,
                editorInput.mouseY,
                editorViewport,
                imguiWantsMouse);
            const bool keyboardCaptured =
                debugUi.WantsKeyboardCapture() || debugUi.WantsTextInput();
            const bool dragging = levelEditorState.gizmo.dragging;
            const editor::EditorWheelIntent wheelIntent = editor::ResolveEditorWheel(
                mouseCaptured,
                dragging,
                editorInput.altHeld,
                editorInput.wheelDelta);
            editor::UpdateEditorCamera(
                levelEditorState.editorCamera,
                editorInput,
                deltaSeconds,
                false,
                !keyboardCaptured,
                wheelIntent == editor::EditorWheelIntent::NavigationSpeed);
            if (wheelIntent == editor::EditorWheelIntent::Dolly)
            {
                editor::ApplyEditorCameraDolly(
                    levelEditorState.editorCamera, editorInput.dollyWheelDelta);
            }

            const editor::Ray3 ray = editor::ScreenToWorldRayFromWindow(
                cameraView,
                editorInput.mouseX,
                editorInput.mouseY,
                editorViewport);

            bool widgetConsumedPointer = false;
            if (!mouseCaptured && !dragging && editorInput.selectPressed && !editorInput.lookHeld)
            {
                const editor::OrientationWidgetLayout widgetLayout =
                    editor::MakeOrientationWidgetLayout(
                        static_cast<float>(window.Width()),
                        static_cast<float>(window.Height()),
                        editor::OrientationWidgetLiveExtraTopInset(
                            levelEditorState.active,
                            levelEditorState.menuBarHeight,
                            levelEditorState.toolbarHeight,
                            EditorQuickToolbarVisible(levelEditorState)));
                const editor::OrientationWidgetAxes widgetAxes =
                    editor::ProjectOrientationWidgetAxes(levelEditorState.editorCamera);
                const editor::CanonicalEditorView canonical = editor::PickOrientationWidget(
                    editorInput.mouseX,
                    editorInput.mouseY,
                    widgetLayout,
                    widgetAxes);
                if (canonical != editor::CanonicalEditorView::None)
                {
                    editor::ApplyCanonicalEditorView(levelEditorState.editorCamera, canonical);
                    widgetConsumedPointer = true;
                }
            }

            const bool selectPressedForGizmo =
                editorInput.selectPressed && !widgetConsumedPointer;
            bool gizmoConsumedPointer = false;
            if (levelEditorState.transformMode == editor::EditorTransformMode::Resize)
            {
                gizmoConsumedPointer = editor::UpdateResizeInteraction(
                    levelEditorState.gizmo,
                    levelEditorState.selection,
                    levelEditorState.workingCopy,
                    cameraView,
                    ray,
                    mouseCaptured,
                    editorInput.lookHeld,
                    selectPressedForGizmo,
                    editorInput.selectHeld,
                    editorInput.selectReleased);
            }
            else if (levelEditorState.transformMode == editor::EditorTransformMode::Scale)
            {
                gizmoConsumedPointer = editor::UpdateScaleInteraction(
                    levelEditorState.gizmo,
                    levelEditorState.selection,
                    levelEditorState.workingCopy,
                    cameraView,
                    ray,
                    mouseCaptured,
                    editorInput.lookHeld,
                    selectPressedForGizmo,
                    editorInput.selectHeld,
                    editorInput.selectReleased);
            }
            else
            {
                gizmoConsumedPointer = editor::UpdateGizmoInteraction(
                    levelEditorState.gizmo,
                    levelEditorState.selection,
                    levelEditorState.workingCopy,
                    cameraView,
                    ray,
                    mouseCaptured,
                    editorInput.lookHeld,
                    selectPressedForGizmo,
                    editorInput.selectHeld,
                    editorInput.selectReleased);
            }
            if (editor::ShouldCancelPlacementMode(
                    levelEditorState.placementMode,
                    editorInput.escapePressed,
                    keyboardCaptured)
                || editor::ShouldCancelStaticPropPlacement(
                    levelEditorState.staticPropPlacement,
                    editorInput.escapePressed,
                    keyboardCaptured))
            {
                editor::CancelAllEditorPlacement(
                    levelEditorState.placementMode,
                    levelEditorState.placementPointerBlocked,
                    levelEditorState.staticPropPlacement);
                window.SetEscapeClosesWindow(true);
            }

            const bool gizmoHoveredOnPress =
                editorInput.selectPressed
                && levelEditorState.gizmo.hovered != editor::EditorAxis::None;
            const bool pointerClaimed = editor::PlacementInteractionClaimsPointer(
                mouseCaptured,
                editorInput.lookHeld,
                widgetConsumedPointer,
                gizmoConsumedPointer,
                levelEditorState.gizmo.dragging,
                gizmoHoveredOnPress);
            editor::UpdatePlacementPointerBlock(
                levelEditorState.placementPointerBlocked,
                editorInput.selectPressed,
                editorInput.selectHeld,
                editorInput.selectReleased,
                pointerClaimed);

            const bool placingStaticProp =
                editor::IsLevelAuthoringAvailable()
                && editor::StaticPropPlacementIsActive(levelEditorState.staticPropPlacement);
            const bool placing =
                editor::IsLevelAuthoringAvailable()
                && editor::PlacementModeIsActive(levelEditorState.placementMode);
            if (placingStaticProp)
            {
                core::Vec3 localMin = editor::kStaticPropDefaultLocalMin;
                core::Vec3 localMax = editor::kStaticPropDefaultLocalMax;
                if (renderer.StaticPropModels() != nullptr)
                {
                    (void)renderer.StaticPropModels()->TryGetLoadedLocalBounds(
                        levelEditorState.staticPropPlacement.modelIdentity, localMin, localMax);
                }
                const editor::StaticPropPlacementPreview preview =
                    editor::ResolveStaticPropPlacementPreview(
                        levelEditorState.staticPropPlacement,
                        ray,
                        editor::BuildPickingSet(
                            levelDefinition,
                            MakeRuntimePickingWorldState(movingPlatform, dynamicBoxes, runtimeDoors)),
                        localMin,
                        localMax);
                const bool canAdd = editor::CanIssueAuthoredLifecycleRequest(
                    true,
                    levelEditorState.workingCopy,
                    levelEditorState.selection,
                    levelEditorState.gizmo.dragging,
                    editor::LevelEditorRequest::AddStaticProp,
                    levelEditorState.staticPropPlacement.modelIdentity);
                if (editor::ShouldConfirmStaticPropPlacement(
                        levelEditorState.staticPropPlacement,
                        preview.valid,
                        editorInput.selectPressed,
                        mouseCaptured,
                        editorInput.lookHeld,
                        widgetConsumedPointer,
                        gizmoConsumedPointer,
                        levelEditorState.placementPointerBlocked,
                        canAdd))
                {
                    editor::HandleAuthoredLifecycleRequest(
                        levelEditorState,
                        levelDefinition,
                        editor::LevelEditorRequest::AddStaticProp,
                        true,
                        preview.position,
                        true,
                        levelEditorState.staticPropPlacement.modelIdentity);
                }
            }
            else if (placing
                && editor::ShouldConfirmPlacement(
                    levelEditorState.placementMode,
                    editorInput.selectPressed,
                    mouseCaptured,
                    editorInput.lookHeld,
                    widgetConsumedPointer,
                    gizmoConsumedPointer,
                    levelEditorState.placementPointerBlocked,
                    true))
            {
                const editor::PlacementCandidate candidate = editor::ResolvePlacementCandidate(
                    levelEditorState.placementMode,
                    ray,
                    editor::BuildPickingSet(
                        levelDefinition,
                        MakeRuntimePickingWorldState(movingPlatform, dynamicBoxes, runtimeDoors)),
                    editor::EditorAddPlacementAnchor(levelEditorState.editorCamera));
                editor::HandleAuthoredLifecycleRequest(
                    levelEditorState,
                    levelDefinition,
                    editor::PlacementAddRequest(levelEditorState.placementMode),
                    true,
                    candidate.center,
                    true);
            }
            if (!placing && !placingStaticProp
                && editor::ShouldAttemptEditorViewportPick(
                    editorInput.selectPressed,
                    mouseCaptured,
                    editorInput.lookHeld,
                    widgetConsumedPointer,
                    gizmoConsumedPointer))
            {
                const editor::EditorPickingWorldState pickingWorld =
                    MakeRuntimePickingWorldState(movingPlatform, dynamicBoxes, runtimeDoors);
                const std::vector<editor::PendingPickProxy> pendingProxies =
                    editor::BuildPendingPickProxies(editor::CollectPendingAuthoringVisuals(
                        levelDefinition,
                        levelEditorState.workingCopy,
                        levelEditorState.structuralMap,
                        levelEditorState.selection));
                editor::EditorSelection workingPick{};
                if (editor::TryResolveEditorViewportPick(
                        ray,
                        editor::BuildPickingSet(levelDefinition, pickingWorld),
                        pendingProxies,
                        levelEditorState.structuralMap,
                        workingPick))
                {
                    levelEditorState.selection = workingPick;
                }
            }

            if (editorInput.selectReleased || !editorInput.selectHeld)
            {
                editor::ClearPlacementPointerBlock(levelEditorState.placementPointerBlocked);
            }

            if (editor::NudgeAllowed(
                    levelEditorState.transformMode, keyboardCaptured, levelEditorState.gizmo.dragging))
            {
                if (editorInput.nudgeX != 0)
                {
                    editor::ApplyNudge(
                        levelEditorState.workingCopy,
                        levelEditorState.selection,
                        editor::EditorAxis::X,
                        static_cast<float>(editorInput.nudgeX),
                        editorInput.nudgePrecision,
                        levelEditorState.transformMode);
                }
                if (editorInput.nudgeY != 0)
                {
                    editor::ApplyNudge(
                        levelEditorState.workingCopy,
                        levelEditorState.selection,
                        editor::EditorAxis::Y,
                        static_cast<float>(editorInput.nudgeY),
                        editorInput.nudgePrecision,
                        levelEditorState.transformMode);
                }
                if (editorInput.nudgeZ != 0)
                {
                    editor::ApplyNudge(
                        levelEditorState.workingCopy,
                        levelEditorState.selection,
                        editor::EditorAxis::Z,
                        static_cast<float>(editorInput.nudgeZ),
                        editorInput.nudgePrecision,
                        levelEditorState.transformMode);
                }
            }
            if (editorRequest == editor::LevelEditorRequest::None
                && editor::ShouldEmitDeleteSelectedRequest(
                    editorInput.deletePressed,
                    keyboardCaptured,
                    editor::IsLevelAuthoringAvailable(),
                    levelEditorState.workingCopy,
                    levelEditorState.selection,
                    levelEditorState.gizmo.dragging))
            {
                editorRequest = editor::LevelEditorRequest::DeleteSelected;
            }
        }
#endif
        renderer.EndFrame();

#if defined(PLATFORMER_ENABLE_DEBUG_UI)
        // Executed after the frame is presented so a rebuild never lands
        // between DrawWorld and the physics state it was drawn from.
        if (!HandleLevelEditorRequest(editorRequest))
        {
            fatalError = true;
        }
        FinishCookStageAndReloadIfReady();
#endif
    }

    Shutdown();
    return fatalError ? 1 : 0;
}

void Application::Initialize()
{
    if (!window.Initialize())
    {
        initialized = false;
        return;
    }

    const std::filesystem::path levelPath =
        platform::RuntimeAssetPath(world::kLevel01RuntimeLogicalId);
    runtimeLevelPathDisplay = levelPath.empty() ? "(unavailable)" : levelPath.string();
    const world::ParseLevelFileResult loadedLevel = world::LoadLevelFile(levelPath);
    levelLoadStatus = loadedLevel.status;
    levelFormatVersion = loadedLevel.formatVersion;

    if (loadedLevel.status != world::LoadLevelFileStatus::Loaded)
    {
        ReportRequiredLevelFailure(levelPath, loadedLevel, nullptr);
        window.Shutdown();
        initialized = false;
        return;
    }
    if (loadedLevel.level.id != world::kLevel01Id)
    {
        ReportRequiredLevelFailure(
            levelPath, loadedLevel, "expected Level ID level_01");
        window.Shutdown();
        initialized = false;
        return;
    }
    if (!world::LevelDefinitionHasRequiredAuthoredContent(loadedLevel.level))
    {
        ReportRequiredLevelFailure(
            levelPath, loadedLevel, "semantic validation failed");
        window.Shutdown();
        initialized = false;
        return;
    }

    levelDefinition = loadedLevel.level;
#if defined(PLATFORMER_ENABLE_DEBUG_UI)
    // M32 dirty baseline: the staged file we just loaded. Dirty therefore
    // starts false and tracks only edits applied during this session. The
    // staged copy is assumed to match the repository source; M32 does not
    // reconcile a developer who edited source without cooking.
    levelEditorState.workingCopy = levelDefinition;
    levelEditorState.savedSourceBaseline = levelDefinition;
#endif

    camera.ApplyLevelFraming(
        levelDefinition.camera.offset, levelDefinition.camera.fieldOfViewY);
    respawnState.respawnPosition = levelDefinition.initialSpawnVisualCenter;
    collectibleRunState =
        gameplay::MakeClearedCollectibleRunState(levelDefinition.collectibles.size());

    renderer.LoadRuntimeAssets();

    if (!physicsWorld.Initialize(levelDefinition))
    {
        renderer.UnloadRuntimeAssets();
        window.Shutdown();
        initialized = false;
        return;
    }

    if (!physicsWorld.InitializePlayer(
            levelDefinition.initialSpawnVisualCenter, player.Size()))
    {
        physicsWorld.Shutdown();
        renderer.UnloadRuntimeAssets();
        window.Shutdown();
        initialized = false;
        return;
    }

    player.ApplyPhysicsState(physicsWorld.GetPlayerPhysicsState());
    camera.Initialize(player.Position());
    runTimerState = gameplay::RunTimerState{};
    sessionBestTimeState = gameplay::SessionBestTimeState{};
    bestTimeSaveStatus = persistence::SaveBestTimeStatus::NotAttempted;
    bestTimeSavePathDisplay = persistence::BestTimeSavePath().string();
    if (bestTimeSavePathDisplay.empty())
    {
        bestTimeSavePathDisplay = "(unavailable)";
    }
    if (!RunTimeFormatScaffoldingOk())
    {
        std::fprintf(stderr, "RunTimeFormat scaffolding check failed.\n");
        physicsWorld.Shutdown();
        renderer.UnloadRuntimeAssets();
        window.Shutdown();
        initialized = false;
        return;
    }
    if (!BestTimeSaveFormatScaffoldingOk())
    {
        std::fprintf(stderr, "BestTimeSave format scaffolding check failed.\n");
        physicsWorld.Shutdown();
        renderer.UnloadRuntimeAssets();
        window.Shutdown();
        initialized = false;
        return;
    }

    const persistence::LoadBestTimeResult loaded = persistence::LoadBestTime();
    bestTimeLoadStatus = loaded.status;
    if (loaded.status == persistence::LoadBestTimeStatus::Loaded)
    {
        sessionBestTimeState.hasBestTime = true;
        sessionBestTimeState.bestSeconds = loaded.bestSeconds;
    }
#if defined(GAME_DEVELOPMENT_TOOLS)
    ReportBestTimeLoadDiagnostic(bestTimeLoadStatus);
#endif
#if defined(PLATFORMER_ENABLE_DEBUG_UI)
    debugUi.Initialize();
    levelEditorState.selectedBuildTarget = editor::LoadEditorBuildSelection();
    levelEditorState.contentBrowser.viewMode = editor::LoadContentBrowserViewMode();
#endif
    initialized = true;
}

void Application::PerformRespawn(gameplay::RespawnReason reason)
{
    if (reason == gameplay::RespawnReason::Fall
        || reason == gameplay::RespawnReason::Hazard)
    {
        ++respawnState.deathCount;
    }
    respawnState.lastRespawnReason = reason;

    physicsWorld.ResetCharacter(respawnState.respawnPosition, {});
    player.ResetMovementState();
    player.ApplyPhysicsState(physicsWorld.GetPlayerPhysicsState());
    camera.SnapToTarget(player.Position());
}

void Application::RestartRun()
{
    physicsWorld.ResetMovingPlatform();
    physicsWorld.ResetDynamicBoxes();
    physicsWorld.ResetCharacter(levelDefinition.initialSpawnVisualCenter, {});
    player.ResetMovementState();
    player.ApplyPhysicsState(physicsWorld.GetPlayerPhysicsState());

    respawnState = gameplay::RespawnState{};
    respawnState.respawnPosition = levelDefinition.initialSpawnVisualCenter;
    levelCompletionState.completed = false;
    collectibleRunState =
        gameplay::MakeClearedCollectibleRunState(levelDefinition.collectibles.size());
    runTimerState = gameplay::RunTimerState{};
    camera.SnapToTarget(player.Position());
}

#if defined(PLATFORMER_ENABLE_DEBUG_UI)
void Application::SetLevelEditorActive(bool active)
{
    if (active && !levelEditorState.active)
    {
        // Opening is non-destructive: it reads the active definition and
        // touches neither gameplay nor any file. Unapplied edits from a
        // previous session are discarded so the toggle stays deterministic.
        levelEditorState.workingCopy = levelDefinition;
        levelEditorState.modified = false;
        editor::ClearCategoryStructuralPending(levelEditorState.structuralPending);
        editor::ResetStructuralIndexMap(levelEditorState.structuralMap, levelDefinition);
        editor::ResetLevelActionStatuses(levelEditorState);
        levelEditorState.lastMessage.clear();
        editor::SeedEditorCameraFromGameplay(
            levelEditorState.editorCamera,
            camera.Target(),
            camera.offset,
            camera.fieldOfViewY);
        if (!editor::IsValidSelection(
                levelEditorState.workingCopy, levelEditorState.selection))
        {
            levelEditorState.selection = editor::ClearSelection();
        }
        editor::ClearGizmoInteraction(levelEditorState.gizmo);
        editor::CancelAllEditorPlacement(
            levelEditorState.placementMode,
            levelEditorState.placementPointerBlocked,
            levelEditorState.staticPropPlacement);
#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
        editor::RefreshContentBrowser(
            levelEditorState.contentBrowser, editor::AuthoringSourceRoot());
#endif
    }
    if (!active && levelEditorState.active)
    {
        editor::ClearGizmoInteraction(levelEditorState.gizmo);
        editor::CancelAllEditorPlacement(
            levelEditorState.placementMode,
            levelEditorState.placementPointerBlocked,
            levelEditorState.staticPropPlacement);
        camera.SnapToTarget(player.Position());
        input::SetMouseLookActive(false);
        window.SetEscapeClosesWindow(true);
    }

    levelEditorState.active = active;
}

void Application::ImportStaticGlbAsset()
{
    // Import copies canonical source content. It must not touch workingCopy,
    // active, savedSourceBaseline, Modified, Dirty, or Save status.
    if (editorToolRunner.IsRunning() || cookStageReload.IsPending())
    {
        editorToolRunner.ReportLocalResult(
            editor::EditorToolKind::ImportStaticGlb,
            false,
            "error: a tool job is already running.");
        levelEditorState.workspace.showToolOutput = true;
        return;
    }

#if !defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
    editorToolRunner.ReportLocalResult(
        editor::EditorToolKind::ImportStaticGlb,
        false,
        "error: static GLB import is unavailable in this configuration.");
    levelEditorState.workspace.showToolOutput = true;
#else
    const std::filesystem::path sourceRoot = editor::AuthoringSourceRoot();
    if (sourceRoot.empty())
    {
        editorToolRunner.ReportLocalResult(
            editor::EditorToolKind::ImportStaticGlb,
            false,
            "error: authoring source root is unavailable.");
        levelEditorState.workspace.showToolOutput = true;
        return;
    }

    platform::OpenFileDialogRequest dialogRequest{};
    dialogRequest.title = "Import Static GLB";
    dialogRequest.filters.push_back({"Static GLB", "*.glb"});
    const platform::OpenFileDialogResult selected = platform::OpenSingleFileDialog(dialogRequest);
    if (selected.status == platform::OpenFileDialogStatus::Cancelled)
    {
        editorToolRunner.ReportLocalResult(
            editor::EditorToolKind::ImportStaticGlb,
            true,
            "Import Static GLB cancelled. Canonical source was not changed.");
        levelEditorState.workspace.showToolOutput = true;
        return;
    }
    if (selected.status != platform::OpenFileDialogStatus::Succeeded)
    {
        std::string message = "error: file selection failed.";
        if (!selected.message.empty())
        {
            message += " ";
            message += selected.message;
        }
        editorToolRunner.ReportLocalResult(
            editor::EditorToolKind::ImportStaticGlb, false, message);
        levelEditorState.workspace.showToolOutput = true;
        return;
    }

    const assets::StaticGlbImportResult imported = assets::ImportStaticGlb(
        selected.path, sourceRoot, &levelEditorState.contentBrowser.catalog);
    if (assets::StaticGlbImportSucceeded(imported.status))
    {
        editor::SelectContentBrowserIdentity(
            levelEditorState.contentBrowser, imported.canonicalIdentity);
        levelEditorState.contentBrowser.statusMessage = imported.message;
    }
    editorToolRunner.ReportLocalResult(
        editor::EditorToolKind::ImportStaticGlb,
        assets::StaticGlbImportSucceeded(imported.status),
        imported.message);
    levelEditorState.workspace.showToolOutput = true;
#endif
}

void Application::DeleteContentBrowserAsset()
{
    if (editorToolRunner.IsRunning() || cookStageReload.IsPending())
    {
        editorToolRunner.ReportLocalResult(
            editor::EditorToolKind::DeleteStaticModel,
            false,
            "error: a tool job is already running.");
        levelEditorState.workspace.showToolOutput = true;
        return;
    }

#if !defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
    editorToolRunner.ReportLocalResult(
        editor::EditorToolKind::DeleteStaticModel,
        false,
        "error: static-model delete is unavailable in this configuration.");
    levelEditorState.workspace.showToolOutput = true;
#else
    const std::filesystem::path sourceRoot = editor::AuthoringSourceRoot();
    const std::filesystem::path repositoryRoot = editor::RepositoryRoot();
    if (sourceRoot.empty() || repositoryRoot.empty())
    {
        editorToolRunner.ReportLocalResult(
            editor::EditorToolKind::DeleteStaticModel,
            false,
            "error: authoring source root or repository root is unavailable.");
        levelEditorState.workspace.showToolOutput = true;
        return;
    }

    assets::StaticModelDeleteRoots roots{};
    roots.sourceRoot = sourceRoot;
    roots.cookedRoot = editor::CookedAssetsRoot(repositoryRoot);
    roots.stagedRoots = editor::AuthorizedStaticModelStagedRoots(repositoryRoot);
    const std::string identity = levelEditorState.contentBrowser.selectedIdentity;
    if (levelEditorState.staticPropPlacement.modelIdentity == identity)
    {
        editor::CancelStaticPropPlacement(levelEditorState.staticPropPlacement);
    }
    if (editor::AuthoredLevelsProtectStaticPropIdentity(
            levelEditorState.workingCopy,
            levelDefinition,
            levelEditorState.savedSourceBaseline,
            identity))
    {
        const std::string message = editor::StaticPropReferencedDeleteMessage(identity);
        levelEditorState.contentBrowser.statusMessage = message;
        editorToolRunner.ReportLocalResult(
            editor::EditorToolKind::DeleteStaticModel, false, message);
        levelEditorState.workspace.showToolOutput = true;
        return;
    }
    const assets::StaticModelDeleteResult deleted =
        assets::DeleteStaticModel(identity, roots, &levelEditorState.contentBrowser.catalog);
    editor::RefreshContentBrowser(levelEditorState.contentBrowser, sourceRoot);
    if (assets::StaticModelDeleteSucceeded(deleted.status))
    {
        editor::ClearContentBrowserSelection(levelEditorState.contentBrowser);
        thumbnailStore.Forget(identity);
        editor::RemoveThumbnailCacheEntry(identity, editor::ThumbnailCacheRoot());
        modelPreview.Clear();
        levelEditorState.modelPreviewFramedIdentity.clear();
    }
    levelEditorState.contentBrowser.statusMessage = deleted.message;
    editorToolRunner.ReportLocalResult(
        editor::EditorToolKind::DeleteStaticModel,
        assets::StaticModelDeleteSucceeded(deleted.status),
        deleted.message);
    levelEditorState.workspace.showToolOutput = true;
#endif
}

bool Application::HandleLevelEditorRequest(editor::LevelEditorRequest request)
{
    // Edit > Add / Duplicate / Delete share this set. Listing cases here
    // independently is how AddDynamicBox previously fell through to Apply.
    if (editor::IsAuthoredLifecycleRequest(request))
    {
        editor::HandleAuthoredLifecycleRequest(
            levelEditorState,
            levelDefinition,
            request,
            editor::IsLevelAuthoringAvailable(),
            editor::EditorAddPlacementAnchor(levelEditorState.editorCamera));
        return true;
    }

    switch (request)
    {
    case editor::LevelEditorRequest::None:
        return true;
    case editor::LevelEditorRequest::RevertWorkingCopy:
        levelEditorState.workingCopy = levelDefinition;
        levelEditorState.modified = false;
        editor::ClearCategoryStructuralPending(levelEditorState.structuralPending);
        editor::ResetStructuralIndexMap(levelEditorState.structuralMap, levelDefinition);
        editor::CancelAllEditorPlacement(
            levelEditorState.placementMode,
            levelEditorState.placementPointerBlocked,
            levelEditorState.staticPropPlacement);
        levelEditorState.selection =
            editor::ReconcileSelection(levelEditorState.workingCopy, levelEditorState.selection);
        editor::ResetLevelActionStatuses(levelEditorState);
        levelEditorState.lastMessage = "Working copy reverted to the applied level.";
        editor::ClearGizmoInteraction(levelEditorState.gizmo);
        return true;
    case editor::LevelEditorRequest::SaveLevelSource:
        SaveLevelEditorSource();
        return true;
    case editor::LevelEditorRequest::ReloadRuntimeLevel:
        editor::ClearGizmoInteraction(levelEditorState.gizmo);
        return ReloadRuntimeLevelFromStaged();
    case editor::LevelEditorRequest::CookStageAndReload:
        return StartCookStageAndReload();
    case editor::LevelEditorRequest::ImportStaticGlb:
        ImportStaticGlbAsset();
        return true;
    case editor::LevelEditorRequest::DeleteContentBrowserAsset:
        DeleteContentBrowserAsset();
        return true;
    case editor::LevelEditorRequest::ApplyPreview:
        editor::ClearGizmoInteraction(levelEditorState.gizmo);
        break;
    default:
        break;
    }

    return ApplyLevelEditorPreview();
}

bool Application::ApplyLevelEditorPreview()
{
    // Validate the candidate while the live world is still intact, so an
    // invalid working copy cannot shut physics down or move the camera.
    if (!world::IsWritableLevelDefinition(levelEditorState.workingCopy))
    {
        editor::ResetLevelActionStatuses(levelEditorState);
        levelEditorState.lastApplyStatus = editor::LevelEditorApplyStatus::Invalid;
        levelEditorState.lastMessage =
            "Apply rejected: authored validation failed. Active level unchanged.";
        return true;
    }
    if (levelEditorState.workingCopy.id != world::kLevel01Id)
    {
        editor::ResetLevelActionStatuses(levelEditorState);
        levelEditorState.lastApplyStatus = editor::LevelEditorApplyStatus::Invalid;
        levelEditorState.lastMessage = "Apply rejected: Level ID must remain level_01.";
        return true;
    }

    const world::LevelDefinition candidate = levelEditorState.workingCopy;

    if (!physicsWorld.TryRebuild(
            candidate, candidate.initialSpawnVisualCenter, player.Size()))
    {
        editor::ResetLevelActionStatuses(levelEditorState);
        levelEditorState.lastApplyStatus = editor::LevelEditorApplyStatus::Error;
        levelEditorState.lastMessage = "Physics rebuild failed. Active level unchanged.";
        return true;
    }

    levelDefinition = candidate;
    ResetGameplayAfterCommittedLevel();

    // sessionBestTimeState, the persisted BEST, and the level-loading
    // diagnostics are intentionally untouched.
    levelEditorState.workingCopy = levelDefinition;
    levelEditorState.modified = false;
    editor::ClearCategoryStructuralPending(levelEditorState.structuralPending);
    editor::ResetStructuralIndexMap(levelEditorState.structuralMap, levelDefinition);
    levelEditorState.selection =
        editor::ReconcileSelection(levelEditorState.workingCopy, levelEditorState.selection);
    editor::ResetLevelActionStatuses(levelEditorState);
    levelEditorState.lastApplyStatus = editor::LevelEditorApplyStatus::Applied;
    levelEditorState.lastMessage =
        "Applied. Rendering and collision were rebuilt from the same authored data.";
    editor::CancelAllEditorPlacement(
        levelEditorState.placementMode,
        levelEditorState.placementPointerBlocked,
        levelEditorState.staticPropPlacement);
    return true;
}

bool Application::ReloadRuntimeLevelFromStaged()
{
    const bool authoringAvailable = editor::IsLevelAuthoringAvailable();
    const bool toolRunning = editorToolRunner.IsRunning();
    if (!editor::CanReloadRuntimeLevel(
            authoringAvailable,
            levelEditorState.modified,
            toolRunning,
            cookStageReload.IsPending()))
    {
        editor::ResetLevelActionStatuses(levelEditorState);
        levelEditorState.lastReloadStatus = editor::LevelEditorReloadStatus::Rejected;
        if (!authoringAvailable)
        {
            levelEditorState.lastMessage =
                "Reload Runtime Level is available in Development only. Active level unchanged.";
        }
        else if (levelEditorState.modified)
        {
            levelEditorState.lastMessage =
                "Reload rejected: apply or revert unapplied working-copy edits first. "
                "Active level unchanged.";
        }
        else if (toolRunning)
        {
            levelEditorState.lastMessage =
                "Reload rejected: a Build tool is still running. Active level unchanged.";
        }
        else
        {
            levelEditorState.lastMessage =
                "Reload rejected: Cook, Stage & Reload is still finishing. "
                "Active level unchanged.";
        }
        return true;
    }

    const std::filesystem::path stagedPath =
        platform::RuntimeAssetPath(world::kLevel01RuntimeLogicalId);
    const editor::RuntimeLevelReloadPrepareResult prepared =
        editor::PrepareRuntimeLevelReload(stagedPath, levelEditorState.modified);
    if (prepared.status != editor::RuntimeLevelReloadStatus::Ready)
    {
        editor::ResetLevelActionStatuses(levelEditorState);
        switch (prepared.status)
        {
        case editor::RuntimeLevelReloadStatus::RejectedModified:
            levelEditorState.lastReloadStatus = editor::LevelEditorReloadStatus::Rejected;
            break;
        case editor::RuntimeLevelReloadStatus::Missing:
            levelEditorState.lastReloadStatus = editor::LevelEditorReloadStatus::Missing;
            break;
        case editor::RuntimeLevelReloadStatus::Invalid:
        case editor::RuntimeLevelReloadStatus::UnsupportedVersion:
            levelEditorState.lastReloadStatus = editor::LevelEditorReloadStatus::Invalid;
            break;
        default:
            levelEditorState.lastReloadStatus = editor::LevelEditorReloadStatus::Error;
            break;
        }
        levelEditorState.lastMessage = prepared.message;
        return true;
    }

    if (!physicsWorld.TryRebuild(
            prepared.candidate,
            prepared.candidate.initialSpawnVisualCenter,
            player.Size()))
    {
        editor::ResetLevelActionStatuses(levelEditorState);
        levelEditorState.lastReloadStatus = editor::LevelEditorReloadStatus::Error;
        levelEditorState.lastMessage = "Physics rebuild failed. Active level unchanged.";
        return true;
    }

    levelDefinition = prepared.candidate;
    ResetGameplayAfterCommittedLevel();

    const editor::RuntimeLevelReloadReconcileResult reconciled =
        editor::ReconcileAfterRuntimeLevelReload(
            levelDefinition, levelEditorState.savedSourceBaseline);
    levelEditorState.workingCopy = reconciled.workingCopy;
    levelEditorState.modified = reconciled.modified;
    levelEditorState.dirty = reconciled.dirty;
    levelEditorState.selection = reconciled.selection;
    editor::ClearCategoryStructuralPending(levelEditorState.structuralPending);
    editor::ResetStructuralIndexMap(levelEditorState.structuralMap, levelDefinition);
    editor::ClearGizmoInteraction(levelEditorState.gizmo);
    editor::CancelAllEditorPlacement(
        levelEditorState.placementMode,
        levelEditorState.placementPointerBlocked,
        levelEditorState.staticPropPlacement);
    editor::ResetLevelActionStatuses(levelEditorState);
    levelEditorState.lastReloadStatus = editor::LevelEditorReloadStatus::Reloaded;
    levelEditorState.lastMessage =
        "Reloaded staged runtime level. Source, cooked, and staged files were not written.";
    return true;
}

bool Application::StartCookStageAndReload()
{
    const bool authoringAvailable = editor::IsLevelAuthoringAvailable();
    if (!editor::CanStartCookStageReload(
            authoringAvailable,
            levelEditorState.modified,
            editorToolRunner.IsRunning(),
            cookStageReload.IsPending()))
    {
        cookStageReload.MarkRejected();
        editor::ResetLevelActionStatuses(levelEditorState);
        if (!authoringAvailable)
        {
            levelEditorState.lastMessage =
                "Cook, Stage & Reload is available in Development only.";
        }
        else if (levelEditorState.modified)
        {
            levelEditorState.lastMessage =
                "Cook, Stage & Reload rejected: apply or revert unapplied working-copy "
                "edits first.";
        }
        else if (cookStageReload.IsPending())
        {
            levelEditorState.lastMessage =
                "Cook, Stage & Reload rejected: the workflow is still finishing.";
        }
        else
        {
            levelEditorState.lastMessage =
                "Cook, Stage & Reload rejected: a Build tool is still running.";
        }
        return true;
    }

    if (!editorToolRunner.TryStart(
            editor::EditorToolKind::CookAndStage,
            editor::RepositoryRoot(),
            editor::IsEditorToolExecutionAvailable()))
    {
        cookStageReload.MarkRejected();
        editor::ResetLevelActionStatuses(levelEditorState);
        levelEditorState.lastMessage =
            "Cook, Stage & Reload rejected: could not start Cook & Stage.";
        return true;
    }

    levelEditorState.workspace.showToolOutput = true;
    if (!editorToolRunner.IsRunning())
    {
        editor::ResetLevelActionStatuses(levelEditorState);
        levelEditorState.lastMessage =
            "Cook, Stage & Reload stopped: Cook & Stage failed to start. See Tool Output.";
        return true;
    }

    if (!cookStageReload.BeginWaiting())
    {
        cookStageReload.MarkRejected();
        editor::ResetLevelActionStatuses(levelEditorState);
        levelEditorState.lastMessage =
            "Cook, Stage & Reload rejected: workflow was already pending. "
            "Cook & Stage continues in Tool Output.";
        return true;
    }

    editor::ResetLevelActionStatuses(levelEditorState);
    levelEditorState.lastMessage =
        "Cook, Stage & Reload running. See Tool Output for Cook & Stage.";
    return true;
}

void Application::FinishCookStageAndReloadIfReady()
{
    if (!cookStageReload.TakeReloadRequest())
    {
        return;
    }

    ReloadRuntimeLevelFromStaged();
    const bool succeeded =
        levelEditorState.lastReloadStatus == editor::LevelEditorReloadStatus::Reloaded;
    cookStageReload.NotifyReloadFinished(succeeded);
    if (succeeded)
    {
        levelEditorState.lastMessage =
            "Cook, Stage & Reload completed. Staged runtime level is now loaded.";
    }
    else
    {
        levelEditorState.lastMessage =
            "Cook & Stage succeeded, but Reload Runtime Level failed. "
            + levelEditorState.lastMessage;
    }
}

void Application::ResetGameplayAfterCommittedLevel()
{
    player.ResetMovementState();
    player.ApplyPhysicsState(physicsWorld.GetPlayerPhysicsState());
    respawnState = gameplay::RespawnState{};
    respawnState.respawnPosition = levelDefinition.initialSpawnVisualCenter;
    levelCompletionState = gameplay::LevelCompletionState{};
    collectibleRunState =
        gameplay::MakeClearedCollectibleRunState(levelDefinition.collectibles.size());
    runTimerState = gameplay::RunTimerState{};
    camera.ApplyLevelFraming(
        levelDefinition.camera.offset, levelDefinition.camera.fieldOfViewY);
    camera.Initialize(player.Position());
}

void Application::SaveLevelEditorSource()
{
    // Always the active/applied definition, never the working copy.
    const editor::LevelEditorSaveResult result = editor::SaveLevelSource(levelDefinition);
    editor::ResetLevelActionStatuses(levelEditorState);
    levelEditorState.lastSaveStatus = result.status;
    if (result.status == editor::LevelEditorSaveStatus::Saved)
    {
        levelEditorState.savedSourceBaseline = levelDefinition;
        levelEditorState.lastMessage =
            "Source saved. Cook & Stage, then Reload Runtime Level, to load staged data. "
            "Source only was written.";
        return;
    }

    levelEditorState.lastMessage = result.message.empty()
        ? std::string("Save failed. Source left unchanged.")
        : "Save failed: " + result.message + ". Source left unchanged.";
}
#endif

void Application::Shutdown()
{
#if defined(PLATFORMER_ENABLE_DEBUG_UI)
    input::SetMouseLookActive(false);
    cookStageReload.Cancel();
    editorToolRunner.Shutdown();
#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
    thumbnailStore.Shutdown();
    modelPreview.Shutdown();
#endif
    debugUi.Shutdown();
#endif
    physicsWorld.Shutdown();
    renderer.UnloadRuntimeAssets();
    window.Shutdown();
    initialized = false;
}
}
