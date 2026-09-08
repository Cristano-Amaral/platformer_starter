#include "editor/AuthoredLifecycleCommands.h"
#include "editor/AuthoredObjectLifecycle.h"
#include "editor/EditorGizmo.h"
#include "editor/EditorHierarchy.h"
#include "editor/EditorPicking.h"
#include "editor/EditorPlacement.h"
#include "editor/EditorSelection.h"
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
        editor::IsAuthoredLifecycleRequest(editor::EditAddMenuRequest(EditorObjectKind::DynamicBox)),
        "Edit Add Dynamic Box is owner-dispatched lifecycle");
    Expect(
        editor::PlacementAddRequest(editor::PlacementMode::DynamicBox)
            == LevelEditorRequest::AddDynamicBox,
        "Object Palette Dynamic Box still confirms AddDynamicBox");

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

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d authored lifecycle integration test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Authored lifecycle integration tests passed.\n");
    return 0;
}
