// Milestone 44: canonical scene cleanup invariants. No window, raylib, or Jolt.

#include "editor/EditorHierarchy.h"
#include "editor/EditorPicking.h"
#include "editor/EditorPlacement.h"
#include "editor/EditorSelection.h"
#include "physics/PhysicsCapacity.h"
#include "render/Renderer.h"
#include "world/LevelDefinition.h"

#include <cstddef>
#include <cstdio>
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

world::LevelDefinition MakeStubLevel()
{
    world::LevelDefinition level{};
    level.id = "level_01";
    level.initialSpawnVisualCenter = {0.0f, 0.8f, 0.0f};
    level.ground = {{0.0f, -0.25f, 0.0f}, {10.0f, 0.5f, 8.0f}};
    level.elevatedPlatforms.assign(
        static_cast<std::size_t>(world::kLevel01ElevatedPlatformCount),
        world::Box{{0.0f, 1.0f, 0.0f}, {1.0f, 0.5f, 1.0f}});
    level.slopes[0] = {{21.7f, 1.6732f, 0.0f}, {6.0f, 0.4f, 4.0f}, 30.0f};
    level.slopes[1] = {{25.6f, 0.966f, 0.0f}, {2.0f, 0.4f, 3.0f}, 60.0f};
    level.movingPlatform.size = {4.0f, 0.4f, 3.0f};
    level.movingPlatform.centerY = 1.3f;
    level.movingPlatform.startX = 0.0f;
    level.camera = {{2.0f, 3.5f, 12.0f}, 40.0f};
    level.checkpoints.resize(static_cast<std::size_t>(world::kLevel01CheckpointCount));
    level.hazards.resize(static_cast<std::size_t>(world::kLevel01HazardCount));
    level.collectibles.resize(static_cast<std::size_t>(world::kLevel01CollectibleCount));
    return level;
}
}

