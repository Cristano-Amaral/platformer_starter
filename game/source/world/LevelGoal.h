#pragma once

#include "core/Vec3.h"
#include "world/RespawnWorld.h"

namespace world
{
struct LevelGoalSpec
{
    core::Vec3 center;
    core::Vec3 size;
};

constexpr bool PointInsideGoal(const LevelGoalSpec& spec, core::Vec3 visualCenter)
{
    return PointInsideAabb(spec.center, spec.size, visualCenter);
}
}
