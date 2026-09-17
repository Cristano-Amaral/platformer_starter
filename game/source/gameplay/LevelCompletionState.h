#pragma once

// Application-owned one-way run completion. Not a generic GameState machine.
// PhysicsWorld, Renderer, and authored Level Goal data do not own this.
// PerformRespawn must not clear it. Restart/Apply/reload reset it.

#include "core/Vec3.h"
#include "world/LevelGoal.h"

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
    core::Vec3 playerVisualCenter)
{
    if (state.completed)
    {
        return false;
    }
    if (!world::PlayerOverlapsAnyLevelGoal(goals, playerVisualCenter))
    {
        return false;
    }
    state.completed = true;
    return true;
}
}
