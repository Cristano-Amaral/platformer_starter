#pragma once

// Concrete authored Level Goal volume. Repeatable v1 category. Not a generic
// Objective, Trigger, Receiver, Event, or Interactable.

#include "core/Vec3.h"
#include "world/RespawnWorld.h"

#include <cmath>
#include <vector>

namespace world
{
// Matches editor::kMinAuthoredBoxExtent so Resize and parse share one floor.
inline constexpr float kMinLevelGoalExtent = 0.12f;
inline constexpr core::Vec3 kDefaultLevelGoalSize{2.0f, 1.6f, 1.8f};
inline constexpr int kLevel01LevelGoalCount = 0;

struct LevelGoalSpec
{
    core::Vec3 center{};
    core::Vec3 size{};
};

inline bool LevelGoalSizeIsValid(core::Vec3 size)
{
    return std::isfinite(size.x) && std::isfinite(size.y) && std::isfinite(size.z)
        && size.x >= kMinLevelGoalExtent && size.y >= kMinLevelGoalExtent
        && size.z >= kMinLevelGoalExtent;
}

inline bool LevelGoalCenterIsValid(core::Vec3 center)
{
    return std::isfinite(center.x) && std::isfinite(center.y) && std::isfinite(center.z);
}

inline bool LevelGoalSpecIsValid(const LevelGoalSpec& spec)
{
    return LevelGoalCenterIsValid(spec.center) && LevelGoalSizeIsValid(spec.size);
}

constexpr bool PointInsideGoal(const LevelGoalSpec& spec, core::Vec3 visualCenter)
{
    return PointInsideAabb(spec.center, spec.size, visualCenter);
}

inline bool PlayerOverlapsAnyLevelGoal(
    const std::vector<LevelGoalSpec>& goals,
    core::Vec3 playerVisualCenter)
{
    for (const LevelGoalSpec& goal : goals)
    {
        if (PointInsideGoal(goal, playerVisualCenter))
        {
            return true;
        }
    }
    return false;
}
}
