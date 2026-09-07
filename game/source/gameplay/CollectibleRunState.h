#pragma once

#include "world/CollectibleWorld.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace gameplay
{
// Application-owned per-run collection flags. Sized to the active level's
// collectible count on load / Apply / Reload / RestartRun.
struct CollectibleRunState
{
    std::vector<std::uint8_t> collected{};
};

inline CollectibleRunState MakeClearedCollectibleRunState(std::size_t count)
{
    CollectibleRunState state{};
    state.collected.assign(count, 0);
    return state;
}

inline int CollectedCount(const CollectibleRunState& state)
{
    int count = 0;
    for (std::uint8_t collected : state.collected)
    {
        if (collected != 0)
        {
            ++count;
        }
    }
    return count;
}

inline int FindAvailableCollectibleIndexContaining(
    core::Vec3 visualCenter,
    const CollectibleRunState& state,
    std::span<const world::CollectibleSpec> collectibles)
{
    const std::size_t count =
        state.collected.size() < collectibles.size() ? state.collected.size() : collectibles.size();
    for (std::size_t index = 0; index < count; ++index)
    {
        if (state.collected[index] != 0)
        {
            continue;
        }
        if (world::PointInsideCollectible(collectibles[index], visualCenter))
        {
            return static_cast<int>(index);
        }
    }
    return world::kNoCollectibleIndex;
}
}
