#include "gameplay/Player.h"

namespace gameplay
{
Player::Player(core::Vec3 position, core::Vec3 size)
    : position(position)
    , size(size)
{
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
