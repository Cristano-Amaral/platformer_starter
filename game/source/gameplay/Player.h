#pragma once

#include "core/Vec3.h"

namespace gameplay
{
class Player
{
public:
    Player(core::Vec3 position, core::Vec3 size);

    const core::Vec3& Position() const;
    const core::Vec3& Size() const;

private:
    core::Vec3 position;
    core::Vec3 size;
};
}
