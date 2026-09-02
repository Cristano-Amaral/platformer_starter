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

// Left elevated platform in kElevatedPlatforms[1]: center (-4.5, 2.25, 0),
// size (3, 0.5, 2.5), top Y = 2.5. Visual-center standing height is 3.3.
// Checkpoint is on the right platform (X ~ 5); these AABBs do not overlap.
inline constexpr LevelGoalSpec kLevelGoal{
    {-4.5f, TopY(kElevatedPlatforms[1]) + kPlayerVisualSize.y * 0.5f, 0.0f},
    {2.0f, 1.6f, 1.8f}};

// Test the project-facing visual center (Player::Position), not CharacterVirtual feet.
constexpr bool PointInsideGoal(const LevelGoalSpec& spec, core::Vec3 visualCenter)
{
    return PointInsideAabb(spec.center, spec.size, visualCenter);
}
}
