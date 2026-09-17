#pragma once

// Narrow top-level player flow: Main Menu vs Gameplay, plus M65 Run Complete.
// Application-owned only. Not a GameState machine, SceneManager, campaign
// graph, screen stack, or event bus. Destination-bearing goals keep M64.1
// LevelTransitionSchedule authority.

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
inline constexpr const char* kRunCompleteMainMenuHint = "ESC     MAIN MENU";
inline constexpr const char* kDestinationCompleteHint = "PRESS ENTER TO CONTINUE";
inline constexpr const char* kFailedDestinationRestartHint = "PRESS ENTER TO RESTART";

inline constexpr const char* kMainMenuTitle = "Platformer3D";
inline constexpr const char* kMainMenuPlayLabel = "PLAY";
inline constexpr const char* kMainMenuQuitLabel = "QUIT";
inline constexpr int kMainMenuItemCount = 2;

enum class TopLevelFlow
{
    MainMenu,
    Gameplay,
};

enum class MainMenuItem
{
    Play,
    Quit,
};

enum class MainMenuInputAction
{
    None,
    Play,
    Quit,
};

enum class RunCompleteInputAction
{
    None,
    PlayAgain,
    RestartCurrentLevel,
    ReturnToMainMenu,
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

struct MainMenuState
{
    MainMenuItem selected = MainMenuItem::Play;
    bool playPending = false;
    bool playFailed = false;
    std::string playFailureMessage;
    bool quitRequested = false;
};

inline void ResetRunCompleteState(RunCompleteState& state)
{
    state = RunCompleteState{};
}

inline void ResetMainMenuState(MainMenuState& state)
{
    state = MainMenuState{};
}

inline bool MainMenuBlocksGameplay(TopLevelFlow flow)
{
    return flow == TopLevelFlow::MainMenu;
}

inline bool RunTimerAdvancesInFlow(TopLevelFlow flow)
{
    return flow == TopLevelFlow::Gameplay;
}

inline bool InventoryUiIsAvailable(
    TopLevelFlow flow,
    bool editorActive,
    bool runCompleteActive)
{
    return flow == TopLevelFlow::Gameplay && !editorActive && !runCompleteActive;
}

inline bool GameplayHudIsActive(TopLevelFlow flow, bool editorActive)
{
    return editorActive || flow == TopLevelFlow::Gameplay;
}

inline bool MainMenuOverlayIsVisible(TopLevelFlow flow, bool editorActive)
{
    return flow == TopLevelFlow::MainMenu && !editorActive;
}

inline bool ResultsHudShowsRunComplete(TopLevelFlow flow, const RunCompleteState& state)
{
    return flow == TopLevelFlow::Gameplay && state.active;
}

inline bool RunCompleteBlocksGameplay(const RunCompleteState& state)
{
    return state.active;
}

inline double FrozenRunCompleteSeconds(const RunCompleteState& state)
{
    return state.capturedFinalSeconds;
}

inline TopLevelFlow FlowAfterEditorToggle(TopLevelFlow current)
{
    return current;
}

inline TopLevelFlow FlowAfterEditorOpenOrSwitch(TopLevelFlow current)
{
    return current;
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

inline bool PlayIsInFlight(const MainMenuState& state)
{
    return state.playPending && !state.playFailed;
}

inline void RequestPlayFromMainMenu(MainMenuState& state, TopLevelFlow flow)
{
    if (flow != TopLevelFlow::MainMenu || state.playPending || state.quitRequested)
    {
        return;
    }
    state.playPending = true;
    state.playFailed = false;
    state.playFailureMessage.clear();
}

inline void MarkPlayFailed(MainMenuState& state, std::string_view message)
{
    state.playPending = false;
    state.playFailed = true;
    state.playFailureMessage = std::string(message);
}

inline void RequestQuitFromMainMenu(MainMenuState& state, TopLevelFlow flow)
{
    if (flow != TopLevelFlow::MainMenu || state.playPending)
    {
        return;
    }
    state.quitRequested = true;
}

inline void NavigateMainMenu(MainMenuState& state, int step)
{
    if (step == 0 || state.playPending || state.quitRequested)
    {
        return;
    }
    const int count = kMainMenuItemCount;
    int index = static_cast<int>(state.selected) + step;
    index %= count;
    if (index < 0)
    {
        index += count;
    }
    state.selected = static_cast<MainMenuItem>(index);
}

inline void EnterMainMenu(
    TopLevelFlow& flow,
    MainMenuState& menu,
    RunCompleteState& runComplete)
{
    flow = TopLevelFlow::MainMenu;
    ResetMainMenuState(menu);
    ResetRunCompleteState(runComplete);
}

inline void EnterGameplayFromSuccessfulPlay(TopLevelFlow& flow, MainMenuState& menu)
{
    flow = TopLevelFlow::Gameplay;
    ResetMainMenuState(menu);
}

// Activate uses the selection captured at call time. Navigation is a
// separate edge so Enter does not also move. Menu input is ignored unless
// Main Menu was already active at frame start (no F2/Play leak).
inline MainMenuInputAction ResolveMainMenuInput(
    bool mainMenuAvailableAtFrameStart,
    bool previousPressed,
    bool nextPressed,
    bool activatePressed,
    MainMenuState& state,
    TopLevelFlow flow)
{
    if (!mainMenuAvailableAtFrameStart || flow != TopLevelFlow::MainMenu
        || state.playPending || state.quitRequested)
    {
        return MainMenuInputAction::None;
    }
    if (activatePressed)
    {
        return state.selected == MainMenuItem::Play ? MainMenuInputAction::Play
                                                    : MainMenuInputAction::Quit;
    }
    if (previousPressed && !nextPressed)
    {
        NavigateMainMenu(state, -1);
    }
    else if (nextPressed && !previousPressed)
    {
        NavigateMainMenu(state, 1);
    }
    return MainMenuInputAction::None;
}

// Requires results already visible at frame start so completion+Enter cannot
// Play Again on the same frame. Priority: Enter Play Again, then Esc Main
// Menu, then R Restart. Destination hold never uses this path.
inline RunCompleteInputAction ResolveRunCompleteInput(
    bool runCompleteAvailableAtFrameStart,
    bool enterPressed,
    bool restartCurrentLevelPressed,
    bool cancelPressed,
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
    if (cancelPressed)
    {
        return RunCompleteInputAction::ReturnToMainMenu;
    }
    if (restartCurrentLevelPressed)
    {
        return RunCompleteInputAction::RestartCurrentLevel;
    }
    return RunCompleteInputAction::None;
}

inline CompletionPresentation SelectCompletionPresentation(
    TopLevelFlow flow,
    bool levelCompleted,
    bool destinationContinueHint,
    const RunCompleteState& runComplete)
{
    if (flow != TopLevelFlow::Gameplay)
    {
        return CompletionPresentation::None;
    }
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

inline bool EscapeClosesWindowInFlow(
    TopLevelFlow flow,
    bool editorActive,
    bool runCompleteActive,
    bool inventoryOpen)
{
    if (editorActive || flow != TopLevelFlow::Gameplay || runCompleteActive || inventoryOpen)
    {
        return false;
    }
    return true;
}
}
