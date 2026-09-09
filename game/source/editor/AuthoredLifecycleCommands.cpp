#include "editor/AuthoredLifecycleCommands.h"

#include "editor/AuthoredObjectLifecycle.h"
#include "editor/EditorGizmo.h"

#include <string>
#include <string_view>

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
    case LevelEditorRequest::AddDynamicBox:
        return EditorObjectKind::DynamicBox;
    case LevelEditorRequest::AddStaticProp:
        return EditorObjectKind::StaticProp;
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
    case EditorObjectKind::DynamicBox:
        return "Dynamic Box";
    case EditorObjectKind::StaticProp:
        return "Static Prop";
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
        case EditorObjectKind::StaticProp:
            return "Level file record limit reached.";
        case EditorObjectKind::DynamicBox:
            return "Physics body capacity reached.";
        default:
            return "Technical capacity reached.";
        }
    case LifecycleEditStatus::ReferencedPlatform:
        return "Delete blocked: Platform is referenced by level support metadata.";
    case LifecycleEditStatus::MinimumCount:
        return "At least one Platform is required.";
    case LifecycleEditStatus::InvalidAssetReference:
        return "Add Static Prop requires a valid Content Browser static model selection.";
    case LifecycleEditStatus::Success:
        break;
    }
    return "Lifecycle request rejected.";
}

LifecycleEditResult RunLifecycleMutation(
    world::LevelDefinition& workingCopy,
    EditorSelection selection,
    LevelEditorRequest request,
    core::Vec3 placementAnchor,
    bool worldCenterPlacement,
    std::string_view staticPropIdentity)
{
    switch (request)
    {
    case LevelEditorRequest::AddPlatform:
        return worldCenterPlacement ? AddPlatformAt(workingCopy, placementAnchor)
                                    : AddPlatform(workingCopy, placementAnchor);
    case LevelEditorRequest::AddCheckpoint:
        return worldCenterPlacement ? AddCheckpointAt(workingCopy, placementAnchor)
                                    : AddCheckpoint(workingCopy, placementAnchor);
    case LevelEditorRequest::AddHazard:
        return worldCenterPlacement ? AddHazardAt(workingCopy, placementAnchor)
                                    : AddHazard(workingCopy, placementAnchor);
    case LevelEditorRequest::AddCollectible:
        return worldCenterPlacement ? AddCollectibleAt(workingCopy, placementAnchor)
                                    : AddCollectible(workingCopy, placementAnchor);
    case LevelEditorRequest::AddDynamicBox:
        return worldCenterPlacement ? AddDynamicBoxAt(workingCopy, placementAnchor)
                                    : AddDynamicBox(workingCopy, placementAnchor);
    case LevelEditorRequest::AddStaticProp:
        return worldCenterPlacement
            ? AddStaticPropAt(workingCopy, placementAnchor, staticPropIdentity)
            : AddStaticProp(workingCopy, placementAnchor, staticPropIdentity);
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
    case LevelEditorRequest::AddDynamicBox:
    case LevelEditorRequest::AddStaticProp:
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
    case LevelEditorRequest::AddDynamicBox:
    case LevelEditorRequest::AddStaticProp:
    case LevelEditorRequest::DuplicateSelected:
    case LevelEditorRequest::DeleteSelected:
        return true;
    default:
        return false;
    }
}

LevelEditorRequest EditAddMenuRequest(EditorObjectKind kind)
{
    switch (kind)
    {
    case EditorObjectKind::ElevatedPlatform:
        return LevelEditorRequest::AddPlatform;
    case EditorObjectKind::Checkpoint:
        return LevelEditorRequest::AddCheckpoint;
    case EditorObjectKind::Hazard:
        return LevelEditorRequest::AddHazard;
    case EditorObjectKind::Collectible:
        return LevelEditorRequest::AddCollectible;
    case EditorObjectKind::DynamicBox:
        return LevelEditorRequest::AddDynamicBox;
    case EditorObjectKind::StaticProp:
        return LevelEditorRequest::AddStaticProp;
    default:
        return LevelEditorRequest::None;
    }
}

