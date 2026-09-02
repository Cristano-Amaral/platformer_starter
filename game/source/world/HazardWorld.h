#pragma once

// Project-owned static hazard volumes. Renderer draws these specs as
// primitives. PhysicsWorld does not interpret hazard meaning. No Jolt sensor.
// Detection uses Player visual center, matching checkpoint/goal. Not a
// generic trigger type.

#include "core/Vec3.h"
#include "world/GreyboxWorld.h"
#include "world/LevelGoal.h"
#include "world/MovingPlatform.h"
#include "world/RespawnWorld.h"

#include <array>

namespace world
{
struct HazardSpec
{
    core::Vec3 center;
    core::Vec3 size;
};

inline constexpr int kHazardCount = 2;
inline constexpr int kNoHazardIndex = -1;

// Index 0 = corridor spikes between CP1 and the central/MP region.
// Index 1 = gap spikes between CP2 and the goal platform.
inline constexpr std::array<HazardSpec, kHazardCount> kHazards{{
    {{11.5f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}},
    {{-18.5f, 0.5f, 0.0f}, {1.2f, 1.0f, 2.0f}},
}};

constexpr bool PointInsideHazard(const HazardSpec& spec, core::Vec3 visualCenter)
{
    return PointInsideAabb(spec.center, spec.size, visualCenter);
}

constexpr int FindHazardIndexContaining(core::Vec3 visualCenter)
{
    for (int index = 0; index < kHazardCount; ++index)
    {
        if (PointInsideHazard(kHazards[static_cast<std::size_t>(index)], visualCenter))
        {
            return index;
        }
    }
    return kNoHazardIndex;
}

static_assert(kHazards.size() == kHazardCount);
static_assert(kHazards[0].center.y == kHazards[0].size.y * 0.5f);
static_assert(kHazards[1].center.y == kHazards[1].size.y * 0.5f);
static_assert(kHazards[0].center.x - kHazards[0].size.x * 0.5f > 8.0f);
static_assert(
    kHazards[0].center.x + kHazards[0].size.x * 0.5f
    < kElevatedPlatforms[kCheckpoint1PlatformIndex].center.x
        - kElevatedPlatforms[kCheckpoint1PlatformIndex].size.x * 0.5f);
static_assert(
    kHazards[1].center.x + kHazards[1].size.x * 0.5f
    < kElevatedPlatforms[kCheckpoint2PlatformIndex].center.x
        - kElevatedPlatforms[kCheckpoint2PlatformIndex].size.x * 0.5f);
static_assert(
    kHazards[1].center.x - kHazards[1].size.x * 0.5f
    > kElevatedPlatforms[kGoalPlatformIndex].center.x
        + kElevatedPlatforms[kGoalPlatformIndex].size.x * 0.5f);
static_assert(FindHazardIndexContaining(kInitialSpawnVisualCenter) == kNoHazardIndex);
static_assert(FindHazardIndexContaining(kCheckpoints[0].respawnPosition) == kNoHazardIndex);
static_assert(FindHazardIndexContaining(kCheckpoints[1].respawnPosition) == kNoHazardIndex);

static_assert(!AabbOverlaps(
    kHazards[0].center,
    kHazards[0].size,
    kInitialSpawnVisualCenter,
    kPlayerVisualSize));
static_assert(!AabbOverlaps(
    kHazards[0].center,
    kHazards[0].size,
    kCheckpoints[0].respawnPosition,
    kPlayerVisualSize));
static_assert(!AabbOverlaps(
    kHazards[0].center,
    kHazards[0].size,
    kCheckpoints[1].respawnPosition,
    kPlayerVisualSize));
static_assert(!AabbOverlaps(
    kHazards[1].center,
    kHazards[1].size,
    kInitialSpawnVisualCenter,
    kPlayerVisualSize));
static_assert(!AabbOverlaps(
    kHazards[1].center,
    kHazards[1].size,
    kCheckpoints[0].respawnPosition,
    kPlayerVisualSize));
static_assert(!AabbOverlaps(
    kHazards[1].center,
    kHazards[1].size,
    kCheckpoints[1].respawnPosition,
    kPlayerVisualSize));

static_assert(!AabbOverlaps(
    kHazards[0].center,
    kHazards[0].size,
    kCheckpoints[0].center,
    kCheckpoints[0].size));
static_assert(!AabbOverlaps(
    kHazards[0].center,
    kHazards[0].size,
    kCheckpoints[1].center,
    kCheckpoints[1].size));
static_assert(!AabbOverlaps(
    kHazards[1].center,
    kHazards[1].size,
    kCheckpoints[0].center,
    kCheckpoints[0].size));
static_assert(!AabbOverlaps(
    kHazards[1].center,
    kHazards[1].size,
    kCheckpoints[1].center,
    kCheckpoints[1].size));

static_assert(!AabbOverlaps(
    kHazards[0].center,
    kHazards[0].size,
    kLevelGoal.center,
    kLevelGoal.size));
static_assert(!AabbOverlaps(
    kHazards[1].center,
    kHazards[1].size,
    kLevelGoal.center,
    kLevelGoal.size));

static_assert(!AabbOverlaps(
    kHazards[0].center,
    kHazards[0].size,
    kMovingPlatformSweptAabb.center,
    kMovingPlatformSweptAabb.size));
static_assert(!AabbOverlaps(
    kHazards[1].center,
    kHazards[1].size,
    kMovingPlatformSweptAabb.center,
    kMovingPlatformSweptAabb.size));
}
