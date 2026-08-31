#include "gameplay/Player.h"

#include "world/Collision.h"

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

void Player::Update(const input::InputState& input, float deltaSeconds)
{
    if (input.jumpPressed)
    {
        jumpBufferRemaining = kJumpBufferTime;
    }

    UpdateHorizontalVelocity(input.moveX, deltaSeconds);

    const core::Vec3 positionBeforeX = position;
    const float proposedX = position.x + horizontalVelocity * deltaSeconds;
    position.x = proposedX;
    position.x = world::ResolveHorizontalPosition(positionBeforeX, position, size);
    if (horizontalVelocity > 0.0f && position.x < proposedX)
    {
        horizontalVelocity = 0.0f;
    }
    else if (horizontalVelocity < 0.0f && position.x > proposedX)
    {
        horizontalVelocity = 0.0f;
    }

    // Jump if already eligible (grounded or coyote) so this frame still integrates upward.
    TryJump();

    verticalVelocity -= kGravity * deltaSeconds;

    const core::Vec3 positionBeforeY = position;
    position.y += verticalVelocity * deltaSeconds;

    if (verticalVelocity > 0.0f)
    {
        const world::CeilingContact contact =
            world::ResolveCeilingContact(positionBeforeY, position, size, verticalVelocity);
        position.y = contact.positionY;
        verticalVelocity = contact.verticalVelocity;
        grounded = false;
    }
    else
    {
        const world::SupportContact contact =
            world::ResolveGroundContact(positionBeforeY, position, size, verticalVelocity);
        position.y = contact.positionY;
        verticalVelocity = contact.verticalVelocity;
        grounded = contact.grounded;
    }

    UpdateCoyoteState(deltaSeconds);

    // Buffered jump after this frame's landing, without a second Y integration.
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
}
