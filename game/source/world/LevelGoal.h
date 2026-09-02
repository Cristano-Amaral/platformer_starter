#pragma once

// Project-owned single level-goal volume. Renderer and gameplay consume these
// values; PhysicsWorld does not interpret goal meaning. No Jolt sensor.
// Not a generic trigger type: CheckpointSpec stays in RespawnWorld.h.

#include "core/Vec3.h"
#include "world/GreyboxWorld.h"
#include "world/RespawnWorld.h"

namespace world
{
struct LevelGoalSpec
{
    core::Vec3 center;
    core::Vec3 size;
};

// Goal support is kElevatedPlatforms[kGoalPlatformIndex]: top Y = 3.0.
// Visual-center standing height is 3.8. CP2 trigger does not overlap this AABB.
inline constexpr LevelGoalSpec kLevelGoal{
    {-21.0f, TopY(kElevatedPlatforms[kGoalPlatformIndex]) + kPlayerVisualSize.y * 0.5f, 0.0f},
    {2.0f, 1.6f, 1.8f}};

// Test the project-facing visual center (Player::Position), not CharacterVirtual feet.
constexpr bool PointInsideGoal(const LevelGoalSpec& spec, core::Vec3 visualCenter)
{
    return PointInsideAabb(spec.center, spec.size, visualCenter);
}

static_assert(kLevelGoal.center.x == -21.0f);
static_assert(kLevelGoal.center.y == 3.8f);
static_assert(!AabbOverlaps(
    kCheckpoints[1].center,
    kCheckpoints[1].size,
    kLevelGoal.center,
    kLevelGoal.size));
static_assert(!AabbOverlaps(
    kCheckpoints[1].respawnPosition,
    kPlayerVisualSize,
    kLevelGoal.center,
    kLevelGoal.size));
static_assert(!AabbOverlaps(
    kCheckpoints[0].center,
    kCheckpoints[0].size,
    kLevelGoal.center,
    kLevelGoal.size));
}
