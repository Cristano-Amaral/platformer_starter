#pragma once

// Project-owned static collectible volumes. Renderer draws available items
// as primitives. PhysicsWorld does not interpret collectible meaning. No Jolt
// sensor. Detection uses Player visual center, matching checkpoint/goal/hazard.
// Not a generic trigger type.

#include "core/Vec3.h"
#include "world/GreyboxWorld.h"
#include "world/HazardWorld.h"
#include "world/LevelGoal.h"
#include "world/MovingPlatform.h"
#include "world/RespawnWorld.h"

#include <array>

namespace world
{
struct CollectibleSpec
{
    core::Vec3 center;
    core::Vec3 size;
};

inline constexpr int kCollectibleCount = 3;
inline constexpr int kNoCollectibleIndex = -1;

// Hover so a standing Player center stays just below the AABB (optional hop).
inline constexpr float kCollectibleHoverAboveSupport = 1.5f;
inline constexpr core::Vec3 kCollectibleSize{1.0f, 1.2f, 1.0f};

// Index 0 = right-platform hop (CP1 side).
// Index 1 = left-landing hop (central / moving-platform arrival).
// Index 2 = middle-left-step hop (CP2-side route).
inline constexpr std::array<CollectibleSpec, kCollectibleCount> kCollectibles{{
    {{5.0f, TopY(kElevatedPlatforms[0]) + kCollectibleHoverAboveSupport, 0.0f},
     kCollectibleSize},
    {{-4.5f, TopY(kElevatedPlatforms[1]) + kCollectibleHoverAboveSupport, 0.0f},
     kCollectibleSize},
    {{-10.0f, TopY(kElevatedPlatforms[3]) + kCollectibleHoverAboveSupport, 0.0f},
     kCollectibleSize},
}};

constexpr bool PointInsideCollectible(const CollectibleSpec& spec, core::Vec3 visualCenter)
{
    return PointInsideAabb(spec.center, spec.size, visualCenter);
}

constexpr int FindCollectibleIndexContaining(core::Vec3 visualCenter)
{
    for (int index = 0; index < kCollectibleCount; ++index)
    {
        if (PointInsideCollectible(
                kCollectibles[static_cast<std::size_t>(index)], visualCenter))
        {
            return index;
        }
    }
    return kNoCollectibleIndex;
}

constexpr core::Vec3 StandingCenterOn(const Box& support)
{
    return {support.center.x, TopY(support) + kPlayerVisualSize.y * 0.5f, 0.0f};
}

static_assert(kCollectibles.size() == kCollectibleCount);
static_assert(kCollectibles[0].center.y == 2.5f);
static_assert(kCollectibles[1].center.y == 4.0f);
static_assert(kCollectibles[2].center.y == 3.75f);

// Walking the support must not auto-collect; a hop is required.
static_assert(!PointInsideCollectible(
    kCollectibles[0],
    StandingCenterOn(kElevatedPlatforms[0])));
static_assert(!PointInsideCollectible(
    kCollectibles[1],
    StandingCenterOn(kElevatedPlatforms[1])));
static_assert(!PointInsideCollectible(
    kCollectibles[2],
    StandingCenterOn(kElevatedPlatforms[3])));

static_assert(FindCollectibleIndexContaining(kInitialSpawnVisualCenter) == kNoCollectibleIndex);
static_assert(
    FindCollectibleIndexContaining(kCheckpoints[0].respawnPosition) == kNoCollectibleIndex);
static_assert(
    FindCollectibleIndexContaining(kCheckpoints[1].respawnPosition) == kNoCollectibleIndex);

static_assert(!AabbOverlaps(
    kCollectibles[0].center,
    kCollectibles[0].size,
    kHazards[0].center,
    kHazards[0].size));
static_assert(!AabbOverlaps(
    kCollectibles[0].center,
    kCollectibles[0].size,
    kHazards[1].center,
    kHazards[1].size));
static_assert(!AabbOverlaps(
    kCollectibles[1].center,
    kCollectibles[1].size,
    kHazards[0].center,
    kHazards[0].size));
static_assert(!AabbOverlaps(
    kCollectibles[1].center,
    kCollectibles[1].size,
    kHazards[1].center,
    kHazards[1].size));
static_assert(!AabbOverlaps(
    kCollectibles[2].center,
    kCollectibles[2].size,
    kHazards[0].center,
    kHazards[0].size));
static_assert(!AabbOverlaps(
    kCollectibles[2].center,
    kCollectibles[2].size,
    kHazards[1].center,
    kHazards[1].size));

static_assert(!AabbOverlaps(
    kCollectibles[0].center,
    kCollectibles[0].size,
    kCheckpoints[0].center,
    kCheckpoints[0].size));
static_assert(!AabbOverlaps(
    kCollectibles[0].center,
    kCollectibles[0].size,
    kCheckpoints[1].center,
    kCheckpoints[1].size));
static_assert(!AabbOverlaps(
    kCollectibles[1].center,
    kCollectibles[1].size,
    kCheckpoints[0].center,
    kCheckpoints[0].size));
static_assert(!AabbOverlaps(
    kCollectibles[1].center,
    kCollectibles[1].size,
    kCheckpoints[1].center,
    kCheckpoints[1].size));
static_assert(!AabbOverlaps(
    kCollectibles[2].center,
    kCollectibles[2].size,
    kCheckpoints[0].center,
    kCheckpoints[0].size));
static_assert(!AabbOverlaps(
    kCollectibles[2].center,
    kCollectibles[2].size,
    kCheckpoints[1].center,
    kCheckpoints[1].size));

static_assert(!AabbOverlaps(
    kCollectibles[0].center,
    kCollectibles[0].size,
    kCheckpoints[0].respawnPosition,
    kPlayerVisualSize));
static_assert(!AabbOverlaps(
    kCollectibles[0].center,
    kCollectibles[0].size,
    kCheckpoints[1].respawnPosition,
    kPlayerVisualSize));
static_assert(!AabbOverlaps(
    kCollectibles[1].center,
    kCollectibles[1].size,
    kCheckpoints[0].respawnPosition,
    kPlayerVisualSize));
static_assert(!AabbOverlaps(
    kCollectibles[1].center,
    kCollectibles[1].size,
    kCheckpoints[1].respawnPosition,
    kPlayerVisualSize));
static_assert(!AabbOverlaps(
    kCollectibles[2].center,
    kCollectibles[2].size,
    kCheckpoints[0].respawnPosition,
    kPlayerVisualSize));
static_assert(!AabbOverlaps(
    kCollectibles[2].center,
    kCollectibles[2].size,
    kCheckpoints[1].respawnPosition,
    kPlayerVisualSize));

static_assert(!AabbOverlaps(
    kCollectibles[0].center,
    kCollectibles[0].size,
    kLevelGoal.center,
    kLevelGoal.size));
static_assert(!AabbOverlaps(
    kCollectibles[1].center,
    kCollectibles[1].size,
    kLevelGoal.center,
    kLevelGoal.size));
static_assert(!AabbOverlaps(
    kCollectibles[2].center,
    kCollectibles[2].size,
    kLevelGoal.center,
    kLevelGoal.size));

static_assert(!AabbOverlaps(
    kCollectibles[0].center,
    kCollectibles[0].size,
    kMovingPlatformSweptAabb.center,
    kMovingPlatformSweptAabb.size));
static_assert(!AabbOverlaps(
    kCollectibles[1].center,
    kCollectibles[1].size,
    kMovingPlatformSweptAabb.center,
    kMovingPlatformSweptAabb.size));
static_assert(!AabbOverlaps(
    kCollectibles[2].center,
    kCollectibles[2].size,
    kMovingPlatformSweptAabb.center,
    kMovingPlatformSweptAabb.size));

static_assert(!AabbOverlaps(
    kCollectibles[0].center,
    kCollectibles[0].size,
    kCollectibles[1].center,
    kCollectibles[1].size));
static_assert(!AabbOverlaps(
    kCollectibles[0].center,
    kCollectibles[0].size,
    kCollectibles[2].center,
    kCollectibles[2].size));
static_assert(!AabbOverlaps(
    kCollectibles[1].center,
    kCollectibles[1].size,
    kCollectibles[2].center,
    kCollectibles[2].size));
}
