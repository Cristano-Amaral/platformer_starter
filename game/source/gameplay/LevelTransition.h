#pragma once

// Narrowest M64 level-to-level transition helpers. Not a SceneManager,
// campaign graph, Objective/Trigger/Receiver, or GameState machine.
// Application owns when the deferred replace happens.

#include "world/LevelDefinition.h"
#include "world/LevelFile.h"
#include "world/LevelIdentity.h"

#include <filesystem>
#include <string>
#include <string_view>

namespace gameplay
{
struct LevelTransitionSchedule
{
    bool pending = false;
    std::string destinationId;
    bool failed = false;
    std::string failureMessage;
};

inline void ResetLevelTransitionSchedule(LevelTransitionSchedule& schedule)
{
    schedule = LevelTransitionSchedule{};
}

// Capture a destination exactly once after M63 completion. Empty nextLevelId
// is terminal and does not schedule. Failed schedules do not recapture, so a
// missing destination cannot retry every frame.
inline void CaptureCompletedGoalDestination(
    LevelTransitionSchedule& schedule,
    std::string_view nextLevelId)
{
    if (schedule.pending || schedule.failed)
    {
        return;
    }
    if (nextLevelId.empty())
    {
        return;
    }
    schedule.pending = true;
    schedule.destinationId = std::string(nextLevelId);
}

inline void MarkLevelTransitionFailed(
    LevelTransitionSchedule& schedule,
    std::string_view message)
{
    schedule.pending = false;
    schedule.failed = true;
    schedule.failureMessage = std::string(message);
}

enum class LevelTransitionPrepareStatus
{
    Ready,
    Missing,
    Invalid,
    Unsafe,
    Error,
};

inline const char* LevelTransitionPrepareStatusName(LevelTransitionPrepareStatus status)
{
    switch (status)
    {
    case LevelTransitionPrepareStatus::Ready:
        return "Ready";
    case LevelTransitionPrepareStatus::Missing:
        return "Missing";
    case LevelTransitionPrepareStatus::Invalid:
        return "Invalid";
    case LevelTransitionPrepareStatus::Unsafe:
        return "Unsafe";
    case LevelTransitionPrepareStatus::Error:
        return "Error";
    }
    return "Error";
}

struct LevelTransitionPrepareResult
{
    LevelTransitionPrepareStatus status = LevelTransitionPrepareStatus::Invalid;
    std::string message;
    world::LevelDefinition candidate{};
    world::LoadLevelFileStatus loadStatus = world::LoadLevelFileStatus::Error;
};

// Load and validate a staged destination from an absolute runtime path.
// Never reads source or cooked trees. Failure leaves candidate empty so the
// caller can keep the current active level.
LevelTransitionPrepareResult PrepareStagedLevelDestination(
    const std::filesystem::path& stagedAbsolutePath,
    std::string_view expectedLevelId);

// Replace active only after Ready. Failure leaves active unchanged.
inline bool TryCommitPreparedDestination(
    world::LevelDefinition& active,
    const LevelTransitionPrepareResult& prepared)
{
    if (prepared.status != LevelTransitionPrepareStatus::Ready)
    {
        return false;
    }
    active = prepared.candidate;
    return true;
}
}
