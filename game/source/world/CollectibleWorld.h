#pragma once

#include "core/Vec3.h"
#include "world/RespawnWorld.h"

#include <array>
#include <cstddef>

namespace world
{
struct CollectibleSpec
{
    core::Vec3 center;
    core::Vec3 size;
};

inline constexpr int kCollectibleCount = 3;
inline constexpr int kNoCollectibleIndex = -1;
// Visual/picking cube. Shared so editor proxies match what Renderer draws.
inline constexpr float kCollectibleVisualSize = 0.45f;

constexpr bool PointInsideCollectible(const CollectibleSpec& spec, core::Vec3 visualCenter)
{
    return PointInsideAabb(spec.center, spec.size, visualCenter);
}

constexpr int FindCollectibleIndexContaining(
    core::Vec3 visualCenter,
    const std::array<CollectibleSpec, kCollectibleCount>& collectibles)
{
    for (int index = 0; index < kCollectibleCount; ++index)
    {
        if (PointInsideCollectible(collectibles[static_cast<std::size_t>(index)], visualCenter))
        {
            return index;
        }
    }
    return kNoCollectibleIndex;
}
}
