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
}

ui::DebugMetricsSnapshot MakeDebugMetricsSnapshot(
    const gameplay::Player& player,
    const gameplay::PlatformerCamera& camera,
    const physics::PhysicsWorld& physicsWorld,
    const input::InputState& inputState,
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
        physicsWorld.Update(deltaSeconds);
        camera.Update(player.Position(), deltaSeconds);

        const physics::DynamicTestBox testBox = physicsWorld.GetDynamicTestBox();
        const physics::MovingPlatformState movingPlatform = physicsWorld.GetMovingPlatform();
        renderer.BeginFrame();
        renderer.DrawWorld(
            player,
            camera,
            testBox.position,
            testBox.size,
            movingPlatform.position,
            movingPlatform.size);
#if defined(PLATFORMER_ENABLE_DEBUG_UI)
        debugUi.Draw(
            MakeDebugMetricsSnapshot(player, camera, physicsWorld, inputState, deltaSeconds));
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

    if (!physicsWorld.Initialize())
    {
        window.Shutdown();
        initialized = false;
        return;
    }

    if (!physicsWorld.InitializePlayer(player.Position(), player.Size()))
    {
        physicsWorld.Shutdown();
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

void Application::Shutdown()
{
#if defined(PLATFORMER_ENABLE_DEBUG_UI)
    debugUi.Shutdown();
#endif
    physicsWorld.Shutdown();
    window.Shutdown();
    initialized = false;
}
}
