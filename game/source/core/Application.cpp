#include "core/Application.h"

#include "input/Input.h"
#include "platform/Time.h"

#include <cmath>

#if defined(PLATFORMER_ENABLE_DEBUG_UI)
#include "ui/debug/DebugMetrics.h"
#endif

namespace core
{
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

    snapshot.checkpointActive = respawnState.checkpointActive;
    snapshot.respawnPosition = respawnState.respawnPosition;
    snapshot.killPlaneY = world::kKillPlaneY;
    snapshot.deathCount = respawnState.deathCount;
    snapshot.lastRespawnReason = RespawnReasonName(respawnState.lastRespawnReason);
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
        physicsWorld.UpdateMovingPlatform(deltaSeconds);
        player.Update(inputState, deltaSeconds, physicsWorld);

        bool respawnedThisFrame = false;
        if (player.Position().y < world::kKillPlaneY)
        {
            PerformRespawn(gameplay::RespawnReason::Fall);
            respawnedThisFrame = true;
        }
        else if (inputState.respawnPressed)
        {
            PerformRespawn(gameplay::RespawnReason::Manual);
            respawnedThisFrame = true;
        }
        else if (!respawnState.checkpointActive
            && world::PointInsideCheckpoint(world::kCheckpoint, player.Position()))
        {
            respawnState.checkpointActive = true;
            respawnState.respawnPosition = world::kCheckpoint.respawnPosition;
        }

        physicsWorld.Update(deltaSeconds);
        if (!respawnedThisFrame)
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
            respawnState.checkpointActive);
#if defined(PLATFORMER_ENABLE_DEBUG_UI)
        debugUi.Draw(
            MakeDebugMetricsSnapshot(
                player,
                camera,
                physicsWorld,
                inputState,
                renderer,
                respawnState,
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
    if (reason == gameplay::RespawnReason::Fall)
    {
        ++respawnState.deathCount;
    }
    respawnState.lastRespawnReason = reason;

    physicsWorld.ResetCharacter(respawnState.respawnPosition, {});
    player.ResetMovementState();
    player.ApplyPhysicsState(physicsWorld.GetPlayerPhysicsState());
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
