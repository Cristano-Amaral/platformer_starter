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
    const core::Vec3 positionBeforeX = position;
    position.x += input.moveX * kMoveSpeed * deltaSeconds;
    position.x = world::ResolveHorizontalPosition(positionBeforeX, position, size);

    if (input.jumpPressed && grounded)
    {
        verticalVelocity = kJumpSpeed;
        grounded = false;
    }

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
