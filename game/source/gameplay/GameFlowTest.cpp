#include "gameplay/CollectibleRunState.h"
#include "gameplay/DoorLockRuntime.h"
#include "gameplay/GameFlowState.h"
#include "gameplay/GameplayObjectiveHud.h"
#include "gameplay/PlayerHealth.h"
#include "gameplay/PlayerDeath.h"
#include "gameplay/Inventory.h"
#include "gameplay/InventoryUi.h"
#include "gameplay/ItemPickupCollectionFeedback.h"
#include "gameplay/ItemPickupCollectionHud.h"
#include "gameplay/ItemPickupRuntime.h"
#include "gameplay/LevelCompletionState.h"
#include "gameplay/LevelTransition.h"
#include "gameplay/RespawnState.h"
#include "gameplay/RunTimerState.h"
#include "gameplay/SessionBestTimeState.h"
#include "world/Door.h"
#include "world/ItemPickup.h"
#include "world/LevelDefinition.h"
#include "world/LevelFile.h"
#include "world/LevelGoal.h"
#include "world/LevelIdentity.h"
#include "world/LevelWriter.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace
{
int gFailures = 0;

void Expect(bool condition, const char* name)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++gFailures;
    }
}

void WriteAll(const std::filesystem::path& path, std::string_view text)
{
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
}

