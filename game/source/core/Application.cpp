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

static_assert(world::kHazardCount == 2);
static_assert(world::kCollectibleCount == 3);
static_assert(gameplay::CollectedCount(gameplay::CollectibleRunState{}) == 0);
static_assert(!gameplay::SessionBestTimeState{}.hasBestTime);
static_assert(core::RunTimePartsEqual(core::RunTimePartsFromSeconds(0.0), 0, 0, 0));

#if defined(PLATFORMER_ENABLE_DEBUG_UI)
#include "editor/EditorCamera.h"
#include "editor/EditorInput.h"
#include "editor/EditorPicking.h"
#include "editor/LevelEditor.h"
#include "ui/debug/DebugMetrics.h"
#include "world/LevelWriter.h"

#include <string>
#endif

namespace core
{
namespace
{
std::array<world::CheckpointVisualState, world::kCheckpointCount> MakeCheckpointVisuals(
    int activeCheckpointIndex)
{
    return {
        world::CheckpointVisualStateForIndex(0, activeCheckpointIndex),
        world::CheckpointVisualStateForIndex(1, activeCheckpointIndex),
    };
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
    switch (activeCheckpointIndex)
    {
    case 0:
        return "1";
    case 1:
        return "2";
    default:
        return "None";
    }
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
    switch (hazardIndex)
    {
    case 0:
        return "1";
    case 1:
        return "2";
    default:
        return "None";
    }
}

const char* CollectibleIndexLabel(int collectibleIndex)
{
    switch (collectibleIndex)
    {
    case 0:
        return "1";
    case 1:
        return "2";
    case 2:
        return "3";
    default:
        return "None";
    }
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

    const physics::DynamicTestBox testBox = physicsWorld.GetDynamicTestBox();
    snapshot.physicsInitialized = physicsWorld.IsInitialized();
    snapshot.physicsTestBoxPosition = testBox.position;
    snapshot.physicsTestBoxLinearVelocity = testBox.linearVelocity;
    snapshot.physicsTestBoxActive = testBox.active;
    snapshot.staticBodyCount = physicsWorld.StaticBodyCount();
    snapshot.dynamicTestBodyValid = testBox.valid;

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
    snapshot.checkpoint1Inside =
        world::PointInsideCheckpoint(level.checkpoints[0], player.Position());
    snapshot.checkpoint1VisualState = CheckpointVisualStateName(
        world::CheckpointVisualStateForIndex(0, respawnState.activeCheckpointIndex));
    snapshot.checkpoint2Inside =
        world::PointInsideCheckpoint(level.checkpoints[1], player.Position());
    snapshot.checkpoint2VisualState = CheckpointVisualStateName(
        world::CheckpointVisualStateForIndex(1, respawnState.activeCheckpointIndex));

    snapshot.insideHazardLabel = HazardIndexLabel(
        world::FindHazardIndexContaining(player.Position(), level.hazards));
    snapshot.hazardContactThisFrame = hazardContactThisFrame;

    snapshot.collectedCount = gameplay::CollectedCount(collectibleRunState);
    snapshot.collectedThisFrameLabel = CollectibleIndexLabel(collectedThisFrameIndex);
    for (int index = 0; index < world::kCollectibleCount; ++index)
    {
        snapshot.collectibleCollected[static_cast<std::size_t>(index)] =
            collectibleRunState.collected[static_cast<std::size_t>(index)];
        snapshot.collectibleInside[static_cast<std::size_t>(index)] =
            world::PointInsideCollectible(
                level.collectibles[static_cast<std::size_t>(index)],
                player.Position());
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
    snapshot.levelHasDynamicBox = level.dynamicBox.size.x > 0.0f && level.dynamicBox.size.y > 0.0f
        && level.dynamicBox.size.z > 0.0f && level.dynamicBox.mass > 0.0f;
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
                const int expectedIndex =
                    world::NextExpectedCheckpointIndex(respawnState.activeCheckpointIndex);
                if (world::IsValidCheckpointIndex(expectedIndex)
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

            physicsWorld.Update(deltaSeconds);
            if (!respawnedThisFrame && !restartedThisFrame)
            {
                camera.Update(player.Position(), deltaSeconds);
            }
        }

        const physics::DynamicTestBox testBox = physicsWorld.GetDynamicTestBox();
        const physics::MovingPlatformState movingPlatform = physicsWorld.GetMovingPlatform();
        render::CameraView cameraView = MakeGameplayCameraView(camera);
        render::DebugWorldOverlay overlay{};
#if defined(PLATFORMER_ENABLE_DEBUG_UI)
        editor::EditorInputState editorInput{};
        if (levelEditorState.active)
        {
            editorInput = editor::PollEditorInput();
            const bool applyLook =
                !debugUi.WantsMouseCapture() && !levelEditorState.gizmo.dragging;
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

            const editor::EditorPickingWorldState pickingWorld{
                movingPlatform.position,
                movingPlatform.size,
                testBox.position,
                testBox.size};
            const editor::EditorHighlightRequest highlight = editor::MakeHighlightRequest(
                levelEditorState.selection,
                editor::BuildPickingSet(levelDefinition, pickingWorld));
            overlay.drawHighlight = highlight.visible;
            overlay.highlightCenter = highlight.center;
            overlay.highlightSize = highlight.size;
            overlay.highlightRotationZDegrees = highlight.rotationZDegrees;

            const editor::EditorPendingTransformPreview pending =
                editor::MakePendingTransformPreview(
                    levelEditorState.selection,
                    levelDefinition,
                    levelEditorState.workingCopy);
            overlay.drawPendingPreview = pending.visible;
            overlay.pendingPreviewCenter = pending.center;
            overlay.pendingPreviewSize = pending.size;

            const editor::GizmoDrawRequest gizmo = editor::MakeGizmoDrawRequest(
                levelEditorState.selection,
                levelEditorState.workingCopy,
                cameraView,
                levelEditorState.gizmo);
            overlay.drawTranslationGizmo = gizmo.visible;
            overlay.gizmoOrigin = gizmo.origin;
            overlay.gizmoAxisLength = gizmo.axisLength;
            overlay.gizmoHoveredAxis = static_cast<int>(gizmo.hovered);
            overlay.gizmoActiveAxis = static_cast<int>(gizmo.active);
        }
        else
        {
            input::SetMouseLookActive(false);
        }
#endif
        renderer.BeginFrame();
        renderer.DrawWorld(
            player,
            cameraView,
            levelDefinition,
            testBox.position,
            testBox.size,
            movingPlatform.position,
            movingPlatform.size,
            MakeCheckpointVisuals(respawnState.activeCheckpointIndex),
            levelCompletionState.completed,
            collectibleRunState.collected,
            gameplay::CollectedCount(collectibleRunState),
            runTimerState.elapsedSeconds,
            sessionBestTimeState.hasBestTime,
            sessionBestTimeState.bestSeconds,
            overlay);
#if defined(PLATFORMER_ENABLE_DEBUG_UI)
        const editor::LevelEditorViewContext levelEditorView{
            runtimeLevelPathDisplay.c_str(),
            movingPlatform.position,
            testBox.position,
            static_cast<float>(window.Width()),
            static_cast<float>(window.Height()),
            false};
        const editor::LevelEditorRequest editorRequest = debugUi.Draw(
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
            levelEditorView);
        if (levelEditorState.active)
        {
            // Keyboard move, wheel and world pick use this frame's ImGui capture
            // so typing into Inspector and clicking/scrolling panels cannot
            // drive the viewport behind them.
            const bool mouseCaptured = debugUi.WantsMouseCapture();
            const bool keyboardCaptured = debugUi.WantsKeyboardCapture();
            editor::UpdateEditorCamera(
                levelEditorState.editorCamera,
                editorInput,
                deltaSeconds,
                false,
                !keyboardCaptured,
                !mouseCaptured);
            const editor::Ray3 ray = editor::ScreenToWorldRay(
                cameraView,
                editorInput.mouseX,
                editorInput.mouseY,
                static_cast<float>(window.Width()),
                static_cast<float>(window.Height()));
            const bool gizmoConsumedPointer = editor::UpdateGizmoInteraction(
                levelEditorState.gizmo,
                levelEditorState.selection,
                levelEditorState.workingCopy,
                cameraView,
                ray,
                mouseCaptured,
                editorInput.lookHeld,
                editorInput.selectPressed,
                editorInput.selectHeld,
                editorInput.selectReleased);
            if (!mouseCaptured && editorInput.selectPressed && !editorInput.lookHeld
                && !gizmoConsumedPointer)
            {
                const editor::EditorPickingWorldState pickingWorld{
                    movingPlatform.position,
                    movingPlatform.size,
                    testBox.position,
                    testBox.size};
                levelEditorState.selection = editor::PickNearest(
                    ray, editor::BuildPickingSet(levelDefinition, pickingWorld));
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
    physicsWorld.ResetDynamicTestBox();
    physicsWorld.ResetCharacter(levelDefinition.initialSpawnVisualCenter, {});
    player.ResetMovementState();
    player.ApplyPhysicsState(physicsWorld.GetPlayerPhysicsState());

    respawnState = gameplay::RespawnState{};
    respawnState.respawnPosition = levelDefinition.initialSpawnVisualCenter;
    levelCompletionState.completed = false;
    collectibleRunState = gameplay::CollectibleRunState{};
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
        levelEditorState.lastApplyStatus = editor::LevelEditorApplyStatus::NotAttempted;
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
    }
    if (!active && levelEditorState.active)
    {
        editor::ClearGizmoInteraction(levelEditorState.gizmo);
        camera.SnapToTarget(player.Position());
        input::SetMouseLookActive(false);
    }

    levelEditorState.active = active;
}

bool Application::HandleLevelEditorRequest(editor::LevelEditorRequest request)
{
    switch (request)
    {
    case editor::LevelEditorRequest::None:
        return true;
    case editor::LevelEditorRequest::RevertWorkingCopy:
        levelEditorState.workingCopy = levelDefinition;
        levelEditorState.modified = false;
        levelEditorState.lastApplyStatus = editor::LevelEditorApplyStatus::NotAttempted;
        levelEditorState.lastMessage = "Working copy reverted to the applied level.";
        editor::ClearGizmoInteraction(levelEditorState.gizmo);
        return true;
    case editor::LevelEditorRequest::SaveLevelSource:
        SaveLevelEditorSource();
        return true;
    case editor::LevelEditorRequest::ApplyPreview:
        editor::ClearGizmoInteraction(levelEditorState.gizmo);
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
        levelEditorState.lastApplyStatus = editor::LevelEditorApplyStatus::Invalid;
        levelEditorState.lastMessage =
            "Apply rejected: authored validation failed. Active level unchanged.";
        return true;
    }
    if (levelEditorState.workingCopy.id != world::kLevel01Id)
    {
        levelEditorState.lastApplyStatus = editor::LevelEditorApplyStatus::Invalid;
        levelEditorState.lastMessage = "Apply rejected: Level ID must remain level_01.";
        return true;
    }

    const world::LevelDefinition candidate = levelEditorState.workingCopy;

    // Rebuild before committing: if Jolt cannot build the candidate, the
    // active definition never describes a world that failed to exist.
    physicsWorld.Shutdown();
    if (!physicsWorld.Initialize(candidate))
    {
        std::fprintf(
            stderr,
            "Level editor: PhysicsWorld::Initialize failed during Apply Preview. "
            "Physics cannot be restored; shutting down.\n");
        levelEditorState.lastApplyStatus = editor::LevelEditorApplyStatus::Error;
        levelEditorState.lastMessage = "Physics rebuild failed. Shutting down.";
        return false;
    }
    if (!physicsWorld.InitializePlayer(candidate.initialSpawnVisualCenter, player.Size()))
    {
        std::fprintf(
            stderr,
            "Level editor: PhysicsWorld::InitializePlayer failed during Apply Preview. "
            "Physics cannot be restored; shutting down.\n");
        levelEditorState.lastApplyStatus = editor::LevelEditorApplyStatus::Error;
        levelEditorState.lastMessage = "Player rebuild failed. Shutting down.";
        return false;
    }

    levelDefinition = candidate;

    // Editor-only fresh preview run. Deliberately not RestartRun: the whole
    // level was rebuilt, not just reset.
    player.ResetMovementState();
    player.ApplyPhysicsState(physicsWorld.GetPlayerPhysicsState());
    respawnState = gameplay::RespawnState{};
    respawnState.respawnPosition = levelDefinition.initialSpawnVisualCenter;
    levelCompletionState = gameplay::LevelCompletionState{};
    collectibleRunState = gameplay::CollectibleRunState{};
    runTimerState = gameplay::RunTimerState{};
    camera.ApplyLevelFraming(
        levelDefinition.camera.offset, levelDefinition.camera.fieldOfViewY);
    camera.Initialize(player.Position());

    // sessionBestTimeState, the persisted BEST, and the level-loading
    // diagnostics are intentionally untouched.
    levelEditorState.workingCopy = levelDefinition;
    levelEditorState.modified = false;
    levelEditorState.lastApplyStatus = editor::LevelEditorApplyStatus::Applied;
    levelEditorState.lastMessage =
        "Applied. Rendering and collision were rebuilt from the same authored data.";
    return true;
}

void Application::SaveLevelEditorSource()
{
    // Always the active/applied definition, never the working copy.
    const editor::LevelEditorSaveResult result = editor::SaveLevelSource(levelDefinition);
    levelEditorState.lastSaveStatus = result.status;
    if (result.status == editor::LevelEditorSaveStatus::Saved)
    {
        levelEditorState.savedSourceBaseline = levelDefinition;
        levelEditorState.lastMessage =
            "Source saved. Run cooker/build to update cooked/staged runtime files.";
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
    debugUi.Shutdown();
#endif
    physicsWorld.Shutdown();
    renderer.UnloadRuntimeAssets();
    window.Shutdown();
    initialized = false;
}
}
