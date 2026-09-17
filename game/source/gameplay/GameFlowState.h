#pragma once

// Narrow M65 run-completion flow. Application-owned only.
// Not a GameState machine, SceneManager, campaign graph, or event bus.
// Destination-bearing goals keep M64.1 LevelTransitionSchedule authority.

#include "world/LevelIdentity.h"

#include <string>
#include <string_view>

namespace gameplay
{
inline constexpr std::string_view kInitialRuntimeLevelId = world::kLevel01Id;

// SessionBestTimeState is process/session-scoped, updated on any completion
// (including destination-bearing goals), reset only at Initialize, and may
// be seeded from the M29 save file. After M64 the run timer resets on
// destination replace, so that record is not a full-run best. Results HUD
// therefore omits BEST rather than widening or redefining that meaning.
inline constexpr bool kRunCompleteShowsSessionBest = false;

inline constexpr const char* kRunCompleteTitle = "RUN COMPLETE";
inline constexpr const char* kRunCompletePlayAgainHint = "ENTER   PLAY AGAIN";
inline constexpr const char* kRunCompleteRestartHint = "R       RESTART LEVEL";
inline constexpr const char* kDestinationCompleteHint = "PRESS ENTER TO CONTINUE";
inline constexpr const char* kFailedDestinationRestartHint = "PRESS ENTER TO RESTART";

enum class RunCompleteInputAction
{
    None,
    PlayAgain,
    RestartCurrentLevel,
};

enum class CompletionPresentation
{
    None,
    DestinationContinue,
    FailedDestinationRestart,
    RunComplete,
};

struct RunCompleteState
{
    bool active = false;
    double capturedFinalSeconds = 0.0;
    bool playAgainPending = false;
    bool playAgainFailed = false;
    std::string playAgainFailureMessage;
};

inline void ResetRunCompleteState(RunCompleteState& state)
{
    state = RunCompleteState{};
}

inline bool RunCompleteBlocksGameplay(const RunCompleteState& state)
{
    return state.active;
}

inline bool ResultsHudShowsRunComplete(const RunCompleteState& state)
{
    return state.active;
}

inline double FrozenRunCompleteSeconds(const RunCompleteState& state)
{
    return state.capturedFinalSeconds;
}

// Terminal nextLevelId (empty) enters results exactly once. A destination
// identity never activates Run Complete.
inline bool TryEnterRunCompleteFromTerminalGoal(
    RunCompleteState& state,
    std::string_view nextLevelId,
    double frozenElapsedSeconds)
{
    if (state.active || state.playAgainPending)
    {
        return false;
    }
    if (!nextLevelId.empty())
    {
        return false;
    }
    state.active = true;
    state.capturedFinalSeconds = frozenElapsedSeconds;
    state.playAgainPending = false;
    state.playAgainFailed = false;
    state.playAgainFailureMessage.clear();
    return true;
}

inline void RequestPlayAgain(RunCompleteState& state)
{
    if (!state.active || state.playAgainPending || state.playAgainFailed)
    {
        return;
    }
    state.playAgainPending = true;
}

inline void MarkPlayAgainFailed(RunCompleteState& state, std::string_view message)
{
    state.playAgainPending = false;
    state.playAgainFailed = true;
    state.playAgainFailureMessage = std::string(message);
}

inline bool PlayAgainIsInFlight(const RunCompleteState& state)
{
    return state.playAgainPending && !state.playAgainFailed;
}

// Requires results already visible at frame start so completion+Enter cannot
// Play Again on the same frame. Enter wins over R if both edges fire.
inline RunCompleteInputAction ResolveRunCompleteInput(
    bool runCompleteAvailableAtFrameStart,
    bool enterPressed,
    bool restartCurrentLevelPressed,
    const RunCompleteState& state)
{
    if (!runCompleteAvailableAtFrameStart || !state.active || state.playAgainPending)
    {
        return RunCompleteInputAction::None;
    }
    if (enterPressed)
    {
        return state.playAgainFailed ? RunCompleteInputAction::None
                                     : RunCompleteInputAction::PlayAgain;
    }
    if (restartCurrentLevelPressed)
    {
        return RunCompleteInputAction::RestartCurrentLevel;
    }
    return RunCompleteInputAction::None;
}

inline CompletionPresentation SelectCompletionPresentation(
    bool levelCompleted,
    bool destinationContinueHint,
    const RunCompleteState& runComplete)
{
    if (runComplete.active)
    {
        return CompletionPresentation::RunComplete;
    }
    if (!levelCompleted)
    {
        return CompletionPresentation::None;
    }
    if (destinationContinueHint)
    {
        return CompletionPresentation::DestinationContinue;
    }
    return CompletionPresentation::FailedDestinationRestart;
}

inline bool PlayAgainLoadsInitialRuntimeLevel(std::string_view loadedId)
{
    return loadedId == kInitialRuntimeLevelId;
}

inline bool RestartStaysOnCurrentLevel(
    std::string_view currentId,
    std::string_view afterRestartId)
{
    return currentId == afterRestartId;
}
}