std::string ReadAll(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    return std::string(
        (std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

bool Vec3Equal(core::Vec3 a, core::Vec3 b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
}

int main()
{
    Expect(gameplay::kInitialRuntimeLevelId == world::kLevel01Id, "bootstrap identity is level_01");
    Expect(!gameplay::kRunCompleteShowsSessionBest, "results HUD omits incompatible session BEST");
    Expect(
        std::string_view(gameplay::kRunCompleteTitle) == "RUN COMPLETE", "results title");
    Expect(
        std::string_view(gameplay::kRunCompletePlayAgainHint) == "ENTER   PLAY AGAIN",
        "Play Again hint");
    Expect(
        std::string_view(gameplay::kRunCompleteRestartHint) == "R       RESTART LEVEL",
        "Restart Level hint");
    Expect(
        std::string_view(gameplay::kRunCompleteMainMenuHint) == "ESC     MAIN MENU",
        "Main Menu hint");
    Expect(std::string_view(gameplay::kMainMenuTitle) == "Platformer3D", "Main Menu title");
    Expect(std::string_view(gameplay::kMainMenuPlayLabel) == "PLAY", "PLAY label");
    Expect(std::string_view(gameplay::kMainMenuQuitLabel) == "QUIT", "QUIT label");
    Expect(
        std::string_view(gameplay::kDestinationCompleteHint) == "PRESS ENTER TO CONTINUE",
        "destination continue hint preserved");

    world::LevelGoalSpec terminal{{-21.0f, 3.8f, 0.0f}, {2.0f, 1.6f, 1.8f}, {}};
    world::LevelGoalSpec destination = terminal;
    destination.nextLevelId = "level_02";

    gameplay::LevelCompletionState completion{};
    gameplay::RunCompleteState runComplete{};
    gameplay::LevelTransitionSchedule transition{};
    gameplay::RunTimerState timer{};
    timer.elapsedSeconds = 12.5;

    std::string captured;
    Expect(
        gameplay::TryCompleteLevelFromPlayerOverlap(
            completion, {terminal}, terminal.center, &captured),
        "terminal goal completes once");
    Expect(captured.empty(), "terminal nextLevelId is empty");
    Expect(
        gameplay::TryEnterRunCompleteFromTerminalGoal(runComplete, captured, timer.elapsedSeconds),
        "terminal completion enters Run Complete");
    Expect(runComplete.active, "Run Complete is active");
    Expect(runComplete.capturedFinalSeconds == 12.5, "final time captured exactly once");
    Expect(
        !gameplay::TryEnterRunCompleteFromTerminalGoal(runComplete, captured, 99.0),
        "terminal completion does not re-enter");
    Expect(runComplete.capturedFinalSeconds == 12.5, "displayed final time stays frozen");
    Expect(
        gameplay::SelectCompletionPresentation(
            gameplay::TopLevelFlow::Gameplay,
            completion.completed,
            gameplay::DestinationCompletionShowsContinueHint(transition),
            runComplete)
            == gameplay::CompletionPresentation::RunComplete,
        "results HUD is terminal-only");
    gameplay::CaptureCompletedGoalDestination(transition, captured);
    Expect(!transition.pending && !transition.holding, "terminal does not schedule destination");

    gameplay::LevelCompletionState destinationCompletion{};
    gameplay::RunCompleteState destinationResults{};
    gameplay::LevelTransitionSchedule destinationHold{};
    std::string destinationId;
    Expect(
        gameplay::TryCompleteLevelFromPlayerOverlap(
            destinationCompletion, {destination}, destination.center, &destinationId),
        "destination goal still completes");
    Expect(
        !gameplay::TryEnterRunCompleteFromTerminalGoal(
            destinationResults, destinationId, 3.0),
        "destination goal never enters Run Complete");
    Expect(!destinationResults.active, "destination leaves results inactive");
    gameplay::CaptureCompletedGoalDestination(destinationHold, destinationId);
    Expect(destinationHold.holding, "M64.1 destination hold remains");
    Expect(
        gameplay::DestinationTransitionHoldBlocksCommit(destinationHold),
        "destination hold still blocks commit");
    gameplay::TickDestinationTransitionHold(destinationHold, 1.75f);
    Expect(
        !gameplay::DestinationTransitionHoldBlocksCommit(destinationHold),
        "destination timeout remains");
    gameplay::ResetLevelTransitionSchedule(destinationHold);
    gameplay::CaptureCompletedGoalDestination(destinationHold, destinationId);
    gameplay::SkipDestinationTransitionHold(destinationHold);
    Expect(
        !gameplay::DestinationTransitionHoldBlocksCommit(destinationHold),
        "destination Enter skip remains");
    Expect(
        gameplay::SelectCompletionPresentation(
            gameplay::TopLevelFlow::Gameplay,
            destinationCompletion.completed,
            gameplay::DestinationCompletionShowsContinueHint(destinationHold),
            destinationResults)
            == gameplay::CompletionPresentation::DestinationContinue,
        "destination HUD is LEVEL COMPLETE continue, not Run Complete");
    Expect(
        !gameplay::ObjectiveHudIsVisible(
            gameplay::TopLevelFlow::Gameplay,
            false,
            false,
            false,
            destinationCompletion.completed),
        "destination completion overlay suppresses objective HUD");
    Expect(
        !gameplay::HealthHudIsVisible(
            gameplay::TopLevelFlow::Gameplay,
            false,
            false,
            false,
            destinationCompletion.completed),
        "destination completion overlay suppresses Health HUD");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay,
            false,
            false,
            false,
            destinationCompletion.completed,
            false),
        "destination LEVEL COMPLETE hold prevents Hazard damage");

    Expect(
        gameplay::ResolveRunCompleteInput(false, true, true, false, runComplete)
            == gameplay::RunCompleteInputAction::None,
        "completion frame cannot Play Again or Restart");
    Expect(
        gameplay::ResolveRunCompleteInput(true, true, true, true, runComplete)
            == gameplay::RunCompleteInputAction::PlayAgain,
        "Enter Play Again wins over R and Esc on the same edge frame");
    Expect(
        gameplay::ResolveRunCompleteInput(true, false, true, true, runComplete)
            == gameplay::RunCompleteInputAction::ReturnToMainMenu,
        "Esc Main Menu wins over R on the same edge frame");
    Expect(
        gameplay::ResolveRunCompleteInput(true, false, true, false, runComplete)
            == gameplay::RunCompleteInputAction::RestartCurrentLevel,
        "R restarts the current terminal Level");
    Expect(
        gameplay::ResolveRunCompleteInput(true, false, false, false, runComplete)
            == gameplay::RunCompleteInputAction::None,
        "held absence does not emit an action");

    gameplay::RequestPlayAgain(runComplete);
    Expect(gameplay::PlayAgainIsInFlight(runComplete), "Enter Play Again arms exactly once");
    gameplay::RequestPlayAgain(runComplete);
    Expect(runComplete.playAgainPending, "repeated Enter cannot double-schedule Play Again");
    Expect(
        gameplay::ResolveRunCompleteInput(true, true, true, true, runComplete)
            == gameplay::RunCompleteInputAction::None,
        "in-flight Play Again ignores further Enter/R/Esc");

    const world::ParseLevelFileResult level01 = world::LoadLevelFile(PLATFORMER_LEVEL01_SOURCE_PATH);
    Expect(level01.status == world::LoadLevelFileStatus::Loaded, "canonical level_01 loads");
    Expect(level01.level.levelGoals.size() == 1, "canonical level_01 has one Level Goal");
    Expect(level01.level.levelGoals[0].nextLevelId == world::kLevel02Id,
        "canonical level_01 destination is level_02");
    char hudLabel[gameplay::kGameplayObjectiveHudLevelLabelCapacity]{};
    char hudObjective[gameplay::kGameplayObjectiveHudObjectiveCapacity]{};
    gameplay::FormatGameplayLevelLabel(hudLabel, sizeof(hudLabel), level01.level.id);
    gameplay::FormatGameplayObjectiveLine(
        hudObjective, sizeof(hudObjective), level01.level.levelGoals);
    Expect(std::string_view(hudLabel) == "LEVEL 01", "canonical level_01 HUD label");
    Expect(
        std::string_view(hudObjective) == gameplay::kGameplayObjectiveHudDestinationText,
        "canonical level_01 destination objective");
    Expect(
        !gameplay::ObjectiveHudIsVisible(
            gameplay::TopLevelFlow::MainMenu, false, false, false, false),
        "Main Menu hides objective HUD");
    Expect(
        gameplay::ObjectiveHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, false, false, false, false),
        "Play shows objective HUD");
    Expect(
        std::string(PLATFORMER_LEVEL01_SOURCE_PATH).find("source") != std::string::npos,
        "source path is test-only and never the runtime loader");

    std::error_code cleanupError;
    const std::filesystem::path scratch =
        std::filesystem::temp_directory_path() / "platformer3d_m65_game_flow";
    std::filesystem::remove_all(scratch, cleanupError);

    const std::filesystem::path staged01 = scratch / "assets" / "levels" / "level_01.level";
    WriteAll(staged01, world::SerializeLevelText(level01.level));
    const gameplay::LevelTransitionPrepareResult playAgainReady =
        gameplay::PrepareStagedLevelDestination(staged01, world::kLevel01Id);
    Expect(playAgainReady.status == gameplay::LevelTransitionPrepareStatus::Ready,
        "Play Again prepares staged level_01");
    Expect(
        gameplay::PlayAgainLoadsInitialRuntimeLevel(playAgainReady.candidate.id),
        "Play Again loads staged level_01");
    Expect(
        world::MakeRuntimeLevelLogicalId(world::kLevel01Id).find("source") == std::string::npos,
        "Play Again logical id never names source");
    Expect(
        world::MakeRuntimeLevelLogicalId(world::kLevel01Id).find("cooked") == std::string::npos,
        "Play Again logical id never names cooked");

    world::LevelDefinition active = level01.level;
    active.id = "level_02";
    Expect(
        gameplay::TryCommitPreparedDestination(active, playAgainReady),
        "Ready Play Again replaces active");
    Expect(active.id == world::kLevel01Id, "committed identity is level_01");
    Expect(
        Vec3Equal(active.initialSpawnVisualCenter, level01.level.initialSpawnVisualCenter),
        "Play Again starts at level_01 spawn");

    gameplay::Inventory inventory;
    Expect(inventory.TryAdd("key", 2), "seed inventory before fresh run");
    gameplay::InventoryUiState ui{};
    gameplay::OpenInventoryUi(ui, inventory);
    gameplay::ApplyInventoryLifecycle(inventory, gameplay::InventoryLifecycleEvent::NewRun);
    gameplay::ApplyInventoryUiLifecycle(ui, gameplay::InventoryLifecycleEvent::NewRun, inventory);
    Expect(inventory.Entries().empty(), "Play Again fresh Inventory via NewRun");
    Expect(!ui.open, "Play Again closes Inventory UI");

    gameplay::PlayerHealthState playAgainHealth{};
    gameplay::HazardContactState playAgainContact{};
    gameplay::InitializePlayerHealth(playAgainHealth);
    gameplay::TickHazardContactDamage(playAgainHealth, playAgainContact, true, 1.0f / 60.0f, true);
    Expect(
        playAgainHealth.currentHealth == gameplay::kMaxPlayerHealth - gameplay::kHazardDamageAmount,
        "seed damaged Health before Play Again");
    gameplay::ResetPlayerHealthForNewRuntime(playAgainHealth, playAgainContact);
    Expect(
        playAgainHealth.currentHealth == gameplay::kMaxPlayerHealth,
        "Play Again restores maximum Health through the existing NewRun path");

    gameplay::RespawnState respawn{};
    respawn.activeCheckpointIndex = 0;
    respawn.deathCount = 4;
    respawn = gameplay::RespawnState{};
    Expect(respawn.activeCheckpointIndex == world::kNoActiveCheckpointIndex,
        "Play Again checkpoint reset");
    Expect(respawn.deathCount == 0, "Play Again death count reset");

    gameplay::CollectibleRunState collectibles = gameplay::MakeClearedCollectibleRunState(2);
    collectibles.collected[0] = 1;
    collectibles = gameplay::MakeClearedCollectibleRunState(2);
    Expect(collectibles.collected[0] == 0 && collectibles.collected[1] == 0,
        "Play Again collectible reset");

    gameplay::ItemPickupRunState pickups = gameplay::MakeClearedItemPickupRunState(1);
    pickups.collected[0] = 1;
    pickups = gameplay::MakeClearedItemPickupRunState(1);
    Expect(pickups.collected[0] == 0, "Play Again pickup collected reset");

    world::ItemPickupSpec pickup{};
    pickup.position = {1.0f, 1.0f, 0.0f};
    pickup.itemId = "key";
    pickup.quantity = 1;
    gameplay::ItemPickupCollectionFeedbackState feedback{};
    Expect(gameplay::SpawnItemPickupCollectionFeedback(feedback, pickup, 0.0), "seed M61");
    gameplay::ClearItemPickupCollectionFeedback(feedback);
    Expect(feedback.emittedCount == 0 && !feedback.effects[0].active, "Play Again M61 reset");

    gameplay::ItemPickupCollectionHudState hud{};
    Expect(gameplay::SpawnItemPickupCollectionHud(hud, "key", 1), "seed M62");
    gameplay::ClearItemPickupCollectionHud(hud);
    Expect(hud.count == 0 && hud.emittedCount == 0, "Play Again M62 reset");

    world::DoorSpec lockedDoor{};
    lockedDoor.center = {0.0f, 1.5f, 0.0f};
    lockedDoor.size = world::kDefaultDoorSize;
    lockedDoor.openDistance = world::kDefaultDoorOpenDistance;
    lockedDoor.requiredItemId = "key";
    gameplay::DoorLockRunState doors = gameplay::MakeDoorLockRunState(std::vector{lockedDoor});
    Expect(doors.unlocked[0] == 0, "locked door starts locked");
    doors.unlocked[0] = 1;
    doors = gameplay::MakeDoorLockRunState(std::vector{lockedDoor});
    Expect(doors.unlocked[0] == 0, "Play Again Door lock reset");

    gameplay::RunTimerState playAgainTimer{};
    playAgainTimer.elapsedSeconds = 40.0;
    playAgainTimer.frozen = true;
    playAgainTimer = gameplay::RunTimerState{};
    Expect(playAgainTimer.elapsedSeconds == 0.0 && !playAgainTimer.frozen,
        "Play Again timer follows NewRun");

    gameplay::ResetLevelTransitionSchedule(transition);
    gameplay::ResetRunCompleteState(runComplete);
    Expect(!runComplete.active && !transition.pending,
        "Play Again completion/transition/results reset");

    gameplay::LevelCompletionState restartCompletion{};
    restartCompletion.completed = true;
    gameplay::RunCompleteState restartResults{};
    Expect(
        gameplay::TryEnterRunCompleteFromTerminalGoal(restartResults, "", 8.0),
        "seed terminal results before R");
    const std::string currentTerminalId{"level_02"};
    std::string afterRestartId = currentTerminalId;
    Expect(
        gameplay::RestartStaysOnCurrentLevel(currentTerminalId, afterRestartId),
        "R keeps the current terminal identity");
    Expect(afterRestartId != world::kLevel01Id, "R does not load level_01 as Play Again");
    gameplay::Inventory restartInventory;
    Expect(restartInventory.TryAdd("key", 1), "seed Restart Inventory");
    gameplay::ApplyInventoryLifecycle(
        restartInventory, gameplay::InventoryLifecycleEvent::RestartRun);
    Expect(restartInventory.Entries().empty(), "existing Restart Inventory clear is preserved");
    gameplay::PlayerHealthState restartHealth{};
    gameplay::HazardContactState restartContact{};
    gameplay::InitializePlayerHealth(restartHealth);
    gameplay::TickHazardContactDamage(restartHealth, restartContact, true, 1.0f / 60.0f, true);
    gameplay::ResetPlayerHealthForNewRuntime(restartHealth, restartContact);
    Expect(
        restartHealth.currentHealth == gameplay::kMaxPlayerHealth,
        "Restart Current Level restores maximum Health");
    gameplay::RunTimerState restartTimer{};
    restartTimer.elapsedSeconds = 22.0;
    restartTimer.frozen = true;
    restartTimer = gameplay::RunTimerState{};
    Expect(restartTimer.elapsedSeconds == 0.0 && !restartTimer.frozen,
        "existing Restart timer reset is preserved");
    gameplay::ResetRunCompleteState(restartResults);
    gameplay::ResetLevelCompletionState(restartCompletion);
    Expect(!restartResults.active && !restartCompletion.completed,
        "R leaves Run Complete and returns to Gameplay");

    const std::filesystem::path missing01 =
        scratch / "missing" / "assets" / "levels" / "level_01.level";
    world::LevelDefinition preserved = level01.level;
    const gameplay::LevelTransitionPrepareResult missing =
        gameplay::PrepareStagedLevelDestination(missing01, world::kLevel01Id);
    Expect(missing.status == gameplay::LevelTransitionPrepareStatus::Missing,
        "missing staged level_01 is atomic");
    Expect(!gameplay::TryCommitPreparedDestination(preserved, missing),
        "missing Play Again does not replace");
    Expect(world::AuthoredLevelDataEqual(preserved, level01.level),
        "failed Play Again leaves authored data intact");

    gameplay::RunCompleteState failedPlayAgain{};
    Expect(gameplay::TryEnterRunCompleteFromTerminalGoal(failedPlayAgain, "", 5.0),
        "failed Play Again still shows results");
    gameplay::RequestPlayAgain(failedPlayAgain);
    gameplay::MarkPlayAgainFailed(failedPlayAgain, "Initial staged level is missing. Active run unchanged.");
    Expect(failedPlayAgain.active && failedPlayAgain.playAgainFailed, "failure stays on results");
    gameplay::RequestPlayAgain(failedPlayAgain);
    Expect(!failedPlayAgain.playAgainPending, "failed Play Again does not retry every frame");
    Expect(
        gameplay::ResolveRunCompleteInput(true, true, false, false, failedPlayAgain)
            == gameplay::RunCompleteInputAction::None,
        "Enter cannot retry a failed Play Again");
    Expect(
        gameplay::ResolveRunCompleteInput(true, false, true, false, failedPlayAgain)
            == gameplay::RunCompleteInputAction::RestartCurrentLevel,
        "R still restarts after a failed Play Again");
    Expect(
        gameplay::ResolveRunCompleteInput(true, false, true, true, failedPlayAgain)
            == gameplay::RunCompleteInputAction::ReturnToMainMenu,
        "Esc still returns to Main Menu after a failed Play Again");

    WriteAll(staged01, "NOT_A_LEVEL\n");
    const gameplay::LevelTransitionPrepareResult malformed =
        gameplay::PrepareStagedLevelDestination(staged01, world::kLevel01Id);
    Expect(malformed.status == gameplay::LevelTransitionPrepareStatus::Invalid,
        "malformed staged level_01 is invalid");
    world::LevelDefinition afterMalformed = level01.level;
    Expect(!gameplay::TryCommitPreparedDestination(afterMalformed, malformed),
        "malformed Play Again does not replace");

    const std::filesystem::path sourceTreePath =
        scratch / "source" / "levels" / "level_01.level";
    WriteAll(sourceTreePath, world::SerializeLevelText(level01.level));
    const gameplay::LevelTransitionPrepareResult sourceAttempt =
        gameplay::PrepareStagedLevelDestination(sourceTreePath, world::kLevel01Id);
    Expect(
        sourceAttempt.status != gameplay::LevelTransitionPrepareStatus::Ready
            || sourceTreePath.string().find("assets") == std::string::npos,
        "source-tree path is not the staged runtime convention");

    gameplay::Inventory carry;
    Expect(carry.TryAdd("key", 1), "seed linked Inventory");
    gameplay::ApplyInventoryLifecycle(carry, gameplay::InventoryLifecycleEvent::LevelTransition);
    Expect(carry.GetQuantity("key") == 1, "linked Level transition still preserves Inventory");
    gameplay::PlayerHealthState carryHealth{};
    gameplay::HazardContactState carryContact{};
    gameplay::InitializePlayerHealth(carryHealth);
    gameplay::TickHazardContactDamage(carryHealth, carryContact, true, 1.0f / 60.0f, true);
    const int carryHealthBefore = carryHealth.currentHealth;
    gameplay::ResetHazardContactState(carryContact);
    Expect(
        carryHealth.currentHealth == carryHealthBefore,
        "successful destination transition preserves Health");
    Expect(
        carryContact.cooldownRemaining == 0.0f,
        "destination replace resets local Hazard contact only");
    gameplay::RunCompleteState afterTransition{};
    gameplay::ResetRunCompleteState(afterTransition);
    Expect(!afterTransition.active, "successful destination does not leak results");

    gameplay::RunCompleteState applyState{};
    Expect(gameplay::TryEnterRunCompleteFromTerminalGoal(applyState, "", 4.0), "seed Apply leak");
    gameplay::ResetRunCompleteState(applyState);
    Expect(!applyState.active, "Apply/Reload/Open-Switch clears results");

    gameplay::RunCompleteState checkpointState{};
    gameplay::RespawnState checkpointRespawn{};
    checkpointRespawn.activeCheckpointIndex = 0;
    Expect(!checkpointState.active, "checkpoint does not fabricate results");
    Expect(
        !gameplay::TryEnterRunCompleteFromTerminalGoal(checkpointState, "level_02", 1.0),
        "PhysicsWorld rebuild path cannot invent terminal results");

    gameplay::SessionBestTimeState session{};
    Expect(gameplay::IsBetterSessionCompletion(session, 12.5), "session best still compares");
    session.hasBestTime = true;
    session.bestSeconds = 10.0;
    Expect(!gameplay::IsBetterSessionCompletion(session, 12.5), "slower terminal does not replace");
    Expect(!gameplay::kRunCompleteShowsSessionBest,
        "BEST stays off the results overlay without changing SessionBestTimeState");

    const world::ParseLevelFileResult level02 = world::LoadLevelFile(PLATFORMER_LEVEL02_SOURCE_PATH);
    Expect(level02.status == world::LoadLevelFileStatus::Loaded, "canonical level_02 loads");
    Expect(level02.level.levelGoals.size() == 1, "canonical level_02 has one terminal Goal");
    Expect(level02.level.levelGoals[0].nextLevelId.empty(), "canonical level_02 Goal is terminal");
    Expect(world::kLevel01LevelGoalCount == 1, "canonical Level 01 goal count");
    gameplay::FormatGameplayLevelLabel(hudLabel, sizeof(hudLabel), level02.level.id);
    gameplay::FormatGameplayObjectiveLine(
        hudObjective, sizeof(hudObjective), level02.level.levelGoals);
    Expect(std::string_view(hudLabel) == "LEVEL 02", "canonical level_02 HUD label");
    Expect(
        std::string_view(hudObjective) == gameplay::kGameplayObjectiveHudTerminalText,
        "canonical level_02 terminal objective");
    world::LevelDefinition hudAuthored = level01.level;
    gameplay::FormatGameplayLevelLabel(hudLabel, sizeof(hudLabel), hudAuthored.id);
    gameplay::FormatGameplayObjectiveLine(
        hudObjective, sizeof(hudObjective), hudAuthored.levelGoals);
    Expect(
        world::AuthoredLevelDataEqual(hudAuthored, level01.level),
        "objective HUD does not mutate canonical authored data");

    const std::string level01Text = ReadAll(PLATFORMER_LEVEL01_SOURCE_PATH);
    const std::string level02Text = ReadAll(PLATFORMER_LEVEL02_SOURCE_PATH);
    Expect(level01Text.find("goal -21 3.8 0 2 1.6 1.8") == std::string::npos,
        "canonical level_01 has no legacy goal singleton");
    Expect(level01Text.find("dynamic_box 0 5 0 1 1 1 30") == std::string::npos,
        "canonical level_01 has no legacy dynamic_box probe");
    Expect(level02Text.find("goal -21 3.8 0 2 1.6 1.8") == std::string::npos,
        "canonical level_02 has no legacy goal singleton");
    Expect(level02Text.find("level_3") == std::string::npos
            && level02Text.find("level_03") == std::string::npos,
        "canonical files do not name disposable level_3");

    gameplay::TopLevelFlow flow = gameplay::TopLevelFlow::MainMenu;
    gameplay::MainMenuState menu{};
    Expect(gameplay::MainMenuBlocksGameplay(flow), "Initialize enters Main Menu");
    Expect(!gameplay::RunTimerAdvancesInFlow(flow), "Main Menu does not advance timer");
    Expect(
        !gameplay::InventoryUiIsAvailable(flow, false, false),
        "Inventory cannot open in Main Menu");
    Expect(!gameplay::GameplayHudIsActive(flow, false), "Main Menu hides gameplay HUD");
    Expect(gameplay::MainMenuOverlayIsVisible(flow, false), "Main Menu overlay is visible");
    Expect(
        !gameplay::MainMenuOverlayIsVisible(flow, true),
        "F2 hides Main Menu overlay while editor is open");
    Expect(
        gameplay::GameplayHudIsActive(flow, true),
        "Development editor from Main Menu keeps editor chrome");
    Expect(
        gameplay::FlowAfterEditorToggle(flow) == gameplay::TopLevelFlow::MainMenu,
        "F2 round-trip preserves Main Menu");
    Expect(
        gameplay::FlowAfterEditorOpenOrSwitch(flow) == gameplay::TopLevelFlow::MainMenu,
        "editor Open/Switch does not fabricate Gameplay");
    Expect(
        !gameplay::EscapeClosesWindowInFlow(flow, false, false, false),
        "Esc does not close the window on Main Menu");

    gameplay::Inventory menuInventory;
    Expect(menuInventory.TryAdd("key", 1), "seed Inventory before Main Menu idle");
    gameplay::InventoryUiState menuUi{};
    Expect(
        !gameplay::InventoryUiIsAvailable(flow, false, false),
        "Tab/E stay inactive because Inventory UI is unavailable in Main Menu");
    Expect(menuInventory.GetQuantity("key") == 1,
        "Main Menu idle does not apply a fresh-run Inventory reset");
    gameplay::PlayerHealthState menuHealth{};
    gameplay::HazardContactState menuContact{};
    gameplay::InitializePlayerHealth(menuHealth);
    Expect(
        !gameplay::HazardDamageIsAllowed(flow, false, false, false, false, false),
        "Main Menu prevents Hazard damage");
    Expect(
        !gameplay::TickHazardContactDamage(menuHealth, menuContact, true, 1.0f / 60.0f, false),
        "Main Menu overlapping Hazard does not damage");
    Expect(
        menuHealth.currentHealth == gameplay::kMaxPlayerHealth,
        "Main Menu idle leaves Health at maximum");
    Expect(
        !gameplay::HealthHudIsVisible(flow, false, false, false, false),
        "Main Menu hides Health HUD");

    gameplay::RunCompleteState menuCannotFabricate{};
    Expect(
        gameplay::SelectCompletionPresentation(
            flow, true, true, menuCannotFabricate)
            == gameplay::CompletionPresentation::None,
        "Main Menu cannot fabricate destination or Run Complete HUD");
    Expect(
        !gameplay::ResultsHudShowsRunComplete(flow, menuCannotFabricate),
        "Main Menu hides results HUD");

    Expect(menu.selected == gameplay::MainMenuItem::Play, "PLAY is the default selection");
    Expect(
        gameplay::ResolveMainMenuInput(false, false, false, true, menu, flow)
            == gameplay::MainMenuInputAction::None,
        "menu Enter cannot fire unless Main Menu was already visible");
    Expect(
        gameplay::ResolveMainMenuInput(true, false, true, false, menu, flow)
            == gameplay::MainMenuInputAction::None,
        "Down moves selection without activating");
    Expect(menu.selected == gameplay::MainMenuItem::Quit, "Down selects QUIT");
    Expect(
        gameplay::ResolveMainMenuInput(true, true, false, false, menu, flow)
            == gameplay::MainMenuInputAction::None,
        "Up returns to PLAY");
    Expect(menu.selected == gameplay::MainMenuItem::Play, "Up selects PLAY");
    Expect(
        gameplay::ResolveMainMenuInput(true, true, true, false, menu, flow)
            == gameplay::MainMenuInputAction::None,
        "Up+Down together is a no-op");
    Expect(menu.selected == gameplay::MainMenuItem::Play, "Up+Down leaves PLAY selected");
    Expect(
        gameplay::ResolveMainMenuInput(true, false, true, true, menu, flow)
            == gameplay::MainMenuInputAction::Play,
        "Enter activates PLAY without also moving");
    Expect(menu.selected == gameplay::MainMenuItem::Play, "activate uses the captured PLAY item");

    gameplay::RequestPlayFromMainMenu(menu, flow);
    Expect(gameplay::PlayIsInFlight(menu), "Play requests a fresh run exactly once");
    gameplay::RequestPlayFromMainMenu(menu, flow);
    Expect(menu.playPending, "repeated Enter cannot double-schedule Play");
    Expect(
        gameplay::ResolveMainMenuInput(true, false, false, true, menu, flow)
            == gameplay::MainMenuInputAction::None,
        "in-flight Play ignores further menu Enter");

    gameplay::MarkPlayFailed(menu, "Initial staged level is missing. Main Menu unchanged.");
    Expect(flow == gameplay::TopLevelFlow::MainMenu, "Play failure remains in Main Menu");
    Expect(!gameplay::PlayIsInFlight(menu), "failed Play is not in flight");
    Expect(menu.playFailed, "Play failure is sticky until the next explicit Play");
    gameplay::RequestPlayFromMainMenu(menu, flow);
    Expect(gameplay::PlayIsInFlight(menu), "a later Enter may retry Play without a per-frame storm");
    menu.playPending = false;

    gameplay::NavigateMainMenu(menu, 1);
    Expect(
        gameplay::ResolveMainMenuInput(true, false, false, true, menu, flow)
            == gameplay::MainMenuInputAction::Quit,
        "Enter on QUIT requests close");
    gameplay::RequestQuitFromMainMenu(menu, flow);
    Expect(menu.quitRequested, "Quit requests normal close");
    gameplay::RequestPlayFromMainMenu(menu, flow);
    Expect(!menu.playPending, "Quit blocks a same-state Play request");

    gameplay::ResetMainMenuState(menu);
    const std::filesystem::path menuMissing =
        scratch / "menu-missing" / "assets" / "levels" / "level_01.level";
    world::LevelDefinition menuPreserved = level01.level;
    const gameplay::LevelTransitionPrepareResult menuMissingPrepare =
        gameplay::PrepareStagedLevelDestination(menuMissing, world::kLevel01Id);
    Expect(menuMissingPrepare.status == gameplay::LevelTransitionPrepareStatus::Missing,
        "missing staged level_01 is atomic for Play");
    Expect(!gameplay::TryCommitPreparedDestination(menuPreserved, menuMissingPrepare),
        "failed Play does not replace");
    Expect(world::AuthoredLevelDataEqual(menuPreserved, level01.level),
        "failed Play leaves authored data intact");
    gameplay::FormatGameplayLevelLabel(hudLabel, sizeof(hudLabel), menuPreserved.id);
    Expect(std::string_view(hudLabel) == "LEVEL 01", "failed Play does not present a destination Level");
    Expect(
        !gameplay::ObjectiveHudIsVisible(flow, false, false, false, false),
        "failed Play remains without objective HUD");
    Expect(
        !gameplay::HealthHudIsVisible(flow, false, false, false, false),
        "failed Play remains without Health HUD");
    Expect(
        menuMissing.string().find("source") == std::string::npos
            || menuMissingPrepare.status != gameplay::LevelTransitionPrepareStatus::Ready,
        "Play does not source-fallback");

    gameplay::PauseMenuState playPause{};
    gameplay::EnterGameplayFromSuccessfulPlay(flow, menu, playPause);
    Expect(flow == gameplay::TopLevelFlow::Gameplay, "successful Play enters Gameplay once");
    Expect(!playPause.active, "successful Play leaves Pause inactive");
    Expect(!menu.playPending && !menu.quitRequested, "successful Play clears menu transients");
    Expect(
        gameplay::ObjectiveHudIsVisible(flow, false, false, false, false),
        "Play from Main Menu shows objective HUD");
    gameplay::ResetPlayerHealthForNewRuntime(menuHealth, menuContact);
    Expect(
        menuHealth.currentHealth == gameplay::kMaxPlayerHealth,
        "Main Menu PLAY restores maximum Health");
    Expect(
        gameplay::HealthHudIsVisible(flow, false, false, false, false),
        "Play from Main Menu shows Health HUD");
    Expect(
        !gameplay::ObjectiveHudIsVisible(flow, false, true, false, false),
        "Inventory hides objective HUD");
    Expect(
        !gameplay::HealthHudIsVisible(flow, false, true, false, false),
        "Inventory hides Health HUD with the existing Gameplay HUD convention");
    Expect(
        !gameplay::HazardDamageIsAllowed(flow, false, true, false, false, false),
        "Inventory gameplay blocking prevents Hazard damage");
    Expect(
        !gameplay::ObjectiveHudIsVisible(flow, true, false, false, false),
        "F2/editor hides objective HUD");
    Expect(
        !gameplay::HealthHudIsVisible(flow, true, false, false, false),
        "F2/editor hides Health HUD");
    Expect(
        !gameplay::HazardDamageIsAllowed(flow, true, false, false, false, false),
        "Development editor gameplay blocking prevents Hazard damage");
    gameplay::FormatGameplayLevelLabel(hudLabel, sizeof(hudLabel), world::kLevel02Id);
    gameplay::FormatGameplayObjectiveLine(
        hudObjective, sizeof(hudObjective), level02.level.levelGoals);
    Expect(std::string_view(hudLabel) == "LEVEL 02", "successful transition refreshes LEVEL 02");
    Expect(
        std::string_view(hudObjective) == gameplay::kGameplayObjectiveHudTerminalText,
        "successful transition refreshes terminal objective");
    gameplay::LevelTransitionSchedule failedReplace{};
    gameplay::CaptureCompletedGoalDestination(failedReplace, world::kLevel02Id);
    gameplay::MarkLevelTransitionFailed(failedReplace, "destination missing");
    gameplay::FormatGameplayLevelLabel(hudLabel, sizeof(hudLabel), world::kLevel01Id);
    Expect(
        std::string_view(hudLabel) == "LEVEL 01",
        "failed transition does not display destination before replace");
    gameplay::PlayerHealthState failedHealth{};
    gameplay::HazardContactState failedContact{};
    gameplay::InitializePlayerHealth(failedHealth);
    gameplay::TickHazardContactDamage(failedHealth, failedContact, true, 1.0f / 60.0f, true);
    const int failedHealthCaptured = failedHealth.currentHealth;
    Expect(!gameplay::TryCommitPreparedDestination(menuPreserved, menuMissingPrepare),
        "failed destination replace is atomic");
    Expect(
        failedHealth.currentHealth == failedHealthCaptured,
        "failed destination transition preserves Health atomically");
    Expect(gameplay::RunTimerAdvancesInFlow(flow), "Gameplay timer may advance after Play");
    Expect(
        gameplay::InventoryUiIsAvailable(flow, false, false),
        "Inventory is available only after Play");
    Expect(
        !gameplay::EscapeClosesWindowInFlow(flow, false, false, false),
        "ordinary Gameplay Esc does not close the window");
    Expect(
        !gameplay::EscapeClosesWindowInFlow(flow, false, true, false),
        "Esc during Run Complete does not close the window");
    Expect(
        !gameplay::EscapeClosesWindowInFlow(flow, false, false, false, true),
        "Esc during Pause does not close the window");

    gameplay::RunCompleteState resultsToMenu{};
    Expect(
        gameplay::TryEnterRunCompleteFromTerminalGoal(resultsToMenu, "", 9.0),
        "seed Run Complete before Esc");
    Expect(
        gameplay::ResultsHudShowsRunComplete(flow, resultsToMenu),
        "results HUD advertises Run Complete before Esc");
    Expect(
        !gameplay::ObjectiveHudIsVisible(flow, false, false, resultsToMenu.active, false),
        "Run Complete hides objective HUD");
    Expect(
        !gameplay::HealthHudIsVisible(flow, false, false, resultsToMenu.active, false),
        "Run Complete hides Health HUD");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            flow, false, false, resultsToMenu.active, true, false),
        "RUN COMPLETE prevents Hazard damage");
    Expect(
        gameplay::ResolveRunCompleteInput(true, false, false, true, resultsToMenu)
            == gameplay::RunCompleteInputAction::ReturnToMainMenu,
        "Esc Run Complete enters Main Menu exactly once");
    gameplay::PauseMenuState resultsPause{};
    gameplay::EnterMainMenu(flow, menu, resultsToMenu, resultsPause);
    Expect(flow == gameplay::TopLevelFlow::MainMenu, "Esc leaves Gameplay");
    Expect(!resultsPause.active, "Run Complete Esc does not enter Pause");
    Expect(!resultsToMenu.active, "Esc stops showing Run Complete");
    Expect(
        !gameplay::ObjectiveHudIsVisible(flow, false, false, resultsToMenu.active, false),
        "Run Complete to Main Menu hides objective HUD");
    Expect(menu.selected == gameplay::MainMenuItem::Play, "returned Main Menu selection is fresh");
    Expect(
        gameplay::ResolveRunCompleteInput(true, true, true, true, resultsToMenu)
            == gameplay::RunCompleteInputAction::None,
        "stale result input cannot leak into Main Menu");
    Expect(
        gameplay::ResolveMainMenuInput(false, false, false, true, menu, flow)
            == gameplay::MainMenuInputAction::None,
        "result-frame Enter cannot also activate PLAY");

    gameplay::RequestPlayFromMainMenu(menu, flow);
    Expect(gameplay::PlayIsInFlight(menu), "Play after Run Complete→Menu is a fresh-run request");
    gameplay::EnterGameplayFromSuccessfulPlay(flow, menu, resultsPause);
    Expect(flow == gameplay::TopLevelFlow::Gameplay, "Play after menu return enters Gameplay");
    Expect(
        gameplay::ObjectiveHudIsVisible(flow, false, false, false, false),
        "Play after Main Menu return restores objective HUD");
    gameplay::FormatGameplayLevelLabel(hudLabel, sizeof(hudLabel), world::kLevel01Id);
    gameplay::FormatGameplayObjectiveLine(
        hudObjective, sizeof(hudObjective), level01.level.levelGoals);
    Expect(std::string_view(hudLabel) == "LEVEL 01", "fresh Play presents LEVEL 01");
    Expect(
        std::string_view(hudObjective) == gameplay::kGameplayObjectiveHudDestinationText,
        "fresh Play presents destination objective");
    Expect(
        Vec3Equal(level01.level.initialSpawnVisualCenter, {0.0f, 0.8f, 0.0f}),
        "Play starts at level_01 spawn");

    gameplay::RunCompleteState noDouble{};
    Expect(gameplay::TryEnterRunCompleteFromTerminalGoal(noDouble, "", 3.0), "seed arbitration");
    Expect(
        gameplay::ResolveRunCompleteInput(true, true, true, true, noDouble)
            == gameplay::RunCompleteInputAction::PlayAgain,
        "result arbitration emits one action");
    gameplay::RequestPlayAgain(noDouble);
    Expect(
        gameplay::ResolveRunCompleteInput(true, false, true, true, noDouble)
            == gameplay::RunCompleteInputAction::None,
        "in-flight results action cannot double-trigger");

    Expect(std::string_view(gameplay::kPauseMenuTitle) == "PAUSED", "Pause title");
    Expect(std::string_view(gameplay::kPauseMenuResumeLabel) == "RESUME", "RESUME label");
    Expect(
        std::string_view(gameplay::kPauseMenuMainMenuLabel) == "MAIN MENU",
        "Pause MAIN MENU label");

    gameplay::TopLevelFlow pauseFlow = gameplay::TopLevelFlow::Gameplay;
    gameplay::PauseMenuState pause{};
    gameplay::MainMenuState pauseMenu{};
    gameplay::RunCompleteState pauseResults{};
    gameplay::RunTimerState pauseTimer{};
    pauseTimer.elapsedSeconds = 7.25;
    gameplay::Inventory pauseInventory;
    Expect(pauseInventory.TryAdd("key", 1), "seed Inventory before Pause");
    gameplay::ItemPickupRunState pausePickups = gameplay::MakeClearedItemPickupRunState(1);
    pausePickups.collected[0] = 1;
    gameplay::DoorLockRunState pauseDoors{};
    pauseDoors.unlocked = {1};
    gameplay::LevelCompletionState pauseCompletion{};
    std::string pauseRuntimeId{world::kLevel01Id};
    world::LevelDefinition pauseAuthored = level01.level;
    const bool pauseDirtyBefore = false;

    Expect(
        gameplay::PauseCanBeEntered(pauseFlow, false, false, false, false, false),
        "Esc during active Gameplay can enter Pause");
    Expect(
        gameplay::ResolvePauseInput(
            false, true, false, false, false, true, pause, pauseFlow)
            == gameplay::PauseMenuInputAction::EnterPause,
        "Gameplay Esc enters Pause instead of closing");
    gameplay::EnterPause(pause);
    Expect(pause.active, "Pause is active");
    Expect(pause.selected == gameplay::PauseMenuItem::Resume, "RESUME is the default selection");
    Expect(
        gameplay::PauseBlocksGameplay(false, true),
        "entering Pause blocks simulation on that frame");
    Expect(
        gameplay::PauseBlocksGameplay(true, true),
        "Pause freezes gameplay simulation while active");
    Expect(!gameplay::RunTimerAdvancesInFlow(pauseFlow, pause.active), "Pause freezes the run timer");
    Expect(
        !gameplay::InventoryUiIsAvailable(pauseFlow, false, false, pause.active),
        "Pause disables Inventory open/E");
    Expect(
        gameplay::PauseMenuOverlayIsVisible(pauseFlow, false, pause.active, false),
        "Pause overlay is visible during Gameplay");
    Expect(
        !gameplay::ObjectiveHudIsVisible(pauseFlow, false, false, false, false, pause.active),
        "Pause suppresses the objective HUD");
    Expect(
        !gameplay::EscapeClosesWindowInFlow(pauseFlow, false, false, false, pause.active),
        "Pause Esc does not quit");

    const double pauseTimerCaptured = pauseTimer.elapsedSeconds;
    if (gameplay::RunTimerAdvancesInFlow(pauseFlow, pause.active) && !pauseTimer.frozen)
    {
        pauseTimer.elapsedSeconds += 0.16;
    }
    Expect(pauseTimer.elapsedSeconds == pauseTimerCaptured, "paused timer does not advance");
    Expect(pauseInventory.GetQuantity("key") == 1, "Pause preserves Inventory");
    Expect(pausePickups.collected[0] == 1, "Pause preserves pickup collected flags");
    Expect(pauseDoors.unlocked[0] == 1, "Pause preserves Door unlock flags");
    Expect(pauseRuntimeId == world::kLevel01Id, "Pause preserves currentRuntimeLevelId");
    Expect(!pauseCompletion.completed, "Pause does not fabricate completion");
    Expect(
        world::AuthoredLevelDataEqual(pauseAuthored, level01.level),
        "Pause does not mutate authored Level data");
    Expect(!pauseDirtyBefore, "Pause does not mark Dirty");

    gameplay::PlayerHealthState pauseHealth{};
    gameplay::HazardContactState pauseContact{};
    gameplay::InitializePlayerHealth(pauseHealth);
    Expect(
        gameplay::TickHazardContactDamage(pauseHealth, pauseContact, true, 1.0f / 60.0f, true),
        "seed Hazard damage before Pause");
    const int pauseHealthCaptured = pauseHealth.currentHealth;
    const float pauseCooldownCaptured = pauseContact.cooldownRemaining;
    Expect(
        !gameplay::HazardDamageIsAllowed(
            pauseFlow, false, false, false, false, pause.active),
        "Pause prevents Hazard damage");
    Expect(
        !gameplay::TickHazardContactDamage(
            pauseHealth,
            pauseContact,
            true,
            gameplay::kHazardDamageCadenceSeconds,
            gameplay::HazardDamageIsAllowed(
                pauseFlow, false, false, false, false, pause.active)),
        "paused overlap does not apply Hazard damage");
    Expect(pauseHealth.currentHealth == pauseHealthCaptured, "Pause preserves damaged Health");
    Expect(
        pauseContact.cooldownRemaining == pauseCooldownCaptured,
        "Pause does not progress Hazard cadence");
    Expect(
        !gameplay::HealthHudIsVisible(pauseFlow, false, false, false, false, pause.active),
        "Pause suppresses the Health HUD");

    Expect(
        gameplay::ResolvePauseInput(
            false, true, false, false, false, true, pause, pauseFlow)
            == gameplay::PauseMenuInputAction::EnterPause,
        "the entering Esc edge cannot Resume because Pause was not active at frame start");
    Expect(pause.active, "same-edge Esc leaves Pause active");
    Expect(
        gameplay::ResolvePauseInput(
            true, false, false, false, false, true, pause, pauseFlow)
            == gameplay::PauseMenuInputAction::Resume,
        "Esc while paused resumes");
    gameplay::ResumePause(pause);
    Expect(!pause.active, "RESUME clears Pause");
    Expect(
        gameplay::PauseBlocksGameplay(true, false),
        "Resume still blocks gameplay on the carry-through frame");
    Expect(gameplay::RunTimerAdvancesInFlow(pauseFlow, pause.active), "Resume allows the timer again");
    Expect(pauseTimer.elapsedSeconds == pauseTimerCaptured, "Resume preserves the frozen timer value");
    Expect(pauseInventory.GetQuantity("key") == 1, "Resume preserves Inventory");
    Expect(pausePickups.collected[0] == 1, "Resume preserves pickups");
    Expect(pauseDoors.unlocked[0] == 1, "Resume preserves Door unlock");
    Expect(pauseRuntimeId == world::kLevel01Id, "Resume preserves currentRuntimeLevelId");
    Expect(
        world::AuthoredLevelDataEqual(pauseAuthored, level01.level),
        "Resume does not mutate authored data");
    Expect(
        gameplay::ObjectiveHudIsVisible(pauseFlow, false, false, false, false, pause.active),
        "Resume restores the objective HUD");
    Expect(pauseHealth.currentHealth == pauseHealthCaptured, "Resume preserves Health");
    Expect(
        pauseContact.cooldownRemaining == pauseCooldownCaptured,
        "Resume does not catch up Hazard cadence");
    Expect(
        gameplay::HealthHudIsVisible(pauseFlow, false, false, false, false, pause.active),
        "Resume restores the Health HUD");

    gameplay::EnterPause(pause);
    Expect(
        gameplay::ResolvePauseInput(
            true, false, false, true, false, false, pause, pauseFlow)
            == gameplay::PauseMenuInputAction::None,
        "Down moves Pause selection without activating");
    Expect(pause.selected == gameplay::PauseMenuItem::MainMenu, "Down selects MAIN MENU");
    Expect(
        gameplay::ResolvePauseInput(
            true, false, false, false, false, true, pause, pauseFlow)
            == gameplay::PauseMenuInputAction::Resume,
        "Esc resumes even when MAIN MENU is selected");
    Expect(
        gameplay::ResolvePauseInput(
            true, false, false, false, true, false, pause, pauseFlow)
            == gameplay::PauseMenuInputAction::ReturnToMainMenu,
        "Enter on MAIN MENU abandons the run");
    gameplay::EnterMainMenu(pauseFlow, pauseMenu, pauseResults, pause);
    Expect(pauseFlow == gameplay::TopLevelFlow::MainMenu, "Pause MAIN MENU enters Main Menu");
    Expect(!pause.active, "Pause UI is cleared on Main Menu");
    Expect(!pauseResults.active, "Pause MAIN MENU does not fabricate Run Complete");
    Expect(gameplay::MainMenuOverlayIsVisible(pauseFlow, false), "existing Main Menu is visible");
    Expect(gameplay::MainMenuBlocksGameplay(pauseFlow), "Main Menu keeps simulation inactive");
    Expect(
        !gameplay::ObjectiveHudIsVisible(pauseFlow, false, false, false, false, pause.active),
        "Main Menu from Pause hides objective HUD");
    Expect(
        !gameplay::HealthHudIsVisible(pauseFlow, false, false, false, false, pause.active),
        "Main Menu from Pause hides Health HUD");
    Expect(
        !gameplay::HazardDamageIsAllowed(pauseFlow, false, false, false, false, pause.active),
        "Main Menu from Pause prevents Hazard damage");
    Expect(
        world::AuthoredLevelDataEqual(pauseAuthored, level01.level),
        "Pause MAIN MENU does not mutate authored data");

    gameplay::RequestPlayFromMainMenu(pauseMenu, pauseFlow);
    Expect(gameplay::PlayIsInFlight(pauseMenu), "PLAY after Pause MAIN MENU uses existing Play");
    WriteAll(staged01, world::SerializeLevelText(level01.level));
    const gameplay::LevelTransitionPrepareResult pausePlayReady =
        gameplay::PrepareStagedLevelDestination(staged01, world::kLevel01Id);
    Expect(
        pausePlayReady.status == gameplay::LevelTransitionPrepareStatus::Ready,
        "PLAY after Pause still prepares staged level_01");
    Expect(
        gameplay::PlayAgainLoadsInitialRuntimeLevel(pausePlayReady.candidate.id),
        "PLAY after Pause starts staged level_01");
    gameplay::EnterGameplayFromSuccessfulPlay(pauseFlow, pauseMenu, pause);
    Expect(pauseFlow == gameplay::TopLevelFlow::Gameplay, "PLAY after Pause enters Gameplay");
    Expect(!pause.active, "fresh PLAY leaves Pause inactive");
    gameplay::ResetPlayerHealthForNewRuntime(pauseHealth, pauseContact);
    Expect(
        pauseHealth.currentHealth == gameplay::kMaxPlayerHealth,
        "PLAY after Pause MAIN MENU restores maximum Health");

    Expect(
        !gameplay::PauseCanBeEntered(
            gameplay::TopLevelFlow::Gameplay, false, true, false, false, false),
        "Inventory Esc is consumed before Pause can be entered");
    Expect(
        gameplay::ResolvePauseInput(
            false, false, false, false, false, true, pause, gameplay::TopLevelFlow::Gameplay)
            == gameplay::PauseMenuInputAction::None,
        "Esc while Inventory blocks gameplay does not enter Pause");
    Expect(
        gameplay::PauseCanBeEntered(
            gameplay::TopLevelFlow::Gameplay, false, false, false, false, false),
        "a later Esc after Inventory close may enter Pause");

    Expect(
        !gameplay::PauseCanBeEntered(
            gameplay::TopLevelFlow::Gameplay, false, false, false, true, false),
        "destination LEVEL COMPLETE keeps authority over Pause");
    Expect(
        gameplay::SelectCompletionPresentation(
            gameplay::TopLevelFlow::Gameplay, true, true, pauseResults)
            == gameplay::CompletionPresentation::DestinationContinue,
        "destination completion overlay remains LEVEL COMPLETE");
    Expect(
        !gameplay::HealthHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, false, false, false, true),
        "destination LEVEL COMPLETE hides Health HUD");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, false, false, false, true, false),
        "destination LEVEL COMPLETE prevents Hazard damage");

    gameplay::RunCompleteState runCompleteOwnsEsc{};
    Expect(
        gameplay::TryEnterRunCompleteFromTerminalGoal(runCompleteOwnsEsc, "", 4.0),
        "seed Run Complete for Pause arbitration");
    Expect(
        !gameplay::PauseCanBeEntered(
            gameplay::TopLevelFlow::Gameplay, false, false, true, true, false),
        "Run Complete cannot enter Pause");
    Expect(
        gameplay::ResolveRunCompleteInput(true, false, false, true, runCompleteOwnsEsc)
            == gameplay::RunCompleteInputAction::ReturnToMainMenu,
        "Run Complete Esc MAIN MENU is unchanged");
    Expect(
        gameplay::ResolveRunCompleteInput(true, true, true, true, runCompleteOwnsEsc)
            == gameplay::RunCompleteInputAction::PlayAgain,
        "Run Complete still uses Enter > Esc > R");
    Expect(
        !gameplay::PauseMenuOverlayIsVisible(
            gameplay::TopLevelFlow::Gameplay, false, false, true),
        "Pause is not shown over Run Complete");
    Expect(
        !gameplay::HealthHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, false, false, true, true),
        "Run Complete hides Health HUD while results own the overlay");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, false, false, true, true, false),
        "Run Complete prevents Hazard damage");
    Expect(
        !gameplay::PauseMenuOverlayIsVisible(
            gameplay::TopLevelFlow::MainMenu, false, true, false),
        "Pause is not shown over Main Menu");

    gameplay::PauseMenuState editorPause{};
    gameplay::EnterPause(editorPause);
    Expect(
        gameplay::FlowAfterEditorToggle(gameplay::TopLevelFlow::Gameplay)
            == gameplay::TopLevelFlow::Gameplay,
        "F2 from Pause keeps Gameplay flow");
    Expect(editorPause.active, "F2 round-trip preserves Pause");
    Expect(
        !gameplay::PauseMenuOverlayIsVisible(
            gameplay::TopLevelFlow::Gameplay, true, editorPause.active, false),
        "F2 hides Pause overlay while the editor is open");
    Expect(
        gameplay::PauseBlocksGameplay(true, editorPause.active),
        "closing F2 does not convert Pause into active Gameplay");
    Expect(
        gameplay::FlowAfterEditorOpenOrSwitch(gameplay::TopLevelFlow::Gameplay)
            == gameplay::TopLevelFlow::Gameplay,
        "Open/Switch is not introduced as Pause behavior");

    gameplay::PauseMenuState restartPause{};
    gameplay::EnterPause(restartPause);
    gameplay::ResumePause(restartPause);
    Expect(!restartPause.active, "Restart after Resume sees inactive Pause");
    Expect(
        gameplay::InventoryUiIsAvailable(
            gameplay::TopLevelFlow::Gameplay, false, false, restartPause.active),
        "Restart after Resume uses ordinary Gameplay availability");

    gameplay::PlayerDeathState flowDeath{};
    Expect(
        gameplay::TryBeginPlayerDeath(flowDeath, gameplay::kHazardDamageAmount, 0),
        "lethal Health transition starts death from Gameplay");
    Expect(
        !gameplay::PauseCanBeEntered(
            gameplay::TopLevelFlow::Gameplay, false, false, false, false, false, true),
        "death prevents Pause from opening");
    Expect(
        !gameplay::InventoryUiIsAvailable(
            gameplay::TopLevelFlow::Gameplay, false, false, false, true),
        "death prevents Inventory from opening");
    Expect(
        gameplay::ResolvePauseInput(
            false, false, false, false, false, true, restartPause, gameplay::TopLevelFlow::Gameplay)
            == gameplay::PauseMenuInputAction::None,
        "Esc during death does not enter Pause");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, false, false, false, false, false, true),
        "death blocks Hazard damage");
    Expect(
        !gameplay::TryBeginPlayerDeath(flowDeath, 0, 0),
        "F2-blocked death does not duplicate itself");
    gameplay::ClearPlayerDeath(flowDeath);
    Expect(
        gameplay::PauseCanBeEntered(
            gameplay::TopLevelFlow::Gameplay, false, false, false, false, false, false),
        "Pause returns after death respawn completes");

    std::filesystem::remove_all(scratch, cleanupError);

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d GameFlowTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("GameFlowTest passed\n");
    return 0;
}
