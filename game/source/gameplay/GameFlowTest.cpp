#include "gameplay/CollectibleRunState.h"
#include "gameplay/DoorLockRuntime.h"
#include "gameplay/GameFlowState.h"
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
            destinationCompletion.completed,
            gameplay::DestinationCompletionShowsContinueHint(destinationHold),
            destinationResults)
            == gameplay::CompletionPresentation::DestinationContinue,
        "destination HUD is LEVEL COMPLETE continue, not Run Complete");

    Expect(
        gameplay::ResolveRunCompleteInput(false, true, true, runComplete)
            == gameplay::RunCompleteInputAction::None,
        "completion frame cannot Play Again or Restart");
    Expect(
        gameplay::ResolveRunCompleteInput(true, true, true, runComplete)
            == gameplay::RunCompleteInputAction::PlayAgain,
        "Enter Play Again wins over R on the same edge frame");
    Expect(
        gameplay::ResolveRunCompleteInput(true, false, true, runComplete)
            == gameplay::RunCompleteInputAction::RestartCurrentLevel,
        "R restarts the current terminal Level");
    Expect(
        gameplay::ResolveRunCompleteInput(true, false, false, runComplete)
            == gameplay::RunCompleteInputAction::None,
        "held absence does not emit an action");

    gameplay::RequestPlayAgain(runComplete);
    Expect(gameplay::PlayAgainIsInFlight(runComplete), "Enter Play Again arms exactly once");
    gameplay::RequestPlayAgain(runComplete);
    Expect(runComplete.playAgainPending, "repeated Enter cannot double-schedule Play Again");
    Expect(
        gameplay::ResolveRunCompleteInput(true, true, true, runComplete)
            == gameplay::RunCompleteInputAction::None,
        "in-flight Play Again ignores further Enter/R");

    const world::ParseLevelFileResult level01 = world::LoadLevelFile(PLATFORMER_LEVEL01_SOURCE_PATH);
    Expect(level01.status == world::LoadLevelFileStatus::Loaded, "canonical level_01 loads");
    Expect(level01.level.levelGoals.empty(), "canonical level_01 has zero Level Goals");
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
        gameplay::ResolveRunCompleteInput(true, true, false, failedPlayAgain)
            == gameplay::RunCompleteInputAction::None,
        "Enter cannot retry a failed Play Again");
    Expect(
        gameplay::ResolveRunCompleteInput(true, false, true, failedPlayAgain)
            == gameplay::RunCompleteInputAction::RestartCurrentLevel,
        "R still restarts after a failed Play Again");

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
    Expect(level02.level.levelGoals.empty(), "canonical level_02 stays fixture-free");
    Expect(world::kLevel01LevelGoalCount == 0, "canonical Level 01 goal count");

    std::filesystem::remove_all(scratch, cleanupError);

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d GameFlowTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("GameFlowTest passed\n");
    return 0;
}
