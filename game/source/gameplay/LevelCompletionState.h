#pragma once

// Application-owned one-way run completion. Not a generic GameState machine.
// PhysicsWorld, Renderer, and authored Level Goal data do not own this.
// PerformRespawn must not clear it. Restart/Apply/reload reset it.

#include "core/Vec3.h"
#include "world/LevelGoal.h"

#include <string>
#include <vector>

namespace gameplay
{
struct LevelCompletionState
{
    bool completed = false;
};

inline void ResetLevelCompletionState(LevelCompletionState& state)
{
    state = LevelCompletionState{};
}

// First valid player overlap completes exactly once. Remaining inside or
// entering another goal returns false without changing authored data.
inline bool TryCompleteLevelFromPlayerOverlap(
    LevelCompletionState& state,
    const std::vector<world::LevelGoalSpec>& goals,
    core::Vec3 playerVisualCenter,
    std::string* outNextLevelId = nullptr)
{
    if (state.completed)
    {
        return false;
    }
    for (const world::LevelGoalSpec& goal : goals)
    {
        if (world::PointInsideGoal(goal, playerVisualCenter))
        {
            state.completed = true;
            if (outNextLevelId != nullptr)
            {
                *outNextLevelId = goal.nextLevelId;
            }
            return true;
        }
    }
    return false;
}
}
