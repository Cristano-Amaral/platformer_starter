#include "gameplay/Player.h"

namespace gameplay
{
namespace
{
float MoveToward(float current, float target, float maxStep)
{
    const float delta = target - current;
    if (delta > maxStep)
    {
        return current + maxStep;
    }
    if (delta < -maxStep)
    {
        return current - maxStep;
    }
    return target;
}

float Clamp(float value, float minValue, float maxValue)
{
    if (value < minValue)
    {
        return minValue;
    }
    if (value > maxValue)
    {
        return maxValue;
    }
    return value;
}
}

Player::Player(core::Vec3 position, core::Vec3 size)
    : position(position)
    , size(size)
{
}

void Player::Update(
    const input::InputState& input,
    float deltaSeconds,
    physics::PhysicsWorld& physicsWorld)
{
    if (input.jumpPressed)
    {
        jumpBufferRemaining = kJumpBufferTime;
    }

    UpdateHorizontalVelocity(input.moveX, deltaSeconds);

    // Jump if already eligible (grounded or coyote) so this frame still moves upward.
    const bool jumped = TryJump();

    // CharacterVirtual::Update does not cancel mLinearVelocity against the floor.
    // Integrating gravity while supported would accumulate a large downward speed
    // that is inherited when walking off a ledge.
    if (!grounded)
    {
        verticalVelocity -= kGravity * deltaSeconds;
    }

    physicsWorld.MovePlayer({horizontalVelocity, verticalVelocity}, deltaSeconds);
    ApplyPhysicsState(physicsWorld.GetPlayerPhysicsState());

    if (jumped)
    {
        grounded = false;
        coyoteAvailable = false;
    }

    UpdateCoyoteState(deltaSeconds);

    // Buffered jump after this frame's landing, without a second independent Y integrate.
    TryJump();

    if (jumpBufferRemaining > 0.0f)
    {
        jumpBufferRemaining -= deltaSeconds;
        if (jumpBufferRemaining < 0.0f)
        {
            jumpBufferRemaining = 0.0f;
        }
    }
}

void Player::ApplyPhysicsState(const physics::PlayerPhysicsState& state)
{
    position = state.visualCenter;
    // World X from CharacterVirtual includes moving-ground velocity. Keep
    // gameplay horizontalVelocity as Player-relative accel/decel state.
    verticalVelocity = state.verticalVelocity;
    groundSupport = state.groundSupport;
    physicsContactCount = state.contactCount;
    characterVirtualInitialized = state.characterInitialized;
    groundVelocity = state.groundVelocity;
    supportingGroundMoving = state.supportingGroundMoving;
    groundNormal = state.groundNormal;
    groundSlopeAngleDegrees = state.groundSlopeAngleDegrees;
    currentSupportWalkable = state.currentSupportWalkable;
    // Residual OnGround during jump takeoff must not cancel a positive launch.
    // Only downward/non-positive vertical speed is cleared on valid support.
    if (state.supported && verticalVelocity <= 0.0f)
    {
        verticalVelocity = 0.0f;
    }
    grounded = state.supported && verticalVelocity <= 0.0f;
}

bool Player::CanJump() const
{
    if (grounded)
    {
        return true;
    }
    return coyoteAvailable && timeSinceGrounded <= kCoyoteTime;
}

bool Player::TryJump()
{
    if (jumpBufferRemaining <= 0.0f || !CanJump())
    {
        return false;
    }

    verticalVelocity = kJumpSpeed;
    grounded = false;
    coyoteAvailable = false;
    jumpBufferRemaining = 0.0f;
    return true;
}

void Player::UpdateHorizontalVelocity(float moveX, float deltaSeconds)
{
    const float desiredVelocityX = moveX * kMaxMoveSpeed;
    if (moveX != 0.0f)
    {
        horizontalVelocity =
            MoveToward(horizontalVelocity, desiredVelocityX, kAcceleration * deltaSeconds);
    }
    else
    {
        horizontalVelocity = MoveToward(horizontalVelocity, 0.0f, kDeceleration * deltaSeconds);
    }

    horizontalVelocity = Clamp(horizontalVelocity, -kMaxMoveSpeed, kMaxMoveSpeed);
}

void Player::UpdateCoyoteState(float deltaSeconds)
{
    if (grounded)
    {
        timeSinceGrounded = 0.0f;
        coyoteAvailable = true;
        return;
    }

    timeSinceGrounded += deltaSeconds;
}

const core::Vec3& Player::Position() const
{
    return position;
}

const core::Vec3& Player::Size() const
{
    return size;
}

float Player::HorizontalVelocity() const
{
    return horizontalVelocity;
}

float Player::VerticalVelocity() const
{
    return verticalVelocity;
}

bool Player::IsGrounded() const
{
    return grounded;
}

float Player::TimeSinceGrounded() const
{
    return timeSinceGrounded;
}

bool Player::IsCoyoteAvailable() const
{
    return coyoteAvailable;
}

float Player::JumpBufferRemaining() const
{
    return jumpBufferRemaining;
}

physics::PlayerGroundSupport Player::GroundSupport() const
{
    return groundSupport;
}

int Player::PhysicsContactCount() const
{
    return physicsContactCount;
}

bool Player::CharacterVirtualInitialized() const
{
    return characterVirtualInitialized;
}

core::Vec3 Player::GroundVelocity() const
{
    return groundVelocity;
}

bool Player::IsSupportingGroundMoving() const
{
    return supportingGroundMoving;
}

core::Vec3 Player::GroundNormal() const
{
    return groundNormal;
}

float Player::GroundSlopeAngleDegrees() const
{
    return groundSlopeAngleDegrees;
}

bool Player::IsCurrentSupportWalkable() const
{
    return currentSupportWalkable;
}
}
