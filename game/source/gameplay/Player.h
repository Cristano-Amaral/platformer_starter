#pragma once

#include "core/Vec3.h"
#include "input/InputState.h"
#include "physics/PhysicsWorld.h"

namespace gameplay
{
class Player
{
public:
    Player(core::Vec3 position, core::Vec3 size);

    void Update(
        const input::InputState& input,
        float deltaSeconds,
        physics::PhysicsWorld& physicsWorld);
    void ApplyPhysicsState(const physics::PlayerPhysicsState& state);
    void ResetMovementState();

    const core::Vec3& Position() const;
    const core::Vec3& Size() const;
    float HorizontalVelocity() const;
    float VerticalVelocity() const;
    bool IsGrounded() const;
    float TimeSinceGrounded() const;
    bool IsCoyoteAvailable() const;
    float JumpBufferRemaining() const;
    physics::PlayerGroundSupport GroundSupport() const;
    int PhysicsContactCount() const;
    bool CharacterVirtualInitialized() const;
    core::Vec3 GroundVelocity() const;
    bool IsSupportingGroundMoving() const;
    core::Vec3 GroundNormal() const;
    float GroundSlopeAngleDegrees() const;
    bool IsCurrentSupportWalkable() const;

    static constexpr float kMaxMoveSpeed = 6.0f;
    static constexpr float kAcceleration = 40.0f;
    static constexpr float kDeceleration = 50.0f;
    static constexpr float kJumpSpeed = 8.0f;
    static constexpr float kGravity = 20.0f;
    static constexpr float kCoyoteTime = 0.10f;
    static constexpr float kJumpBufferTime = 0.10f;

private:
    bool CanJump() const;
    bool TryJump();
    void UpdateHorizontalVelocity(float moveX, float deltaSeconds);
    void UpdateCoyoteState(float deltaSeconds);

    core::Vec3 position;
    core::Vec3 size;
    float horizontalVelocity = 0.0f;
    float verticalVelocity = 0.0f;
    bool grounded = true;
    bool coyoteAvailable = true;
    float timeSinceGrounded = 0.0f;
    float jumpBufferRemaining = 0.0f;
    physics::PlayerGroundSupport groundSupport = physics::PlayerGroundSupport::OnGround;
    int physicsContactCount = 0;
    bool characterVirtualInitialized = false;
    core::Vec3 groundVelocity{};
    bool supportingGroundMoving = false;
    core::Vec3 groundNormal{};
    float groundSlopeAngleDegrees = 0.0f;
    bool currentSupportWalkable = false;
};
}
