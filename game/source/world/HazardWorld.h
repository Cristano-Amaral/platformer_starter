#pragma once

#include "core/Vec3.h"
#include "world/RespawnWorld.h"

#include <array>
#include <cstddef>

namespace world
{
struct HazardSpec
{
    core::Vec3 center;
    core::Vec3 size;
};

inline constexpr int kHazardCount = 2;
inline constexpr int kNoHazardIndex = -1;

constexpr bool PointInsideHazard(const HazardSpec& spec, core::Vec3 visualCenter)
{
    return PointInsideAabb(spec.center, spec.size, visualCenter);
}

constexpr int FindHazardIndexContaining(
    core::Vec3 visualCenter,
    const std::array<HazardSpec, kHazardCount>& hazards)
{
    for (int index = 0; index < kHazardCount; ++index)
    {
        if (PointInsideHazard(hazards[static_cast<std::size_t>(index)], visualCenter))
        {
            return index;
        }
    }
    return kNoHazardIndex;
}
}
