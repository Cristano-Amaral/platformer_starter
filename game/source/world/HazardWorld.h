#pragma once

#include "core/Vec3.h"
#include "world/RespawnWorld.h"

#include <cstddef>
#include <span>

namespace world
{
struct HazardSpec
{
    core::Vec3 center;
    core::Vec3 size;
};

inline constexpr int kLevel01HazardCount = 2;
inline constexpr int kNoHazardIndex = -1;

constexpr bool PointInsideHazard(const HazardSpec& spec, core::Vec3 visualCenter)
{
    return PointInsideAabb(spec.center, spec.size, visualCenter);
}

inline int FindHazardIndexContaining(
    core::Vec3 visualCenter,
    std::span<const HazardSpec> hazards)
{
    for (std::size_t index = 0; index < hazards.size(); ++index)
    {
        if (PointInsideHazard(hazards[index], visualCenter))
        {
            return static_cast<int>(index);
        }
    }
    return kNoHazardIndex;
}
}