bool CanIssueAuthoredLifecycleRequest(
    bool authoringAvailable,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection,
    bool gizmoDragging,
    LevelEditorRequest request,
    std::string_view staticPropIdentity)
{
    switch (request)
    {
    case LevelEditorRequest::AddPlatform:
    case LevelEditorRequest::AddCheckpoint:
    case LevelEditorRequest::AddHazard:
    case LevelEditorRequest::AddCollectible:
    case LevelEditorRequest::AddDynamicBox:
        return CanAddLifecycleObject(
            authoringAvailable, workingCopy, AddKindForRequest(request), gizmoDragging);
    case LevelEditorRequest::AddStaticProp:
        return CanAddLifecycleObject(
                   authoringAvailable, workingCopy, EditorObjectKind::StaticProp, gizmoDragging)
            && world::StaticPropIdentityIsValid(staticPropIdentity);
    case LevelEditorRequest::DuplicateSelected:
        return CanDuplicateSelected(authoringAvailable, workingCopy, selection, gizmoDragging);
    case LevelEditorRequest::DeleteSelected:
        return CanDeleteSelected(authoringAvailable, workingCopy, selection, gizmoDragging);
    default:
        return false;
    }
}

std::string AddStaticPropMenuHint(std::string_view staticPropIdentity)
{
    if (!world::StaticPropIdentityIsValid(staticPropIdentity))
    {
        return std::string("select asset");
    }
    const std::size_t nameStart = staticPropIdentity.find_last_of('/');
    return std::string(
        nameStart == std::string_view::npos
            ? staticPropIdentity
            : staticPropIdentity.substr(nameStart + 1));
}

const char* AddStaticPropDisableReason(
    bool authoringAvailable,
    const world::LevelDefinition& workingCopy,
    bool gizmoDragging,
    std::string_view staticPropIdentity)
{
    if (CanIssueAuthoredLifecycleRequest(
            authoringAvailable,
            workingCopy,
            {},
            gizmoDragging,
            LevelEditorRequest::AddStaticProp,
            staticPropIdentity))
    {
        return nullptr;
    }
    if (!authoringAvailable)
    {
        return "Lifecycle editing is available in Development only.";
    }
    if (gizmoDragging)
    {
        return "Lifecycle blocked: finish the gizmo drag first.";
    }
    if (CategoryAtCountLimit(workingCopy, EditorObjectKind::StaticProp))
    {
        return RejectionMessage(LifecycleEditStatus::AtLimit, EditorObjectKind::StaticProp);
    }
    if (!world::StaticPropIdentityIsValid(staticPropIdentity))
    {
        return "Select a static model in the Content Browser first.";
    }
    return RejectionMessage(LifecycleEditStatus::InvalidAssetReference, EditorObjectKind::StaticProp);
}

bool HandleAuthoredLifecycleRequest(
    LevelEditorState& state,
    const world::LevelDefinition& activeLevel,
    LevelEditorRequest request,
    bool authoringAvailable,
    core::Vec3 placementAnchor,
    bool worldCenterPlacement,
    std::string_view staticPropIdentityOverride)
{
    if (!IsAuthoredLifecycleRequest(request))
    {
        return false;
    }

    const std::string_view staticPropIdentity = staticPropIdentityOverride.empty()
        ? std::string_view(state.contentBrowser.selectedIdentity)
        : staticPropIdentityOverride;

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
            request,
            staticPropIdentity))
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
                || request == LevelEditorRequest::AddDynamicBox
                || request == LevelEditorRequest::AddStaticProp
                || request == LevelEditorRequest::DuplicateSelected))
        {
            state.lastMessage = RejectionMessage(LifecycleEditStatus::AtLimit, affectedKind);
        }
        else if (request == LevelEditorRequest::AddStaticProp)
        {
            state.lastMessage =
                RejectionMessage(LifecycleEditStatus::InvalidAssetReference, affectedKind);
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
    const LifecycleEditResult result = RunLifecycleMutation(
        state.workingCopy,
        previousSelection,
        request,
        placementAnchor,
        worldCenterPlacement,
        staticPropIdentity);
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
