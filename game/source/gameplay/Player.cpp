#include "gameplay/Player.h"

#include "gameplay/Greybox.h"

namespace gameplay
{
Player::Player(core::Vec3 position, core::Vec3 size)
    : position(position)
    , size(size)
{
}

void Player::Update(const input::InputState& input, float deltaSeconds)
{
    position.x += input.moveX * kMoveSpeed * deltaSeconds;

    if (input.jumpPressed && grounded)
    {
        verticalVelocity = kJumpSpeed;
        grounded = false;
    }

    verticalVelocity -= kGravity * deltaSeconds;
    position.y += verticalVelocity * deltaSeconds;

    const float groundY = GroundSurfaceY();
    const float playerBottom = position.y - size.y * 0.5f;
    if (playerBottom <= groundY && verticalVelocity <= 0.0f)
    {
        position.y = groundY + size.y * 0.5f;
        verticalVelocity = 0.0f;
        grounded = true;
    }
    else
    {
        grounded = false;
    }
}

const core::Vec3& Player::Position() const
{
    return position;
}

const core::Vec3& Player::Size() const
{
    return size;
}
}
