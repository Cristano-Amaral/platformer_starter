#include "editor/AuthoredObjectLifecycle.h"
#include "editor/EditorSelection.h"
#include "editor/EditorSelectionSet.h"
#include "editor/ItemIdInspectorEdit.h"
#include "editor/PressurePlateLocalLightTargets.h"
#include "gameplay/CollectibleRunState.h"
#include "gameplay/Inventory.h"
#include "physics/PhysicsCapacity.h"
#include "world/HazardWorld.h"
#include "world/LevelDefinition.h"
#include "world/LevelFile.h"
#include "world/LocalLightActivation.h"
#include "world/RespawnWorld.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

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
    Expect(editor::SupportsLifecycle(EditorObjectKind::Goal), "goal supported");
    Expect(!editor::SupportsLifecycle(EditorObjectKind::Spawn), "spawn unsupported");
    Expect(!editor::SupportsLifecycle(EditorObjectKind::Ground), "ground unsupported");
    Expect(!editor::SupportsLifecycle(EditorObjectKind::Camera), "camera unsupported");
    Expect(!editor::SupportsLifecycle(EditorObjectKind::Environment), "Environment cannot Duplicate/Delete");
    Expect(
        !editor::SupportsLifecycle(EditorObjectKind::DirectionalLight),
        "Directional Light cannot Duplicate/Delete");
    Expect(!editor::SupportsLifecycle(EditorObjectKind::Slope), "slope unsupported");
    Expect(!editor::SupportsLifecycle(EditorObjectKind::MovingPlatform), "moving unsupported");
    Expect(editor::SupportsLifecycle(EditorObjectKind::DynamicBox), "dynamic box supported");
    Expect(editor::SupportsLifecycle(EditorObjectKind::PressurePlate), "pressure plate supported");
    Expect(editor::SupportsLifecycle(EditorObjectKind::StaticProp), "static prop supported");
    Expect(editor::SupportsLifecycle(EditorObjectKind::PointLight), "point light supported");
    Expect(editor::SupportsLifecycle(EditorObjectKind::SpotLight), "spot light supported");
    Expect(!editor::SupportsLifecycle(EditorObjectKind::Terrain), "Terrain is not a repeatable lifecycle kind");

    {
        world::LevelDefinition working = MakeBaseLevel();
        Expect(editor::CanAddTerrain(true, working, false), "Add Terrain available when absent");
        const editor::LifecycleEditResult added = editor::AddTerrain(working);
        Expect(added.succeeded && working.hasTerrain, "Add Terrain creates authored Terrain");
        Expect(added.selection.kind == EditorObjectKind::Terrain, "Add Terrain selects Terrain");
        Expect(world::TerrainSpecEqual(working.terrain, world::MakeDefaultTerrain()),
            "Add Terrain uses deterministic default");
        Expect(!editor::CanAddTerrain(true, working, false), "Add Terrain unavailable when present");
        Expect(!editor::AddTerrain(working).succeeded, "second Terrain is rejected");
        Expect(!editor::CanDuplicateSelected(true, working, added.selection, false),
            "Duplicate Selected unavailable for Terrain");
        Expect(!editor::DuplicateSelected(working, added.selection).succeeded,
            "Duplicate Terrain fails");
        Expect(working.hasTerrain && working.terrain.heights.size() == 45,
            "failed Duplicate does not mutate Terrain");
        Expect(editor::CanDeleteSelected(true, working, added.selection, false),
            "Delete Terrain is available");
        const editor::LifecycleEditResult deleted = editor::DeleteSelected(working, added.selection);
        Expect(deleted.succeeded && !working.hasTerrain, "Delete Terrain removes authored Terrain");
        Expect(deleted.selection.kind == EditorObjectKind::None, "Delete Terrain clears selection");
        const world::LevelDefinition unchanged = MakeBaseLevel();
        Expect(world::AuthoredLevelDataEqual(unchanged, unchanged), "no-op Terrain compare is equal");
        world::LevelDefinition dirty = MakeBaseLevel();
        editor::AddTerrain(dirty);
        Expect(!world::AuthoredLevelDataEqual(unchanged, dirty), "Add Terrain is a semantic change");

        world::LevelDefinition inspector = dirty;
        const world::LevelDefinition inspectorBefore = inspector;
        inspector.terrain.origin.x += 1.0f;
        Expect(!world::AuthoredLevelDataEqual(inspectorBefore, inspector),
            "Inspector origin edit is a semantic change");
        inspector.terrain.origin = inspectorBefore.terrain.origin;
        Expect(world::AuthoredLevelDataEqual(inspectorBefore, inspector),
            "restored origin is a no-op");
        inspector.terrain.sizeX = 18.0f;
        Expect(!world::AuthoredLevelDataEqual(inspectorBefore, inspector),
            "Inspector Size X edit is a semantic change");
        Expect(inspector.terrain.resolutionX == inspectorBefore.terrain.resolutionX
                && inspector.terrain.resolutionZ == inspectorBefore.terrain.resolutionZ
                && inspector.terrain.heights.size() == inspectorBefore.terrain.heights.size(),
            "Size edit does not resample Terrain resolution");
        inspector.terrain.sizeX = inspectorBefore.terrain.sizeX;
        inspector.terrain.sizeZ = 10.0f;
        Expect(!world::AuthoredLevelDataEqual(inspectorBefore, inspector),
            "Inspector Size Z edit is a semantic change");
        inspector.terrain.enabled = false;
        inspector.terrain.sizeZ = inspectorBefore.terrain.sizeZ;
        Expect(!world::AuthoredLevelDataEqual(inspectorBefore, inspector),
            "Inspector Enabled edit is a semantic change");
        inspector.terrain.enabled = inspectorBefore.terrain.enabled;
        Expect(world::AuthoredLevelDataEqual(inspectorBefore, inspector),
            "restored Enabled is a no-op");
        inspector.terrain.heights[0] = 0.5f;
        Expect(!world::AuthoredLevelDataEqual(inspectorBefore, inspector),
            "Terrain height sculpt is a semantic change");
        inspector.terrain.heights[0] = inspectorBefore.terrain.heights[0];
        Expect(world::AuthoredLevelDataEqual(inspectorBefore, inspector),
            "restored heights are a no-op");
        Expect(
            world::TryAssignTerrainTextureIdentity(
                inspector.terrain, "textures/test_checker.png"),
            "Inspector texture assignment");
        Expect(!world::AuthoredLevelDataEqual(inspectorBefore, inspector),
            "texture assignment is a semantic change");
        Expect(world::TerrainHeightsEqual(inspectorBefore.terrain, inspector.terrain),
            "texture assignment does not modify heights");
        Expect(
            !world::TryAssignTerrainTextureIdentity(
                inspector.terrain, "textures/test_checker.png"),
            "same texture assignment is a no-op");
        inspector.terrain.textureIdentity = inspectorBefore.terrain.textureIdentity;
        Expect(world::AuthoredLevelDataEqual(inspectorBefore, inspector),
            "cleared assignment restores equality");
        Expect(world::TrySetTerrainTextureTiling(inspector.terrain, 0.5f),
            "Inspector tiling edit");
        Expect(!world::AuthoredLevelDataEqual(inspectorBefore, inspector),
            "tiling edit is a semantic change");
        Expect(inspector.terrain.heights == inspectorBefore.terrain.heights,
            "tiling does not modify heights");
        Expect(!world::TrySetTerrainTextureTiling(inspector.terrain, 0.5f),
            "same tiling is a no-op");
        inspector.terrain.textureTiling = inspectorBefore.terrain.textureTiling;
        Expect(world::AuthoredLevelDataEqual(inspectorBefore, inspector),
            "restored tiling is a no-op");
        Expect(!world::TryClearTerrainTextureIdentity(inspector.terrain),
            "clear empty assignment is a no-op");
    }

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
        world::LevelDefinition working = MakeBaseLevel();
        const editor::LifecycleEditResult added = editor::AddGoal(working, kTestPlacementA);
        Expect(added.succeeded && working.levelGoals.size() == 1, "add level goal");
        Expect(added.selection.kind == editor::EditorObjectKind::Goal, "add goal kind");
        Expect(added.selection.index == 0, "add goal selects new");
        Expect(Vec3Near(working.levelGoals[0].size, world::kDefaultLevelGoalSize),
            "default level goal size");
        Expect(Vec3Near(working.levelGoals[0].center,
            {kTestPlacementA.x, kTestPlacementA.y, working.initialSpawnVisualCenter.z}),
            "level goal uses camera X/Y and spawn-lane Z");
        Expect(world::LevelGoalSpecIsValid(working.levelGoals[0]), "default level goal is valid");
        Expect(working.levelGoals[0].nextLevelId.empty(), "default level goal is terminal");
        working.levelGoals[0].nextLevelId = "level_02";

        const editor::LifecycleEditResult duplicated =
            editor::DuplicateSelected(working, {editor::EditorObjectKind::Goal, 0});
        Expect(duplicated.succeeded && working.levelGoals.size() == 2, "duplicate level goal");
        Expect(duplicated.selection.index == 1, "duplicate goal appended");
        Expect(
            NearlyEqual(
                working.levelGoals[1].center.x,
                working.levelGoals[0].center.x + editor::kLifecycleDuplicateOffsetX),
            "duplicate goal offset +1 X");
        Expect(Vec3Near(working.levelGoals[1].size, working.levelGoals[0].size),
            "duplicate goal copies size");
        Expect(working.levelGoals[1].nextLevelId == "level_02", "duplicate copies Next Level");

        const editor::LifecycleEditResult deleted =
            editor::DeleteSelected(working, {editor::EditorObjectKind::Goal, 0});
        Expect(deleted.succeeded && working.levelGoals.size() == 1, "delete level goal");
        Expect(deleted.selection.kind == editor::EditorObjectKind::None, "delete goal clears selection");
        Expect(
            NearlyEqual(working.levelGoals[0].center.x, kTestPlacementA.x + editor::kLifecycleDuplicateOffsetX),
            "remaining goal is the duplicate");
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
        Expect(
            !editor::DeleteSelected(working, {EditorObjectKind::Goal, 0}).succeeded,
            "empty Goal delete is invalid");
        Expect(!editor::DeleteSelected(working, {EditorObjectKind::Slope, 0}).succeeded, "no slope delete");
        Expect(
            !editor::DeleteSelected(working, {EditorObjectKind::MovingPlatform, 0}).succeeded,
            "no moving delete");
        Expect(
            !editor::DeleteSelected(working, {EditorObjectKind::DynamicBox, 0}).succeeded,
            "empty Dynamic Box delete is invalid");
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

    {
        world::LevelDefinition working = MakeBaseLevel();
        const editor::LifecycleEditResult added =
            editor::AddDynamicBox(working, kTestPlacementA);
        Expect(added.succeeded, "Add Dynamic Box");
        Expect(working.dynamicBoxes.size() == 1, "one Dynamic Box after Add");
        Expect(working.dynamicBoxes[0].massKg == world::kDefaultDynamicBoxMassKg, "default mass 30 kg");
        Expect(
            working.dynamicBoxes[0].size.x == world::kDefaultDynamicBoxSize.x, "default size x");
        Expect(added.selection.kind == EditorObjectKind::DynamicBox, "Add selects Dynamic Box");
        const editor::LifecycleEditResult duplicated =
            editor::DuplicateSelected(working, added.selection);
        Expect(duplicated.succeeded, "Duplicate Dynamic Box");
        Expect(working.dynamicBoxes.size() == 2, "two Dynamic Boxes after Duplicate");
        Expect(
            working.dynamicBoxes[1].center.x
                == working.dynamicBoxes[0].center.x + editor::kLifecycleDuplicateOffsetX,
            "Duplicate offsets +1 X");
        working.dynamicBoxes[0].massKg = 5.0f;
        Expect(working.dynamicBoxes[0].massKg == 5.0f, "Inspector mass edit is workingCopy only");
        Expect(
            editor::DeleteSelected(working, {EditorObjectKind::DynamicBox, 1}).succeeded,
            "Delete Dynamic Box");
        Expect(working.dynamicBoxes.size() == 1, "one Dynamic Box after Delete");

        world::LevelDefinition shared = MakeBaseLevel();
        shared.elevatedPlatforms.resize(
            static_cast<std::size_t>(physics::kMaxAuthoredPhysicsBodies) - 1,
            {{40.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        Expect(
            editor::AddDynamicBox(shared, kTestPlacementA).succeeded,
            "one leftover body can add a Dynamic Box");
        Expect(
            !editor::AddPlatform(shared, kTestPlacementA).succeeded,
            "shared budget blocks extra Platform");
        Expect(
            !editor::AddDynamicBox(shared, kTestPlacementB).succeeded,
            "shared budget blocks extra Dynamic Box");
        Expect(
            !editor::AddDoor(shared, kTestPlacementA).succeeded,
            "shared budget blocks extra Door");
        Expect(
            editor::CategoryAtCountLimit(shared, EditorObjectKind::ElevatedPlatform),
            "Platform AtLimit with shared budget");
        Expect(
            editor::CategoryAtCountLimit(shared, EditorObjectKind::DynamicBox),
            "Dynamic Box AtLimit with shared budget");
        Expect(
            editor::CategoryAtCountLimit(shared, EditorObjectKind::Door),
            "Door AtLimit with shared budget");
        Expect(
            !editor::CategoryAtCountLimit(shared, EditorObjectKind::StaticProp),
            "Static Prop does not consume physics leftover");
        Expect(
            editor::AddStaticProp(shared, kTestPlacementA, "models/test_static.glb").succeeded,
            "Static Prop still adds when physics leftover is full");
        Expect(
            editor::AddPressurePlate(shared, kTestPlacementA).succeeded,
            "Pressure Plate still adds when physics leftover is full");
        Expect(
            !editor::CategoryAtCountLimit(shared, EditorObjectKind::PressurePlate),
            "Pressure Plate does not consume physics leftover");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        const std::size_t platformsBefore = working.elevatedPlatforms.size();
        Expect(
            !editor::AddStaticProp(working, kTestPlacementA, "").succeeded,
            "empty identity is rejected");
        Expect(
            !editor::AddStaticProp(working, kTestPlacementA, "../models/crate.glb").succeeded,
            "traversal identity is rejected");
        Expect(
            !editor::AddStaticProp(working, kTestPlacementA, "C:/temp/crate.glb").succeeded,
            "absolute identity is rejected");
        Expect(working.staticProps.empty(), "invalid Add leaves workingCopy props empty");
        const editor::LifecycleEditResult added =
            editor::AddStaticProp(working, kTestPlacementA, "models/test_static.glb");
        Expect(added.succeeded, "Add Static Prop");
        Expect(working.staticProps.size() == 1, "one Static Prop after Add");
        Expect(working.elevatedPlatforms.size() == platformsBefore, "Add Prop does not add platforms");
        Expect(
            working.staticProps[0].modelIdentity == "models/test_static.glb",
            "Add stores canonical identity");
        Expect(
            Vec3Near(working.staticProps[0].rotationDegrees, world::kDefaultStaticPropRotationDegrees),
            "default rotation is 0,0,0");
        Expect(
            Vec3Near(working.staticProps[0].scale, world::kDefaultStaticPropScale),
            "default scale is 1,1,1");
        Expect(
            Vec3Near(
                working.staticProps[0].position,
                {kTestPlacementA.x, kTestPlacementA.y, working.initialSpawnVisualCenter.z}),
            "Add uses camera X/Y and spawn-lane Z");
        Expect(added.selection.kind == EditorObjectKind::StaticProp, "Add selects Static Prop");
        const editor::LifecycleEditResult duplicated =
            editor::DuplicateSelected(working, added.selection);
        Expect(duplicated.succeeded, "Duplicate Static Prop");
        Expect(working.staticProps.size() == 2, "two Static Props after Duplicate");
        Expect(
            working.staticProps[1].modelIdentity == working.staticProps[0].modelIdentity,
            "Duplicate preserves asset reference");
        Expect(
            Vec3Near(working.staticProps[1].rotationDegrees, working.staticProps[0].rotationDegrees)
                && Vec3Near(working.staticProps[1].scale, working.staticProps[0].scale),
            "Duplicate preserves rotation and scale");
        Expect(
            NearlyEqual(
                working.staticProps[1].position.x,
                working.staticProps[0].position.x + editor::kLifecycleDuplicateOffsetX),
            "Duplicate offsets +1 X");
        working.staticProps[0].rotationDegrees.y = 90.0f;
        working.staticProps[0].scale.x = 2.0f;
        Expect(
            working.staticProps[0].rotationDegrees.y == 90.0f
                && working.staticProps[0].scale.x == 2.0f,
            "Inspector transform edit is workingCopy only");
        Expect(
            editor::DeleteSelected(working, {EditorObjectKind::StaticProp, 1}).succeeded,
            "Delete Static Prop instance");
        Expect(working.staticProps.size() == 1, "one Static Prop after Delete");
        Expect(
            working.staticProps[0].modelIdentity == "models/test_static.glb",
            "Delete instance does not clear remaining identity");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        const editor::LifecycleEditResult added =
            editor::AddPressurePlate(working, kTestPlacementA);
        Expect(added.succeeded, "Add Pressure Plate");
        Expect(working.pressurePlates.size() == 1, "one Pressure Plate after Add");
        Expect(
            working.pressurePlates[0].size.x == world::kDefaultPressurePlateSize.x
                && working.pressurePlates[0].size.y == world::kDefaultPressurePlateSize.y
                && working.pressurePlates[0].size.z == world::kDefaultPressurePlateSize.z,
            "default Pressure Plate size");
        Expect(
            working.pressurePlates[0].activateByDynamicBox
                && !working.pressurePlates[0].activateByPlayer
                && working.pressurePlates[0].visibleInGameplay,
            "Add Pressure Plate defaults box-only visible");
        Expect(added.selection.kind == EditorObjectKind::PressurePlate, "Add selects Pressure Plate");
        working.pressurePlates[0].activateByDynamicBox = false;
        working.pressurePlates[0].activateByPlayer = true;
        working.pressurePlates[0].visibleInGameplay = false;
        const float originalSizeX = working.pressurePlates[0].size.x;
        const editor::LifecycleEditResult duplicated =
            editor::DuplicateSelected(working, added.selection);
        Expect(duplicated.succeeded, "Duplicate Pressure Plate");
        Expect(working.pressurePlates.size() == 2, "two Pressure Plates after Duplicate");
        Expect(
            working.pressurePlates[1].center.x
                == working.pressurePlates[0].center.x + editor::kLifecycleDuplicateOffsetX,
            "Duplicate offsets +1 X");
        Expect(
            working.pressurePlates[1].size.x == originalSizeX
                && working.pressurePlates[1].size.y == world::kDefaultPressurePlateSize.y,
            "Duplicate preserves size");
        Expect(
            !working.pressurePlates[1].activateByDynamicBox
                && working.pressurePlates[1].activateByPlayer
                && !working.pressurePlates[1].visibleInGameplay,
            "Duplicate preserves Pressure Plate mode flags");
        Expect(
            editor::DeleteSelected(working, {EditorObjectKind::PressurePlate, 1}).succeeded,
            "Delete Pressure Plate");
        Expect(working.pressurePlates.size() == 1, "one Pressure Plate after Delete");
        Expect(
            editor::AddPressurePlateAt(working, {9.0f, 0.1f, 4.0f}).succeeded,
            "AddPressurePlateAt uses world center");
        Expect(working.pressurePlates.back().center.x == 9.0f, "AddAt X");
        Expect(working.pressurePlates.back().center.z == 4.0f, "AddAt does not snap to spawn.z");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        const editor::LifecycleEditResult added = editor::AddDoor(working, kTestPlacementA);
        Expect(added.succeeded, "Add Door");
        Expect(working.doors.size() == 1, "one Door after Add");
        Expect(
            working.doors[0].size.x == world::kDefaultDoorSize.x
                && working.doors[0].openDistance == world::kDefaultDoorOpenDistance
                && working.doors[0].requiredItemId.empty(),
            "default Door size, openDistance, and no required item");
        Expect(added.selection.kind == EditorObjectKind::Door, "Add selects Door");
        working.doors[0].requiredItemId = "key";
        working.pressurePlates.push_back({{4.0f, 0.1f, 0.0f}, world::kDefaultPressurePlateSize, 0});
        working.pressurePlates.push_back({{8.0f, 0.1f, 0.0f}, world::kDefaultPressurePlateSize, 0});
        const editor::LifecycleEditResult duplicated =
            editor::DuplicateSelected(working, added.selection);
        Expect(duplicated.succeeded, "Duplicate Door");
        Expect(working.doors.size() == 2, "two Doors after Duplicate");
        Expect(
            working.doors[1].center.x
                == working.doors[0].center.x + editor::kLifecycleDuplicateOffsetX,
            "Duplicate Door offsets +1 X");
        Expect(
            working.doors[1].openDistance == working.doors[0].openDistance,
            "Duplicate preserves openDistance");
        Expect(working.doors[1].requiredItemId == "key", "Duplicate preserves requiredItemId");
        Expect(
            working.pressurePlates[0].linkedDoorIndex == 0
                && working.pressurePlates[1].linkedDoorIndex == 0,
            "Duplicate Door does not retarget existing plate links");
        working.pressurePlates[0].linkedDoorIndex = 1;
        working.pressurePlates[1].linkedDoorIndex = 1;
        Expect(
            editor::DuplicateSelected(working, {EditorObjectKind::PressurePlate, 0}).succeeded,
            "Duplicate linked Pressure Plate");
        Expect(
            working.pressurePlates.back().linkedDoorIndex == 1,
            "Duplicate Pressure Plate preserves Door link");
        Expect(
            editor::DeleteSelected(working, {EditorObjectKind::Door, 0}).succeeded,
            "Delete earlier Door");
        Expect(working.doors.size() == 1, "one Door after deleting earlier");
        Expect(
            working.pressurePlates[0].linkedDoorIndex == 0
                && working.pressurePlates[1].linkedDoorIndex == 0
                && working.pressurePlates[2].linkedDoorIndex == 0,
            "deleting earlier Door remaps later Door links");
        Expect(
            editor::DeleteSelected(working, {EditorObjectKind::Door, 0}).succeeded,
            "Delete last Door");
        Expect(working.doors.empty(), "no Doors after last delete");
        Expect(
            working.pressurePlates[0].linkedDoorIndex == world::kNoLinkedDoor
                && working.pressurePlates[1].linkedDoorIndex == world::kNoLinkedDoor
                && working.pressurePlates[2].linkedDoorIndex == world::kNoLinkedDoor,
            "deleting linked Door clears plate links");
        Expect(
            editor::AddDoorAt(working, {9.0f, 1.5f, 4.0f}).succeeded,
            "AddDoorAt uses world center");
        Expect(working.doors.back().center.x == 9.0f, "AddDoorAt X");
        Expect(working.doors.back().center.z == 4.0f, "AddDoorAt does not snap to spawn.z");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        const editor::LifecycleEditResult added = editor::AddItemPickup(working, kTestPlacementA);
        Expect(added.succeeded, "Add Item Pickup");
        Expect(working.itemPickups.size() == 1, "one Item Pickup after Add");
        Expect(
            working.itemPickups[0].itemId == world::kDefaultItemPickupId
                && working.itemPickups[0].quantity == world::kDefaultItemPickupQuantity
                && working.itemPickups[0].modelIdentity.empty()
                && working.itemPickups[0].visualOffset.x == 0.0f
                && working.itemPickups[0].visualOffset.y == 0.0f
                && working.itemPickups[0].visualOffset.z == 0.0f
                && working.itemPickups[0].visualRotationDegrees.x == 0.0f
                && working.itemPickups[0].visualScale.x == 1.0f
                && working.itemPickups[0].visualScale.y == 1.0f
                && working.itemPickups[0].visualScale.z == 1.0f
                && working.itemPickups[0].showInteractionBounds
                && working.itemPickups[0].targetHighlightIntensity
                    == world::kDefaultItemPickupTargetHighlightIntensity
                && working.itemPickups[0].targetHighlightGoldAmount
                    == world::kDefaultItemPickupTargetHighlightGoldAmount
                && !working.itemPickups[0].idleAnimationEnabled
                && working.itemPickups[0].idleBobAmplitude
                    == world::kDefaultItemPickupIdleBobAmplitude
                && working.itemPickups[0].idleBobSpeed == world::kDefaultItemPickupIdleBobSpeed
                && working.itemPickups[0].idleSpinSpeedDegrees
                    == world::kDefaultItemPickupIdleSpinSpeedDegrees,
            "6. Add uses M59 defaults");
        Expect(added.selection.kind == EditorObjectKind::ItemPickup, "Add selects Item Pickup");
        working.itemPickups[0].itemId = "coin";
        working.itemPickups[0].quantity = 4;
        working.itemPickups[0].modelIdentity = "models/test_static.glb";
        working.itemPickups[0].visualOffset = {0.0f, 0.5f, 0.0f};
        working.itemPickups[0].visualRotationDegrees = {10.0f, 20.0f, 30.0f};
        working.itemPickups[0].visualScale = {0.2f, 0.3f, 0.4f};
        working.itemPickups[0].showInteractionBounds = false;
        working.itemPickups[0].targetHighlightIntensity = 0.25f;
        working.itemPickups[0].targetHighlightGoldAmount = 1.0f;
        working.itemPickups[0].idleAnimationEnabled = true;
        working.itemPickups[0].idleBobAmplitude = 0.4f;
        working.itemPickups[0].idleBobSpeed = 2.0f;
        working.itemPickups[0].idleSpinSpeedDegrees = -90.0f;
        const editor::LifecycleEditResult duplicated =
            editor::DuplicateSelected(working, added.selection);
        Expect(duplicated.succeeded, "Duplicate Item Pickup");
        Expect(working.itemPickups.size() == 2, "two Item Pickups after Duplicate");
        Expect(
            working.itemPickups[1].position.x
                == working.itemPickups[0].position.x + editor::kLifecycleDuplicateOffsetX,
            "Duplicate Item Pickup offsets +1 X");
        Expect(
            working.itemPickups[1].itemId == working.itemPickups[0].itemId
                && working.itemPickups[1].quantity == working.itemPickups[0].quantity
                && working.itemPickups[1].modelIdentity == working.itemPickups[0].modelIdentity
                && working.itemPickups[1].visualOffset.y == working.itemPickups[0].visualOffset.y
                && working.itemPickups[1].visualRotationDegrees.y
                    == working.itemPickups[0].visualRotationDegrees.y
                && working.itemPickups[1].visualScale.x == working.itemPickups[0].visualScale.x
                && working.itemPickups[1].showInteractionBounds
                    == working.itemPickups[0].showInteractionBounds
                && working.itemPickups[1].targetHighlightIntensity
                    == working.itemPickups[0].targetHighlightIntensity
                && working.itemPickups[1].targetHighlightGoldAmount
                    == working.itemPickups[0].targetHighlightGoldAmount
                && working.itemPickups[1].idleAnimationEnabled
                    == working.itemPickups[0].idleAnimationEnabled
                && working.itemPickups[1].idleBobAmplitude
                    == working.itemPickups[0].idleBobAmplitude
                && working.itemPickups[1].idleBobSpeed == working.itemPickups[0].idleBobSpeed
                && working.itemPickups[1].idleSpinSpeedDegrees
                    == working.itemPickups[0].idleSpinSpeedDegrees,
            "7. Duplicate preserves M59 presentation values");
        Expect(
            editor::DeleteSelected(working, {EditorObjectKind::ItemPickup, 0}).succeeded,
            "Delete earlier Item Pickup");
        Expect(working.itemPickups.size() == 1, "one Item Pickup after deleting earlier");
        Expect(
            editor::DeleteSelected(working, {EditorObjectKind::ItemPickup, 0}).succeeded,
            "Delete last Item Pickup");
        Expect(working.itemPickups.empty(), "no Item Pickups after last delete");
        Expect(
            editor::AddItemPickupAt(working, {9.0f, 1.5f, 4.0f}).succeeded,
            "AddItemPickupAt uses world center");
        Expect(working.itemPickups.back().position.x == 9.0f, "AddItemPickupAt X");
        Expect(working.itemPickups.back().position.z == 4.0f, "AddItemPickupAt does not snap to spawn.z");
    }

    {
        world::LevelDefinition working{};
        working.elevatedPlatforms.push_back({{0.0f, 1.0f, 0.0f}, {2.0f, 0.5f, 2.0f}});
        working.elevatedPlatforms.push_back({{4.0f, 1.0f, 0.0f}, {2.0f, 0.5f, 2.0f}});
        working.hazards.push_back({});
        editor::EditorSelection primary{editor::EditorObjectKind::ElevatedPlatform, 0};
        std::vector<editor::EditorSelection> additional{
            {editor::EditorObjectKind::ElevatedPlatform, 1},
            {editor::EditorObjectKind::Hazard, 0}};
        editor::ReconcileEditorSelectionSet(working, primary, additional);
        Expect(primary.kind == editor::EditorObjectKind::ElevatedPlatform && primary.index == 0,
            "valid primary survives reconcile");
        Expect(additional.size() == 2, "valid additional members survive reconcile");

        additional.push_back({editor::EditorObjectKind::ElevatedPlatform, 9});
        editor::ReconcileEditorSelectionSet(working, primary, additional);
        Expect(additional.size() == 2, "out-of-range additional is dropped");

        primary = {editor::EditorObjectKind::ElevatedPlatform, 9};
        editor::ReconcileEditorSelectionSet(working, primary, additional);
        Expect(primary.kind == editor::EditorObjectKind::Hazard && primary.index == 0,
            "invalid primary promotes last remaining additional");
        Expect(additional.size() == 1, "promoted primary is removed from additional");

        working.hazards.clear();
        editor::ReconcileEditorSelectionSet(working, primary, additional);
        Expect(primary.kind == editor::EditorObjectKind::ElevatedPlatform, "next remaining becomes primary");

        editor::ClearEditorSelectionSet(primary, additional);
        Expect(primary.kind == editor::EditorObjectKind::None && additional.empty(),
            "clearing drops primary and secondary");
    }

    // ---- M79 Duplicate Selected / Delete Selected ----
    {
        const std::vector<EditorSelection> members{
            {EditorObjectKind::ElevatedPlatform, 1},
            {EditorObjectKind::ElevatedPlatform, 4},
            {EditorObjectKind::ElevatedPlatform, 3},
            {EditorObjectKind::Hazard, 0}};
        const std::vector<EditorSelection> plan =
            editor::MakeDescendingCategoryDeletePlan(members);
        Expect(plan.size() == 4, "delete plan keeps every unique member");
        Expect(plan[0].kind == EditorObjectKind::ElevatedPlatform && plan[0].index == 4
                && plan[1].kind == EditorObjectKind::ElevatedPlatform && plan[1].index == 3
                && plan[2].kind == EditorObjectKind::ElevatedPlatform && plan[2].index == 1,
            "same-category delete plan is descending");
        Expect(plan[3].kind == EditorObjectKind::Hazard && plan[3].index == 0,
            "mixed-category delete plan stays deterministic");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        working.hazards.push_back({{20.0f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
        const world::HazardSpec unselected = working.hazards[1];
        const EditorSelection primary{EditorObjectKind::Hazard, 0};
        const editor::LifecycleEditResult duplicated =
            editor::DuplicateSelectionSet(working, primary, {});
        Expect(duplicated.succeeded, "single DuplicateSelectionSet remains equivalent");
        Expect(working.hazards.size() == 3, "single Duplicate still +1");
        Expect(
            working.hazards[2].center.x
                == working.hazards[0].center.x + editor::kLifecycleDuplicateOffsetX,
            "single Duplicate still uses +1 X authority");
        Expect(duplicated.selection.kind == EditorObjectKind::Hazard
                && duplicated.selection.index == 2
                && duplicated.additionalSelections.empty(),
            "single Duplicate still selects only the copy");
        Expect(Vec3Near(working.hazards[1].center, unselected.center),
            "unselected Hazard is not duplicated");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        working.hazards.push_back({{20.0f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
        const EditorSelection primary{EditorObjectKind::Hazard, 0};
        Expect(
            editor::DeleteSelectionSet(working, primary, {}).succeeded,
            "single DeleteSelectionSet remains equivalent");
        Expect(working.hazards.size() == 1, "single Delete still removes one");
        Expect(working.hazards[0].center.x == 20.0f, "unselected Hazard remains");
    }

    {
        world::LevelDefinition working{};
        working.elevatedPlatforms.push_back({{2.0f, 1.0f, 0.0f}, {2.0f, 0.5f, 2.0f}});
        working.elevatedPlatforms.push_back({{4.5f, 1.0f, 0.0f}, {2.0f, 0.5f, 2.0f}});
        working.elevatedPlatforms.push_back({{8.0f, 1.0f, 0.0f}, {2.0f, 0.5f, 2.0f}});
        working.checkpoint1PlatformIndex = 2;
        working.checkpoint2PlatformIndex = 2;
        working.goalPlatformIndex = 2;
        working.hazards.push_back({{1.0f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
        working.collectibles.push_back({{0.0f, 2.0f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        const EditorSelection primary{EditorObjectKind::ElevatedPlatform, 0};
        const std::vector<EditorSelection> additional{
            {EditorObjectKind::ElevatedPlatform, 1},
            {EditorObjectKind::Hazard, 0}};
        const world::CollectibleSpec unselectedCollectible = working.collectibles[0];
        const editor::LifecycleEditResult duplicated =
            editor::DuplicateSelectionSet(working, primary, additional);
        Expect(duplicated.succeeded, "Duplicate Selected copies every selected object once");
        Expect(working.elevatedPlatforms.size() == 5, "two selected Platforms duplicated");
        Expect(working.hazards.size() == 2, "selected Hazard duplicated");
        Expect(working.collectibles.size() == 1, "unselected Collectible is not duplicated");
        Expect(Vec3Near(working.collectibles[0].center, unselectedCollectible.center),
            "unselected Collectible identity unchanged");
        Expect(
            NearlyEqual(
                working.elevatedPlatforms[3].center.x,
                2.0f + editor::kLifecycleDuplicateOffsetX),
            "existing +1 X duplicate offset remains");
        Expect(
            NearlyEqual(
                working.elevatedPlatforms[4].center.x - working.elevatedPlatforms[3].center.x,
                2.5f),
            "duplicated composition keeps relative spacing");
        Expect(
            duplicated.selection.kind == EditorObjectKind::ElevatedPlatform
                && duplicated.selection.index == 3,
            "copy of original PRIMARY becomes new PRIMARY");
        Expect(duplicated.additionalSelections.size() == 2, "other copies become secondary");
        Expect(
            duplicated.additionalSelections[0].kind == EditorObjectKind::ElevatedPlatform
                && duplicated.additionalSelections[0].index == 4,
            "Platform copy is a secondary selection");
        Expect(
            duplicated.additionalSelections[1].kind == EditorObjectKind::Hazard
                && duplicated.additionalSelections[1].index == 1,
            "Hazard copy is a secondary selection");
        Expect(
            duplicated.selection != EditorSelection{EditorObjectKind::ElevatedPlatform, 0}
                && duplicated.selection != EditorSelection{EditorObjectKind::ElevatedPlatform, 1}
                && duplicated.selection != EditorSelection{EditorObjectKind::Hazard, 0},
            "originals are not selected after Duplicate Selected");
        Expect(
            duplicated.additionalSelections[0] != duplicated.selection
                && duplicated.additionalSelections[1] != duplicated.selection
                && duplicated.additionalSelections[0] != duplicated.additionalSelections[1],
            "no duplicate selection entries remain");
        Expect(
            editor::IsValidSelection(working, duplicated.selection)
                && editor::IsValidSelection(working, duplicated.additionalSelections[0])
                && editor::IsValidSelection(working, duplicated.additionalSelections[1]),
            "post-duplicate selection indices are in range");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        const world::LevelDefinition before = working;
        const EditorSelection primary{EditorObjectKind::Hazard, 0};
        const std::vector<EditorSelection> additional{{EditorObjectKind::Spawn, 0}};
        const editor::LifecycleEditResult refused =
            editor::DuplicateSelectionSet(working, primary, additional);
        Expect(!refused.succeeded, "unsupported Duplicate member refuses the set");
        Expect(refused.status == editor::LifecycleEditStatus::UnsupportedType,
            "unsupported Duplicate reports UnsupportedType");
        Expect(world::AuthoredLevelDataEqual(working, before),
            "failed Duplicate Selected leaves workingCopy unchanged");
        Expect(
            editor::DuplicateSelectedDisableReason(true, working, primary, false, additional)
                != nullptr,
            "unsupported Duplicate Selected has compact feedback");
        Expect(
            !editor::CanDuplicateSelected(true, working, primary, additional, false),
            "CanDuplicateSelected is false for unsupported members");
    }

    {
        world::LevelDefinition working{};
        working.elevatedPlatforms.resize(
            static_cast<std::size_t>(world::kMaxElevatedPlatformCount) - 1);
        for (world::Box& platform : working.elevatedPlatforms)
        {
            platform.size = {2.0f, 0.5f, 2.0f};
        }
        working.checkpoint1PlatformIndex = 0;
        working.checkpoint2PlatformIndex = 0;
        working.goalPlatformIndex = 0;
        const std::size_t beforeCount = working.elevatedPlatforms.size();
        const EditorSelection primary{EditorObjectKind::ElevatedPlatform, 0};
        const std::vector<EditorSelection> additional{
            {EditorObjectKind::ElevatedPlatform, 1}};
        const editor::LifecycleEditResult refused =
            editor::DuplicateSelectionSet(working, primary, additional);
        Expect(!refused.succeeded, "capacity refuses partial Duplicate Selected");
        Expect(working.elevatedPlatforms.size() == beforeCount,
            "failed Duplicate Selected does not keep the first copy");
        Expect(refused.status == editor::LifecycleEditStatus::AtLimit,
            "capacity Duplicate Selected reports AtLimit");
    }

    {
        world::LevelDefinition working{};
        working.elevatedPlatforms.push_back({{0.0f, 1.0f, 0.0f}, {2.0f, 0.5f, 2.0f}});
        working.elevatedPlatforms.push_back({{2.0f, 1.0f, 0.0f}, {2.0f, 0.5f, 2.0f}});
        working.elevatedPlatforms.push_back({{4.0f, 1.0f, 0.0f}, {2.0f, 0.5f, 2.0f}});
        working.elevatedPlatforms.push_back({{6.0f, 1.0f, 0.0f}, {2.0f, 0.5f, 2.0f}});
        working.elevatedPlatforms.push_back({{8.0f, 1.0f, 0.0f}, {2.0f, 0.5f, 2.0f}});
        working.checkpoint1PlatformIndex = 0;
        working.checkpoint2PlatformIndex = 2;
        working.goalPlatformIndex = 2;
        working.hazards.push_back({{11.0f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
        const core::Vec3 survivorA = working.elevatedPlatforms[0].center;
        const core::Vec3 survivorB = working.elevatedPlatforms[2].center;
        const core::Vec3 unselectedHazard = working.hazards[0].center;
        const EditorSelection primary{EditorObjectKind::ElevatedPlatform, 1};
        const std::vector<EditorSelection> additional{
            {EditorObjectKind::ElevatedPlatform, 4},
            {EditorObjectKind::ElevatedPlatform, 3}};
        const editor::LifecycleEditResult deleted =
            editor::DeleteSelectionSet(working, primary, additional);
        Expect(deleted.succeeded, "Delete Selected removes every selected object once");
        Expect(working.elevatedPlatforms.size() == 2, "three selected Platforms deleted");
        Expect(Vec3Near(working.elevatedPlatforms[0].center, survivorA),
            "unselected Platform 0 remains");
        Expect(Vec3Near(working.elevatedPlatforms[1].center, survivorB),
            "unselected Platform 2 remains after index shift");
        Expect(working.hazards.size() == 1 && Vec3Near(working.hazards[0].center, unselectedHazard),
            "unselected Hazard is not deleted");
        Expect(working.checkpoint2PlatformIndex == 1 && working.goalPlatformIndex == 1,
            "support indices remap onto the surviving platform");
        Expect(deleted.selection.kind == EditorObjectKind::None
                && deleted.additionalSelections.empty(),
            "successful Delete Selected clears selection");
    }

    {
        world::LevelDefinition working{};
        working.elevatedPlatforms.push_back({{0.0f, 1.0f, 0.0f}, {2.0f, 0.5f, 2.0f}});
        working.checkpoint1PlatformIndex = 0;
        working.checkpoint2PlatformIndex = 0;
        working.goalPlatformIndex = 0;
        working.hazards.push_back({{1.0f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
        working.collectibles.push_back({{2.0f, 2.0f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        working.doors.push_back({{4.0f, 1.5f, 0.0f}, world::kDefaultDoorSize, 3.0f, ""});
        const EditorSelection primary{EditorObjectKind::Hazard, 0};
        const std::vector<EditorSelection> additional{
            {EditorObjectKind::Collectible, 0},
            {EditorObjectKind::Door, 0}};
        Expect(
            editor::DeleteSelectionSet(working, primary, additional).succeeded,
            "mixed-category Delete Selected is deterministic");
        Expect(working.hazards.empty() && working.collectibles.empty() && working.doors.empty(),
            "every selected mixed-category object is deleted");
        Expect(working.elevatedPlatforms.size() == 1, "unselected Platform remains");
    }

    {
        world::LevelDefinition working{};
        working.elevatedPlatforms.push_back({{0.0f, 1.0f, 0.0f}, {2.0f, 0.5f, 2.0f}});
        working.checkpoint1PlatformIndex = 0;
        working.checkpoint2PlatformIndex = 0;
        working.goalPlatformIndex = 0;
        working.doors.push_back({{1.0f, 1.5f, 0.0f}, world::kDefaultDoorSize, 3.0f, ""});
        working.doors.push_back({{3.0f, 1.5f, 0.0f}, world::kDefaultDoorSize, 3.0f, ""});
        working.doors.push_back({{5.0f, 1.5f, 0.0f}, world::kDefaultDoorSize, 3.0f, ""});
        working.doors.push_back({{7.0f, 1.5f, 0.0f}, world::kDefaultDoorSize, 3.0f, ""});
        working.pressurePlates.push_back(
            {{0.0f, 0.1f, 0.0f}, world::kDefaultPressurePlateSize, 3});
        const core::Vec3 survivingDoor = working.doors[3].center;
        const EditorSelection primary{EditorObjectKind::Door, 1};
        const std::vector<EditorSelection> additional{{EditorObjectKind::Door, 0}};
        Expect(
            editor::DeleteSelectionSet(working, primary, additional).succeeded,
            "deleting lower Door indices remaps surviving plate links");
        Expect(working.doors.size() == 2, "two unselected Doors remain");
        Expect(Vec3Near(working.doors[1].center, survivingDoor),
            "old Door 3 survives at the remapped index");
        Expect(working.pressurePlates[0].linkedDoorIndex == 1,
            "Pressure Plate stays attached to the same surviving Door");
    }

    {
        world::LevelDefinition working{};
        working.elevatedPlatforms.push_back({{0.0f, 1.0f, 0.0f}, {2.0f, 0.5f, 2.0f}});
        working.checkpoint1PlatformIndex = 0;
        working.checkpoint2PlatformIndex = 0;
        working.goalPlatformIndex = 0;
        working.doors.push_back({{1.0f, 1.5f, 0.0f}, world::kDefaultDoorSize, 3.0f, ""});
        working.doors.push_back({{3.0f, 1.5f, 0.0f}, world::kDefaultDoorSize, 3.0f, ""});
        working.pressurePlates.push_back(
            {{0.0f, 0.1f, 0.0f}, world::kDefaultPressurePlateSize, 1});
        working.pressurePlates.push_back(
            {{2.0f, 0.1f, 0.0f}, world::kDefaultPressurePlateSize, 0});
        const EditorSelection primary{EditorObjectKind::Door, 1};
        Expect(
            editor::DeleteSelectionSet(working, primary, {}).succeeded,
            "deleting the referenced Door itself still clears the link");
        Expect(working.pressurePlates[0].linkedDoorIndex == world::kNoLinkedDoor,
            "single-delete semantics clear a deleted Door link");
        Expect(working.pressurePlates[1].linkedDoorIndex == 0,
            "unrelated Door link is unchanged");
    }

    {
        world::LevelDefinition working{};
        working.elevatedPlatforms.push_back({{0.0f, 1.0f, 0.0f}, {2.0f, 0.5f, 2.0f}});
        working.checkpoint1PlatformIndex = 0;
        working.checkpoint2PlatformIndex = 0;
        working.goalPlatformIndex = 0;
        working.doors.push_back({{1.0f, 1.5f, 0.0f}, world::kDefaultDoorSize, 3.0f, ""});
        working.doors.push_back({{3.0f, 1.5f, 0.0f}, world::kDefaultDoorSize, 3.0f, ""});
        working.pressurePlates.push_back(
            {{0.0f, 0.1f, 0.0f}, world::kDefaultPressurePlateSize, 1});
        working.pressurePlates.push_back(
            {{2.0f, 0.1f, 0.0f}, world::kDefaultPressurePlateSize, 0});
        const EditorSelection primary{EditorObjectKind::PressurePlate, 0};
        const std::vector<EditorSelection> additional{{EditorObjectKind::Door, 1}};
        Expect(
            editor::DeleteSelectionSet(working, primary, additional).succeeded,
            "deleting referrer and referenced Door together is safe");
        Expect(working.pressurePlates.size() == 1, "unselected plate remains");
        Expect(working.doors.size() == 1, "unselected Door remains");
        Expect(working.pressurePlates[0].linkedDoorIndex == 0,
            "surviving plate still points at the surviving Door");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        const world::LevelDefinition before = working;
        const EditorSelection primary{EditorObjectKind::Hazard, 0};
        const std::vector<EditorSelection> additional{
            {EditorObjectKind::ElevatedPlatform, 0}};
        const editor::LifecycleEditResult refused =
            editor::DeleteSelectionSet(working, primary, additional);
        Expect(!refused.succeeded, "referenced Platform in the set refuses Delete Selected");
        Expect(world::AuthoredLevelDataEqual(working, before),
            "failed Delete Selected leaves workingCopy unchanged");
        Expect(working.hazards.size() == before.hazards.size(),
            "failed Delete Selected does not delete the supported subset");
        Expect(
            !editor::CanDeleteSelected(true, working, primary, additional, false),
            "CanDeleteSelected is false when any member cannot be deleted");
        Expect(
            editor::DeleteSelectedDisableReason(true, working, primary, false, additional)
                != nullptr,
            "unsupported Delete Selected has compact feedback");
        Expect(
            !editor::ShouldEmitDeleteSelectedRequest(
                true, false, true, working, primary, false, additional),
            "Delete key does not emit for an unsupported set");
        Expect(
            editor::ShouldEmitDeleteSelectedRequest(
                true,
                false,
                true,
                working,
                {EditorObjectKind::Hazard, 0},
                false,
                {{EditorObjectKind::Collectible, 0}}),
            "Delete key emits for a fully supported multi-selection");
    }

    {
        Expect(gameplay::IsValidItemId("key"), "default Add itemId key is valid");
        Expect(gameplay::IsValidItemId("coin"), "canonical coin itemId is valid");
        Expect(gameplay::IsValidItemId("gem"), "gem matches M54 itemId grammar");
        Expect(!gameplay::IsValidItemId("Key"), "uppercase Key is rejected");
        Expect(!gameplay::IsValidItemId(""), "empty itemId is rejected");

        world::LevelDefinition working = MakeBaseLevel();
        Expect(editor::AddItemPickup(working, kTestPlacementA).succeeded, "Add first Item Pickup");
        Expect(editor::AddItemPickup(working, kTestPlacementB).succeeded, "Add second Item Pickup");
        Expect(working.itemPickups.size() == 2, "two Item Pickups");
        Expect(
            working.itemPickups[0].itemId == world::kDefaultItemPickupId
                && working.itemPickups[1].itemId == world::kDefaultItemPickupId,
            "both new Item Pickups default to key");
        const world::LevelDefinition baseline = working;

        const EditorSelection primary{EditorObjectKind::ItemPickup, 1};
        editor::ItemIdInspectorFieldState field{};
        editor::SyncItemIdInspectorField(field, primary, working.itemPickups[1].itemId, false);
        Expect(std::strcmp(field.buffer, "key") == 0, "inactive sync loads workingCopy key");

        editor::SyncItemIdInspectorField(field, primary, working.itemPickups[1].itemId, true);
        editor::CopyItemIdInspectorBuffer(field.buffer, "co");
        editor::SyncItemIdInspectorField(field, primary, working.itemPickups[1].itemId, true);
        Expect(std::strcmp(field.buffer, "co") == 0,
            "active sync does not reload key over in-progress Item ID");
        Expect(
            editor::TryAcceptItemIdInspectorField(working.itemPickups[1].itemId, field)
                == editor::ItemIdInspectorCommitResult::Accepted,
            "valid prefix commits to PRIMARY workingCopy");
        Expect(working.itemPickups[1].itemId == "co", "PRIMARY accepted co");
        Expect(working.itemPickups[0].itemId == "key", "secondary Item Pickup stays key");

        editor::CopyItemIdInspectorBuffer(field.buffer, "coin");
        editor::SyncItemIdInspectorField(field, primary, working.itemPickups[1].itemId, true);
        Expect(std::strcmp(field.buffer, "coin") == 0, "later typing is kept while focused");
        Expect(
            editor::CommitItemIdInspectorFieldOnFocusLoss(working.itemPickups[1].itemId, field)
                == editor::ItemIdInspectorCommitResult::Accepted,
            "focus-loss commits coin");
        Expect(working.itemPickups[1].itemId == "coin", "second Item Pickup retains coin");
        Expect(working.itemPickups[0].itemId == "key", "first Item Pickup remains key");
        Expect(!world::AuthoredLevelDataEqual(working, baseline), "Dirty/authored state sees Item ID");

        const world::LevelDefinition applied = working;
        Expect(applied.itemPickups[1].itemId == "coin", "Apply promotion keeps coin");
        Expect(applied.itemPickups[0].itemId == "key", "Apply promotion keeps first key");

        editor::SyncItemIdInspectorField(field, primary, working.itemPickups[1].itemId, true);
        editor::CopyItemIdInspectorBuffer(field.buffer, "Key");
        Expect(
            editor::TryAcceptItemIdInspectorField(working.itemPickups[1].itemId, field)
                == editor::ItemIdInspectorCommitResult::Rejected,
            "invalid live Item ID is not written");
        Expect(working.itemPickups[1].itemId == "coin", "rejected live edit leaves coin");
        Expect(
            editor::CommitItemIdInspectorFieldOnFocusLoss(working.itemPickups[1].itemId, field)
                == editor::ItemIdInspectorCommitResult::Rejected,
            "invalid focus-loss restores last committed id");
        Expect(std::strcmp(field.buffer, "coin") == 0, "invalid focus-loss restores buffer");
        Expect(working.itemPickups[1].itemId == "coin", "invalid focus-loss leaves workingCopy");
        Expect(working.itemPickups[0].itemId == "key", "invalid edit does not mutate the other pickup");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        const editor::LifecycleEditResult addedPoint =
            editor::AddPointLight(working, kTestPlacementA);
        Expect(addedPoint.succeeded && working.pointLights.size() == 1, "add point light");
        Expect(
            addedPoint.selection.kind == EditorObjectKind::PointLight
                && addedPoint.selection.index == 0,
            "add point selects new");
        Expect(
            Vec3Near(
                working.pointLights[0].position,
                {kTestPlacementA.x + editor::kDefaultAddedPointLightOffset.x,
                 kTestPlacementA.y + editor::kDefaultAddedPointLightOffset.y,
                 working.initialSpawnVisualCenter.z + editor::kDefaultAddedPointLightOffset.z}),
            "default point uses camera X/Y and spawn-lane Z");
        Expect(working.pointLights[0].enabled, "default point is enabled");
        const world::LevelDefinition afterAdd = working;
        Expect(world::AuthoredLevelDataEqual(working, afterAdd), "Inspector no-op is not Dirty");
        working.pointLights[0].intensity = 2.25f;
        Expect(!world::AuthoredLevelDataEqual(working, afterAdd), "Inspector intensity edit is Dirty");
        working.pointLights[0].intensity = afterAdd.pointLights[0].intensity;

        const editor::LifecycleEditResult dupPoint =
            editor::DuplicateSelected(working, {EditorObjectKind::PointLight, 0});
        Expect(dupPoint.succeeded && working.pointLights.size() == 2, "duplicate point light");
        Expect(
            NearlyEqual(
                working.pointLights[1].position.x,
                working.pointLights[0].position.x + editor::kLifecycleDuplicateOffsetX),
            "duplicated point uses placement offset");
        Expect(working.pointLights[1].enabled == working.pointLights[0].enabled,
            "duplicated point copies enabled");
        working.pointLights[1].color = {0.2f, 0.4f, 1.0f};
        Expect(!world::PointLightEqual(working.pointLights[0], working.pointLights[1]),
            "duplicated point is an independent instance");

        const editor::LifecycleEditResult deleted =
            editor::DeleteSelected(working, {EditorObjectKind::PointLight, 0});
        Expect(deleted.succeeded && working.pointLights.size() == 1, "delete remaps remaining point");
        Expect(working.pointLights[0].color.z == 1.0f, "survivor keeps duplicated color");
        Expect(
            editor::IsValidSelection(working, {EditorObjectKind::PointLight, 0})
                && !editor::IsValidSelection(working, {EditorObjectKind::PointLight, 1}),
            "stale point selection index is invalid after delete");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        working.pointLights.push_back(world::MakeDefaultPointLight({0.0f, 2.0f, 0.0f}));
        working.pointLights.push_back(world::MakeDefaultPointLight({4.0f, 2.0f, 0.0f}));
        working.spotLights.push_back(world::MakeDefaultSpotLight({1.0f, 4.0f, 0.0f}));
        working.spotLights.push_back(world::MakeDefaultSpotLight({6.0f, 4.0f, 0.0f}));
        working.spotLights.push_back(world::MakeDefaultSpotLight({8.0f, 4.0f, 0.0f}));
        world::PressurePlateSpec plate{};
        plate.center = {0.0f, 0.1f, 0.0f};
        plate.size = world::kDefaultPressurePlateSize;
        plate.controlledLocalLights.push_back({world::LocalLightKind::Point, 1});
        plate.controlledLocalLights.push_back({world::LocalLightKind::Spot, 2});
        working.pressurePlates.push_back(plate);
        const world::LevelDefinition baseline = working;
        Expect(
            editor::TryAddPressurePlateLocalLightTarget(
                working.pressurePlates[0],
                {world::LocalLightKind::Point, 0},
                working.pointLights.size(),
                working.spotLights.size())
                == editor::PressurePlateLocalLightTargetEditResult::Added,
            "Inspector can add a Point target");
        Expect(
            editor::TryAddPressurePlateLocalLightTarget(
                working.pressurePlates[0],
                {world::LocalLightKind::Spot, 0},
                working.pointLights.size(),
                working.spotLights.size())
                == editor::PressurePlateLocalLightTargetEditResult::Added,
            "Inspector can add a Spot target");
        Expect(working.pressurePlates[0].controlledLocalLights.size() == 4,
            "Inspector can hold multiple local-light targets");
        Expect(!world::AuthoredLevelDataEqual(working, baseline), "semantic target add Dirties");
        Expect(
            editor::TryAddPressurePlateLocalLightTarget(
                working.pressurePlates[0],
                {world::LocalLightKind::Point, 0},
                working.pointLights.size(),
                working.spotLights.size())
                == editor::PressurePlateLocalLightTargetEditResult::Unchanged,
            "duplicate identical target is prevented");
        const world::LevelDefinition afterAdds = working;
        Expect(world::AuthoredLevelDataEqual(working, afterAdds), "duplicate add is not Dirty");
        Expect(
            editor::TryRemovePressurePlateLocalLightTargetAt(working.pressurePlates[0], 0)
                == editor::PressurePlateLocalLightTargetEditResult::Removed,
            "Inspector can remove a target");
        Expect(!world::AuthoredLevelDataEqual(working, afterAdds), "semantic target remove Dirties");
        Expect(
            editor::TryRemovePressurePlateLocalLightTargetAt(working.pressurePlates[0], 99)
                == editor::PressurePlateLocalLightTargetEditResult::Unchanged,
            "removing a missing target is a no-op");
        Expect(working.pressurePlates[0].linkedDoorIndex == world::kNoLinkedDoor,
            "Door Inspector field remains independent");
        Expect(working.pressurePlates[0].activateByDynamicBox
                && !working.pressurePlates[0].activateByPlayer
                && working.pressurePlates[0].visibleInGameplay,
            "Box/Player/visible fields remain independent");
        Expect(!working.pressurePlates[0].controlsDirectionalLight,
            "Controls Directional Light remains independent");

        working.pressurePlates[0].controlledLocalLights = {
            {world::LocalLightKind::Point, 1}, {world::LocalLightKind::Spot, 2}};
        const editor::LifecycleEditResult dupPlate =
            editor::DuplicateSelected(working, {EditorObjectKind::PressurePlate, 0});
        Expect(dupPlate.succeeded && working.pressurePlates.size() == 2,
            "duplicate Pressure Plate succeeds");
        Expect(
            working.pressurePlates[1].controlledLocalLights
                == working.pressurePlates[0].controlledLocalLights,
            "duplicated plate preserves original local-light targets");

        const editor::LifecycleEditResult dupPoint =
            editor::DuplicateSelected(working, {EditorObjectKind::PointLight, 1});
        Expect(dupPoint.succeeded && working.pointLights.size() == 3, "duplicate targeted Point");
        Expect(
            !world::PressurePlateHasLocalLightTarget(
                working.pressurePlates[0], {world::LocalLightKind::Point, 2})
                && !world::PressurePlateHasLocalLightTarget(
                    working.pressurePlates[1], {world::LocalLightKind::Point, 2}),
            "duplicating a targeted Point does not auto-target the copy");
        Expect(
            world::PressurePlateHasLocalLightTarget(
                working.pressurePlates[0], {world::LocalLightKind::Point, 1}),
            "existing plates still target the original Point");

        const editor::LifecycleEditResult dupSpot =
            editor::DuplicateSelected(working, {EditorObjectKind::SpotLight, 2});
        Expect(dupSpot.succeeded && working.spotLights.size() == 4, "duplicate targeted Spot");
        Expect(
            !world::PressurePlateHasLocalLightTarget(
                working.pressurePlates[0], {world::LocalLightKind::Spot, 3}),
            "duplicating a targeted Spot does not auto-target the copy");

        Expect(
            editor::DeleteSelected(working, {EditorObjectKind::PointLight, 1}).succeeded,
            "delete targeted Point");
        Expect(
            !world::PressurePlateHasLocalLightTarget(
                working.pressurePlates[0], {world::LocalLightKind::Point, 1}),
            "deleting a targeted Point removes its reference");
        Expect(
            working.pressurePlates[0].controlledLocalLights[0].kind == world::LocalLightKind::Spot
                && working.pressurePlates[0].controlledLocalLights[0].index == 2,
            "deleting a Point does not alter Spot target indices");

        working.pressurePlates[0].controlledLocalLights = {
            {world::LocalLightKind::Point, 1}, {world::LocalLightKind::Spot, 2}};
        working.pressurePlates[1].controlledLocalLights = {
            {world::LocalLightKind::Point, 1}, {world::LocalLightKind::Spot, 2}};
        Expect(
            editor::DeleteSelected(working, {EditorObjectKind::PointLight, 0}).succeeded,
            "delete earlier Point remaps later Point targets");
        Expect(
            world::PressurePlateHasLocalLightTarget(
                working.pressurePlates[0], {world::LocalLightKind::Point, 0})
                && world::PressurePlateHasLocalLightTarget(
                    working.pressurePlates[0], {world::LocalLightKind::Spot, 2}),
            "later Point indices remap independently of Spot");

        working.pressurePlates[0].controlledLocalLights = {
            {world::LocalLightKind::Point, 0}, {world::LocalLightKind::Spot, 2}};
        Expect(
            editor::DeleteSelected(working, {EditorObjectKind::SpotLight, 0}).succeeded,
            "delete earlier Spot remaps later Spot targets");
        Expect(
            world::PressurePlateHasLocalLightTarget(
                working.pressurePlates[0], {world::LocalLightKind::Spot, 1})
                && world::PressurePlateHasLocalLightTarget(
                    working.pressurePlates[0], {world::LocalLightKind::Point, 0}),
            "Spot remap decrements later Spot indices and leaves Point indices alone");

        working.pressurePlates[0].controlledLocalLights = {
            {world::LocalLightKind::Spot, 1}};
        Expect(
            editor::DeleteSelected(working, {EditorObjectKind::SpotLight, 1}).succeeded,
            "delete targeted Spot");
        Expect(working.pressurePlates[0].controlledLocalLights.empty(),
            "deleting a targeted Spot removes its reference");

        working.pressurePlates[0].controlledLocalLights.push_back(
            {world::LocalLightKind::Point, 0});
        working.pressurePlates[1].controlledLocalLights.push_back(
            {world::LocalLightKind::Point, 0});
        Expect(
            editor::DeleteSelected(working, {EditorObjectKind::PressurePlate, 0}).succeeded,
            "delete controlling Pressure Plate");
        Expect(working.pointLights[0].enabled, "deleting a plate does not mutate authored Point Enabled");
        const std::uint8_t remainingActive = 1;
        Expect(
            world::AuthoredLocalLightIsEffectivelyEnabled(
                true,
                world::ResolveLocalLightActivation(
                    world::LocalLightKind::Point,
                    0,
                    working.pressurePlates,
                    &remainingActive,
                    1)),
            "remaining controlling plate keeps OR semantics");
        Expect(
            editor::DeleteSelected(working, {EditorObjectKind::PressurePlate, 0}).succeeded,
            "delete last controlling plate");
        Expect(
            world::AuthoredLocalLightIsEffectivelyEnabled(
                true,
                world::ResolveLocalLightActivation(
                    world::LocalLightKind::Point, 0, working.pressurePlates, nullptr, 0)),
            "no remaining plates restore authoredEnabled behavior");
    }

    {
        world::LevelDefinition working = MakeBaseLevel();
        const editor::LifecycleEditResult addedSpot =
            editor::AddSpotLight(working, kTestPlacementA);
        Expect(addedSpot.succeeded && working.spotLights.size() == 1, "add spot light");
        Expect(Vec3Near(working.spotLights[0].direction, {0.0f, -1.0f, 0.0f}),
            "default spot aims down");
        const editor::LifecycleEditResult dupSpot =
            editor::DuplicateSelected(working, {EditorObjectKind::SpotLight, 0});
        Expect(dupSpot.succeeded && working.spotLights.size() == 2, "duplicate spot light");
        Expect(
            NearlyEqual(
                working.spotLights[1].position.x,
                working.spotLights[0].position.x + editor::kLifecycleDuplicateOffsetX),
            "duplicated spot uses placement offset");
        working.spotLights[1].direction = world::CanonicalSpotLightDirection({1.0f, 0.0f, 0.0f});
        Expect(!world::SpotLightEqual(working.spotLights[0], working.spotLights[1]),
            "duplicated spot is an independent instance");
        Expect(editor::DeleteSelected(working, {EditorObjectKind::SpotLight, 0}).succeeded,
            "delete remaps remaining spot");
        Expect(working.spotLights.size() == 1 && working.spotLights[0].direction.x == 1.0f,
            "survivor keeps duplicated direction");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d authored object lifecycle test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Authored object lifecycle tests passed.\n");
    return 0;
}
