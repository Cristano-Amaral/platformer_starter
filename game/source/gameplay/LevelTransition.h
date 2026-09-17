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
// Runtime-only destination completion hold. Not authored, not serialized, not
// score/run time. Preferred readable delay before the existing deferred replace.
inline constexpr float kDestinationTransitionHoldSeconds = 1.75f;

struct LevelTransitionSchedule
{
    bool pending = false;
    std::string destinationId;
    bool failed = false;
    std::string failureMessage;
    bool holding = false;
    float holdElapsedSeconds = 0.0f;
    bool skipRequested = false;
};

inline void ResetLevelTransitionSchedule(LevelTransitionSchedule& schedule)
{
    schedule = LevelTransitionSchedule{};
}

// Capture a destination exactly once after M63 completion. Empty nextLevelId
// is terminal and does not schedule. Failed schedules do not recapture, so a
// missing destination cannot retry every frame. Destination capture starts the
// runtime hold; commit still waits for timeout or Enter skip.
inline void CaptureCompletedGoalDestination(
    LevelTransitionSchedule& schedule,
    std::string_view nextLevelId)
{
    if (schedule.pending || schedule.failed || schedule.holding)
    {
        return;
    }
    if (nextLevelId.empty())
    {
        return;
    }
    schedule.pending = true;
    schedule.destinationId = std::string(nextLevelId);
    schedule.holding = true;
    schedule.holdElapsedSeconds = 0.0f;
    schedule.skipRequested = false;
}

inline void MarkLevelTransitionFailed(
    LevelTransitionSchedule& schedule,
    std::string_view message)
{
    schedule.pending = false;
    schedule.holding = false;
    schedule.skipRequested = false;
    schedule.holdElapsedSeconds = 0.0f;
    schedule.failed = true;
    schedule.failureMessage = std::string(message);
}

inline bool DestinationTransitionIsInFlight(const LevelTransitionSchedule& schedule)
{
    return !schedule.failed && schedule.pending;
}

inline bool DestinationCompletionShowsContinueHint(const LevelTransitionSchedule& schedule)
{
    return DestinationTransitionIsInFlight(schedule);
}

inline void TickDestinationTransitionHold(LevelTransitionSchedule& schedule, float deltaSeconds)
{
    if (!schedule.holding || !schedule.pending || schedule.failed)
    {
        return;
    }
    if (deltaSeconds > 0.0f)
    {
        schedule.holdElapsedSeconds += deltaSeconds;
    }
}

inline void SkipDestinationTransitionHold(LevelTransitionSchedule& schedule)
{
    if (!schedule.holding || !schedule.pending || schedule.failed)
    {
        return;
    }
    schedule.skipRequested = true;
}

inline bool DestinationTransitionHoldBlocksCommit(const LevelTransitionSchedule& schedule)
{
    if (!schedule.pending || schedule.failed || schedule.skipRequested)
    {
        return false;
    }
    return schedule.holding
        && schedule.holdElapsedSeconds < kDestinationTransitionHoldSeconds;
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
