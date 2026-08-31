#include "gameplay/Player.h"

#include "world/Collision.h"

namespace gameplay
{
Player::Player(core::Vec3 position, core::Vec3 size)
    : position(position)
    , size(size)
{
}

void Player::Update(const input::InputState& input, float deltaSeconds)
{
    const core::Vec3 previousPosition = position;

    position.x += input.moveX * kMoveSpeed * deltaSeconds;

    if (input.jumpPressed && grounded)
    {
        verticalVelocity = kJumpSpeed;
        grounded = false;
    }

    verticalVelocity -= kGravity * deltaSeconds;
    position.y += verticalVelocity * deltaSeconds;

    const world::SupportContact contact =
        world::ResolveGroundContact(previousPosition, position, size, verticalVelocity);
    position.y = contact.positionY;
    verticalVelocity = contact.verticalVelocity;
    grounded = contact.grounded;
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
