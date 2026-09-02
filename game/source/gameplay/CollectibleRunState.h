#pragma once

#include "world/CollectibleWorld.h"

#include <array>
#include <cstddef>

namespace gameplay
{
// Application-owned per-run collection flags. Not owned by Player,
// PhysicsWorld, or Renderer. PerformRespawn must not clear this.
// The bool array is the source of truth; count is derived.
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
    const CollectibleRunState& state)
{
    for (int index = 0; index < world::kCollectibleCount; ++index)
    {
        if (state.collected[static_cast<std::size_t>(index)])
        {
            continue;
        }
        if (world::PointInsideCollectible(
                world::kCollectibles[static_cast<std::size_t>(index)], visualCenter))
        {
            return index;
        }
    }
    return world::kNoCollectibleIndex;
}

static_assert(CollectedCount(CollectibleRunState{}) == 0);
}
