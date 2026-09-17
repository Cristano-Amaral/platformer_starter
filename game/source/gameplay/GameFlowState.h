#pragma once

// Narrow top-level player flow: Main Menu vs Gameplay, plus M65 Run Complete
// and M68 Pause. Application-owned only. Pause is transient runtime overlay
// state on Gameplay, not a GameState machine, SceneManager, campaign graph,
// screen stack, or event bus. Destination-bearing goals keep M64.1
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

inline constexpr const char* kPauseMenuTitle = "PAUSED";
inline constexpr const char* kPauseMenuResumeLabel = "RESUME";
inline constexpr const char* kPauseMenuMainMenuLabel = "MAIN MENU";
inline constexpr int kPauseMenuItemCount = 2;

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

enum class PauseMenuItem
{
    Resume,
    MainMenu,
};

enum class PauseMenuInputAction
{
    None,
    EnterPause,
    Resume,
    ReturnToMainMenu,
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

struct PauseMenuState
{
    bool active = false;
    PauseMenuItem selected = PauseMenuItem::Resume;
};

inline void ResetRunCompleteState(RunCompleteState& state)
{
    state = RunCompleteState{};
}

inline void ResetMainMenuState(MainMenuState& state)
{
    state = MainMenuState{};
}

inline void ResetPauseMenuState(PauseMenuState& state)
{
    state = PauseMenuState{};
}

inline bool MainMenuBlocksGameplay(TopLevelFlow flow)
{
    return flow == TopLevelFlow::MainMenu;
}

inline bool RunTimerAdvancesInFlow(TopLevelFlow flow, bool pauseActive = false)
{
    return flow == TopLevelFlow::Gameplay && !pauseActive;
}

inline bool InventoryUiIsAvailable(
    TopLevelFlow flow,
    bool editorActive,
    bool runCompleteActive,
    bool pauseActive = false)
{
    return flow == TopLevelFlow::Gameplay && !editorActive && !runCompleteActive && !pauseActive;
}

// True for the whole frame that Pause is or was active, so the Resume Enter/Esc
// edge cannot leak into gameplay on the first resumed frame.
inline bool PauseBlocksGameplay(bool wasActive, bool isActive)
{
    return wasActive || isActive;
}

inline bool PauseMenuOverlayIsVisible(
    TopLevelFlow flow,
    bool editorActive,
    bool pauseActive,
    bool runCompleteActive)
{
    return flow == TopLevelFlow::Gameplay && pauseActive && !editorActive && !runCompleteActive;
}

inline bool PauseCanBeEntered(
    TopLevelFlow flow,
    bool editorActive,
    bool inventoryBlocksGameplay,
    bool runCompleteActive,
    bool levelCompleted,
    bool pauseActive)
{
    return flow == TopLevelFlow::Gameplay && !editorActive && !inventoryBlocksGameplay
        && !runCompleteActive && !levelCompleted && !pauseActive;
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

inline void EnterPause(PauseMenuState& state)
{
    if (state.active)
    {
        return;
    }
    state.active = true;
    state.selected = PauseMenuItem::Resume;
}

inline void ResumePause(PauseMenuState& state)
{
    state.active = false;
    state.selected = PauseMenuItem::Resume;
}

inline void NavigatePauseMenu(PauseMenuState& state, int step)
{
    if (step == 0 || !state.active)
    {
        return;
    }
    const int count = kPauseMenuItemCount;
    int index = static_cast<int>(state.selected) + step;
    index %= count;
    if (index < 0)
    {
        index += count;
    }
    state.selected = static_cast<PauseMenuItem>(index);
}

inline void EnterMainMenu(
    TopLevelFlow& flow,
    MainMenuState& menu,
    RunCompleteState& runComplete,
    PauseMenuState& pause)
{
    flow = TopLevelFlow::MainMenu;
    ResetMainMenuState(menu);
    ResetRunCompleteState(runComplete);
    ResetPauseMenuState(pause);
}

inline void EnterGameplayFromSuccessfulPlay(
    TopLevelFlow& flow,
    MainMenuState& menu,
    PauseMenuState& pause)
{
    flow = TopLevelFlow::Gameplay;
    ResetMainMenuState(menu);
    ResetPauseMenuState(pause);
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

// Pause uses the same Up/Down/Enter edges as Main Menu. The Esc edge that
// enters Pause cannot Resume in the same Resolve because Resume requires
// Pause already active at frame start. Esc while paused is always Resume,
// even if MAIN MENU is selected. Editor/Inventory/completion callers must
// not invoke this when a higher-priority overlay owns Esc.
inline PauseMenuInputAction ResolvePauseInput(
    bool pauseWasActiveAtFrameStart,
    bool pauseCanBeEntered,
    bool previousPressed,
    bool nextPressed,
    bool activatePressed,
    bool cancelPressed,
    PauseMenuState& state,
    TopLevelFlow flow)
{
    if (flow != TopLevelFlow::Gameplay)
    {
        return PauseMenuInputAction::None;
    }
    if (!pauseWasActiveAtFrameStart)
    {
        if (pauseCanBeEntered && cancelPressed)
        {
            return PauseMenuInputAction::EnterPause;
        }
        return PauseMenuInputAction::None;
    }
    if (!state.active)
    {
        return PauseMenuInputAction::None;
    }
    if (cancelPressed)
    {
        return PauseMenuInputAction::Resume;
    }
    if (activatePressed)
    {
        return state.selected == PauseMenuItem::Resume
            ? PauseMenuInputAction::Resume
            : PauseMenuInputAction::ReturnToMainMenu;
    }
    if (previousPressed && !nextPressed)
    {
        NavigatePauseMenu(state, -1);
    }
    else if (nextPressed && !previousPressed)
    {
        NavigatePauseMenu(state, 1);
    }
    return PauseMenuInputAction::None;
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
    bool inventoryOpen,
    bool pauseActive = false)
{
    if (editorActive || inventoryOpen || runCompleteActive || pauseActive
        || flow != TopLevelFlow::Gameplay)
    {
        return false;
    }
    // M68: ordinary Gameplay Esc enters Pause instead of closing the window.
    return false;
}
}
