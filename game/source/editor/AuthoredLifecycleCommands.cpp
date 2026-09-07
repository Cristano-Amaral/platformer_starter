#include "editor/AuthoredLifecycleCommands.h"

#include "editor/AuthoredObjectLifecycle.h"
#include "editor/EditorGizmo.h"

#include <string>

namespace editor
{
namespace
{
EditorObjectKind AddKindForRequest(LevelEditorRequest request)
{
    switch (request)
    {
    case LevelEditorRequest::AddPlatform:
        return EditorObjectKind::ElevatedPlatform;
    case LevelEditorRequest::AddCheckpoint:
        return EditorObjectKind::Checkpoint;
    case LevelEditorRequest::AddHazard:
        return EditorObjectKind::Hazard;
    case LevelEditorRequest::AddCollectible:
        return EditorObjectKind::Collectible;
    default:
        return EditorObjectKind::None;
    }
}

const char* LifecycleCategoryLabel(EditorObjectKind kind)
{
    switch (kind)
    {
    case EditorObjectKind::ElevatedPlatform:
        return "Platform";
    case EditorObjectKind::Checkpoint:
        return "Checkpoint";
    case EditorObjectKind::Hazard:
        return "Hazard";
    case EditorObjectKind::Collectible:
        return "Collectible";
    default:
        return "Object";
    }
}

const char* RejectionMessage(LifecycleEditStatus status, EditorObjectKind kind)
{
    switch (status)
    {
    case LifecycleEditStatus::InvalidSelection:
        return "Lifecycle blocked: selection is invalid.";
    case LifecycleEditStatus::UnsupportedType:
        return "Lifecycle blocked: selected object type is not supported.";
    case LifecycleEditStatus::AtLimit:
        switch (kind)
        {
        case EditorObjectKind::ElevatedPlatform:
            return "Physics body capacity reached.";
        case EditorObjectKind::Checkpoint:
        case EditorObjectKind::Hazard:
        case EditorObjectKind::Collectible:
            return "Level file record limit reached.";
        default:
            return "Technical capacity reached.";
        }
    case LifecycleEditStatus::ReferencedPlatform:
        return "Delete blocked: Platform is referenced by level support metadata.";
    case LifecycleEditStatus::MinimumCount:
        return "At least one Platform is required.";
    case LifecycleEditStatus::Success:
        break;
    }
    return "Lifecycle request rejected.";
}

LifecycleEditResult RunLifecycleMutation(
    world::LevelDefinition& workingCopy,
    EditorSelection selection,
    LevelEditorRequest request,
    core::Vec3 placementAnchor)
{
    switch (request)
    {
    case LevelEditorRequest::AddPlatform:
        return AddPlatform(workingCopy, placementAnchor);
    case LevelEditorRequest::AddCheckpoint:
        return AddCheckpoint(workingCopy, placementAnchor);
    case LevelEditorRequest::AddHazard:
        return AddHazard(workingCopy, placementAnchor);
    case LevelEditorRequest::AddCollectible:
        return AddCollectible(workingCopy, placementAnchor);
    case LevelEditorRequest::DuplicateSelected:
        return DuplicateSelected(workingCopy, selection);
    case LevelEditorRequest::DeleteSelected:
        return DeleteSelected(workingCopy, selection);
    default:
        break;
    }
    LifecycleEditResult result{};
    result.status = LifecycleEditStatus::UnsupportedType;
    result.selection = selection;
    return result;
}

void SetSuccessMessage(LevelEditorState& state, LevelEditorRequest request, EditorObjectKind kind)
{
    const char* label = LifecycleCategoryLabel(kind);
    switch (request)
    {
    case LevelEditorRequest::AddPlatform:
    case LevelEditorRequest::AddCheckpoint:
    case LevelEditorRequest::AddHazard:
    case LevelEditorRequest::AddCollectible:
        state.lastMessage = std::string(label) + " added.";
        return;
    case LevelEditorRequest::DuplicateSelected:
        state.lastMessage = std::string(label) + " duplicated.";
        return;
    case LevelEditorRequest::DeleteSelected:
        state.lastMessage = std::string(label) + " deleted.";
        return;
    default:
        state.lastMessage = "Lifecycle edit applied.";
        break;
    }
}
}

bool IsAuthoredLifecycleRequest(LevelEditorRequest request)
{
    switch (request)
    {
    case LevelEditorRequest::AddPlatform:
    case LevelEditorRequest::AddCheckpoint:
    case LevelEditorRequest::AddHazard:
    case LevelEditorRequest::AddCollectible:
    case LevelEditorRequest::DuplicateSelected:
    case LevelEditorRequest::DeleteSelected:
        return true;
    default:
        return false;
    }
}

bool CanIssueAuthoredLifecycleRequest(
    bool authoringAvailable,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection,
    bool gizmoDragging,
    LevelEditorRequest request)
{
    switch (request)
    {
    case LevelEditorRequest::AddPlatform:
    case LevelEditorRequest::AddCheckpoint:
    case LevelEditorRequest::AddHazard:
    case LevelEditorRequest::AddCollectible:
        return CanAddLifecycleObject(
            authoringAvailable, workingCopy, AddKindForRequest(request), gizmoDragging);
    case LevelEditorRequest::DuplicateSelected:
        return CanDuplicateSelected(authoringAvailable, workingCopy, selection, gizmoDragging);
    case LevelEditorRequest::DeleteSelected:
        return CanDeleteSelected(authoringAvailable, workingCopy, selection, gizmoDragging);
    default:
        return false;
    }
}

bool HandleAuthoredLifecycleRequest(
    LevelEditorState& state,
    const world::LevelDefinition& activeLevel,
    LevelEditorRequest request,
    bool authoringAvailable,
    core::Vec3 placementAnchor)
{
    if (!IsAuthoredLifecycleRequest(request))
    {
        return false;
    }

    const EditorSelection previousSelection = state.selection;
    const CategoryStructuralPending previousPending = state.structuralPending;
    const StructuralIndexMap previousMap = state.structuralMap;
    EditorObjectKind affectedKind = AddKindForRequest(request);
    if (affectedKind == EditorObjectKind::None)
    {
        affectedKind = previousSelection.kind;
    }

    if (!CanIssueAuthoredLifecycleRequest(
            authoringAvailable,
            state.workingCopy,
            state.selection,
            state.gizmo.dragging,
            request))
    {
        ResetLevelActionStatuses(state);
        if (!authoringAvailable)
        {
            state.lastMessage = "Lifecycle editing is available in Development only.";
        }
        else if (state.gizmo.dragging)
        {
            state.lastMessage = "Lifecycle blocked: finish the gizmo drag first.";
        }
        else if (
            request == LevelEditorRequest::DeleteSelected
            && previousSelection.kind == EditorObjectKind::ElevatedPlatform
            && state.workingCopy.elevatedPlatforms.size()
                <= static_cast<std::size_t>(world::kMinElevatedPlatformCount))
        {
            state.lastMessage = "At least one Platform is required.";
        }
        else if (
            request == LevelEditorRequest::DeleteSelected
            && previousSelection.kind == EditorObjectKind::ElevatedPlatform
            && IsAuthoredPlatformReferenced(state.workingCopy, previousSelection.index))
        {
            state.lastMessage =
                "Delete blocked: Platform is referenced by level support metadata.";
        }
        else if (
            request == LevelEditorRequest::DuplicateSelected
            && !SupportsLifecycle(previousSelection.kind))
        {
            state.lastMessage =
                RejectionMessage(LifecycleEditStatus::UnsupportedType, previousSelection.kind);
        }
        else if (CategoryAtCountLimit(state.workingCopy, affectedKind)
            && (request == LevelEditorRequest::AddPlatform
                || request == LevelEditorRequest::AddCheckpoint
                || request == LevelEditorRequest::AddHazard
                || request == LevelEditorRequest::AddCollectible
                || request == LevelEditorRequest::DuplicateSelected))
        {
            state.lastMessage = RejectionMessage(LifecycleEditStatus::AtLimit, affectedKind);
        }
        else
        {
            state.lastMessage =
                RejectionMessage(LifecycleEditStatus::InvalidSelection, previousSelection.kind);
        }
        RefreshLevelEditorDerivedFlags(state, activeLevel);
        return true;
    }

    EnsureStructuralIndexMap(state.structuralMap, activeLevel);
    const LifecycleEditResult result =
        RunLifecycleMutation(state.workingCopy, previousSelection, request, placementAnchor);
    if (!result.succeeded)
    {
        state.selection = previousSelection;
        state.structuralPending = previousPending;
        state.structuralMap = previousMap;
        ResetLevelActionStatuses(state);
        state.lastMessage = RejectionMessage(result.status, affectedKind);
        RefreshLevelEditorDerivedFlags(state, activeLevel);
        return true;
    }

    state.selection = result.selection;
    MarkCategoryStructuralPending(state.structuralPending, affectedKind);
    ApplyLifecycleToStructuralMap(
        state.structuralMap,
        affectedKind,
        request == LevelEditorRequest::DeleteSelected,
        previousSelection.index);
    ClearGizmoInteraction(state.gizmo);
    ResetLevelActionStatuses(state);
    SetSuccessMessage(state, request, affectedKind);
    RefreshLevelEditorDerivedFlags(state, activeLevel);
    return true;
}
}
