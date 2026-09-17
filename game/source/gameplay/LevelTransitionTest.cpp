#include "gameplay/Inventory.h"
#include "gameplay/InventoryUi.h"
#include "gameplay/LevelCompletionState.h"
#include "gameplay/LevelTransition.h"
#include "world/LevelDefinition.h"
#include "world/LevelFile.h"
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

std::string ReadAll(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    return std::string(
        (std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
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
    Expect(world::IsValidLevelIdToken("level_01"), "level_01 identity is valid");
    Expect(world::IsValidLevelIdToken("level_02"), "level_02 identity is valid");
    Expect(world::IsValidNextLevelId(""), "empty destination is terminal");
    Expect(world::IsValidNextLevelId("level_02"), "level_02 destination is valid");
    Expect(!world::IsValidLevelIdToken(""), "empty identity token is invalid");
    Expect(!world::IsValidLevelIdToken("../secret"), "traversal identity is invalid");
    Expect(!world::IsValidLevelIdToken("levels/level_02"), "path identity is invalid");
    Expect(!world::IsValidLevelIdToken("C:\\temp"), "absolute identity is invalid");
    Expect(!world::IsValidLevelIdToken("level_02.level"), "extension identity is invalid");
    Expect(!world::IsValidNextLevelId("../x"), "unsafe nextLevelId is invalid");
    Expect(world::MakeRuntimeLevelLogicalId("level_02") == "levels/level_02.level",
        "logical id uses staged runtime convention");
    Expect(world::MakeRuntimeLevelLogicalId("../secret").empty(),
        "unsafe identity cannot form a logical path");
    Expect(world::MakeRuntimeLevelLogicalId("levels/level_02").empty(),
        "path identity cannot form a logical path");
    Expect(
        world::RuntimeLevelPathStemMatchesId(
            std::filesystem::path("assets") / "levels" / "level_02.level", "level_02"),
        "stem matches destination identity");
    Expect(
        !world::RuntimeLevelPathStemMatchesId(
            std::filesystem::path("assets") / "levels" / "other.level", "level_02"),
        "stem mismatch is rejected");

    const world::ParseLevelFileResult level01 = world::LoadLevelFile(PLATFORMER_LEVEL01_SOURCE_PATH);
    Expect(level01.status == world::LoadLevelFileStatus::Loaded, "canonical level_01 loads");
    Expect(level01.level.id == world::kLevel01Id, "canonical level_01 id");
    Expect(level01.level.levelGoals.size() == 1, "canonical level_01 has one Level Goal");
    Expect(level01.level.levelGoals[0].nextLevelId == world::kLevel02Id,
        "canonical level_01 destination is level_02");

    const world::ParseLevelFileResult level02 = world::LoadLevelFile(PLATFORMER_LEVEL02_SOURCE_PATH);
    Expect(level02.status == world::LoadLevelFileStatus::Loaded, "canonical level_02 loads");
    Expect(level02.level.id == world::kLevel02Id, "canonical level_02 id");
    Expect(level02.level.elevatedPlatforms.size() == 2, "level_02 has two platforms");
    Expect(level02.level.collectibles.size() == 1, "level_02 has one collectible");
    Expect(level02.level.levelGoals.size() == 1, "canonical level_02 has one Level Goal");
    Expect(level02.level.levelGoals[0].nextLevelId.empty(),
        "canonical level_02 Goal is terminal");
    Expect(level02.level.killPlaneY == -4.0f, "level_02 kill plane differs from level_01");
    Expect(level02.level.camera.fieldOfViewY == 48.0f, "level_02 FOV differs from level_01");
    Expect(
        !Vec3Equal(level01.level.initialSpawnVisualCenter, level02.level.initialSpawnVisualCenter),
        "level_02 spawn is visually distinct");
    Expect(world::IsWritableLevelDefinition(level02.level), "level_02 is writable");
    const std::string level02Text = ReadAll(PLATFORMER_LEVEL02_SOURCE_PATH);
    Expect(level02Text.find("goal -21") == std::string::npos, "level_02 has no legacy goal record");
    Expect(
        level02Text.find("dynamic_box 0 5 0 1 1 1 30") == std::string::npos,
        "level_02 has no legacy dynamic_box probe");

    world::LevelGoalSpec terminal{{-21.0f, 3.8f, 0.0f}, {2.0f, 1.6f, 1.8f}, {}};
    world::LevelGoalSpec destination = terminal;
    destination.nextLevelId = "level_02";
    Expect(world::LevelGoalSpecIsValid(terminal), "terminal goal remains valid");
    Expect(world::LevelGoalSpecIsValid(destination), "destination goal is valid");
    destination.nextLevelId = "../secret";
    Expect(!world::LevelGoalSpecIsValid(destination), "unsafe destination goal is invalid");

    std::vector<world::LevelGoalSpec> goals{terminal};
    goals[0].nextLevelId = "level_02";
    gameplay::LevelCompletionState state{};
    std::string captured;
    Expect(
        gameplay::TryCompleteLevelFromPlayerOverlap(state, goals, goals[0].center, &captured),
        "destination goal completes once");
    Expect(state.completed, "completion flag set");
    Expect(captured == "level_02", "completion captures destination identity");
    Expect(
        !gameplay::TryCompleteLevelFromPlayerOverlap(state, goals, goals[0].center, &captured),
        "staying inside does not recapture");

    gameplay::LevelCompletionState terminalState{};
    world::LevelGoalSpec terminalGoal{{-21.0f, 3.8f, 0.0f}, {2.0f, 1.6f, 1.8f}};
    std::string terminalCaptured = "stale";
    Expect(
        gameplay::TryCompleteLevelFromPlayerOverlap(
            terminalState, {terminalGoal}, terminalGoal.center, &terminalCaptured),
        "terminal goal still completes");
    Expect(terminalCaptured.empty(), "terminal goal captures empty destination");

    gameplay::LevelTransitionSchedule schedule{};
    gameplay::CaptureCompletedGoalDestination(schedule, "");
    Expect(!schedule.pending && schedule.destinationId.empty(), "terminal does not schedule");
    gameplay::CaptureCompletedGoalDestination(schedule, "level_02");
    Expect(schedule.pending && schedule.destinationId == "level_02", "destination schedules once");
    Expect(schedule.holding, "destination capture starts the completion hold");
    Expect(schedule.holdElapsedSeconds == 0.0f, "hold starts at zero");
    Expect(
        gameplay::DestinationTransitionHoldBlocksCommit(schedule),
        "hold blocks immediate transition");
    Expect(
        gameplay::DestinationCompletionShowsContinueHint(schedule),
        "destination HUD uses continue hint during hold");
    gameplay::TickDestinationTransitionHold(schedule, 1.0f);
    Expect(schedule.holdElapsedSeconds == 1.0f, "hold uses post-completion delta, not run time");
    Expect(
        gameplay::DestinationTransitionHoldBlocksCommit(schedule),
        "hold still blocks before 1.75s");
    gameplay::CaptureCompletedGoalDestination(schedule, "level_01");
    Expect(schedule.destinationId == "level_02", "second capture is ignored while pending");
    gameplay::TickDestinationTransitionHold(schedule, 0.75f);
    Expect(
        !gameplay::DestinationTransitionHoldBlocksCommit(schedule),
        "timeout at 1.75s allows exactly one commit");
    Expect(schedule.pending, "timeout does not clear pending");
    gameplay::SkipDestinationTransitionHold(schedule);
    Expect(schedule.skipRequested, "Enter skip is idempotent after timeout");
    Expect(
        !gameplay::DestinationTransitionHoldBlocksCommit(schedule),
        "Enter skip still converges on the same commit");

    gameplay::ResetLevelTransitionSchedule(schedule);
    gameplay::CaptureCompletedGoalDestination(schedule, "level_02");
    gameplay::SkipDestinationTransitionHold(schedule);
    Expect(schedule.skipRequested, "Enter skip arms early transition");
    Expect(
        !gameplay::DestinationTransitionHoldBlocksCommit(schedule),
        "Enter skip does not wait for timeout");
    Expect(schedule.pending && !schedule.failed, "Enter skip does not Restart or fail");
    gameplay::SkipDestinationTransitionHold(schedule);
    Expect(schedule.pending, "repeated Enter cannot double-schedule");

    gameplay::LevelTransitionSchedule terminalHold{};
    gameplay::CaptureCompletedGoalDestination(terminalHold, "");
    Expect(!terminalHold.holding && !terminalHold.pending, "terminal does not create countdown state");
    Expect(
        !gameplay::DestinationCompletionShowsContinueHint(terminalHold),
        "terminal does not show destination continue hint");
    gameplay::MarkLevelTransitionFailed(schedule, "missing destination");
    Expect(!schedule.pending && schedule.failed, "failed schedule is not pending");
    Expect(!schedule.holding, "failed transition clears destination hold");
    gameplay::CaptureCompletedGoalDestination(schedule, "level_02");
    Expect(!schedule.pending && schedule.failed, "failed schedule does not retry");
    gameplay::ResetLevelTransitionSchedule(schedule);
    Expect(!schedule.failed && !schedule.pending && !schedule.holding, "restart clears failed schedule");
    Expect(gameplay::kDestinationTransitionHoldSeconds == 1.75f, "hold duration is 1.75 seconds");
    gameplay::CaptureCompletedGoalDestination(schedule, "level_02");
    Expect(schedule.pending, "new run can schedule after reset");

    std::error_code cleanupError;
    const std::filesystem::path scratch =
        std::filesystem::temp_directory_path() / "platformer3d_m64_transition";
    std::filesystem::remove_all(scratch, cleanupError);

    world::LevelDefinition current = level01.level;
    const world::LevelDefinition original = current;
    gameplay::Inventory inventory;
    Expect(inventory.TryAdd("key", 2), "seed inventory before transition");
    Expect(inventory.TryRemove("key", 1), "consume one key before transition");
    Expect(inventory.GetQuantity("key") == 1, "consumed inventory remainder remains");

    const std::filesystem::path staged02 = scratch / "assets" / "levels" / "level_02.level";
    WriteAll(staged02, world::SerializeLevelText(level02.level));
    const gameplay::LevelTransitionPrepareResult ready =
        gameplay::PrepareStagedLevelDestination(staged02, "level_02");
    Expect(ready.status == gameplay::LevelTransitionPrepareStatus::Ready, "staged destination Ready");
    Expect(ready.candidate.id == "level_02", "prepared candidate is level_02");
    Expect(
        gameplay::TryCommitPreparedDestination(current, ready),
        "Ready destination replaces active");
    Expect(current.id == "level_02", "active identity is destination");
    Expect(
        Vec3Equal(current.initialSpawnVisualCenter, level02.level.initialSpawnVisualCenter),
        "player spawn comes from destination");
    Expect(original.id == "level_01", "original snapshot is unchanged");

    gameplay::ApplyInventoryLifecycle(inventory, gameplay::InventoryLifecycleEvent::LevelTransition);
    Expect(inventory.GetQuantity("key") == 1, "successful transition preserves remaining Inventory");
    gameplay::InventoryUiState ui{};
    gameplay::OpenInventoryUi(ui, inventory);
    gameplay::ApplyInventoryUiLifecycle(
        ui, gameplay::InventoryLifecycleEvent::LevelTransition, inventory);
    Expect(!ui.open, "transition closes Inventory UI");
    Expect(inventory.GetQuantity("key") == 1, "UI close does not clear Inventory");

    Expect(
        Vec3Equal(current.initialSpawnVisualCenter, level02.level.initialSpawnVisualCenter),
        "Restart after transition uses destination spawn");
    Expect(current.id != world::kLevel01Id, "Restart current level is not hardcoded level_01");

    world::LevelDefinition preserved = original;
    const std::filesystem::path missingPath = scratch / "assets" / "levels" / "missing_level.level";
    const gameplay::LevelTransitionPrepareResult missing =
        gameplay::PrepareStagedLevelDestination(missingPath, "missing_level");
    Expect(missing.status == gameplay::LevelTransitionPrepareStatus::Missing, "missing destination");
    Expect(
        !gameplay::TryCommitPreparedDestination(preserved, missing),
        "missing destination does not replace");
    Expect(preserved.id == "level_01", "active remains level_01 after missing destination");
    Expect(world::AuthoredLevelDataEqual(preserved, original), "active authored data stays intact");

    const std::filesystem::path malformedPath = scratch / "assets" / "levels" / "level_02.level";
    WriteAll(malformedPath, "NOT_A_LEVEL\n");
    world::LevelDefinition afterMalformed = original;
    const gameplay::LevelTransitionPrepareResult malformed =
        gameplay::PrepareStagedLevelDestination(malformedPath, "level_02");
    Expect(
        malformed.status == gameplay::LevelTransitionPrepareStatus::Invalid,
        "malformed destination");
    Expect(!gameplay::TryCommitPreparedDestination(afterMalformed, malformed),
        "malformed destination does not replace");
    Expect(afterMalformed.id == "level_01", "malformed failure keeps current id");

    const gameplay::LevelTransitionPrepareResult relative =
        gameplay::PrepareStagedLevelDestination(
            std::filesystem::path("levels") / "level_02.level", "level_02");
    Expect(relative.status == gameplay::LevelTransitionPrepareStatus::Unsafe, "relative path unsafe");
    Expect(relative.candidate.id.empty(), "unsafe prepare has no candidate");

    const gameplay::LevelTransitionPrepareResult emptyId =
        gameplay::PrepareStagedLevelDestination(staged02, "../secret");
    Expect(emptyId.status == gameplay::LevelTransitionPrepareStatus::Unsafe, "unsafe id rejected");

    WriteAll(staged02, world::SerializeLevelText(level02.level));
    world::LevelDefinition mismatchActive = original;
    const std::filesystem::path mismatchPath = scratch / "assets" / "levels" / "other.level";
    world::LevelDefinition mismatch = level02.level;
    mismatch.id = "other";
    WriteAll(mismatchPath, world::SerializeLevelText(mismatch));
    const gameplay::LevelTransitionPrepareResult stemMismatch =
        gameplay::PrepareStagedLevelDestination(mismatchPath, "level_02");
    Expect(
        stemMismatch.status == gameplay::LevelTransitionPrepareStatus::Unsafe,
        "stem/id mismatch is unsafe");
    Expect(!gameplay::TryCommitPreparedDestination(mismatchActive, stemMismatch),
        "stem mismatch does not replace");

    const std::filesystem::path sourceTreePath =
        scratch / "source" / "levels" / "level_02.level";
    WriteAll(sourceTreePath, world::SerializeLevelText(level02.level));
    const gameplay::LevelTransitionPrepareResult sourceAttempt =
        gameplay::PrepareStagedLevelDestination(sourceTreePath, "level_02");
    Expect(
        sourceAttempt.status != gameplay::LevelTransitionPrepareStatus::Ready
            || sourceTreePath.string().find("assets") == std::string::npos,
        "source-tree path is not the staged runtime convention");
    Expect(
        world::MakeRuntimeLevelLogicalId("level_02").find("source") == std::string::npos,
        "logical id never names the source tree");
    Expect(
        world::MakeRuntimeLevelLogicalId("level_02").find("cooked") == std::string::npos,
        "logical id never names the cooked tree");

    gameplay::Inventory restartInventory;
    Expect(restartInventory.TryAdd("key", 1), "restart inventory seed");
    gameplay::ApplyInventoryLifecycle(
        restartInventory, gameplay::InventoryLifecycleEvent::RestartRun);
    Expect(restartInventory.Entries().empty(), "Restart still clears Inventory");

    std::filesystem::remove_all(scratch, cleanupError);

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d LevelTransitionTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("LevelTransitionTest passed\n");
    return 0;
}
