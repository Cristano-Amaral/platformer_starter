#pragma once

#include "world/CollectibleWorld.h"

#include <array>
#include <cstddef>

namespace gameplay
{
// Application-owned per-run collection flags. Array size is compile-time
// coupled to world::kCollectibleCount (Level 01 has exactly 3 collectibles).
struct CollectibleRunState
{
    std::array<bool, world::kCollectibleCount> collected{};
};

constexpr int CollectedCount(const CollectibleRunState& state)
{
    int count = 0;
    for (bool collected : state.collected)
    {
        if (collected)
        {
            ++count;
        }
    }
    return count;
}

constexpr int FindAvailableCollectibleIndexContaining(
    core::Vec3 visualCenter,
    const CollectibleRunState& state,
    const std::array<world::CollectibleSpec, world::kCollectibleCount>& collectibles)
{
    for (int index = 0; index < world::kCollectibleCount; ++index)
    {
        if (state.collected[static_cast<std::size_t>(index)])
        {
            continue;
        }
        if (world::PointInsideCollectible(
                collectibles[static_cast<std::size_t>(index)], visualCenter))
        {
            return index;
        }
    }
    return world::kNoCollectibleIndex;
}

static_assert(CollectedCount(CollectibleRunState{}) == 0);
}