int main()
{
    Expect(physics::kPhysicsFixedBodyCount == 5, "fixed bodies are 5");
    Expect(physics::kPhysicsNonPlatformBodyCount == 5, "non-platform bodies are 5");
    Expect(physics::kMaxPhysicsElevatedPlatformCount == 59, "physics platform leftover is 59");
    Expect(world::kMaxElevatedPlatformCount == 59, "world platform leftover is 59");
    Expect(
        world::kMaxElevatedPlatformCount == physics::kMaxPhysicsElevatedPlatformCount,
        "world and physics leftover match");
    Expect(
        physics::kMaxPhysicsElevatedPlatformCount
            == static_cast<int>(physics::kPhysicsMaxBodies)
                - physics::kPhysicsNonPlatformBodyCount,
        "leftover is max bodies minus non-platform count");
    Expect(
        physics::kMaxAuthoredPhysicsBodies == 59,
        "shared authored-body leftover is 59");
    Expect(
        physics::AuthoredPhysicsBodiesWithinBudget(59, 0, 0),
        "59 platforms and 0 boxes fit");
    Expect(
        physics::AuthoredPhysicsBodiesWithinBudget(0, 59, 0),
        "0 platforms and 59 boxes fit");
    Expect(
        !physics::AuthoredPhysicsBodiesWithinBudget(59, 1, 0),
        "59 platforms and 1 box overflow");
    Expect(
        physics::AuthoredPhysicsBodiesWithinBudget(0, 0, 59),
        "0 platforms and 59 doors fit");
    Expect(
        !physics::AuthoredPhysicsBodiesWithinBudget(58, 0, 2),
        "58 platforms and 2 doors overflow");
    Expect(!render::kCanonicalSceneInstantiatesCookerProbes, "cooker probes are not in the scene");

    Expect(
        editor::IsEligiblePlacementSurface(editor::EditorObjectKind::Ground),
        "Ground remains a placement surface");
    Expect(
        editor::IsEligiblePlacementSurface(editor::EditorObjectKind::ElevatedPlatform),
        "Platform remains a placement surface");
    Expect(
        editor::IsEligiblePlacementSurface(editor::EditorObjectKind::Slope),
        "Slope remains a placement surface");
    Expect(
        !editor::IsEligiblePlacementSurface(editor::EditorObjectKind::DynamicBox),
        "DynamicBox is not a placement surface");
    Expect(
        !editor::IsEligiblePlacementSurface(editor::EditorObjectKind::PressurePlate),
        "PressurePlate is not a placement surface");
    Expect(
        !editor::IsEligiblePlacementSurface(editor::EditorObjectKind::Door),
        "Door is not a placement surface");
    Expect(
        !editor::IsEligiblePlacementSurface(editor::EditorObjectKind::StaticProp),
        "StaticProp is not a placement surface");
    Expect(
        !editor::IsEligiblePlacementSurface(editor::EditorObjectKind::MovingPlatform),
        "MovingPlatform is not a placement surface");

    const world::LevelDefinition level = MakeStubLevel();
    Expect(
        !editor::IsValidSelection(level, {editor::EditorObjectKind::DynamicBox, 0}),
        "empty Dynamic Boxes collection is not selectable");
    Expect(
        editor::IsEditableSelection({editor::EditorObjectKind::DynamicBox, 0}),
        "Dynamic Box has an Inspector path");
    Expect(
        !editor::IsValidSelection(level, {editor::EditorObjectKind::PressurePlate, 0}),
        "empty Pressure Plates collection is not selectable");
    Expect(
        editor::IsEditableSelection({editor::EditorObjectKind::PressurePlate, 0}),
        "Pressure Plate has an Inspector path");
    Expect(
        !editor::IsValidSelection(level, {editor::EditorObjectKind::Door, 0}),
        "empty Doors collection is not selectable");
    Expect(
        editor::IsEditableSelection({editor::EditorObjectKind::Door, 0}),
        "Door has an Inspector path");

    const std::vector<editor::HierarchyEntry> hierarchy = editor::BuildHierarchyEntries(level);
    bool hierarchyHasDynamicBox = false;
    bool hierarchyHasPressurePlate = false;
    bool hierarchyHasDoor = false;
    bool hierarchyHasStaticProp = false;
    bool hierarchyHasSlope0 = false;
    bool hierarchyHasSlope1 = false;
    bool hierarchyHasMovingPlatform = false;
    bool hierarchyHasGoal = false;
    bool hierarchyHasSpawn = false;
    bool hierarchyHasCamera = false;
    bool hierarchyHasGround = false;
    bool hierarchyHasPlatform = false;
    bool hierarchyHasCheckpoint = false;
    bool hierarchyHasHazard = false;
    bool hierarchyHasCollectible = false;
    for (const editor::HierarchyEntry& entry : hierarchy)
    {
        hierarchyHasDynamicBox =
            hierarchyHasDynamicBox || entry.selection.kind == editor::EditorObjectKind::DynamicBox;
        hierarchyHasPressurePlate =
            hierarchyHasPressurePlate || entry.selection.kind == editor::EditorObjectKind::PressurePlate;
        hierarchyHasDoor =
            hierarchyHasDoor || entry.selection.kind == editor::EditorObjectKind::Door;
        hierarchyHasStaticProp =
            hierarchyHasStaticProp || entry.selection.kind == editor::EditorObjectKind::StaticProp;
        hierarchyHasMovingPlatform = hierarchyHasMovingPlatform
            || entry.selection.kind == editor::EditorObjectKind::MovingPlatform;
        hierarchyHasGoal = hierarchyHasGoal || entry.selection.kind == editor::EditorObjectKind::Goal;
        hierarchyHasSpawn = hierarchyHasSpawn || entry.selection.kind == editor::EditorObjectKind::Spawn;
        hierarchyHasCamera =
            hierarchyHasCamera || entry.selection.kind == editor::EditorObjectKind::Camera;
        hierarchyHasGround =
            hierarchyHasGround || entry.selection.kind == editor::EditorObjectKind::Ground;
        hierarchyHasPlatform = hierarchyHasPlatform
            || entry.selection.kind == editor::EditorObjectKind::ElevatedPlatform;
        hierarchyHasCheckpoint = hierarchyHasCheckpoint
            || entry.selection.kind == editor::EditorObjectKind::Checkpoint;
        hierarchyHasHazard =
            hierarchyHasHazard || entry.selection.kind == editor::EditorObjectKind::Hazard;
        hierarchyHasCollectible = hierarchyHasCollectible
            || entry.selection.kind == editor::EditorObjectKind::Collectible;
        if (entry.selection.kind == editor::EditorObjectKind::Slope && entry.selection.index == 0)
        {
            hierarchyHasSlope0 = true;
        }
        if (entry.selection.kind == editor::EditorObjectKind::Slope && entry.selection.index == 1)
        {
            hierarchyHasSlope1 = true;
        }
    }
    Expect(!hierarchyHasDynamicBox, "empty collection has no Dynamic Box hierarchy rows");
    Expect(!hierarchyHasPressurePlate, "empty collection has no Pressure Plate hierarchy rows");
    Expect(!hierarchyHasDoor, "empty collection has no Door hierarchy rows");
    Expect(!hierarchyHasStaticProp, "empty collection has no Static Prop hierarchy rows");
    Expect(hierarchyHasSpawn, "Hierarchy lists Player Spawn");
    Expect(hierarchyHasCamera, "Hierarchy lists Camera");
    Expect(hierarchyHasGround, "Hierarchy lists Ground");
    Expect(hierarchyHasPlatform, "Hierarchy lists Platforms");
    Expect(hierarchyHasMovingPlatform, "Hierarchy lists Moving Platform");
    Expect(hierarchyHasSlope0, "Hierarchy lists slope 0");
    Expect(hierarchyHasSlope1, "Hierarchy lists slope 1");
    Expect(hierarchyHasCheckpoint, "Hierarchy lists Checkpoints");
    Expect(hierarchyHasHazard, "Hierarchy lists Hazards");
    Expect(hierarchyHasCollectible, "Hierarchy lists Collectibles");
    Expect(hierarchyHasGoal, "Hierarchy lists Goal");
    Expect(
        hierarchy.back().selection.kind == editor::EditorObjectKind::Goal,
        "empty collection keeps Goal last");

    const editor::EditorPickingSet set =
        editor::BuildPickingSet(level, editor::AuthoredPickingWorldState(level));
    bool pickHasGround = false;
    bool pickHasPlatform = false;
    bool pickHasSlope = false;
    bool pickHasDynamicBox = false;
    bool pickHasPressurePlate = false;
    bool pickHasDoor = false;
    bool pickHasStaticProp = false;
    bool pickHasMovingPlatform = false;
    for (const editor::PickingProxy& proxy : set.proxies)
    {
        pickHasGround = pickHasGround || proxy.selection.kind == editor::EditorObjectKind::Ground;
        pickHasPlatform =
            pickHasPlatform || proxy.selection.kind == editor::EditorObjectKind::ElevatedPlatform;
        pickHasSlope = pickHasSlope || proxy.selection.kind == editor::EditorObjectKind::Slope;
        pickHasDynamicBox =
            pickHasDynamicBox || proxy.selection.kind == editor::EditorObjectKind::DynamicBox;
        pickHasPressurePlate =
            pickHasPressurePlate || proxy.selection.kind == editor::EditorObjectKind::PressurePlate;
        pickHasDoor = pickHasDoor || proxy.selection.kind == editor::EditorObjectKind::Door;
        pickHasStaticProp =
            pickHasStaticProp || proxy.selection.kind == editor::EditorObjectKind::StaticProp;
        pickHasMovingPlatform = pickHasMovingPlatform
            || proxy.selection.kind == editor::EditorObjectKind::MovingPlatform;
    }
    Expect(pickHasGround, "picking includes Ground");
    Expect(pickHasPlatform, "picking includes Platform");
    Expect(pickHasSlope, "picking includes Slope");
    Expect(pickHasMovingPlatform, "picking includes moving platform");
    Expect(!pickHasDynamicBox, "empty collection has no Dynamic Box pick proxy");
    Expect(!pickHasPressurePlate, "empty collection has no Pressure Plate pick proxy");
    Expect(!pickHasDoor, "empty collection has no Door pick proxy");
    Expect(!pickHasStaticProp, "empty collection has no Static Prop pick proxy");

    const editor::Ray3 atAuthoredCrate{{0.0f, 5.0f, 8.0f}, {0.0f, 0.0f, -1.0f}};
    Expect(
        editor::PickNearest(atAuthoredCrate, set).kind != editor::EditorObjectKind::DynamicBox,
        "legacy cyan-box center is not pickable");

    const editor::Ray3 atSlope{{21.7f, 1.6732f, 8.0f}, {0.0f, 0.0f, -1.0f}};
    Expect(
        editor::PickNearest(atSlope, set).kind == editor::EditorObjectKind::Slope,
        "walkable slope remains pickable");

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d canonical scene cleanup test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("CanonicalSceneCleanupTest passed.\n");
    return 0;
}
