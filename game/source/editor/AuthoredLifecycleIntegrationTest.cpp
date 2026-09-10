#include "editor/AuthoredLifecycleCommands.h"
#include "editor/AuthoredObjectLifecycle.h"
#include "editor/EditorGizmo.h"
#include "editor/EditorHierarchy.h"
#include "editor/EditorPicking.h"
#include "editor/EditorPlacement.h"
#include "editor/EditorSelection.h"
#include "editor/StaticPropTransform.h"
#include "gameplay/CollectibleRunState.h"
#include "physics/PhysicsCapacity.h"
#include "world/LevelDefinition.h"

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

world::LevelDefinition MakeActiveLevel()
{
    world::LevelDefinition level{};
    level.id = "level_01";
    level.initialSpawnVisualCenter = {0.0f, 0.8f, 0.0f};
    level.ground = {{0.0f, -0.25f, 0.0f}, {56.0f, 0.5f, 8.0f}};
    level.elevatedPlatforms.push_back({{0.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
    level.elevatedPlatforms.push_back({{10.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
    level.elevatedPlatforms.push_back({{20.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
    level.checkpoint1PlatformIndex = 0;
    level.checkpoint2PlatformIndex = 2;
    level.goalPlatformIndex = 2;
    level.checkpoints.push_back({{16.5f, 1.8f, 0.0f}, {2.4f, 1.6f, 2.0f}, {16.5f, 1.8f, 0.0f}});
    level.hazards.push_back({{11.5f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
    level.collectibles.push_back({{5.0f, 2.5f, 0.0f}, {1.0f, 1.2f, 1.0f}});
    level.camera = {{2.0f, 3.5f, 12.0f}, 40.0f};
    return level;
}

void SeedEditor(editor::LevelEditorState& state, const world::LevelDefinition& active)
{
    state.workingCopy = active;
    state.savedSourceBaseline = active;
    editor::ClearCategoryStructuralPending(state.structuralPending);
    editor::ResetStructuralIndexMap(state.structuralMap, active);
    editor::RefreshLevelEditorDerivedFlags(state, active);
}

std::size_t HierarchyKindCount(
    const world::LevelDefinition& level,
    editor::EditorObjectKind kind)
{
    std::size_t count = 0;
    for (const editor::HierarchyEntry& entry : editor::BuildHierarchyEntries(level))
    {
        if (entry.selection.kind == kind)
        {
            ++count;
        }
    }
    return count;
}
}

int main()
{
    using editor::EditorObjectKind;
    using editor::EditorSelection;
    using editor::LevelEditorRequest;

    Expect(editor::IsAuthoredLifecycleRequest(LevelEditorRequest::AddPlatform), "add platform is lifecycle");
    Expect(editor::IsAuthoredLifecycleRequest(LevelEditorRequest::DeleteSelected), "delete is lifecycle");
    Expect(!editor::IsAuthoredLifecycleRequest(LevelEditorRequest::ApplyPreview), "Apply is not lifecycle");
    Expect(!editor::IsAuthoredLifecycleRequest(LevelEditorRequest::SaveLevelSource), "Save is not lifecycle");
    Expect(
        !editor::IsAuthoredLifecycleRequest(LevelEditorRequest::ImportStaticGlb),
        "Import Static GLB is not lifecycle");
    Expect(
        !editor::IsAuthoredLifecycleRequest(LevelEditorRequest::DeleteContentBrowserAsset),
        "Content Browser delete is not lifecycle");
    Expect(
        editor::ContentBrowserImportRequest() == LevelEditorRequest::ImportStaticGlb,
        "Content Browser import reuses M47 ImportStaticGlb");
    Expect(
        editor::EditAddMenuRequest(EditorObjectKind::ElevatedPlatform) == LevelEditorRequest::AddPlatform,
        "Edit > Add > Platform maps to AddPlatform");
    Expect(
        editor::EditAddMenuRequest(EditorObjectKind::Checkpoint) == LevelEditorRequest::AddCheckpoint,
        "Edit > Add > Checkpoint maps to AddCheckpoint");
    Expect(
        editor::EditAddMenuRequest(EditorObjectKind::Hazard) == LevelEditorRequest::AddHazard,
        "Edit > Add > Hazard maps to AddHazard");
    Expect(
        editor::EditAddMenuRequest(EditorObjectKind::Collectible) == LevelEditorRequest::AddCollectible,
        "Edit > Add > Collectible maps to AddCollectible");
    Expect(
        editor::EditAddMenuRequest(EditorObjectKind::DynamicBox) == LevelEditorRequest::AddDynamicBox,
        "Edit > Add > Dynamic Box maps to AddDynamicBox");
    Expect(
        editor::EditAddMenuRequest(EditorObjectKind::PressurePlate)
            == LevelEditorRequest::AddPressurePlate,
        "Edit > Add > Pressure Plate maps to AddPressurePlate");
    Expect(
        editor::EditAddMenuRequest(EditorObjectKind::Door) == LevelEditorRequest::AddDoor,
        "Edit > Add > Door maps to AddDoor");
    Expect(
        editor::EditAddMenuRequest(EditorObjectKind::StaticProp) == LevelEditorRequest::AddStaticProp,
        "Edit > Add > Static Prop maps to AddStaticProp");
    Expect(
        editor::ContentBrowserAddStaticPropRequest() == LevelEditorRequest::AddStaticProp,
        "Content Browser Add Static Prop maps to AddStaticProp");
    Expect(
        editor::ContentBrowserAddStaticPropRequest()
            == editor::EditAddMenuRequest(EditorObjectKind::StaticProp),
        "Content Browser button and Edit > Add > Static Prop share AddStaticProp");
    Expect(
        editor::IsAuthoredLifecycleRequest(editor::EditAddMenuRequest(EditorObjectKind::StaticProp)),
        "Edit Add Static Prop is owner-dispatched lifecycle");
    Expect(
        editor::IsAuthoredLifecycleRequest(editor::ContentBrowserAddStaticPropRequest()),
        "Content Browser Add Static Prop is owner-dispatched lifecycle");
    Expect(
        editor::IsAuthoredLifecycleRequest(editor::EditAddMenuRequest(EditorObjectKind::DynamicBox)),
        "Edit Add Dynamic Box is owner-dispatched lifecycle");
    Expect(
        editor::PlacementAddRequest(editor::PlacementMode::DynamicBox)
            == LevelEditorRequest::AddDynamicBox,
        "Object Palette Dynamic Box still confirms AddDynamicBox");
    Expect(
        editor::PlacementAddRequest(editor::PlacementMode::PressurePlate)
            == LevelEditorRequest::AddPressurePlate,
        "Object Palette Pressure Plate confirms AddPressurePlate");
        Expect(
            editor::PlacementAddRequest(editor::PlacementMode::Door) == LevelEditorRequest::AddDoor,
            "Object Palette Door confirms AddDoor");
    Expect(
        editor::EditAddMenuRequest(EditorObjectKind::ItemPickup)
            == LevelEditorRequest::AddItemPickup,
        "Edit > Add > Item Pickup maps to AddItemPickup");
    Expect(
        editor::PlacementAddRequest(editor::PlacementMode::ItemPickup)
            == LevelEditorRequest::AddItemPickup,
        "Object Palette Item Pickup confirms AddItemPickup");

    {
        const world::LevelDefinition active = MakeActiveLevel();
        Expect(
            editor::CanIssueAuthoredLifecycleRequest(
                true, active, {}, false, LevelEditorRequest::AddPlatform),
            "Development can add platform");
        Expect(
            !editor::CanIssueAuthoredLifecycleRequest(
                false, active, {}, false, LevelEditorRequest::AddPlatform),
            "Debug authoring cannot add");
        Expect(
            !editor::CanIssueAuthoredLifecycleRequest(
                true, active, {}, false, LevelEditorRequest::DuplicateSelected),
            "Duplicate disabled with no selection");
        Expect(
            !editor::CanIssueAuthoredLifecycleRequest(
                true, active, {}, false, LevelEditorRequest::DeleteSelected),
            "Delete disabled with no selection");
        Expect(
            editor::CanIssueAuthoredLifecycleRequest(
                true,
                active,
                {EditorObjectKind::Hazard, 0},
                false,
                LevelEditorRequest::DuplicateSelected),
            "Duplicate enabled for hazard");
        Expect(
            !editor::CanIssueAuthoredLifecycleRequest(
                true,
                active,
                {EditorObjectKind::ElevatedPlatform, 0},
                false,
                LevelEditorRequest::DeleteSelected),
            "Delete disabled for referenced platform");
        Expect(
            editor::CanIssueAuthoredLifecycleRequest(
                true,
                active,
                {EditorObjectKind::ElevatedPlatform, 1},
                false,
                LevelEditorRequest::DeleteSelected),
            "Delete enabled for unreferenced platform");
        Expect(
            !editor::CanIssueAuthoredLifecycleRequest(
                true,
                active,
                {EditorObjectKind::ElevatedPlatform, 1},
                true,
                LevelEditorRequest::DeleteSelected),
            "Delete disabled while gizmo dragging");
    }

    {
        world::LevelDefinition lastPlatform = MakeActiveLevel();
        lastPlatform.elevatedPlatforms.erase(
            lastPlatform.elevatedPlatforms.begin() + 1, lastPlatform.elevatedPlatforms.end());
        lastPlatform.checkpoint1PlatformIndex = 0;
        lastPlatform.checkpoint2PlatformIndex = 0;
        lastPlatform.goalPlatformIndex = 0;
        Expect(
            !editor::CanIssueAuthoredLifecycleRequest(
                true,
                lastPlatform,
                {EditorObjectKind::ElevatedPlatform, 0},
                false,
                LevelEditorRequest::DeleteSelected),
            "Delete disabled for last platform");
    }

    {
        world::LevelDefinition atLimit = MakeActiveLevel();
        atLimit.elevatedPlatforms.resize(static_cast<std::size_t>(world::kMaxElevatedPlatformCount));
        Expect(
            !editor::CanIssueAuthoredLifecycleRequest(
                true, atLimit, {}, false, LevelEditorRequest::AddPlatform),
            "Add disabled at platform cap");
        Expect(
            !editor::CanIssueAuthoredLifecycleRequest(
                true,
                atLimit,
                {EditorObjectKind::ElevatedPlatform, 0},
                false,
                LevelEditorRequest::DuplicateSelected),
            "Duplicate disabled at platform cap");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        const bool dirtyBefore = state.dirty;
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                state, active, LevelEditorRequest::AddPlatform, true),
            "Add Platform request handled");
        Expect(state.workingCopy.elevatedPlatforms.size() == 4, "Add Platform count +1");
        Expect(
            world::AuthoredLevelDataEqual(active, MakeActiveLevel())
                && active.elevatedPlatforms.size() == 3,
            "active unchanged by Add");
        Expect(
            state.selection.kind == EditorObjectKind::ElevatedPlatform && state.selection.index == 3,
            "Add selects new platform");
        Expect(state.structuralPending.elevatedPlatforms, "Add marks platform structural pending");
        Expect(!state.structuralPending.checkpoints, "Add Platform does not mark checkpoints");
        Expect(state.modified, "Add sets Modified");
        Expect(state.dirty == dirtyBefore, "Add leaves Dirty unchanged");
        Expect(!state.gizmo.dragging && state.gizmo.active == editor::EditorAxis::None, "Add clears gizmo");
        Expect(state.lastMessage == "Platform added.", "Add Platform status");
        Expect(
            HierarchyKindCount(state.workingCopy, EditorObjectKind::ElevatedPlatform) == 4,
            "hierarchy shows pending platform");
        Expect(
            editor::MappedActiveIndex(state.structuralMap, EditorObjectKind::ElevatedPlatform, 3)
                == editor::kNoStructuralIndex,
            "added platform has no active counterpart");
        Expect(
            editor::MappedWorkingIndex(state.structuralMap, EditorObjectKind::ElevatedPlatform, 2)
                == 2,
            "existing platforms stay world-pickable after Add");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState stateA{};
        editor::LevelEditorState stateB{};
        SeedEditor(stateA, active);
        SeedEditor(stateB, active);
        const core::Vec3 anchorA{12.0f, 5.0f, -6.0f};
        const core::Vec3 anchorB{-18.0f, 2.0f, 9.0f};
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                stateA, active, LevelEditorRequest::AddCheckpoint, true, anchorA),
            "Add Checkpoint at camera A");
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                stateB, active, LevelEditorRequest::AddCheckpoint, true, anchorB),
            "Add Checkpoint at camera B");
        Expect(
            stateA.workingCopy.checkpoints.back().center.x == anchorA.x
                && stateB.workingCopy.checkpoints.back().center.x == anchorB.x
                && stateA.workingCopy.checkpoints.back().center.y == anchorA.y
                && stateB.workingCopy.checkpoints.back().center.y == anchorB.y,
            "HandleAuthoredLifecycleRequest Add uses camera X/Y");
        Expect(
            stateA.workingCopy.checkpoints.back().center.z == active.initialSpawnVisualCenter.z
                && stateB.workingCopy.checkpoints.back().center.z == active.initialSpawnVisualCenter.z,
            "HandleAuthoredLifecycleRequest Add uses spawn-lane Z");
        Expect(active.checkpoints.size() == 1, "active unchanged by placed Add");
        editor::LevelEditorState duplicateState{};
        SeedEditor(duplicateState, active);
        duplicateState.selection = {EditorObjectKind::Checkpoint, 0};
        const float originalX = active.checkpoints[0].center.x;
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                duplicateState, active, LevelEditorRequest::DuplicateSelected, true, anchorB),
            "Duplicate ignores placementAnchor");
        Expect(
            duplicateState.workingCopy.checkpoints[1].center.x
                == originalX + editor::kLifecycleDuplicateOffsetX,
            "Duplicate still +1 X with unused camera anchor");
        Expect(
            editor::MappedActiveIndex(
                duplicateState.structuralMap, EditorObjectKind::Checkpoint, 1)
                == editor::kNoStructuralIndex,
            "duplicate checkpoint has no active counterpart");
        Expect(
            editor::ShouldAcceptActiveWorldPick(
                {EditorObjectKind::Checkpoint, 0}, duplicateState.structuralMap),
            "original checkpoint remains world-pickable after Duplicate");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        state.selection = {EditorObjectKind::Checkpoint, 0};
        const world::CheckpointSpec original = active.checkpoints[0];
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                state, active, LevelEditorRequest::DuplicateSelected, true),
            "Duplicate Checkpoint handled");
        Expect(state.workingCopy.checkpoints.size() == 2, "Duplicate checkpoint count");
        Expect(state.workingCopy.checkpoints[0].center.x == original.center.x, "original checkpoint unchanged");
        Expect(
            state.workingCopy.checkpoints[1].center.x
                == original.center.x + editor::kLifecycleDuplicateOffsetX,
            "duplicate +1 X");
        Expect(
            state.workingCopy.checkpoints[1].respawnPosition.x
                == original.respawnPosition.x + editor::kLifecycleDuplicateOffsetX,
            "duplicate respawn +1 X");
        Expect(state.selection.index == 1, "Duplicate selects copy");
        Expect(state.structuralPending.checkpoints, "Duplicate marks checkpoint pending");
        Expect(state.modified, "Duplicate sets Modified");
        Expect(state.lastMessage == "Checkpoint duplicated.", "Duplicate status");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        state.selection = {EditorObjectKind::Collectible, 0};
        gameplay::CollectibleRunState run =
            gameplay::MakeClearedCollectibleRunState(active.collectibles.size());
        run.collected[0] = 1;
        const int collectedBefore = gameplay::CollectedCount(run);
        const core::Vec3 original = active.collectibles[0].center;
        Expect(
            editor::IsValidSelection(state.workingCopy, state.selection),
            "collected authored collectible remains a valid inspector selection");
        Expect(
            editor::GetEditablePosition(state.workingCopy, state.selection) != nullptr,
            "collected authored collectible still exposes workingCopy center");
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                state, active, LevelEditorRequest::DuplicateSelected, true),
            "Duplicate collected Collectible handled");
        Expect(state.workingCopy.collectibles.size() == 2, "Duplicate collected count +1");
        Expect(
            state.workingCopy.collectibles[1].center.x
                == original.x + editor::kLifecycleDuplicateOffsetX,
            "collected duplicate still +1 X");
        Expect(
            state.workingCopy.collectibles[1].center.z == original.z,
            "collected duplicate keeps source Z");
        Expect(state.selection.index == 1, "Duplicate collected selects copy");
        Expect(state.structuralPending.collectibles, "Duplicate collected marks collectible pending");
        Expect(run.collected[0] == 1 && gameplay::CollectedCount(run) == collectedBefore,
            "Duplicate collected leaves CollectibleRunState untouched");
        Expect(active.collectibles.size() == 1, "Duplicate collected leaves active count");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        state.selection = {EditorObjectKind::Hazard, 0};
        const bool dirtyBefore = state.dirty;
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                state, active, LevelEditorRequest::DeleteSelected, true),
            "Delete Hazard handled");
        Expect(state.workingCopy.hazards.empty(), "Delete hazard count");
        Expect(active.hazards.size() == 1, "active hazard unchanged");
        Expect(state.selection.kind == EditorObjectKind::None, "Delete clears selection");
        Expect(state.structuralPending.hazards, "Delete marks hazard pending");
        Expect(state.modified, "Delete sets Modified");
        Expect(state.dirty == dirtyBefore, "Delete leaves Dirty unchanged");
        Expect(state.lastMessage == "Hazard deleted.", "Delete status");
        Expect(
            HierarchyKindCount(state.workingCopy, EditorObjectKind::Hazard) == 0,
            "hierarchy hides deleted hazard");
        Expect(
            editor::IsPendingDeleteActiveIndex(
                state.structuralMap, EditorObjectKind::Hazard, 0),
            "deleted active hazard is pending-delete");
        editor::EditorSelection ignored{};
        Expect(
            !editor::TryMapActiveWorldPick(
                {EditorObjectKind::Hazard, 0}, state.structuralMap, ignored),
            "pending-deleted hazard viewport pick ignored");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        state.selection = {EditorObjectKind::ElevatedPlatform, 1};
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                state, active, LevelEditorRequest::DeleteSelected, true),
            "Delete unreferenced platform handled");
        editor::EditorSelection workingPick{};
        Expect(
            editor::TryMapActiveWorldPick(
                {EditorObjectKind::ElevatedPlatform, 2}, state.structuralMap, workingPick),
            "surviving platform remains pickable after same-category delete");
        Expect(workingPick.index == 1, "active platform 2 maps to working 1");
        Expect(
            state.workingCopy.elevatedPlatforms[workingPick.index].center.x
                == active.elevatedPlatforms[2].center.x,
            "mapped platform is semantic former platform 2");
        Expect(
            !editor::TryMapActiveWorldPick(
                {EditorObjectKind::ElevatedPlatform, 1}, state.structuralMap, workingPick),
            "pending-deleted platform pick ignored");
        Expect(
            HierarchyKindCount(state.workingCopy, EditorObjectKind::ElevatedPlatform) == 2,
            "hierarchy omits pending-deleted platform");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        state.selection = {EditorObjectKind::ElevatedPlatform, 0};
        const editor::CategoryStructuralPending pendingBefore = state.structuralPending;
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                state, active, LevelEditorRequest::DeleteSelected, true),
            "referenced delete still handled");
        Expect(world::AuthoredLevelDataEqual(state.workingCopy, active), "referenced delete no mutation");
        Expect(state.selection.kind == EditorObjectKind::ElevatedPlatform, "referenced delete keeps selection");
        Expect(state.selection.index == 0, "referenced delete index");
        Expect(
            state.structuralPending.elevatedPlatforms == pendingBefore.elevatedPlatforms,
            "referenced delete pending unchanged");
        Expect(!state.modified, "referenced delete does not set Modified");
        Expect(
            state.lastMessage
                == "Delete blocked: Platform is referenced by level support metadata.",
            "referenced delete status");
    }

    {
        world::LevelDefinition active = MakeActiveLevel();
        active.elevatedPlatforms.erase(
            active.elevatedPlatforms.begin() + 1, active.elevatedPlatforms.end());
        active.checkpoint1PlatformIndex = 0;
        active.checkpoint2PlatformIndex = 0;
        active.goalPlatformIndex = 0;
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        state.selection = {EditorObjectKind::ElevatedPlatform, 0};
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                state, active, LevelEditorRequest::DeleteSelected, true),
            "last-platform delete handled");
        Expect(state.workingCopy.elevatedPlatforms.size() == 1, "last platform remains");
        Expect(state.selection.index == 0, "last-platform selection unchanged");
        Expect(state.lastMessage == "At least one Platform is required.", "minimum count status");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        editor::HandleAuthoredLifecycleRequest(
            state, active, LevelEditorRequest::AddCollectible, true);
        Expect(state.modified, "pending add is Modified");
        state.workingCopy = active;
        editor::ClearCategoryStructuralPending(state.structuralPending);
        editor::ResetStructuralIndexMap(state.structuralMap, active);
        state.selection = editor::ReconcileSelection(state.workingCopy, state.selection);
        editor::ClearGizmoInteraction(state.gizmo);
        editor::RefreshLevelEditorDerivedFlags(state, active);
        Expect(world::AuthoredLevelDataEqual(state.workingCopy, active), "Revert after Add restores");
        Expect(!state.structuralPending.collectibles, "Revert after Add clears pending");
        Expect(editor::MappedWorkingIndex(state.structuralMap, EditorObjectKind::Collectible, 0) == 0,
            "Revert after Add restores identity mapping");
        Expect(!state.modified, "Revert after Add clears Modified");
        Expect(
            editor::IsValidSelection(state.workingCopy, state.selection)
                || state.selection.kind == EditorObjectKind::None,
            "Revert after Add selection safe");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        state.selection = {EditorObjectKind::Collectible, 0};
        editor::HandleAuthoredLifecycleRequest(
            state, active, LevelEditorRequest::DeleteSelected, true);
        Expect(state.workingCopy.collectibles.empty(), "pending delete removed collectible");
        state.workingCopy = active;
        editor::ClearCategoryStructuralPending(state.structuralPending);
        editor::ResetStructuralIndexMap(state.structuralMap, active);
        state.selection = editor::ReconcileSelection(state.workingCopy, state.selection);
        editor::RefreshLevelEditorDerivedFlags(state, active);
        Expect(state.workingCopy.collectibles.size() == 1, "Revert after Delete restores object");
        Expect(active.collectibles.size() == 1, "Revert after Delete leaves active");
        Expect(!state.structuralPending.collectibles, "Revert after Delete clears pending");
        Expect(
            !editor::IsPendingDeleteActiveIndex(
                state.structuralMap, EditorObjectKind::Collectible, 0),
            "Revert after Delete restores collectible mapping");
        Expect(!state.modified, "Revert after Delete clears Modified");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        state.selection = {EditorObjectKind::Hazard, 0};
        Expect(
            editor::GetEditablePosition(state.workingCopy, state.selection) != nullptr,
            "inspector lookup before delete is in range");
        editor::HandleAuthoredLifecycleRequest(
            state, active, LevelEditorRequest::DeleteSelected, true);
        Expect(state.selection.kind == EditorObjectKind::None, "delete clears inspector selection");
        Expect(
            editor::GetEditablePosition(
                state.workingCopy, {EditorObjectKind::Hazard, 0})
                == nullptr,
            "stale hazard index has no inspector pointer");
        Expect(
            !editor::IsValidSelection(state.workingCopy, {EditorObjectKind::Hazard, 0}),
            "stale hazard selection is invalid");
    }

    {
        world::LevelDefinition emptyRepeatables{};
        emptyRepeatables.elevatedPlatforms.push_back({{0.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        Expect(HierarchyKindCount(emptyRepeatables, EditorObjectKind::ElevatedPlatform) == 1, "1 platform");
        Expect(HierarchyKindCount(emptyRepeatables, EditorObjectKind::Checkpoint) == 0, "0 checkpoints");
        Expect(HierarchyKindCount(emptyRepeatables, EditorObjectKind::Hazard) == 0, "0 hazards");
        Expect(HierarchyKindCount(emptyRepeatables, EditorObjectKind::Collectible) == 0, "0 collectibles");
        emptyRepeatables.checkpoints.push_back(
            {{1.0f, 1.0f, 0.0f}, {2.4f, 1.6f, 2.0f}, {1.0f, 1.0f, 0.0f}});
        emptyRepeatables.checkpoints.push_back(
            {{2.0f, 1.0f, 0.0f}, {2.4f, 1.6f, 2.0f}, {2.0f, 1.0f, 0.0f}});
        emptyRepeatables.hazards.push_back({{0.0f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
        emptyRepeatables.collectibles.push_back({{0.0f, 2.0f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        emptyRepeatables.collectibles.push_back({{1.0f, 2.0f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        emptyRepeatables.collectibles.push_back({{2.0f, 2.0f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        Expect(HierarchyKindCount(emptyRepeatables, EditorObjectKind::Checkpoint) == 2, "several checkpoints");
        Expect(HierarchyKindCount(emptyRepeatables, EditorObjectKind::Hazard) == 1, "several hazards");
        Expect(HierarchyKindCount(emptyRepeatables, EditorObjectKind::Collectible) == 3, "several collectibles");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                state, active, LevelEditorRequest::AddCheckpoint, true),
            "Add Checkpoint for pending visual");
        const EditorSelection added = state.selection;
        const editor::EditorPendingTransformPreview bounds =
            editor::MakePendingTransformPreview(added, active, state.workingCopy);
        const editor::EditorPendingObjectVisual visual =
            editor::MakePendingObjectVisual(added, active, state.workingCopy);
        Expect(bounds.visible, "Add Checkpoint pending bounds");
        Expect(visual.visible && visual.kind == editor::PendingObjectVisualKind::Checkpoint,
            "Add Checkpoint pending object visual");
        Expect(active.checkpoints.size() == 1, "Add Checkpoint leaves active count");
        Expect(
            visual.checkpoint.center.x == state.workingCopy.checkpoints[added.index].center.x,
            "Add Checkpoint visual from workingCopy");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        state.selection = {EditorObjectKind::Hazard, 0};
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                state, active, LevelEditorRequest::DuplicateSelected, true),
            "Duplicate Hazard for pending visual");
        const editor::EditorPendingObjectVisual visual = editor::MakePendingObjectVisual(
            state.selection, active, state.workingCopy);
        Expect(visual.visible && visual.kind == editor::PendingObjectVisualKind::Hazard,
            "Duplicate Hazard pending object visual");
        Expect(
            visual.hazard.center.x == state.workingCopy.hazards[1].center.x,
            "Duplicate Hazard visual at copy transform");
        Expect(active.hazards.size() == 1 && active.hazards[0].center.x == 11.5f,
            "Duplicate Hazard leaves active original");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                state, active, LevelEditorRequest::AddCollectible, true),
            "Add Collectible for persistent preview");
        const std::size_t addedIndex = state.workingCopy.collectibles.size() - 1;
        state.selection = {EditorObjectKind::ElevatedPlatform, 0};
        const std::vector<editor::PendingAuthoringVisual> visuals =
            editor::CollectPendingAuthoringVisuals(
                active, state.workingCopy, state.structuralMap, state.selection);
        Expect(
            editor::PendingAuthoringContains(
                visuals, EditorObjectKind::Collectible, addedIndex),
            "integration: pending Add remains after Hierarchy-other selection");
        const core::Vec3 addedCenter = state.workingCopy.collectibles[addedIndex].center;
        editor::EditorSelection picked{};
        Expect(
            editor::TryResolveEditorViewportPick(
                editor::Ray3{{addedCenter.x, addedCenter.y, addedCenter.z + 8.0f}, {0.0f, 0.0f, -1.0f}},
                editor::BuildPickingSet(active, editor::AuthoredPickingWorldState(active)),
                editor::BuildPendingPickProxies(visuals),
                state.structuralMap,
                picked),
            "integration: pending Add Collectible is viewport-pickable after deselect");
        Expect(
            picked.kind == EditorObjectKind::Collectible && picked.index == addedIndex,
            "integration: pending pick restores working Collectible selection");
        state.workingCopy = active;
        editor::ResetStructuralIndexMap(state.structuralMap, active);
        Expect(
            editor::CollectPendingAuthoringVisuals(
                active, state.workingCopy, state.structuralMap, {})
                .empty(),
            "integration: Revert clears pending authoring visuals");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                state, active, LevelEditorRequest::AddPlatform, true),
            "mixed Add Platform");
        state.workingCopy.hazards[0].center.x += 1.0f;
        Expect(
            editor::DeleteSelected(state.workingCopy, {EditorObjectKind::Collectible, 0}).succeeded,
            "mixed pending Delete collectible");
        editor::ApplyLifecycleToStructuralMap(
            state.structuralMap, EditorObjectKind::Collectible, true, 0);
        const std::vector<editor::PendingAuthoringVisual> mixed =
            editor::CollectPendingAuthoringVisuals(
                active, state.workingCopy, state.structuralMap, {EditorObjectKind::Hazard, 0});
        Expect(
            editor::PendingAuthoringContains(
                mixed,
                EditorObjectKind::ElevatedPlatform,
                state.workingCopy.elevatedPlatforms.size() - 1),
            "mixed: pending Add Platform is cyan-family");
        Expect(
            editor::PendingAuthoringContains(mixed, EditorObjectKind::Hazard, 0),
            "mixed: pending Modify Hazard is cyan-family");
        const editor::PendingAuthoringVisual* selectedHazard =
            editor::FindPendingAuthoringVisual(mixed, EditorObjectKind::Hazard, 0);
        Expect(selectedHazard != nullptr && selectedHazard->selected,
            "mixed: selected pending Modify has selected emphasis");
        Expect(
            !editor::PendingAuthoringContains(mixed, EditorObjectKind::Collectible, 0),
            "mixed: pending Delete is not cyan");
        Expect(
            editor::IsPendingDeleteActiveIndex(
                state.structuralMap, EditorObjectKind::Collectible, 0),
            "mixed: pending Delete uses delete identity");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        state.placementMode = editor::PlacementMode::Collectible;
        const core::Vec3 world{8.0f, 3.0f, 4.0f};
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                state,
                active,
                editor::LevelEditorRequest::AddCollectible,
                true,
                world,
                true),
            "palette confirm reuses Handle Add");
        Expect(state.workingCopy.collectibles.size() == active.collectibles.size() + 1,
            "palette confirm appends one");
        Expect(
            state.workingCopy.collectibles.back().center.x == world.x
                && state.workingCopy.collectibles.back().center.y == world.y
                && state.workingCopy.collectibles.back().center.z == world.z,
            "palette confirm uses world center, not spawn.z");
        Expect(
            state.selection.kind == EditorObjectKind::Collectible
                && state.selection.index == state.workingCopy.collectibles.size() - 1,
            "palette confirm selects the new object");
        Expect(
            state.placementMode == editor::PlacementMode::Collectible,
            "Handle does not exit placement mode");
        Expect(
            HierarchyKindCount(state.workingCopy, EditorObjectKind::Collectible)
                == state.workingCopy.collectibles.size(),
            "new Collectible is in Hierarchy");
        Expect(
            editor::MappedWorkingIndex(
                state.structuralMap,
                EditorObjectKind::Collectible,
                0)
                == 0,
            "StructuralIndexMap keeps existing Collectible identity");

        const core::Vec3 second{9.0f, 3.0f, 4.0f};
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                state,
                active,
                editor::LevelEditorRequest::AddCollectible,
                true,
                second,
                true),
            "repeated placement second click");
        Expect(state.workingCopy.collectibles.size() == active.collectibles.size() + 2,
            "repeated placement appends second");
        Expect(
            state.selection.index == state.workingCopy.collectibles.size() - 1,
            "latest repeated placement is selected");
        const std::vector<editor::PendingAuthoringVisual> pending =
            editor::CollectPendingAuthoringVisuals(
                active, state.workingCopy, state.structuralMap, state.selection);
        Expect(
            editor::PendingAuthoringContains(
                pending,
                EditorObjectKind::Collectible,
                state.workingCopy.collectibles.size() - 2),
            "previous pending Collectible remains visible");
        editor::EditorSelection picked{};
        const core::Vec3 firstCenter = state.workingCopy.collectibles[state.workingCopy.collectibles.size() - 2].center;
        Expect(
            editor::TryResolveEditorViewportPick(
                editor::Ray3{{firstCenter.x, firstCenter.y, firstCenter.z + 8.0f}, {0.0f, 0.0f, -1.0f}},
                editor::BuildPickingSet(active, editor::AuthoredPickingWorldState(active)),
                editor::BuildPendingPickProxies(pending),
                state.structuralMap,
                picked),
            "pending Collectible is viewport-pickable after palette placement");
        Expect(
            picked.kind == EditorObjectKind::Collectible
                && picked.index == state.workingCopy.collectibles.size() - 2,
            "pending pick uses working index");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        const float spawnZ = state.workingCopy.initialSpawnVisualCenter.z;
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                state,
                active,
                editor::LevelEditorRequest::AddPlatform,
                true,
                {12.0f, 6.0f, 9.0f},
                false),
            "Edit > Add still uses lane placement");
        Expect(
            state.workingCopy.elevatedPlatforms.back().center.z == spawnZ,
            "Edit > Add regression: spawn.z lane");
        Expect(
            state.workingCopy.elevatedPlatforms.back().center.x == 12.0f,
            "Edit > Add regression: camera X");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        state.placementMode = editor::PlacementMode::Hazard;
        editor::ClearPlacementMode(state.placementMode);
        Expect(state.placementMode == editor::PlacementMode::None, "Apply/Revert/Reload cancel helper");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        Expect(active.dynamicBoxes.empty(), "fixture starts with zero Dynamic Boxes");
        Expect(HierarchyKindCount(active, EditorObjectKind::DynamicBox) == 0,
            "empty collection has no Hierarchy Dynamic Box rows");

        const LevelEditorRequest editAdd =
            editor::EditAddMenuRequest(EditorObjectKind::DynamicBox);
        Expect(editAdd == LevelEditorRequest::AddDynamicBox, "menu action is AddDynamicBox");
        Expect(
            editor::CanIssueAuthoredLifecycleRequest(true, active, {}, false, editAdd),
            "Development can add Dynamic Box");
        Expect(
            !editor::CanIssueAuthoredLifecycleRequest(
                false, active, {}, false, editAdd),
            "Debug authoring cannot add Dynamic Box");

        const core::Vec3 cameraAnchor{12.0f, 5.0f, -6.0f};
        Expect(
            editor::HandleAuthoredLifecycleRequest(state, active, editAdd, true, cameraAnchor),
            "Edit > Add > Dynamic Box reaches workingCopy mutation");
        Expect(state.workingCopy.dynamicBoxes.size() == 1, "workingCopy gains exactly one Dynamic Box");
        Expect(active.dynamicBoxes.empty(), "Edit Add does not mutate active");
        Expect(
            state.workingCopy.dynamicBoxes[0].size.x == world::kDefaultDynamicBoxSize.x
                && state.workingCopy.dynamicBoxes[0].size.y == world::kDefaultDynamicBoxSize.y
                && state.workingCopy.dynamicBoxes[0].size.z == world::kDefaultDynamicBoxSize.z,
            "default size is 1,1,1");
        Expect(
            state.workingCopy.dynamicBoxes[0].massKg == world::kDefaultDynamicBoxMassKg,
            "default mass is 30 kg");
        Expect(
            state.workingCopy.dynamicBoxes[0].center.x == cameraAnchor.x
                && state.workingCopy.dynamicBoxes[0].center.y == cameraAnchor.y,
            "Edit Add uses camera-region X/Y");
        Expect(
            state.workingCopy.dynamicBoxes[0].center.z == active.initialSpawnVisualCenter.z,
            "Edit Add uses spawn-lane Z");
        Expect(state.selection.kind == EditorObjectKind::DynamicBox, "new Dynamic Box is selected");
        Expect(state.selection.index == 0, "selection uses new working index 0");
        Expect(
            editor::MappedActiveIndex(state.structuralMap, EditorObjectKind::DynamicBox, 0)
                == editor::kNoStructuralIndex,
            "pending Add has no active counterpart");
        Expect(state.placementMode == editor::PlacementMode::None, "Edit Add does not enter palette mode");
        Expect(state.modified, "pending Add is Modified");
        Expect(
            state.lastApplyStatus == editor::LevelEditorApplyStatus::NotAttempted,
            "Edit Add does not Apply Preview / create Jolt bodies");

        const std::vector<editor::PendingAuthoringVisual> pending =
            editor::CollectPendingAuthoringVisuals(
                active, state.workingCopy, state.structuralMap, state.selection);
        Expect(
            editor::PendingAuthoringContains(pending, EditorObjectKind::DynamicBox, 0),
            "pending Add ghost exists");

        bool hierarchyHasGroup = false;
        for (const editor::HierarchyEntry& entry : editor::BuildHierarchyEntries(state.workingCopy))
        {
            if (entry.selection.kind == EditorObjectKind::DynamicBox && entry.selection.index == 0)
            {
                hierarchyHasGroup = std::strcmp(entry.group, "Dynamic Boxes") == 0;
            }
        }
        Expect(hierarchyHasGroup, "0 -> 1 shows Dynamic Boxes / Dynamic Box 0");
        Expect(
            editor::IsEditableSelection(state.selection),
            "Inspector path is enabled for Dynamic Box");
        Expect(
            editor::GetEditablePosition(state.workingCopy, state.selection) != nullptr,
            "Inspector Center is available");
        Expect(
            editor::GetEditableSize(state.workingCopy, state.selection) != nullptr,
            "Inspector Size is available");
        Expect(state.workingCopy.dynamicBoxes[0].massKg == 30.0f, "Inspector Mass shows 30");

        const std::size_t platformsBefore = active.elevatedPlatforms.size();
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                state,
                active,
                editor::EditAddMenuRequest(EditorObjectKind::ElevatedPlatform),
                true,
                cameraAnchor),
            "Edit > Add > Platform still works");
        Expect(
            state.workingCopy.elevatedPlatforms.size() == platformsBefore + 1,
            "Platform Add still appends");
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                state,
                active,
                editor::EditAddMenuRequest(EditorObjectKind::Checkpoint),
                true,
                cameraAnchor),
            "Edit > Add > Checkpoint still works");
        Expect(state.workingCopy.checkpoints.size() == active.checkpoints.size() + 1,
            "Checkpoint Add still appends");
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                state,
                active,
                editor::EditAddMenuRequest(EditorObjectKind::Hazard),
                true,
                cameraAnchor),
            "Edit > Add > Hazard still works");
        Expect(state.workingCopy.hazards.size() == active.hazards.size() + 1, "Hazard Add still appends");
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                state,
                active,
                editor::EditAddMenuRequest(EditorObjectKind::Collectible),
                true,
                cameraAnchor),
            "Edit > Add > Collectible still works");
        Expect(
            state.workingCopy.collectibles.size() == active.collectibles.size() + 1,
            "Collectible Add still appends");

        editor::LevelEditorState paletteState{};
        SeedEditor(paletteState, active);
        editor::ApplyPaletteCategoryClick(paletteState.placementMode, editor::PlacementMode::DynamicBox);
        Expect(
            paletteState.placementMode == editor::PlacementMode::DynamicBox,
            "Object Palette Dynamic Box still enters placement");
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                paletteState,
                active,
                editor::PlacementAddRequest(paletteState.placementMode),
                true,
                {3.0f, 1.0f, 0.0f},
                true),
            "palette confirm still uses world-center AddDynamicBoxAt");
        Expect(paletteState.workingCopy.dynamicBoxes.size() == 1, "palette confirm adds one box");
        Expect(
            paletteState.workingCopy.dynamicBoxes[0].center.x == 3.0f
                && paletteState.workingCopy.dynamicBoxes[0].center.z == 0.0f,
            "palette confirm does not snap to spawn.z lane");
        Expect(active.dynamicBoxes.empty(), "palette confirm does not mutate active");

        editor::LevelEditorState dupState{};
        SeedEditor(dupState, active);
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                dupState,
                active,
                editor::EditAddMenuRequest(EditorObjectKind::DynamicBox),
                true,
                cameraAnchor),
            "Add before Duplicate");
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                dupState, active, LevelEditorRequest::DuplicateSelected, true),
            "Duplicate Dynamic Box request");
        Expect(dupState.workingCopy.dynamicBoxes.size() == 2, "Duplicate appends Dynamic Box");
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                dupState, active, LevelEditorRequest::DeleteSelected, true),
            "Delete Dynamic Box request");
        Expect(dupState.workingCopy.dynamicBoxes.size() == 1, "Delete removes working Dynamic Box");

        world::LevelDefinition atLimit = active;
        atLimit.elevatedPlatforms.resize(
            static_cast<std::size_t>(physics::kMaxAuthoredPhysicsBodies),
            {{40.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        editor::LevelEditorState limitState{};
        SeedEditor(limitState, atLimit);
        Expect(
            !editor::CanIssueAuthoredLifecycleRequest(
                true,
                atLimit,
                {},
                false,
                editor::EditAddMenuRequest(EditorObjectKind::DynamicBox)),
            "Edit Add Dynamic Box disabled at shared capacity");
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                limitState,
                atLimit,
                editor::EditAddMenuRequest(EditorObjectKind::DynamicBox),
                true,
                cameraAnchor),
            "AtLimit still handled as lifecycle");
        Expect(limitState.workingCopy.dynamicBoxes.empty(), "AtLimit does not append a Dynamic Box");
        Expect(atLimit.dynamicBoxes.empty(), "AtLimit does not mutate active");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        Expect(active.pressurePlates.empty(), "fixture starts with zero Pressure Plates");
        Expect(
            HierarchyKindCount(active, EditorObjectKind::PressurePlate) == 0,
            "empty collection has no Hierarchy Pressure Plate rows");

        const LevelEditorRequest editAdd =
            editor::EditAddMenuRequest(EditorObjectKind::PressurePlate);
        Expect(editAdd == LevelEditorRequest::AddPressurePlate, "menu action is AddPressurePlate");
        Expect(
            editor::CanIssueAuthoredLifecycleRequest(true, active, {}, false, editAdd),
            "Development can add Pressure Plate");
        Expect(
            !editor::CanIssueAuthoredLifecycleRequest(false, active, {}, false, editAdd),
            "Debug authoring cannot add Pressure Plate");

        const core::Vec3 cameraAnchor{12.0f, 5.0f, -6.0f};
        Expect(
            editor::HandleAuthoredLifecycleRequest(state, active, editAdd, true, cameraAnchor),
            "Edit > Add > Pressure Plate reaches workingCopy mutation");
        Expect(state.workingCopy.pressurePlates.size() == 1, "workingCopy gains exactly one Pressure Plate");
        Expect(active.pressurePlates.empty(), "Edit Add does not mutate active");
        Expect(
            state.workingCopy.pressurePlates[0].size.x == world::kDefaultPressurePlateSize.x
                && state.workingCopy.pressurePlates[0].size.y == world::kDefaultPressurePlateSize.y
                && state.workingCopy.pressurePlates[0].size.z == world::kDefaultPressurePlateSize.z,
            "default size is 2,0.2,2");
        Expect(
            state.workingCopy.pressurePlates[0].activateByDynamicBox
                && !state.workingCopy.pressurePlates[0].activateByPlayer
                && state.workingCopy.pressurePlates[0].visibleInGameplay,
            "Add Pressure Plate defaults box-only visible");
        Expect(
            state.workingCopy.pressurePlates[0].center.x == cameraAnchor.x
                && state.workingCopy.pressurePlates[0].center.y == cameraAnchor.y,
            "Edit Add uses camera-region X/Y");
        Expect(
            state.workingCopy.pressurePlates[0].center.z == active.initialSpawnVisualCenter.z,
            "Edit Add uses spawn-lane Z");
        Expect(state.selection.kind == EditorObjectKind::PressurePlate, "new Pressure Plate is selected");
        Expect(state.selection.index == 0, "selection uses new working index 0");
        Expect(
            editor::MappedActiveIndex(state.structuralMap, EditorObjectKind::PressurePlate, 0)
                == editor::kNoStructuralIndex,
            "pending Add has no active counterpart");
        Expect(state.placementMode == editor::PlacementMode::None, "Edit Add does not enter palette mode");
        Expect(state.modified, "pending Add is Modified");
        Expect(
            state.lastApplyStatus == editor::LevelEditorApplyStatus::NotAttempted,
            "Edit Add does not Apply Preview");
        Expect(
            editor::GetEditablePosition(state.workingCopy, state.selection) != nullptr,
            "Inspector Position is available");
        Expect(
            editor::GetEditableSize(state.workingCopy, state.selection) != nullptr,
            "Inspector Size is available");
        Expect(
            !editor::IsScaleSelection(state.selection),
            "Pressure Plate does not use Static Prop Scale");

        editor::LevelEditorState paletteState{};
        SeedEditor(paletteState, active);
        editor::ApplyPaletteCategoryClick(paletteState.placementMode, editor::PlacementMode::PressurePlate);
        Expect(
            paletteState.placementMode == editor::PlacementMode::PressurePlate,
            "Object Palette Pressure Plate enters placement");
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                paletteState,
                active,
                editor::PlacementAddRequest(paletteState.placementMode),
                true,
                {3.0f, 0.1f, 0.0f},
                true),
            "palette confirm uses world-center AddPressurePlateAt");
        Expect(paletteState.workingCopy.pressurePlates.size() == 1, "palette confirm adds one plate");
        Expect(
            paletteState.workingCopy.pressurePlates[0].center.x == 3.0f
                && paletteState.workingCopy.pressurePlates[0].center.z == 0.0f,
            "palette confirm does not snap to spawn.z lane");
        Expect(active.pressurePlates.empty(), "palette confirm does not mutate active");
        Expect(paletteState.workingCopy.staticProps.empty(), "palette plate does not add Static Props");

        editor::LevelEditorState dupState{};
        SeedEditor(dupState, active);
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                dupState,
                active,
                editor::EditAddMenuRequest(EditorObjectKind::PressurePlate),
                true,
                cameraAnchor),
            "Add before Duplicate");
        dupState.workingCopy.pressurePlates[0].activateByDynamicBox = false;
        dupState.workingCopy.pressurePlates[0].activateByPlayer = true;
        dupState.workingCopy.pressurePlates[0].visibleInGameplay = false;
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                dupState, active, LevelEditorRequest::DuplicateSelected, true),
            "Duplicate Pressure Plate request");
        Expect(dupState.workingCopy.pressurePlates.size() == 2, "Duplicate appends Pressure Plate");
        Expect(
            dupState.workingCopy.pressurePlates[1].size.y == world::kDefaultPressurePlateSize.y,
            "Duplicate preserves size");
        Expect(
            !dupState.workingCopy.pressurePlates[1].activateByDynamicBox
                && dupState.workingCopy.pressurePlates[1].activateByPlayer
                && !dupState.workingCopy.pressurePlates[1].visibleInGameplay,
            "Duplicate preserves Pressure Plate mode flags");
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                dupState, active, LevelEditorRequest::DeleteSelected, true),
            "Delete Pressure Plate request");
        Expect(dupState.workingCopy.pressurePlates.size() == 1, "Delete removes working Pressure Plate");

        world::LevelDefinition atLimit = active;
        atLimit.elevatedPlatforms.resize(
            static_cast<std::size_t>(physics::kMaxAuthoredPhysicsBodies),
            {{40.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        editor::LevelEditorState limitState{};
        SeedEditor(limitState, atLimit);
        Expect(
            editor::CanIssueAuthoredLifecycleRequest(
                true,
                atLimit,
                {},
                false,
                editor::EditAddMenuRequest(EditorObjectKind::PressurePlate)),
            "Edit Add Pressure Plate remains enabled at physics leftover capacity");
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                limitState,
                atLimit,
                editor::EditAddMenuRequest(EditorObjectKind::PressurePlate),
                true,
                cameraAnchor),
            "AtLimit still adds a Pressure Plate");
        Expect(limitState.workingCopy.pressurePlates.size() == 1, "AtLimit appends a Pressure Plate");
        Expect(atLimit.pressurePlates.empty(), "AtLimit does not mutate active");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        Expect(active.doors.empty(), "fixture starts with zero Doors");
        Expect(
            HierarchyKindCount(active, EditorObjectKind::Door) == 0,
            "empty collection has no Hierarchy Door rows");

        const LevelEditorRequest editAdd = editor::EditAddMenuRequest(EditorObjectKind::Door);
        Expect(editAdd == LevelEditorRequest::AddDoor, "menu action is AddDoor");
        Expect(
            editor::CanIssueAuthoredLifecycleRequest(true, active, {}, false, editAdd),
            "Development can add Door");
        Expect(
            !editor::CanIssueAuthoredLifecycleRequest(false, active, {}, false, editAdd),
            "Debug authoring cannot add Door");

        const core::Vec3 cameraAnchor{12.0f, 5.0f, -6.0f};
        Expect(
            editor::HandleAuthoredLifecycleRequest(state, active, editAdd, true, cameraAnchor),
            "Edit > Add > Door reaches workingCopy mutation");
        Expect(state.workingCopy.doors.size() == 1, "workingCopy gains exactly one Door");
        Expect(active.doors.empty(), "Edit Add does not mutate active");
        Expect(
            state.workingCopy.doors[0].size.x == world::kDefaultDoorSize.x
                && state.workingCopy.doors[0].openDistance == world::kDefaultDoorOpenDistance,
            "default Door size and openDistance");
        Expect(
            state.workingCopy.doors[0].requiredItemId.empty(),
            "Add Door defaults no required item");
        Expect(state.selection.kind == EditorObjectKind::Door, "new Door is selected");
        Expect(
            HierarchyKindCount(state.workingCopy, EditorObjectKind::Door) == 1,
            "Hierarchy lists the pending Door");
        Expect(
            !editor::IsScaleSelection(state.selection),
            "Door does not use Static Prop Scale");
        Expect(editor::IsResizeSelection(state.selection), "Door uses primitive Resize");

        editor::LevelEditorState palState{};
        SeedEditor(palState, active);
        palState.placementMode = editor::PlacementMode::Door;
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                palState,
                active,
                editor::PlacementAddRequest(editor::PlacementMode::Door),
                true,
                {3.0f, 1.5f, 1.0f}),
            "palette confirm uses world-center AddDoorAt");
        Expect(palState.workingCopy.doors.size() == 1, "palette confirm adds one Door");
        Expect(palState.workingCopy.doors[0].center.x == 3.0f, "palette confirm X");

        editor::LevelEditorState dupState{};
        SeedEditor(dupState, active);
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                dupState, active, editor::EditAddMenuRequest(EditorObjectKind::Door), true, cameraAnchor),
            "seed Door for Duplicate");
        dupState.workingCopy.doors[0].requiredItemId = "card";
        dupState.selection = {EditorObjectKind::Door, 0};
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                dupState, active, LevelEditorRequest::DuplicateSelected, true),
            "Duplicate Door request");
        Expect(dupState.workingCopy.doors.size() == 2, "Duplicate appends Door");
        Expect(
            dupState.workingCopy.doors[1].requiredItemId == "card",
            "Duplicate preserves requiredItemId");
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                dupState, active, LevelEditorRequest::DeleteSelected, true),
            "Delete Door request");
        Expect(dupState.workingCopy.doors.size() == 1, "Delete removes working Door");

        world::LevelDefinition atLimit = active;
        atLimit.elevatedPlatforms.resize(
            static_cast<std::size_t>(physics::kMaxAuthoredPhysicsBodies),
            {{40.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        editor::LevelEditorState limitState{};
        SeedEditor(limitState, atLimit);
        Expect(
            !editor::CanIssueAuthoredLifecycleRequest(
                true,
                atLimit,
                {},
                false,
                editor::EditAddMenuRequest(EditorObjectKind::Door)),
            "Edit Add Door disables at physics leftover capacity");
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                limitState,
                atLimit,
                editor::EditAddMenuRequest(EditorObjectKind::Door),
                true,
                cameraAnchor),
            "AtLimit Door request is still handled");
        Expect(limitState.workingCopy.doors.empty(), "AtLimit does not append a Door");
        Expect(atLimit.doors.empty(), "AtLimit does not mutate active");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        Expect(active.itemPickups.empty(), "fixture starts with zero Item Pickups");
        Expect(
            HierarchyKindCount(active, EditorObjectKind::ItemPickup) == 0,
            "empty collection has no Hierarchy Item Pickup rows");

        const LevelEditorRequest editAdd =
            editor::EditAddMenuRequest(EditorObjectKind::ItemPickup);
        Expect(editAdd == LevelEditorRequest::AddItemPickup, "menu action is AddItemPickup");
        Expect(
            editor::CanIssueAuthoredLifecycleRequest(true, active, {}, false, editAdd),
            "Development can add Item Pickup");
        Expect(
            !editor::CanIssueAuthoredLifecycleRequest(false, active, {}, false, editAdd),
            "Debug authoring cannot add Item Pickup");

        const core::Vec3 cameraAnchor{12.0f, 5.0f, -6.0f};
        Expect(
            editor::HandleAuthoredLifecycleRequest(state, active, editAdd, true, cameraAnchor),
            "Edit > Add > Item Pickup reaches workingCopy mutation");
        Expect(state.workingCopy.itemPickups.size() == 1, "workingCopy gains exactly one Item Pickup");
        Expect(active.itemPickups.empty(), "Edit Add does not mutate active");
        Expect(
            state.workingCopy.itemPickups[0].itemId == world::kDefaultItemPickupId
                && state.workingCopy.itemPickups[0].quantity == world::kDefaultItemPickupQuantity
                && state.workingCopy.itemPickups[0].modelIdentity.empty()
                && state.workingCopy.itemPickups[0].visualOffset.y == 0.0f
                && state.workingCopy.itemPickups[0].visualScale.x == 1.0f
                && state.workingCopy.itemPickups[0].showInteractionBounds
                && state.workingCopy.itemPickups[0].targetHighlightIntensity
                    == world::kDefaultItemPickupTargetHighlightIntensity,
            "default Item Pickup id/quantity/no model/neutral visual");
        Expect(state.selection.kind == EditorObjectKind::ItemPickup, "new Item Pickup is selected");
        Expect(
            HierarchyKindCount(state.workingCopy, EditorObjectKind::ItemPickup) == 1,
            "Hierarchy lists the pending Item Pickup");
        Expect(
            editor::IsScaleSelection(state.selection),
            "Item Pickup uses Scale for visualScale");
        Expect(
            editor::IsRotateSelection(state.selection),
            "Item Pickup uses Rotate for visualRotationDegrees");
        Expect(
            editor::GetEditableScale(state.workingCopy, state.selection)
                == &state.workingCopy.itemPickups[0].visualScale,
            "Scale gizmo edits visualScale");
        Expect(
            editor::GetEditableRotation(state.workingCopy, state.selection)
                == &state.workingCopy.itemPickups[0].visualRotationDegrees,
            "Rotate gizmo edits visualRotationDegrees");
        Expect(
            editor::GetEditablePosition(state.workingCopy, state.selection)
                == &state.workingCopy.itemPickups[0].position,
            "Translate still edits gameplay position");
        Expect(
            !editor::IsResizeSelection(state.selection),
            "Item Pickup does not use Resize");

        editor::LevelEditorState palState{};
        SeedEditor(palState, active);
        palState.placementMode = editor::PlacementMode::ItemPickup;
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                palState,
                active,
                editor::PlacementAddRequest(editor::PlacementMode::ItemPickup),
                true,
                {3.0f, 1.5f, 1.0f},
                true),
            "palette confirm uses world-center AddItemPickupAt");
        Expect(palState.workingCopy.itemPickups.size() == 1, "palette confirm adds one Item Pickup");
        Expect(palState.workingCopy.itemPickups[0].position.x == 3.0f, "palette confirm X");

        editor::LevelEditorState dupState{};
        SeedEditor(dupState, active);
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                dupState,
                active,
                editor::EditAddMenuRequest(EditorObjectKind::ItemPickup),
                true,
                cameraAnchor),
            "seed Item Pickup for Duplicate");
        dupState.workingCopy.itemPickups[0].itemId = "coin";
        dupState.workingCopy.itemPickups[0].quantity = 3;
        dupState.workingCopy.itemPickups[0].modelIdentity = "models/test_static.glb";
        dupState.workingCopy.itemPickups[0].visualOffset = {0.0f, 0.5f, 0.0f};
        dupState.workingCopy.itemPickups[0].visualRotationDegrees = {0.0f, 90.0f, 0.0f};
        dupState.workingCopy.itemPickups[0].visualScale = {0.15f, 0.2f, 0.25f};
        dupState.workingCopy.itemPickups[0].showInteractionBounds = false;
        dupState.workingCopy.itemPickups[0].targetHighlightIntensity = 0.25f;
        dupState.selection = {EditorObjectKind::ItemPickup, 0};
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                dupState, active, LevelEditorRequest::DuplicateSelected, true),
            "Duplicate Item Pickup request");
        Expect(dupState.workingCopy.itemPickups.size() == 2, "Duplicate appends Item Pickup");
        Expect(
            dupState.workingCopy.itemPickups[1].itemId == "coin"
                && dupState.workingCopy.itemPickups[1].quantity == 3
                && dupState.workingCopy.itemPickups[1].modelIdentity
                    == dupState.workingCopy.itemPickups[0].modelIdentity
                && dupState.workingCopy.itemPickups[1].visualOffset.y == 0.5f
                && dupState.workingCopy.itemPickups[1].visualRotationDegrees.y == 90.0f
                && dupState.workingCopy.itemPickups[1].visualScale.x == 0.15f
                && !dupState.workingCopy.itemPickups[1].showInteractionBounds
                && dupState.workingCopy.itemPickups[1].targetHighlightIntensity == 0.25f,
            "Duplicate preserves itemId/quantity/model/visual transform");
        Expect(
            dupState.workingCopy.itemPickups[1].position.x
                == dupState.workingCopy.itemPickups[0].position.x + editor::kLifecycleDuplicateOffsetX,
            "Duplicate offsets +1 X");
        const core::Vec3 gameplayBefore = dupState.workingCopy.itemPickups[0].position;
        dupState.workingCopy.itemPickups[0].visualOffset.x = 1.25f;
        editor::RefreshLevelEditorDerivedFlags(dupState, active);
        Expect(dupState.modified, "visualOffset edit marks Modified");
        Expect(
            dupState.workingCopy.itemPickups[0].position.x == gameplayBefore.x
                && dupState.workingCopy.itemPickups[0].position.y == gameplayBefore.y
                && dupState.workingCopy.itemPickups[0].position.z == gameplayBefore.z,
            "visualOffset edit does not move gameplay position");
        const core::Vec3 scaleBeforePos = dupState.workingCopy.itemPickups[0].position;
        dupState.workingCopy.itemPickups[0].visualScale = {0.5f, 0.5f, 0.5f};
        editor::RefreshLevelEditorDerivedFlags(dupState, active);
        Expect(
            dupState.workingCopy.itemPickups[0].position.x == scaleBeforePos.x,
            "visualScale edit does not move gameplay position");
        world::LevelDefinition appliedVisual = dupState.workingCopy;
        Expect(
            world::AuthoredLevelDataEqual(appliedVisual, dupState.workingCopy),
            "Apply promotes visual fields with workingCopy");
        editor::LevelEditorState revertVisual{};
        SeedEditor(revertVisual, appliedVisual);
        revertVisual.workingCopy.itemPickups[0].visualOffset = {9.0f, 9.0f, 9.0f};
        editor::RefreshLevelEditorDerivedFlags(revertVisual, appliedVisual);
        Expect(revertVisual.modified, "visual edit vs applied is Modified");
        revertVisual.workingCopy = appliedVisual;
        editor::RefreshLevelEditorDerivedFlags(revertVisual, appliedVisual);
        Expect(
            world::AuthoredLevelDataEqual(revertVisual.workingCopy, appliedVisual),
            "Revert restores visual fields");
        Expect(!revertVisual.modified, "Revert after visual edit clears Modified");
        Expect(
            !revertVisual.workingCopy.itemPickups[0].showInteractionBounds,
            "applied pickup currently has bounds disabled");
        revertVisual.workingCopy.itemPickups[0].showInteractionBounds = true;
        editor::RefreshLevelEditorDerivedFlags(revertVisual, appliedVisual);
        Expect(revertVisual.modified, "showInteractionBounds edit marks Modified");
        Expect(
            !world::AuthoredLevelDataEqual(revertVisual.workingCopy, appliedVisual),
            "equality detects showInteractionBounds change");
        revertVisual.workingCopy = appliedVisual;
        editor::RefreshLevelEditorDerivedFlags(revertVisual, appliedVisual);
        Expect(
            !revertVisual.workingCopy.itemPickups[0].showInteractionBounds,
            "Revert restores showInteractionBounds");
        Expect(!revertVisual.modified, "Revert after bounds edit clears Modified");
        world::LevelDefinition appliedBounds = revertVisual.workingCopy;
        appliedBounds.itemPickups[0].showInteractionBounds = true;
        Expect(
            world::AuthoredLevelDataEqual(appliedBounds, appliedBounds),
            "Apply promotes showInteractionBounds with workingCopy");
        Expect(
            appliedBounds.itemPickups[0].showInteractionBounds,
            "promoted showInteractionBounds is true");
        Expect(
            revertVisual.workingCopy.itemPickups[0].targetHighlightIntensity == 0.25f,
            "applied pickup currently has authored intensity 0.25");
        revertVisual.workingCopy.itemPickups[0].targetHighlightIntensity = 1.0f;
        editor::RefreshLevelEditorDerivedFlags(revertVisual, appliedVisual);
        Expect(revertVisual.modified, "targetHighlightIntensity edit marks Modified");
        Expect(
            !world::AuthoredLevelDataEqual(revertVisual.workingCopy, appliedVisual),
            "equality detects targetHighlightIntensity change");
        revertVisual.workingCopy = appliedVisual;
        editor::RefreshLevelEditorDerivedFlags(revertVisual, appliedVisual);
        Expect(
            revertVisual.workingCopy.itemPickups[0].targetHighlightIntensity == 0.25f,
            "Revert restores targetHighlightIntensity");
        Expect(!revertVisual.modified, "Revert after intensity edit clears Modified");
        world::LevelDefinition appliedIntensity = revertVisual.workingCopy;
        appliedIntensity.itemPickups[0].targetHighlightIntensity = 0.90f;
        Expect(
            world::AuthoredLevelDataEqual(appliedIntensity, appliedIntensity),
            "Apply promotes targetHighlightIntensity with workingCopy");
        Expect(
            appliedIntensity.itemPickups[0].targetHighlightIntensity == 0.90f,
            "promoted targetHighlightIntensity is 0.90");
        const core::Vec3 keptOffset = dupState.workingCopy.itemPickups[0].visualOffset;
        const core::Vec3 keptRotation = dupState.workingCopy.itemPickups[0].visualRotationDegrees;
        const core::Vec3 keptScale = dupState.workingCopy.itemPickups[0].visualScale;
        dupState.workingCopy.itemPickups[0].modelIdentity = "models/test_authored.glb";
        Expect(
            dupState.workingCopy.itemPickups[0].visualOffset.x == keptOffset.x
                && dupState.workingCopy.itemPickups[0].visualRotationDegrees.y == keptRotation.y
                && dupState.workingCopy.itemPickups[0].visualScale.x == keptScale.x,
            "Content Browser assignment preserves visual transform");
        dupState.workingCopy.itemPickups[0].modelIdentity.clear();
        Expect(
            dupState.workingCopy.itemPickups[0].visualOffset.x == keptOffset.x
                && dupState.workingCopy.itemPickups[0].visualScale.z == keptScale.z,
            "Clear Model preserves visual transform");
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                dupState, active, LevelEditorRequest::DeleteSelected, true),
            "Delete Item Pickup request");
        Expect(dupState.workingCopy.itemPickups.size() == 1, "Delete removes working Item Pickup");

        world::LevelDefinition atPhysicsLimit = active;
        atPhysicsLimit.elevatedPlatforms.resize(
            static_cast<std::size_t>(physics::kMaxAuthoredPhysicsBodies),
            {{40.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        editor::LevelEditorState physicsLimitState{};
        SeedEditor(physicsLimitState, atPhysicsLimit);
        Expect(
            editor::CanIssueAuthoredLifecycleRequest(
                true,
                atPhysicsLimit,
                {},
                false,
                editor::EditAddMenuRequest(EditorObjectKind::ItemPickup)),
            "Item Pickup does not consume the Jolt leftover");
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                physicsLimitState,
                atPhysicsLimit,
                editor::EditAddMenuRequest(EditorObjectKind::ItemPickup),
                true,
                cameraAnchor),
            "Add Item Pickup at platform body cap");
        Expect(
            physicsLimitState.workingCopy.itemPickups.size() == 1,
            "Item Pickup still adds when leftover is full");
        Expect(
            editor::AuthoredLevelsProtectStaticPropIdentity(
                physicsLimitState.workingCopy, active, active, "models/test_static.glb")
                == false,
            "empty model does not protect a missing identity");
        physicsLimitState.workingCopy.itemPickups[0].modelIdentity = "models/test_static.glb";
        Expect(
            editor::AuthoredLevelsProtectStaticPropIdentity(
                physicsLimitState.workingCopy, active, active, "models/test_static.glb"),
            "workingCopy Item Pickup model protects Delete Asset");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        const std::string kIdentity = "models/test_static.glb";
        state.contentBrowser.selectedIdentity = kIdentity;
        Expect(active.staticProps.empty(), "fixture starts with zero Static Props");
        Expect(
            HierarchyKindCount(active, EditorObjectKind::StaticProp) == 0,
            "empty collection has no Hierarchy Static Prop rows");
        const LevelEditorRequest editAdd =
            editor::EditAddMenuRequest(EditorObjectKind::StaticProp);
        const LevelEditorRequest browserAdd = editor::ContentBrowserAddStaticPropRequest();
        Expect(editAdd == LevelEditorRequest::AddStaticProp, "menu action is AddStaticProp");
        Expect(browserAdd == editAdd, "button and menu emit the same request");
        Expect(
            !editor::CanIssueAuthoredLifecycleRequest(true, active, {}, false, editAdd),
            "Add Static Prop disabled without Content Browser identity");
        Expect(
            !editor::CanIssueAuthoredLifecycleRequest(true, active, {}, false, browserAdd),
            "Content Browser Add Static Prop disabled without selection");
        Expect(
            editor::CanIssueAuthoredLifecycleRequest(
                true, active, {}, false, editAdd, kIdentity),
            "Development can add Static Prop with valid identity");
        Expect(
            editor::CanIssueAuthoredLifecycleRequest(
                true, active, {}, false, browserAdd, kIdentity),
            "valid Content Browser selection enables Add Static Prop");
        Expect(
            !editor::CanIssueAuthoredLifecycleRequest(
                false, active, {}, false, editAdd, kIdentity),
            "Debug authoring cannot add Static Prop");
        Expect(
            editor::AddStaticPropDisableReason(true, active, false, "") != nullptr,
            "empty identity has a disable reason");
        Expect(
            std::string(editor::AddStaticPropDisableReason(true, active, false, ""))
                == "Select a static model in the Content Browser first.",
            "disabled copy asks for a Content Browser selection");
        Expect(
            editor::AddStaticPropDisableReason(true, active, false, kIdentity) == nullptr,
            "valid identity has no disable reason");
        Expect(
            editor::PlacementModeFromKind(EditorObjectKind::StaticProp) == editor::PlacementMode::None,
            "Static Prop has no Object Palette placement mode");
        // Correction 2: the Edit menu row states the Content Browser asset it
        // would consume, so the command stops reading as unavailable.
        Expect(
            editor::AddStaticPropMenuHint("") == "select asset",
            "Edit menu row names the missing Content Browser selection");
        Expect(
            editor::AddStaticPropMenuHint("barrel.glb") == "select asset",
            "non-canonical identity is not advertised as usable");
        Expect(
            editor::AddStaticPropMenuHint(kIdentity) == "test_static.glb",
            "Edit menu row names the selected asset file");
        Expect(
            (editor::AddStaticPropMenuHint(kIdentity) != "select asset")
                == editor::CanIssueAuthoredLifecycleRequest(
                    true, active, {}, false, editAdd, kIdentity),
            "menu row copy agrees with actual command availability");

        const core::Vec3 cameraAnchor{12.0f, 5.0f, -6.0f};
        editor::LevelEditorState buttonState{};
        SeedEditor(buttonState, active);
        buttonState.contentBrowser.selectedIdentity = kIdentity;
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                buttonState, active, browserAdd, true, cameraAnchor),
            "Content Browser Add Static Prop reaches workingCopy mutation");
        Expect(
            buttonState.workingCopy.staticProps.size() == 1,
            "button creates exactly one Static Prop");
        Expect(active.staticProps.empty(), "button Add does not mutate active");
        Expect(
            buttonState.workingCopy.staticProps[0].modelIdentity == kIdentity,
            "button uses the selected Content Browser identity");
        Expect(
            buttonState.contentBrowser.selectedIdentity == kIdentity,
            "button preserves Content Browser selection");
        Expect(
            buttonState.workingCopy.staticProps[0].position.x == cameraAnchor.x
                && buttonState.workingCopy.staticProps[0].position.y == cameraAnchor.y
                && buttonState.workingCopy.staticProps[0].position.z
                    == active.initialSpawnVisualCenter.z,
            "button uses the existing M49 default position");
        Expect(
            buttonState.workingCopy.staticProps[0].rotationDegrees.x
                    == world::kDefaultStaticPropRotationDegrees.x
                && buttonState.workingCopy.staticProps[0].rotationDegrees.y
                    == world::kDefaultStaticPropRotationDegrees.y
                && buttonState.workingCopy.staticProps[0].rotationDegrees.z
                    == world::kDefaultStaticPropRotationDegrees.z
                && buttonState.workingCopy.staticProps[0].scale.x == world::kDefaultStaticPropScale.x
                && buttonState.workingCopy.staticProps[0].scale.y == world::kDefaultStaticPropScale.y
                && buttonState.workingCopy.staticProps[0].scale.z == world::kDefaultStaticPropScale.z,
            "button uses complete default rotation and scale");
        Expect(
            buttonState.selection.kind == EditorObjectKind::StaticProp
                && buttonState.selection.index == 0,
            "button selects the new authored Static Prop");
        Expect(buttonState.placementMode == editor::PlacementMode::None, "button does not enter placement");
        Expect(!editor::PlacementModeIsActive(buttonState.placementMode), "no placement state after button add");
        Expect(buttonState.modified, "button Add is Modified");
        Expect(
            editor::AuthoredLevelsProtectStaticPropIdentity(
                buttonState.workingCopy,
                active,
                buttonState.savedSourceBaseline,
                kIdentity),
            "direct-add immediately protects Delete Asset");
        Expect(
            !world::LevelReferencesStaticPropIdentity(active, kIdentity),
            "active remains unreferenced until Apply");

        Expect(
            editor::HandleAuthoredLifecycleRequest(state, active, editAdd, true, cameraAnchor),
            "Edit > Add > Static Prop reaches workingCopy mutation");
        Expect(state.workingCopy.staticProps.size() == 1, "workingCopy gains exactly one Static Prop");
        Expect(active.staticProps.empty(), "Edit Add does not mutate active");
        Expect(
            state.workingCopy.staticProps[0].modelIdentity == kIdentity,
            "Add uses Content Browser identity");
        Expect(
            state.contentBrowser.selectedIdentity == kIdentity,
            "Add does not clear Content Browser selection");
        Expect(
            state.workingCopy.staticProps[0].position.x == cameraAnchor.x
                && state.workingCopy.staticProps[0].position.y == cameraAnchor.y,
            "Edit Add uses camera-region X/Y");
        Expect(
            state.workingCopy.staticProps[0].position.z == active.initialSpawnVisualCenter.z,
            "Edit Add uses spawn-lane Z");
        Expect(
            state.workingCopy.staticProps[0].rotationDegrees.x == 0.0f
                && state.workingCopy.staticProps[0].scale.x == 1.0f,
            "default rotation 0 and scale 1");
        Expect(state.selection.kind == EditorObjectKind::StaticProp, "new Static Prop is selected");
        Expect(state.placementMode == editor::PlacementMode::None, "Edit Add does not enter palette mode");
        Expect(state.modified, "pending Add is Modified");
        Expect(
            editor::MappedActiveIndex(state.structuralMap, EditorObjectKind::StaticProp, 0)
                == editor::kNoStructuralIndex,
            "pending Add has no active counterpart");

        bool hierarchyHasGroup = false;
        for (const editor::HierarchyEntry& entry : editor::BuildHierarchyEntries(state.workingCopy))
        {
            if (entry.selection.kind == EditorObjectKind::StaticProp && entry.selection.index == 0)
            {
                hierarchyHasGroup = std::strcmp(entry.group, "Static Props") == 0;
            }
        }
        Expect(hierarchyHasGroup, "0 -> 1 shows Static Props / Static Prop 0");
        Expect(editor::IsEditableSelection(state.selection), "Static Prop is Inspector-editable");
        Expect(
            editor::GetEditablePosition(state.workingCopy, state.selection) != nullptr,
            "Translate gizmo can edit Static Prop position");
        Expect(
            editor::GetEditableSize(state.workingCopy, state.selection) == nullptr,
            "primitive Resize is not Static Prop Scale");
        Expect(!editor::IsResizeSelection(state.selection), "Static Prop is not a Resize selection");
        Expect(editor::IsScaleSelection(state.selection), "Static Prop supports Scale tool");
        Expect(editor::IsRotateSelection(state.selection), "Static Prop supports Rotate tool");
        Expect(
            editor::GetEditableScale(state.workingCopy, state.selection) != nullptr,
            "Scale gizmo edits Static Prop scale");
        Expect(
            editor::GetEditableRotation(state.workingCopy, state.selection)
                == &state.workingCopy.staticProps[0].rotationDegrees,
            "Rotate gizmo edits Static Prop rotation");
        Expect(
            editor::GetEditableScale(state.workingCopy, {EditorObjectKind::ElevatedPlatform, 0})
                == nullptr,
            "Platform is not routed through Static Prop Scale");

        state.workingCopy.staticProps[0].rotationDegrees = {10.0f, 20.0f, 30.0f};
        state.workingCopy.staticProps[0].scale = {2.0f, 0.5f, 3.0f};
        editor::RefreshLevelEditorDerivedFlags(state, active);
        Expect(state.modified, "Inspector rotation/scale edits keep Modified");
        Expect(
            state.workingCopy.staticProps[0].rotationDegrees.y == 20.0f
                && state.workingCopy.staticProps[0].scale.z == 3.0f,
            "Inspector transform fields are workingCopy");
        Expect(active.staticProps.empty(), "Inspector edits do not mutate active");

        editor::LevelEditorState isolated{};
        SeedEditor(isolated, active);
        isolated.contentBrowser.selectedIdentity = kIdentity;
        isolated.selection = {EditorObjectKind::StaticProp, 0};
        Expect(
            isolated.contentBrowser.selectedIdentity == kIdentity,
            "scene selection assignment does not consume Content Browser identity");
        isolated.contentBrowser.selectedIdentity = "models/test_authored.glb";
        Expect(
            isolated.selection.kind == EditorObjectKind::StaticProp,
            "Content Browser selection does not change scene selection");

        world::LevelDefinition invalidApply = state.workingCopy;
        invalidApply.staticProps[0].scale.x = 0.0f;
        Expect(
            !world::StaticPropSpecIsValid(invalidApply.staticProps[0]),
            "zero scale is not Apply-valid");
        Expect(
            world::StaticPropSpecIsValid(state.workingCopy.staticProps[0]),
            "valid workingCopy props can Apply");

        editor::LevelEditorState revertState{};
        SeedEditor(revertState, active);
        revertState.contentBrowser.selectedIdentity = kIdentity;
        editor::HandleAuthoredLifecycleRequest(
            revertState, active, editAdd, true, cameraAnchor);
        Expect(revertState.modified, "pending Static Prop Add is Modified");
        revertState.workingCopy = active;
        editor::ClearCategoryStructuralPending(revertState.structuralPending);
        editor::ResetStructuralIndexMap(revertState.structuralMap, active);
        revertState.selection = editor::ReconcileSelection(revertState.workingCopy, revertState.selection);
        editor::RefreshLevelEditorDerivedFlags(revertState, active);
        Expect(world::AuthoredLevelDataEqual(revertState.workingCopy, active), "Revert after Add restores");
        Expect(!revertState.modified, "Revert after Add clears Modified");
        Expect(revertState.workingCopy.staticProps.empty(), "Revert drops pending Static Prop");
        Expect(
            revertState.contentBrowser.selectedIdentity == kIdentity,
            "Revert does not hijack Content Browser selection");

        editor::LevelEditorState dupState{};
        SeedEditor(dupState, active);
        dupState.contentBrowser.selectedIdentity = kIdentity;
        Expect(
            editor::HandleAuthoredLifecycleRequest(dupState, active, editAdd, true, cameraAnchor),
            "Add before Duplicate Static Prop");
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                dupState, active, LevelEditorRequest::DuplicateSelected, true),
            "Duplicate Static Prop request");
        Expect(dupState.workingCopy.staticProps.size() == 2, "Duplicate appends Static Prop");
        Expect(
            dupState.workingCopy.staticProps[1].modelIdentity
                == dupState.workingCopy.staticProps[0].modelIdentity,
            "Duplicate keeps the same asset reference");
        dupState.workingCopy.staticProps[0].scale = {2.0f, 1.0f, 1.0f};
        dupState.workingCopy.staticProps[1].scale = {1.0f, 3.0f, 1.0f};
        Expect(
            dupState.workingCopy.staticProps[0].scale.x == 2.0f
                && dupState.workingCopy.staticProps[1].scale.y == 3.0f,
            "Duplicate instances keep independent Scale");
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                dupState, active, LevelEditorRequest::DeleteSelected, true),
            "Delete Static Prop request");
        Expect(dupState.workingCopy.staticProps.size() == 1, "Delete removes working Static Prop");
        Expect(
            dupState.contentBrowser.selectedIdentity == kIdentity,
            "Delete instance does not clear Content Browser identity");

        editor::LevelEditorState missing{};
        SeedEditor(missing, active);
        Expect(
            editor::HandleAuthoredLifecycleRequest(missing, active, editAdd, true, cameraAnchor),
            "missing identity still handled as lifecycle");
        Expect(missing.workingCopy.staticProps.empty(), "missing identity does not append");
        Expect(
            missing.lastMessage
                == std::string(
                    "Add Static Prop requires a valid Content Browser static model selection."),
            "missing identity diagnostic");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d authored lifecycle integration test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Authored lifecycle integration tests passed.\n");
    return 0;
}
