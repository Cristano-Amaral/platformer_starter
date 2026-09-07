#pragma once

#include "core/Vec3.h"
#include "world/RespawnWorld.h"

#include <cstddef>
#include <span>

namespace world
{
struct CollectibleSpec
{
    core::Vec3 center;
    core::Vec3 size;
};

inline constexpr int kLevel01CollectibleCount = 3;
inline constexpr int kNoCollectibleIndex = -1;
// Visual/picking cube. Shared so editor proxies match what Renderer draws.
inline constexpr float kCollectibleVisualSize = 0.45f;

constexpr bool PointInsideCollectible(const CollectibleSpec& spec, core::Vec3 visualCenter)
{
    return PointInsideAabb(spec.center, spec.size, visualCenter);
}

inline int FindCollectibleIndexContaining(
    core::Vec3 visualCenter,
    std::span<const CollectibleSpec> collectibles)
{
    for (std::size_t index = 0; index < collectibles.size(); ++index)
    {
        if (PointInsideCollectible(collectibles[index], visualCenter))
        {
            return static_cast<int>(index);
        }
    }
    return kNoCollectibleIndex;
}
}
