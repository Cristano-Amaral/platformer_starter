#include "core/Application.h"

#include "gameplay/CollectibleRunState.h"
#include "input/Input.h"
#include "platform/Time.h"
#include "world/CollectibleWorld.h"
#include "world/HazardWorld.h"
#include "world/LevelGoal.h"

#include <array>
#include <cmath>
#include <cstddef>

static_assert(world::kHazardCount == 2);
static_assert(world::kCollectibleCount == 3);
static_assert(gameplay::CollectedCount(gameplay::CollectibleRunState{}) == 0);

#if defined(PLATFORMER_ENABLE_DEBUG_UI)
#include "ui/debug/DebugMetrics.h"
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
}

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
    snapshot.killPlaneY = world::kKillPlaneY;
    snapshot.deathCount = respawnState.deathCount;
    snapshot.lastRespawnReason = RespawnReasonName(respawnState.lastRespawnReason);
    snapshot.activeCheckpointLabel = ActiveCheckpointLabel(respawnState.activeCheckpointIndex);
    snapshot.checkpoint1Inside =
        world::PointInsideCheckpoint(world::kCheckpoints[0], player.Position());
    snapshot.checkpoint1VisualState = CheckpointVisualStateName(
        world::CheckpointVisualStateForIndex(0, respawnState.activeCheckpointIndex));
    snapshot.checkpoint2Inside =
        world::PointInsideCheckpoint(world::kCheckpoints[1], player.Position());
    snapshot.checkpoint2VisualState = CheckpointVisualStateName(
        world::CheckpointVisualStateForIndex(1, respawnState.activeCheckpointIndex));

    snapshot.insideHazardLabel = HazardIndexLabel(
        world::FindHazardIndexContaining(player.Position()));
    snapshot.hazardContactThisFrame = hazardContactThisFrame;

    snapshot.collectedCount = gameplay::CollectedCount(collectibleRunState);
    snapshot.collectedThisFrameLabel = CollectibleIndexLabel(collectedThisFrameIndex);
    for (int index = 0; index < world::kCollectibleCount; ++index)
    {
        snapshot.collectibleCollected[static_cast<std::size_t>(index)] =
            collectibleRunState.collected[static_cast<std::size_t>(index)];
        snapshot.collectibleInside[static_cast<std::size_t>(index)] =
            world::PointInsideCollectible(
                world::kCollectibles[static_cast<std::size_t>(index)],
                player.Position());
    }

    snapshot.levelCompleted = levelCompletionState.completed;
    snapshot.goalCenter = world::kLevelGoal.center;
    snapshot.goalSize = world::kLevelGoal.size;
    snapshot.playerInsideGoal =
        world::PointInsideGoal(world::kLevelGoal, player.Position());
    snapshot.restartAvailable = levelCompletionState.completed;
    snapshot.restartedThisFrame = restartedThisFrame;
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

    while (!window.ShouldClose())
    {
        const float deltaSeconds = platform::DeltaSeconds();
        const input::InputState inputState = input::Poll();
        const bool restartAvailableAtFrameStart = levelCompletionState.completed;
        physicsWorld.UpdateMovingPlatform(deltaSeconds);
        player.Update(inputState, deltaSeconds, physicsWorld);

        const bool hazardContactThisFrame =
            world::FindHazardIndexContaining(player.Position()) != world::kNoHazardIndex;

        int collectedThisFrameIndex = world::kNoCollectibleIndex;
        bool respawnedThisFrame = false;
        if (player.Position().y < world::kKillPlaneY)
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
                       world::kCheckpoints[static_cast<std::size_t>(expectedIndex)],
                       player.Position()))
            {
                respawnState.activeCheckpointIndex = expectedIndex;
                respawnState.respawnPosition =
                    world::kCheckpoints[static_cast<std::size_t>(expectedIndex)].respawnPosition;
            }

            if (!levelCompletionState.completed
                && world::PointInsideGoal(world::kLevelGoal, player.Position()))
            {
                levelCompletionState.completed = true;
            }

            if (!(restartAvailableAtFrameStart && inputState.restartPressed))
            {
                const int collectibleIndex =
                    gameplay::FindAvailableCollectibleIndexContaining(
                        player.Position(),
                        collectibleRunState);
                if (collectibleIndex != world::kNoCollectibleIndex)
                {
                    collectibleRunState.collected[static_cast<std::size_t>(collectibleIndex)] =
                        true;
                    collectedThisFrameIndex = collectibleIndex;
                }
            }
        }

        bool restartedThisFrame = false;
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

        const physics::DynamicTestBox testBox = physicsWorld.GetDynamicTestBox();
        const physics::MovingPlatformState movingPlatform = physicsWorld.GetMovingPlatform();
        renderer.BeginFrame();
        renderer.DrawWorld(
            player,
            camera,
            testBox.position,
            testBox.size,
            movingPlatform.position,
            movingPlatform.size,
            MakeCheckpointVisuals(respawnState.activeCheckpointIndex),
            levelCompletionState.completed,
            collectibleRunState.collected,
            gameplay::CollectedCount(collectibleRunState));
#if defined(PLATFORMER_ENABLE_DEBUG_UI)
        debugUi.Draw(
            MakeDebugMetricsSnapshot(
                player,
                camera,
                physicsWorld,
                inputState,
                renderer,
                respawnState,
                levelCompletionState,
                collectibleRunState,
                restartedThisFrame,
                hazardContactThisFrame,
                collectedThisFrameIndex,
                deltaSeconds));
#endif
        renderer.EndFrame();
    }

    Shutdown();
    return 0;
}

void Application::Initialize()
{
    if (!window.Initialize())
    {
        initialized = false;
        return;
    }

    renderer.LoadRuntimeAssets();

    if (!physicsWorld.Initialize())
    {
        renderer.UnloadRuntimeAssets();
        window.Shutdown();
        initialized = false;
        return;
    }

    if (!physicsWorld.InitializePlayer(player.Position(), player.Size()))
    {
        physicsWorld.Shutdown();
        renderer.UnloadRuntimeAssets();
        window.Shutdown();
        initialized = false;
        return;
    }

    player.ApplyPhysicsState(physicsWorld.GetPlayerPhysicsState());
    camera.Initialize(player.Position());
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
    physicsWorld.ResetCharacter(world::kInitialSpawnVisualCenter, {});
    player.ResetMovementState();
    player.ApplyPhysicsState(physicsWorld.GetPlayerPhysicsState());

    respawnState = gameplay::RespawnState{};
    levelCompletionState.completed = false;
    collectibleRunState = gameplay::CollectibleRunState{};
    camera.SnapToTarget(player.Position());
}

void Application::Shutdown()
{
#if defined(PLATFORMER_ENABLE_DEBUG_UI)
    debugUi.Shutdown();
#endif
    physicsWorld.Shutdown();
    renderer.UnloadRuntimeAssets();
    window.Shutdown();
    initialized = false;
}
}
