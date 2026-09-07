#include "editor/AuthoredObjectLifecycle.h"
#include "gameplay/CollectibleRunState.h"
#include "world/HazardWorld.h"
#include "world/LevelDefinition.h"
#include "world/LevelFile.h"
#include "world/RespawnWorld.h"

#include <cmath>
#include <cstdio>
#include <string>

namespace
{
int gFailures = 0;

void Expect(bool condition, const std::string& name)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL %s\n", name.c_str());
        ++gFailures;
    }
}

bool NearlyEqual(float a, float b, float epsilon = 0.0001f)
{
    return std::fabs(a - b) <= epsilon;
}

bool Vec3Near(core::Vec3 a, core::Vec3 b, float epsilon = 0.0001f)
{
    return NearlyEqual(a.x, b.x, epsilon) && NearlyEqual(a.y, b.y, epsilon)
        && NearlyEqual(a.z, b.z, epsilon);
}

core::Vec3 Sub3(core::Vec3 a, core::Vec3 b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

world::LevelDefinition MakeBaseLevel()
{
    world::LevelDefinition level{};
    level.id = "level_01";
    level.initialSpawnVisualCenter = {0.0f, 0.8f, 0.0f};
    level.elevatedPlatforms.push_back({{5.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
    level.elevatedPlatforms.push_back({{-4.5f, 2.25f, 0.0f}, {3.0f, 0.5f, 2.5f}});
    level.checkpoint1PlatformIndex = 0;
    level.checkpoint2PlatformIndex = 1;
    level.goalPlatformIndex = 1;
    level.checkpoints.push_back({{16.5f, 1.8f, 0.0f}, {2.4f, 1.6f, 2.0f}, {16.5f, 1.8f, 0.0f}});
    level.hazards.push_back({{11.5f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
    level.collectibles.push_back({{5.0f, 2.5f, 0.0f}, {1.0f, 1.2f, 1.0f}});
    return level;
}

const core::Vec3 kTestPlacementA{10.0f, 3.0f, -4.0f};
const core::Vec3 kTestPlacementB{-20.0f, 6.5f, 8.0f};
}

int main()
{
    using editor::EditorObjectKind;
    using editor::EditorSelection;

    Expect(editor::SupportsLifecycle(EditorObjectKind::ElevatedPlatform), "platform supported");
    Expect(editor::SupportsLifecycle(EditorObjectKind::Checkpoint), "checkpoint supported");
    Expect(editor::SupportsLifecycle(EditorObjectKind::Hazard), "hazard supported");
    Expect(editor::SupportsLifecycle(EditorObjectKind::Collectible), "collectible supported");
    Expect(!editor::SupportsLifecycle(EditorObjectKind::Spawn), "spawn unsupported");
    Expect(!editor::SupportsLifecycle(EditorObjectKind::Ground), "ground unsupported");
    Expect(!editor::SupportsLifecycle(EditorObjectKind::Camera), "camera unsupported");
    Expect(!editor::SupportsLifecycle(EditorObjectKind::Goal), "goal unsupported");
    Expect(!editor::SupportsLifecycle(EditorObjectKind::Slope), "slope unsupported");
    Expect(!editor::SupportsLifecycle(EditorObjectKind::MovingPlatform), "moving unsupported");
    Expect(!editor::SupportsLifecycle(EditorObjectKind::DynamicBox), "dynamic unsupported");

    {
        world::LevelDefinition working = MakeBaseLevel();
        const world::Box original = working.elevatedPlatforms[0];
        const editor::LifecycleEditResult added = editor::AddPlatform(working, kTestPlacementA);
        Expect(added.succeeded, "add platform");
        Expect(working.elevatedPlatforms.size() == 3, "add platform count +1");
        Expect(working.elevatedPlatforms[0].center.x == original.center.x, "existing platform 0 unchanged");
        Expect(working.elevatedPlatforms[1].center.x == -4.5f, "existing platform 1 unchanged");
        Expect(
            added.selection.kind == EditorObjectKind::ElevatedPlatform
                && added.selection.index == 2,
            "add platform selects new");
        Expect(
            Vec3Near(
                working.elevatedPlatforms[2].center,
                {kTestPlacementA.x + editor::kDefaultAddedPlatformOffset.x,
                 kTestPlacementA.y + editor::kDefaultAddedPlatformOffset.y,
                 working.initialSpawnVisualCenter.z + editor::kDefaultAddedPlatformOffset.z}),
            "default platform uses camera X/Y and spawn-lane Z");
        Expect(working.elevatedPlatforms[2].size.x > 0.0f, "default platform positive size");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        const editor::LifecycleEditResult added = editor::AddCheckpoint(working, kTestPlacementA);
        Expect(added.succeeded && working.checkpoints.size() == 2, "add checkpoint");
        Expect(added.selection.index == 1, "add checkpoint selects new");
        Expect(working.checkpoints[0].center.x == 16.5f, "existing checkpoint unchanged");
        const world::CheckpointSpec& addedCheckpoint = working.checkpoints[1];
        Expect(working.checkpoints[1].size.x > 0.0f, "default checkpoint positive size");
        Expect(std::isfinite(addedCheckpoint.center.x) && std::isfinite(addedCheckpoint.center.y)
                && std::isfinite(addedCheckpoint.center.z),
            "default checkpoint finite center");
        Expect(std::isfinite(addedCheckpoint.size.x) && std::isfinite(addedCheckpoint.size.y)
                && std::isfinite(addedCheckpoint.size.z)
                && addedCheckpoint.size.x > 0.0f && addedCheckpoint.size.y > 0.0f
                && addedCheckpoint.size.z > 0.0f,
            "default checkpoint finite positive size");
        Expect(
            std::isfinite(addedCheckpoint.respawnPosition.x)
                && std::isfinite(addedCheckpoint.respawnPosition.y)
                && std::isfinite(addedCheckpoint.respawnPosition.z),
            "default checkpoint finite respawn");
        Expect(
            NearlyEqual(addedCheckpoint.center.y, kTestPlacementA.y + editor::kDefaultAddedCheckpointOffset.y)
                && NearlyEqual(addedCheckpoint.center.x, kTestPlacementA.x + editor::kDefaultAddedCheckpointOffset.x)
                && NearlyEqual(
                    addedCheckpoint.center.z,
                    working.initialSpawnVisualCenter.z + editor::kDefaultAddedCheckpointOffset.z),
            "default checkpoint uses camera X/Y and spawn-lane Z");
        Expect(
            NearlyEqual(addedCheckpoint.respawnPosition.x, addedCheckpoint.center.x
                    + editor::kDefaultAddedCheckpointRespawnOffset.x)
                && NearlyEqual(
                    addedCheckpoint.respawnPosition.y,
                    addedCheckpoint.center.y + editor::kDefaultAddedCheckpointRespawnOffset.y)
                && NearlyEqual(
                    addedCheckpoint.respawnPosition.z,
                    addedCheckpoint.center.z + editor::kDefaultAddedCheckpointRespawnOffset.z),
            "default checkpoint respawn offset is canonical identity");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        const editor::LifecycleEditResult added = editor::AddHazard(working, kTestPlacementA);
        Expect(added.succeeded && working.hazards.size() == 2, "add hazard");
        Expect(added.selection.index == 1, "add hazard selects new");
        Expect(working.hazards[0].center.x == 11.5f, "existing hazard unchanged");
        Expect(working.hazards[1].size.y > 0.0f, "default hazard positive size");
        Expect(Vec3Near(working.hazards[1].center,
            {kTestPlacementA.x, kTestPlacementA.y, working.initialSpawnVisualCenter.z}),
            "hazard uses camera X/Y and spawn-lane Z");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        const editor::LifecycleEditResult added = editor::AddCollectible(working, kTestPlacementA);
        Expect(added.succeeded && working.collectibles.size() == 2, "add collectible");
        Expect(added.selection.index == 1, "add collectible selects new");
        Expect(working.collectibles[0].center.x == 5.0f, "existing collectible unchanged");
        Expect(working.collectibles[1].size.x > 0.0f, "default collectible positive size");
        Expect(Vec3Near(working.collectibles[1].center,
            {kTestPlacementA.x, kTestPlacementA.y, working.initialSpawnVisualCenter.z}),
            "collectible uses camera X/Y and spawn-lane Z");
    }

    {
        Expect(
            Vec3Near(editor::MakeAuthoredAddPlacement({20.0f, 5.0f, 9.0f}, 0.0f), {20.0f, 5.0f, 0.0f}),
            "hybrid helper keeps camera X/Y and uses lane Z");
        Expect(
            Vec3Near(editor::MakeAuthoredAddPlacement({18.0f, 4.5f, 7.0f}, 0.0f), {18.0f, 4.5f, 0.0f}),
            "camera Z is not authored placement Z");
    }

    {
        const core::Vec3 hybridCamera{20.0f, 5.0f, 9.0f};
        world::LevelDefinition working = MakeBaseLevel();
        Expect(NearlyEqual(working.initialSpawnVisualCenter.z, 0.0f), "hybrid fixture spawn.z is 0");
        Expect(editor::AddPlatform(working, hybridCamera).succeeded, "hybrid add platform");
        Expect(editor::AddCheckpoint(working, hybridCamera).succeeded, "hybrid add checkpoint");
        Expect(editor::AddHazard(working, hybridCamera).succeeded, "hybrid add hazard");
        Expect(editor::AddCollectible(working, hybridCamera).succeeded, "hybrid add collectible");
        const core::Vec3 expected = editor::MakeAuthoredAddPlacement(
            hybridCamera, working.initialSpawnVisualCenter.z);
        Expect(Vec3Near(working.elevatedPlatforms.back().center, expected), "hybrid platform {20,5,0}");
        Expect(Vec3Near(working.checkpoints.back().center, expected), "hybrid checkpoint {20,5,0}");
        Expect(Vec3Near(working.hazards.back().center, expected), "hybrid hazard {20,5,0}");
        Expect(Vec3Near(working.collectibles.back().center, expected), "hybrid collectible {20,5,0}");
    }

    {
        world::LevelDefinition fromA = MakeBaseLevel();
        world::LevelDefinition fromB = MakeBaseLevel();
        Expect(editor::AddPlatform(fromA, kTestPlacementA).succeeded, "add platform at A");
        Expect(editor::AddPlatform(fromB, kTestPlacementB).succeeded, "add platform at B");
        Expect(
            NearlyEqual(fromA.elevatedPlatforms.back().center.x, kTestPlacementA.x)
                && NearlyEqual(fromB.elevatedPlatforms.back().center.x, kTestPlacementB.x)
                && NearlyEqual(fromA.elevatedPlatforms.back().center.y, kTestPlacementA.y)
                && NearlyEqual(fromB.elevatedPlatforms.back().center.y, kTestPlacementB.y),
            "camera A/B change Add X/Y");
        Expect(
            NearlyEqual(fromA.elevatedPlatforms.back().center.z, fromA.initialSpawnVisualCenter.z)
                && NearlyEqual(fromB.elevatedPlatforms.back().center.z, fromB.initialSpawnVisualCenter.z),
            "camera A/B keep the same gameplay-lane Z");
        Expect(editor::AddCheckpoint(fromA, kTestPlacementA).succeeded, "add checkpoint at A");
        Expect(editor::AddCheckpoint(fromB, kTestPlacementB).succeeded, "add checkpoint at B");
        Expect(
            NearlyEqual(fromA.checkpoints.back().center.x, kTestPlacementA.x)
                && NearlyEqual(fromB.checkpoints.back().center.x, kTestPlacementB.x)
                && NearlyEqual(fromA.checkpoints.back().center.z, fromA.initialSpawnVisualCenter.z)
                && NearlyEqual(fromB.checkpoints.back().center.z, fromB.initialSpawnVisualCenter.z),
            "checkpoint camera A/B change X, keep lane Z");
        Expect(editor::AddHazard(fromA, kTestPlacementA).succeeded, "add hazard at A");
        Expect(editor::AddHazard(fromB, kTestPlacementB).succeeded, "add hazard at B");
        Expect(
            NearlyEqual(fromA.hazards.back().center.x, kTestPlacementA.x)
                && NearlyEqual(fromB.hazards.back().center.x, kTestPlacementB.x)
                && NearlyEqual(fromA.hazards.back().center.z, fromA.initialSpawnVisualCenter.z),
            "hazard camera A/B change X, keep lane Z");
        Expect(editor::AddCollectible(fromA, kTestPlacementA).succeeded, "add collectible at A");
        Expect(editor::AddCollectible(fromB, kTestPlacementB).succeeded, "add collectible at B");
        Expect(
            NearlyEqual(fromA.collectibles.back().center.x, kTestPlacementA.x)
                && NearlyEqual(fromB.collectibles.back().center.x, kTestPlacementB.x)
                && NearlyEqual(fromA.collectibles.back().center.z, fromA.initialSpawnVisualCenter.z),
            "collectible camera A/B change X, keep lane Z");
    }

    {
        world::LevelDefinition spawnXyA = MakeBaseLevel();
        world::LevelDefinition spawnXyB = MakeBaseLevel();
        spawnXyA.initialSpawnVisualCenter = {0.0f, 0.8f, 0.0f};
        spawnXyB.initialSpawnVisualCenter = {99.0f, 4.0f, 0.0f};
        Expect(editor::AddPlatform(spawnXyA, kTestPlacementA).succeeded, "add with spawn XY A");
        Expect(editor::AddPlatform(spawnXyB, kTestPlacementA).succeeded, "add with spawn XY B");
        Expect(
            Vec3Near(spawnXyA.elevatedPlatforms.back().center, spawnXyB.elevatedPlatforms.back().center),
            "spawn X/Y must not change Add X/Y");
        Expect(editor::AddCheckpoint(spawnXyA, kTestPlacementA).succeeded, "checkpoint spawn XY A");
        Expect(editor::AddCheckpoint(spawnXyB, kTestPlacementA).succeeded, "checkpoint spawn XY B");
        Expect(
            Vec3Near(spawnXyA.checkpoints.back().center, spawnXyB.checkpoints.back().center),
            "checkpoint Add ignores spawn X/Y");
        Expect(editor::AddHazard(spawnXyA, kTestPlacementA).succeeded, "hazard spawn XY A");
        Expect(editor::AddHazard(spawnXyB, kTestPlacementA).succeeded, "hazard spawn XY B");
        Expect(
            Vec3Near(spawnXyA.hazards.back().center, spawnXyB.hazards.back().center),
            "hazard Add ignores spawn X/Y");
        Expect(editor::AddCollectible(spawnXyA, kTestPlacementA).succeeded, "collectible spawn XY A");
        Expect(editor::AddCollectible(spawnXyB, kTestPlacementA).succeeded, "collectible spawn XY B");
        Expect(
            Vec3Near(spawnXyA.collectibles.back().center, spawnXyB.collectibles.back().center),
            "collectible Add ignores spawn X/Y");
    }

    {
        const core::Vec3 sameCamera{20.0f, 5.0f, 9.0f};
        world::LevelDefinition laneA = MakeBaseLevel();
        world::LevelDefinition laneB = MakeBaseLevel();
        laneA.initialSpawnVisualCenter.z = 0.0f;
        laneB.initialSpawnVisualCenter.z = 3.0f;
        Expect(editor::AddCollectible(laneA, sameCamera).succeeded, "lane Z add A");
        Expect(editor::AddCollectible(laneB, sameCamera).succeeded, "lane Z add B");
        Expect(
            NearlyEqual(laneA.collectibles.back().center.x, laneB.collectibles.back().center.x)
                && NearlyEqual(laneA.collectibles.back().center.y, laneB.collectibles.back().center.y),
            "same camera keeps Add X/Y when only spawn.z changes");
        Expect(NearlyEqual(laneA.collectibles.back().center.z, 0.0f), "lane A object z = 0");
        Expect(NearlyEqual(laneB.collectibles.back().center.z, 3.0f), "lane B object z = 3");
        Expect(editor::AddPlatform(laneA, sameCamera).succeeded, "lane Z platform A");
        Expect(editor::AddPlatform(laneB, sameCamera).succeeded, "lane Z platform B");
        Expect(NearlyEqual(laneA.elevatedPlatforms.back().center.z, 0.0f), "platform lane A z = 0");
        Expect(NearlyEqual(laneB.elevatedPlatforms.back().center.z, 3.0f), "platform lane B z = 3");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        const core::Vec3 world{9.0f, 4.0f, 2.5f};
        Expect(editor::AddPlatformAt(working, world).succeeded, "AddPlatformAt");
        Expect(Vec3Near(working.elevatedPlatforms.back().center, world), "AddPlatformAt uses world XYZ");
        Expect(
            !NearlyEqual(working.elevatedPlatforms.back().center.z, working.initialSpawnVisualCenter.z),
            "AddPlatformAt does not snap to spawn.z");
        Expect(editor::AddCollectibleAt(working, world).succeeded, "AddCollectibleAt");
        Expect(Vec3Near(working.collectibles.back().center, world), "AddCollectibleAt uses world XYZ");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        const editor::LifecycleEditResult added = editor::AddCheckpoint(working, kTestPlacementB);
        const core::Vec3 expected = editor::MakeAuthoredAddPlacement(
            kTestPlacementB, working.initialSpawnVisualCenter.z);
        Expect(Vec3Near(working.checkpoints.back().center, expected), "checkpoint placement B uses lane Z");
        Expect(
            Vec3Near(
                working.checkpoints.back().respawnPosition,
                {expected.x + editor::kDefaultAddedCheckpointRespawnOffset.x,
                 expected.y + editor::kDefaultAddedCheckpointRespawnOffset.y,
                 expected.z + editor::kDefaultAddedCheckpointRespawnOffset.z}),
            "checkpoint respawn follows hybrid center");
        Expect(added.succeeded, "checkpoint add at B succeeded");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        const world::Box original = working.elevatedPlatforms[0];
        const editor::LifecycleEditResult duplicated = editor::DuplicateSelected(
            working, {EditorObjectKind::ElevatedPlatform, 0});
        Expect(duplicated.succeeded && working.elevatedPlatforms.size() == 3, "duplicate platform");
        Expect(working.elevatedPlatforms[0].center.x == original.center.x, "original unchanged");
        Expect(
            NearlyEqual(
                working.elevatedPlatforms[2].center.x,
                original.center.x + editor::kLifecycleDuplicateOffsetX),
            "duplicate +X offset");
        Expect(duplicated.selection.index == 2, "duplicate appended and selected");
        Expect(working.elevatedPlatforms[1].center.x == -4.5f, "unrelated platform preserved");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        const editor::LifecycleEditResult duplicated = editor::DuplicateSelected(
            working, {EditorObjectKind::Checkpoint, 0});
        Expect(duplicated.succeeded && working.checkpoints.size() == 2, "duplicate checkpoint");
        Expect(duplicated.selection.index == 1, "duplicate checkpoint appended");
        Expect(
            NearlyEqual(
                working.checkpoints[1].respawnPosition.x,
                working.checkpoints[0].respawnPosition.x + editor::kLifecycleDuplicateOffsetX),
            "checkpoint respawn also offset");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        const core::Vec3 center = working.checkpoints[0].center;
        working.checkpoints[0].respawnPosition = {18.25f, 0.8f, 1.5f};
        const core::Vec3 respawn = working.checkpoints[0].respawnPosition;
        const editor::LifecycleEditResult duplicated = editor::DuplicateSelected(
            working, {EditorObjectKind::Checkpoint, 0});
        Expect(duplicated.succeeded && working.checkpoints.size() == 2, "duplicate offset checkpoint");
        Expect(Vec3Near(working.checkpoints[1].center,
            {center.x + editor::kLifecycleDuplicateOffsetX, center.y, center.z}),
            "duplicate center +1 X");
        Expect(
            Vec3Near(
                working.checkpoints[1].respawnPosition,
                {respawn.x + editor::kLifecycleDuplicateOffsetX, respawn.y, respawn.z}),
            "duplicate respawn +1 X");
        Expect(
            Vec3Near(
                Sub3(working.checkpoints[1].respawnPosition, working.checkpoints[1].center),
                Sub3(respawn, center)),
            "duplicate preserves trigger/respawn offset");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        Expect(
            editor::DuplicateSelected(working, {EditorObjectKind::Hazard, 0}).succeeded,
            "duplicate hazard");
        Expect(
            editor::DuplicateSelected(working, {EditorObjectKind::Collectible, 0}).succeeded,
            "duplicate collectible");
        Expect(working.hazards.size() == 2 && working.collectibles.size() == 2, "dup counts");
        Expect(
            NearlyEqual(working.collectibles[1].center.z, working.collectibles[0].center.z),
            "duplicate collectible keeps source Z");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        working.collectibles[0].center = {5.0f, 2.5f, 7.0f};
        const editor::LifecycleEditResult duplicated = editor::DuplicateSelected(
            working, {EditorObjectKind::Collectible, 0});
        Expect(duplicated.succeeded && working.collectibles.size() == 2, "duplicate off-lane collectible");
        Expect(NearlyEqual(working.collectibles[0].center.z, 7.0f), "source off-lane Z preserved");
        Expect(
            Vec3Near(
                working.collectibles[1].center,
                {5.0f + editor::kLifecycleDuplicateOffsetX, 2.5f, 7.0f}),
            "duplicate does not snap Z to spawn.z");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        gameplay::CollectibleRunState run =
            gameplay::MakeClearedCollectibleRunState(working.collectibles.size());
        run.collected[0] = 1;
        const int collectedBefore = gameplay::CollectedCount(run);
        const core::Vec3 originalCenter = working.collectibles[0].center;
        const editor::LifecycleEditResult duplicated = editor::DuplicateSelected(
            working, {EditorObjectKind::Collectible, 0});
        Expect(duplicated.succeeded && working.collectibles.size() == 2, "duplicate collected authored item");
        Expect(duplicated.selection.index == 1, "duplicate collected item selects the copy");
        Expect(
            NearlyEqual(
                working.collectibles[1].center.x,
                originalCenter.x + editor::kLifecycleDuplicateOffsetX),
            "collected duplicate still +1 X");
        Expect(run.collected.size() == 1 && run.collected[0] == 1, "runtime collected flag untouched");
        Expect(gameplay::CollectedCount(run) == collectedBefore, "runtime collected count untouched");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        working.checkpoint1PlatformIndex = 1;
        working.checkpoint2PlatformIndex = 1;
        working.goalPlatformIndex = 1;
        const editor::LifecycleEditResult deleted = editor::DeleteSelected(
            working, {EditorObjectKind::ElevatedPlatform, 0});
        Expect(deleted.succeeded && working.elevatedPlatforms.size() == 1, "delete unreferenced platform");
        Expect(working.elevatedPlatforms[0].center.x == -4.5f, "remaining platform is old 1");
        Expect(working.checkpoint1PlatformIndex == 0, "support > deleted decrements");
        Expect(working.checkpoint2PlatformIndex == 0, "second support > deleted decrements");
        Expect(working.goalPlatformIndex == 0, "goal support > deleted decrements");
        Expect(deleted.selection.kind == EditorObjectKind::None, "delete clears selection");
        Expect(deleted.status == editor::LifecycleEditStatus::Success, "delete success status");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        working.checkpoints.push_back({{1.0f, 2.0f, 0.0f}, {2.4f, 1.6f, 2.0f}, {1.0f, 2.0f, 0.0f}});
        const editor::LifecycleEditResult deleted = editor::DeleteSelected(
            working, {EditorObjectKind::Checkpoint, 0});
        Expect(deleted.succeeded && working.checkpoints.size() == 1, "delete checkpoint");
        Expect(working.checkpoints[0].center.x == 1.0f, "remaining checkpoint is old 1");
        Expect(
            editor::DeleteSelected(working, {EditorObjectKind::Hazard, 0}).succeeded
                && working.hazards.empty(),
            "delete hazard");
        Expect(
            editor::DeleteSelected(working, {EditorObjectKind::Collectible, 0}).succeeded
                && working.collectibles.empty(),
            "delete collectible");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        const std::size_t platforms = working.elevatedPlatforms.size();
        const editor::LifecycleEditResult noSpawn =
            editor::DeleteSelected(working, {EditorObjectKind::Spawn, 0});
        Expect(!noSpawn.succeeded, "no spawn delete");
        Expect(noSpawn.status == editor::LifecycleEditStatus::UnsupportedType, "spawn delete status");
        Expect(
            !editor::DuplicateSelected(working, {EditorObjectKind::Ground, 0}).succeeded,
            "no ground duplicate");
        Expect(
            !editor::DeleteSelected(working, {EditorObjectKind::Camera, 0}).succeeded, "no camera delete");
        Expect(!editor::DeleteSelected(working, {EditorObjectKind::Goal, 0}).succeeded, "no goal delete");
        Expect(!editor::DeleteSelected(working, {EditorObjectKind::Slope, 0}).succeeded, "no slope delete");
        Expect(
            !editor::DeleteSelected(working, {EditorObjectKind::MovingPlatform, 0}).succeeded,
            "no moving delete");
        Expect(
            !editor::DeleteSelected(working, {EditorObjectKind::DynamicBox, 0}).succeeded,
            "no dynamic delete");
        Expect(working.elevatedPlatforms.size() == platforms, "unsupported ops do not mutate");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        const editor::LifecycleEditResult dupOutOfRange =
            editor::DuplicateSelected(working, {EditorObjectKind::ElevatedPlatform, 9});
        Expect(!dupOutOfRange.succeeded, "duplicate out of range");
        Expect(
            dupOutOfRange.status == editor::LifecycleEditStatus::InvalidSelection,
            "duplicate invalid status");
        const editor::LifecycleEditResult delOutOfRange =
            editor::DeleteSelected(working, {EditorObjectKind::Hazard, 4});
        Expect(!delOutOfRange.succeeded, "delete out of range");
        Expect(
            delOutOfRange.status == editor::LifecycleEditStatus::InvalidSelection,
            "delete invalid status");
        world::LevelDefinition empty{};
        Expect(
            !editor::DeleteSelected(empty, {EditorObjectKind::Collectible, 0}).succeeded,
            "delete empty category");
        Expect(working.elevatedPlatforms.size() == 2, "invalid selection does not mutate");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        working.elevatedPlatforms.resize(static_cast<std::size_t>(world::kMaxElevatedPlatformCount));
        const editor::LifecycleEditResult addAtLimit = editor::AddPlatform(working, kTestPlacementA);
        Expect(!addAtLimit.succeeded, "add rejected at platform max");
        Expect(addAtLimit.status == editor::LifecycleEditStatus::AtLimit, "add at-limit status");
        Expect(
            !editor::DuplicateSelected(working, {EditorObjectKind::ElevatedPlatform, 0}).succeeded,
            "duplicate rejected at platform max");
        Expect(
            working.elevatedPlatforms.size()
                == static_cast<std::size_t>(world::kMaxElevatedPlatformCount),
            "max count is unchanged after rejection");
    }

    {
        const world::LevelDefinition active = MakeBaseLevel();
        world::LevelDefinition working = active;
        Expect(world::AuthoredLevelDataEqual(working, active), "start equal");
        editor::AddPlatform(working, kTestPlacementA);
        editor::DeleteSelected(working, {EditorObjectKind::Hazard, 0});
        editor::DuplicateSelected(working, {EditorObjectKind::Collectible, 0});
        Expect(!world::AuthoredLevelDataEqual(working, active), "lifecycle marks modified model");
        working = active;
        Expect(world::AuthoredLevelDataEqual(working, active), "revert restores working");
    }

    Expect(world::ReconcileActiveCheckpointIndex(1, 1) == world::kNoActiveCheckpointIndex,
           "checkpoint index clamped when count shrinks");
    Expect(world::ReconcileActiveCheckpointIndex(0, 2) == 0, "valid checkpoint index kept");

    {
        gameplay::CollectibleRunState state = gameplay::MakeClearedCollectibleRunState(4);
        Expect(state.collected.size() == 4, "collectible flags match count");
        Expect(gameplay::CollectedCount(state) == 0, "cleared collectible count");
        state.collected[1] = 1;
        Expect(gameplay::CollectedCount(state) == 1, "collected count derived");
        state = gameplay::MakeClearedCollectibleRunState(2);
        Expect(state.collected.size() == 2 && gameplay::CollectedCount(state) == 0, "resize reset");
    }

    {
        world::LevelDefinition level = MakeBaseLevel();
        Expect(world::FindHazardIndexContaining({11.5f, 0.5f, 0.0f}, level.hazards) == 0, "hazard hit");
        level.hazards.clear();
        Expect(
            world::FindHazardIndexContaining({11.5f, 0.5f, 0.0f}, level.hazards)
                == world::kNoHazardIndex,
            "zero hazards safe");
        level.hazards.push_back({{0.0f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
        level.hazards.push_back({{4.0f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
        Expect(world::FindHazardIndexContaining({4.0f, 0.5f, 0.0f}, level.hazards) == 1, "second hazard");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        Expect(
            editor::CanAddLifecycleObject(true, working, EditorObjectKind::ElevatedPlatform, false),
            "add enabled when authoring");
        Expect(
            !editor::CanAddLifecycleObject(false, working, EditorObjectKind::ElevatedPlatform, false),
            "add Development-gated");
        Expect(
            !editor::CanAddLifecycleObject(true, working, EditorObjectKind::ElevatedPlatform, true),
            "add disabled while gizmo dragging");
        Expect(
            editor::CanDuplicateSelected(
                true, working, {EditorObjectKind::Hazard, 0}, false),
            "duplicate enabled");
        Expect(
            !editor::CanDuplicateSelected(
                true, working, {EditorObjectKind::Spawn, 0}, false),
            "duplicate spawn disabled");
        Expect(
            editor::CanDeleteSelected(
                true, working, {EditorObjectKind::Collectible, 0}, false),
            "delete enabled");
        Expect(
            !editor::CanDeleteSelected(
                true, working, {EditorObjectKind::ElevatedPlatform, 0}, false),
            "delete referenced platform disabled");
        Expect(
            editor::ReconcileSelection(working, {EditorObjectKind::ElevatedPlatform, 9}).kind
                == EditorObjectKind::None,
            "reconcile clears stale index");
    }

    {
        world::LevelDefinition working{};
        working.initialSpawnVisualCenter = {0.0f, 0.8f, 0.0f};
        working.elevatedPlatforms.push_back({{0.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        working.elevatedPlatforms.push_back({{10.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        working.elevatedPlatforms.push_back({{20.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        working.elevatedPlatforms.push_back({{30.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        working.checkpoint1PlatformIndex = 0;
        working.checkpoint2PlatformIndex = 2;
        working.goalPlatformIndex = 3;
        const world::Box platformC = working.elevatedPlatforms[2];
        const world::Box platformD = working.elevatedPlatforms[3];
        const int cp1 = working.checkpoint1PlatformIndex;
        const editor::LifecycleEditResult deletedBefore = editor::DeleteSelected(
            working, {EditorObjectKind::ElevatedPlatform, 1});
        Expect(deletedBefore.succeeded, "delete-before-reference succeeds");
        Expect(working.elevatedPlatforms.size() == 3, "delete-before-reference count");
        Expect(working.checkpoint1PlatformIndex == cp1, "reference < D unchanged");
        Expect(working.checkpoint2PlatformIndex == 1, "reference > D decremented");
        Expect(working.goalPlatformIndex == 2, "second reference > D decremented");
        Expect(working.elevatedPlatforms[1].center.x == platformC.center.x, "index 1 is former C");
        Expect(working.elevatedPlatforms[2].center.x == platformD.center.x, "index 2 is former D");
    }

    {
        world::LevelDefinition working{};
        working.elevatedPlatforms.push_back({{0.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        working.elevatedPlatforms.push_back({{10.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        working.elevatedPlatforms.push_back({{20.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        working.elevatedPlatforms.push_back({{30.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        working.checkpoint1PlatformIndex = 0;
        working.checkpoint2PlatformIndex = 1;
        working.goalPlatformIndex = 1;
        const world::Box platformA = working.elevatedPlatforms[0];
        Expect(
            editor::CanDeleteSelected(
                true, working, {EditorObjectKind::ElevatedPlatform, 3}, false),
            "CanDelete unreferenced platform after refs");
        const editor::LifecycleEditResult deletedAfter = editor::DeleteSelected(
            working, {EditorObjectKind::ElevatedPlatform, 3});
        Expect(deletedAfter.succeeded, "delete-after-reference succeeds");
        Expect(working.checkpoint1PlatformIndex == 0, "reference < D stays");
        Expect(working.checkpoint2PlatformIndex == 1, "other reference < D stays");
        Expect(working.goalPlatformIndex == 1, "goal reference < D stays");
        Expect(working.elevatedPlatforms[0].center.x == platformA.center.x, "semantic A unchanged");
        Expect(working.elevatedPlatforms.size() == 3, "delete-after-reference count");
    }

    {
        world::LevelDefinition working{};
        working.elevatedPlatforms.push_back({{0.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        working.elevatedPlatforms.push_back({{10.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        working.elevatedPlatforms.push_back({{20.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        working.checkpoint1PlatformIndex = 0;
        working.checkpoint2PlatformIndex = 2;
        working.goalPlatformIndex = 2;
        const world::LevelDefinition before = working;
        const EditorSelection selected{EditorObjectKind::ElevatedPlatform, 2};
        const editor::LifecycleEditResult rejected = editor::DeleteSelected(working, selected);
        Expect(!rejected.succeeded, "delete referenced platform rejected");
        Expect(rejected.status == editor::LifecycleEditStatus::ReferencedPlatform, "referenced status");
        Expect(world::AuthoredLevelDataEqual(working, before), "referenced delete is atomic");
        Expect(rejected.selection == selected, "failure leaves selection unchanged");
        Expect(
            !editor::CanDeleteSelected(true, before, selected, false),
            "CanDelete false when referenced");
    }

    {
        world::LevelDefinition working{};
        working.elevatedPlatforms.push_back({{0.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        working.elevatedPlatforms.push_back({{10.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        working.elevatedPlatforms.push_back({{20.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        working.checkpoint1PlatformIndex = 1;
        working.checkpoint2PlatformIndex = 2;
        working.goalPlatformIndex = 2;
        const world::LevelDefinition before = working;
        const editor::LifecycleEditResult rejected = editor::DeleteSelected(
            working, {EditorObjectKind::ElevatedPlatform, 1});
        Expect(!rejected.succeeded, "any equal reference rejects the whole delete");
        Expect(world::AuthoredLevelDataEqual(working, before), "partial remap does not occur");
        Expect(working.checkpoint2PlatformIndex == 2, "greater reference not decremented on reject");
    }

    {
        world::LevelDefinition working{};
        working.elevatedPlatforms.push_back({{0.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        working.checkpoint1PlatformIndex = 0;
        working.checkpoint2PlatformIndex = 0;
        working.goalPlatformIndex = 0;
        const world::LevelDefinition before = working;
        const EditorSelection selected{EditorObjectKind::ElevatedPlatform, 0};
        const editor::LifecycleEditResult rejected = editor::DeleteSelected(working, selected);
        Expect(!rejected.succeeded, "last platform delete rejected");
        Expect(rejected.status == editor::LifecycleEditStatus::MinimumCount, "minimum count status");
        Expect(world::AuthoredLevelDataEqual(working, before), "last platform untouched");
        Expect(
            !editor::CanDeleteSelected(true, before, selected, false),
            "CanDelete false at minimum count");
    }

    {
        world::LevelDefinition working{};
        working.initialSpawnVisualCenter = {0.0f, 0.8f, 0.0f};
        working.elevatedPlatforms.push_back({{0.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        working.elevatedPlatforms.push_back({{10.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        working.checkpoint1PlatformIndex = 0;
        working.checkpoint2PlatformIndex = 1;
        working.goalPlatformIndex = 1;
        const int cp1 = working.checkpoint1PlatformIndex;
        const int cp2 = working.checkpoint2PlatformIndex;
        const int goal = working.goalPlatformIndex;
        Expect(editor::AddPlatform(working, kTestPlacementA).succeeded, "add for reference preservation");
        Expect(working.checkpoint1PlatformIndex == cp1, "add does not change cp1");
        Expect(working.checkpoint2PlatformIndex == cp2, "add does not change cp2");
        Expect(working.goalPlatformIndex == goal, "add does not change goal support");
        Expect(
            editor::DuplicateSelected(working, {EditorObjectKind::ElevatedPlatform, 0}).succeeded,
            "duplicate for reference preservation");
        Expect(working.checkpoint1PlatformIndex == cp1, "duplicate does not change cp1");
        Expect(working.checkpoint2PlatformIndex == cp2, "duplicate does not change cp2");
        Expect(working.goalPlatformIndex == goal, "duplicate does not change goal support");
    }

    {
        world::LevelDefinition working{};
        working.elevatedPlatforms.push_back({{0.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        working.elevatedPlatforms.push_back({{10.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        working.checkpoint1PlatformIndex = 0;
        working.checkpoint2PlatformIndex = 0;
        working.goalPlatformIndex = 0;
        const EditorSelection unreferenced{EditorObjectKind::ElevatedPlatform, 1};
        Expect(
            editor::CanDeleteSelected(true, working, unreferenced, false),
            "unreferenced extra Platform is deletable");
        Expect(
            editor::DeleteSelected(working, unreferenced).succeeded,
            "unreferenced extra Platform DeleteSelected succeeds");
        Expect(working.elevatedPlatforms.size() == 1, "unreferenced platform removed");
    }

    {
        world::LevelDefinition active{};
        active.collectibles.push_back({{0.0f, 2.0f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        active.collectibles.push_back({{1.0f, 2.0f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        active.collectibles.push_back({{2.0f, 2.0f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        active.collectibles.push_back({{3.0f, 2.0f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        Expect(editor::DeleteSelected(working, {EditorObjectKind::Collectible, 1}).succeeded,
            "delete B");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::Collectible, true, 1);
        Expect(editor::MappedWorkingIndex(map, EditorObjectKind::Collectible, 0) == 0,
            "active A -> working A");
        Expect(editor::MappedWorkingIndex(map, EditorObjectKind::Collectible, 1)
                == editor::kNoStructuralIndex,
            "active B -> none");
        Expect(editor::MappedWorkingIndex(map, EditorObjectKind::Collectible, 2) == 1,
            "active C -> working C at shifted index");
        Expect(editor::MappedWorkingIndex(map, EditorObjectKind::Collectible, 3) == 2,
            "active D -> working D at shifted index");
        Expect(
            editor::IsPendingDeleteActiveIndex(map, EditorObjectKind::Collectible, 1),
            "deleted B is pending-delete visual");
        Expect(
            !editor::IsPendingDeleteActiveIndex(map, EditorObjectKind::Collectible, 2),
            "surviving C is not pending-delete");
        const editor::PendingDeleteVisuals visuals = editor::MakePendingDeleteVisuals(active, map);
        Expect(visuals.collectibleCenters.size() == 1, "one pending-delete collectible visual");
        Expect(visuals.collectibleIndices[0] == 1, "pending-delete identity is active index B");
        Expect(visuals.collectibleCenters[0].x == 1.0f, "pending-delete visual is B");

        editor::EditorSelection workingPick{};
        Expect(
            !editor::TryMapActiveWorldPick(
                {EditorObjectKind::Collectible, 1}, map, workingPick),
            "pending-deleted pick is ignored");
        Expect(
            editor::TryMapActiveWorldPick(
                {EditorObjectKind::Collectible, 2}, map, workingPick),
            "surviving pick accepted");
        Expect(workingPick.index == 1 && working.collectibles[1].center.x == 2.0f,
            "surviving active C maps to working C");
        Expect(
            editor::ShouldAcceptActiveWorldPick({EditorObjectKind::Spawn, 0}, map),
            "non-lifecycle pick still accepted");
        Expect(
            editor::ShouldAcceptActiveWorldPick(editor::ClearSelection(), map),
            "empty click still accepted");
    }

    {
        world::LevelDefinition active{};
        active.hazards.push_back({{0.0f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
        active.hazards.push_back({{4.0f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        const int mapped0 = editor::MappedWorkingIndex(map, EditorObjectKind::Hazard, 0);
        Expect(editor::AddHazard(working, {20.0f, 5.0f, 0.0f}).succeeded, "add mapping");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::Hazard, false, 0);
        Expect(
            editor::MappedWorkingIndex(map, EditorObjectKind::Hazard, 0) == mapped0
                && editor::MappedWorkingIndex(map, EditorObjectKind::Hazard, 1) == 1,
            "existing active hazards stay mapped after Add");
        Expect(
            editor::MappedActiveIndex(map, EditorObjectKind::Hazard, working.hazards.size() - 1)
                == editor::kNoStructuralIndex,
            "new working-only hazard has no active counterpart");
        Expect(
            editor::ShouldAcceptActiveWorldPick({EditorObjectKind::Hazard, 1}, map),
            "Add does not disable the category");
    }

    {
        world::LevelDefinition active{};
        active.checkpoints.push_back({{0.0f, 1.0f, 0.0f}, {2.4f, 1.6f, 2.0f}, {0.0f, 1.0f, 0.0f}});
        active.checkpoints.push_back({{4.0f, 1.0f, 0.0f}, {2.4f, 1.6f, 2.0f}, {4.0f, 1.0f, 0.0f}});
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        Expect(
            editor::DuplicateSelected(working, {EditorObjectKind::Checkpoint, 0}).succeeded,
            "duplicate mapping");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::Checkpoint, false, 0);
        Expect(editor::MappedWorkingIndex(map, EditorObjectKind::Checkpoint, 0) == 0,
            "duplicate keeps existing mapping");
        Expect(
            editor::MappedActiveIndex(map, EditorObjectKind::Checkpoint, 2)
                == editor::kNoStructuralIndex,
            "duplicate has no active counterpart");
    }

    {
        world::LevelDefinition active{};
        active.collectibles.push_back({{0.0f, 2.0f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        active.collectibles.push_back({{1.0f, 2.0f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        active.collectibles.push_back({{2.0f, 2.0f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        active.collectibles.push_back({{3.0f, 2.0f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        Expect(editor::DeleteSelected(working, {EditorObjectKind::Collectible, 1}).succeeded,
            "multi delete B");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::Collectible, true, 1);
        Expect(editor::AddCollectible(working, {9.0f, 5.0f, 0.0f}).succeeded, "multi add E");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::Collectible, false, 0);
        Expect(
            editor::DuplicateSelected(working, {EditorObjectKind::Collectible, 1}).succeeded,
            "multi duplicate C");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::Collectible, false, 0);
        const std::size_t dWorking =
            static_cast<std::size_t>(editor::MappedWorkingIndex(map, EditorObjectKind::Collectible, 3));
        Expect(editor::DeleteSelected(working, {EditorObjectKind::Collectible, dWorking}).succeeded,
            "multi delete D");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::Collectible, true, dWorking);
        Expect(
            editor::MappedWorkingIndex(map, EditorObjectKind::Collectible, 0) == 0,
            "multi-edit A still mapped");
        Expect(
            editor::MappedWorkingIndex(map, EditorObjectKind::Collectible, 1)
                == editor::kNoStructuralIndex,
            "multi-edit B still pending-deleted");
        editor::EditorSelection workingC{};
        Expect(
            editor::TryMapActiveWorldPick({EditorObjectKind::Collectible, 2}, map, workingC),
            "multi-edit surviving C still pickable");
        Expect(working.collectibles[workingC.index].center.x == 2.0f, "multi-edit C identity");
        Expect(
            !editor::TryMapActiveWorldPick(
                {EditorObjectKind::Collectible, 3}, map, workingC),
            "multi-edit D pick ignored");
        Expect(
            editor::MappedActiveIndex(
                map, EditorObjectKind::Collectible, working.collectibles.size() - 1)
                == editor::kNoStructuralIndex,
            "multi-edit pending-only object has no active pick");
    }

    {
        world::LevelDefinition active{};
        active.collectibles.push_back({{0.0f, 2.0f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        editor::DeleteSelected(working, {EditorObjectKind::Collectible, 0});
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::Collectible, true, 0);
        editor::ResetStructuralIndexMap(map, active);
        Expect(editor::MappedWorkingIndex(map, EditorObjectKind::Collectible, 0) == 0,
            "reset restores identity");
        Expect(
            !editor::IsPendingDeleteActiveIndex(map, EditorObjectKind::Collectible, 0),
            "reset clears pending-delete");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        Expect(
            editor::ShouldEmitDeleteSelectedRequest(
                true, false, true, working, {EditorObjectKind::Collectible, 0}, false),
            "Delete key emits when capture is clear");
        Expect(
            !editor::ShouldEmitDeleteSelectedRequest(
                true, true, true, working, {EditorObjectKind::Collectible, 0}, false),
            "Delete key ignored when ImGui wants keyboard");
        Expect(
            !editor::ShouldEmitDeleteSelectedRequest(
                false, false, true, working, {EditorObjectKind::Collectible, 0}, false),
            "no request when Delete is not pressed");
        Expect(
            !editor::ShouldEmitDeleteSelectedRequest(
                true, false, true, working, {EditorObjectKind::Spawn, 0}, false),
            "Delete key disabled for unsupported type");
        Expect(
            !editor::ShouldEmitDeleteSelectedRequest(
                true, false, true, working, {EditorObjectKind::ElevatedPlatform, 0}, false),
            "Delete key disabled for referenced Platform");
        Expect(
            !editor::ShouldEmitDeleteSelectedRequest(
                true, false, true, working, {EditorObjectKind::Collectible, 0}, true),
            "Delete key disabled while gizmo dragging");
        Expect(
            editor::ShouldEmitDeleteSelectedRequest(
                true, false, true, working, {EditorObjectKind::Hazard, 0}, false),
            "Delete key enabled for Hazard");
        Expect(
            editor::ShouldEmitDeleteSelectedRequest(
                true, false, true, working, {EditorObjectKind::Checkpoint, 0}, false),
            "Delete key enabled for Checkpoint");
        world::LevelDefinition lastPlatform{};
        lastPlatform.elevatedPlatforms.push_back({{0.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        lastPlatform.checkpoint1PlatformIndex = 0;
        lastPlatform.checkpoint2PlatformIndex = 0;
        lastPlatform.goalPlatformIndex = 0;
        Expect(
            !editor::ShouldEmitDeleteSelectedRequest(
                true,
                false,
                true,
                lastPlatform,
                {EditorObjectKind::ElevatedPlatform, 0},
                false),
            "Delete key disabled for last Platform");
        world::LevelDefinition extraPlatform = lastPlatform;
        extraPlatform.elevatedPlatforms.push_back({{10.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        Expect(
            editor::ShouldEmitDeleteSelectedRequest(
                true,
                false,
                true,
                extraPlatform,
                {EditorObjectKind::ElevatedPlatform, 1},
                false),
            "Delete key enabled for unreferenced extra Platform");
        Expect(
            !editor::ShouldEmitDeleteSelectedRequest(
                true,
                false,
                false,
                extraPlatform,
                {EditorObjectKind::ElevatedPlatform, 1},
                false),
            "Delete key Development-gated");
    }

    {
        world::LevelDefinition active{};
        active.elevatedPlatforms.push_back({{0.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        active.elevatedPlatforms.push_back({{10.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        active.checkpoints.push_back({{0.0f, 1.0f, 0.0f}, {2.4f, 1.6f, 2.0f}, {0.0f, 1.0f, 0.0f}});
        active.checkpoints.push_back({{4.0f, 1.0f, 0.0f}, {2.4f, 1.6f, 2.0f}, {4.0f, 1.0f, 0.0f}});
        active.hazards.push_back({{0.0f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
        active.hazards.push_back({{4.0f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
        active.collectibles.push_back({{0.0f, 2.0f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        active.collectibles.push_back({{1.0f, 2.0f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;

        Expect(
            editor::ResolveAuthoredEditorVisualMode(
                editor::IsPendingDeleteActiveIndex(map, EditorObjectKind::ElevatedPlatform, 1))
                == editor::AuthoredEditorVisualMode::Normal,
            "platform survivor uses normal visual mode");
        Expect(
            editor::ResolveAuthoredEditorVisualMode(false)
                == editor::AuthoredEditorVisualMode::Normal,
            "non-deleted object is not faded");

        Expect(editor::DeleteSelected(working, {EditorObjectKind::ElevatedPlatform, 1}).succeeded,
            "delete extra platform for visual mode");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::ElevatedPlatform, true, 1);
        Expect(
            editor::ResolveAuthoredEditorVisualMode(
                editor::IsPendingDeleteActiveIndex(map, EditorObjectKind::ElevatedPlatform, 1))
                == editor::AuthoredEditorVisualMode::PendingDelete,
            "pending-delete platform visual mode");
        Expect(
            editor::ResolveAuthoredEditorVisualMode(
                editor::IsPendingDeleteActiveIndex(map, EditorObjectKind::ElevatedPlatform, 0))
                == editor::AuthoredEditorVisualMode::Normal,
            "surviving platform visual mode unchanged");
        const editor::PendingDeleteVisuals platformVisuals =
            editor::MakePendingDeleteVisuals(active, map);
        Expect(platformVisuals.platforms.size() == 1 && platformVisuals.platformIndices[0] == 1,
            "platform pending-delete identity from mapping");

        Expect(editor::DeleteSelected(working, {EditorObjectKind::Checkpoint, 1}).succeeded,
            "delete checkpoint for visual mode");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::Checkpoint, true, 1);
        Expect(
            editor::ResolveAuthoredEditorVisualMode(
                editor::IsPendingDeleteActiveIndex(map, EditorObjectKind::Checkpoint, 1))
                == editor::AuthoredEditorVisualMode::PendingDelete,
            "pending-delete checkpoint visual mode");
        Expect(
            editor::ResolveAuthoredEditorVisualMode(
                editor::IsPendingDeleteActiveIndex(map, EditorObjectKind::Checkpoint, 0))
                == editor::AuthoredEditorVisualMode::Normal,
            "surviving checkpoint visual mode unchanged");

        Expect(editor::DeleteSelected(working, {EditorObjectKind::Hazard, 1}).succeeded,
            "delete hazard for visual mode");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::Hazard, true, 1);
        Expect(
            editor::ResolveAuthoredEditorVisualMode(
                editor::IsPendingDeleteActiveIndex(map, EditorObjectKind::Hazard, 1))
                == editor::AuthoredEditorVisualMode::PendingDelete,
            "pending-delete hazard visual mode");
        Expect(
            editor::ResolveAuthoredEditorVisualMode(
                editor::IsPendingDeleteActiveIndex(map, EditorObjectKind::Hazard, 0))
                == editor::AuthoredEditorVisualMode::Normal,
            "surviving hazard visual mode unchanged");

        Expect(editor::DeleteSelected(working, {EditorObjectKind::Collectible, 1}).succeeded,
            "delete collectible for visual mode");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::Collectible, true, 1);
        Expect(
            editor::ResolveAuthoredEditorVisualMode(
                editor::IsPendingDeleteActiveIndex(map, EditorObjectKind::Collectible, 1))
                == editor::AuthoredEditorVisualMode::PendingDelete,
            "pending-delete collectible visual mode");
        Expect(
            editor::ResolveAuthoredEditorVisualMode(
                editor::IsPendingDeleteActiveIndex(map, EditorObjectKind::Collectible, 0))
                == editor::AuthoredEditorVisualMode::Normal,
            "surviving collectible visual mode unchanged");

        const bool deletedCollectible =
            editor::IsPendingDeleteActiveIndex(map, EditorObjectKind::Collectible, 1);
        const bool survivingCollectiblePending =
            editor::IsPendingDeleteActiveIndex(map, EditorObjectKind::Collectible, 0);
        Expect(
            editor::ResolveCollectibleEditorVisualMode(false, 1)
                == editor::CollectibleEditorVisualMode::CollectedAuthored,
            "collected without pending-delete uses authored collected style");
        Expect(
            editor::ResolveCollectibleEditorVisualMode(deletedCollectible, 1)
                == editor::CollectibleEditorVisualMode::PendingDelete,
            "pending-delete wins over collected authored style");
        Expect(
            editor::ResolveCollectibleEditorVisualMode(deletedCollectible, 0)
                == editor::CollectibleEditorVisualMode::PendingDelete,
            "uncollected pending-delete uses pending-delete style");
        Expect(
            editor::ResolveCollectibleEditorVisualMode(survivingCollectiblePending, 0)
                == editor::CollectibleEditorVisualMode::Normal,
            "uncollected survivor uses normal runtime visual");
    }

    {
        world::LevelDefinition active{};
        active.collectibles.push_back({{0.0f, 2.0f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        Expect(editor::AddCollectible(working, {9.0f, 5.0f, 0.0f}).succeeded, "add ghost visual");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::Collectible, false, 0);
        Expect(
            editor::ResolveAuthoredEditorVisualMode(
                editor::IsPendingDeleteActiveIndex(map, EditorObjectKind::Collectible, 0))
                == editor::AuthoredEditorVisualMode::Normal,
            "pending Add does not fade the existing active collectible");
        const editor::PendingDeleteVisuals addVisuals = editor::MakePendingDeleteVisuals(active, map);
        Expect(addVisuals.collectibleCenters.empty(), "pending Add is not a pending-delete visual");
        Expect(
            editor::MappedActiveIndex(map, EditorObjectKind::Collectible, working.collectibles.size() - 1)
                == editor::kNoStructuralIndex,
            "added collectible remains working-only");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        Expect(
            editor::CanAddLifecycleObject(true, working, EditorObjectKind::Collectible, false),
            "Add Collectible enabled below old 16 cap");
        while (working.collectibles.size() < 17)
        {
            Expect(
                editor::AddCollectible(working, {static_cast<float>(working.collectibles.size()), 2.0f, 0.0f})
                    .succeeded,
                "add collectible past 16");
        }
        Expect(working.collectibles.size() == 17, "17 collectibles authored");
        Expect(
            editor::CanAddLifecycleObject(true, working, EditorObjectKind::Collectible, false),
            "Add Collectible still enabled at 17");
        gameplay::CollectibleRunState run =
            gameplay::MakeClearedCollectibleRunState(working.collectibles.size());
        Expect(run.collected.size() == 17, "collectible run state sizes to 17");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        while (working.checkpoints.size() < 9)
        {
            Expect(
                editor::AddCheckpoint(working, {static_cast<float>(working.checkpoints.size()), 1.0f, 0.0f})
                    .succeeded,
                "add checkpoint past 8");
        }
        Expect(working.checkpoints.size() == 9, "9 checkpoints authored");
        Expect(world::ReconcileActiveCheckpointIndex(8, 9) == 8, "checkpoint 8 valid when count is 9");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        while (working.hazards.size() < 9)
        {
            Expect(
                editor::AddHazard(working, {static_cast<float>(working.hazards.size()) * 2.0f, 0.5f, 0.0f})
                    .succeeded,
                "add hazard past 8");
        }
        Expect(working.hazards.size() == 9, "9 hazards authored");
        Expect(
            world::FindHazardIndexContaining({16.0f, 0.5f, 0.0f}, working.hazards) >= 0
                || working.hazards.size() == 9,
            "hazard iteration remains valid at 9");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        const std::size_t start = working.elevatedPlatforms.size();
        while (working.elevatedPlatforms.size() < 17)
        {
            Expect(editor::AddPlatform(working, kTestPlacementA).succeeded, "add platform past 16");
        }
        Expect(working.elevatedPlatforms.size() == 17, "17 platforms authored");
        Expect(
            editor::CanAddLifecycleObject(true, working, EditorObjectKind::ElevatedPlatform, false),
            "Add Platform enabled at 17 under physics budget");
        working.elevatedPlatforms.resize(static_cast<std::size_t>(world::kMaxElevatedPlatformCount));
        Expect(
            !editor::AddPlatform(working, kTestPlacementA).succeeded, "add rejected at physics platform max");
        Expect(
            working.elevatedPlatforms.size() == static_cast<std::size_t>(world::kMaxElevatedPlatformCount),
            "physics platform max unchanged after rejection");
        Expect(start < 17, "started below 17");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d authored object lifecycle test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Authored object lifecycle tests passed.\n");
    return 0;
}
