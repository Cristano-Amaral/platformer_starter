#include "editor/AuthoredLifecycleCommands.h"

#include "editor/AuthoredObjectLifecycle.h"
#include "editor/EditorGizmo.h"
#include "editor/EditorSelectionSet.h"

#include <string>
#include <string_view>
#include <vector>

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
    case LevelEditorRequest::AddPressurePlate:
        return EditorObjectKind::PressurePlate;
    case LevelEditorRequest::AddDoor:
        return EditorObjectKind::Door;
    case LevelEditorRequest::AddItemPickup:
        return EditorObjectKind::ItemPickup;
    case LevelEditorRequest::AddGoal:
        return EditorObjectKind::Goal;
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
    case EditorObjectKind::PressurePlate:
        return "Pressure Plate";
    case EditorObjectKind::Door:
        return "Door";
    case EditorObjectKind::ItemPickup:
        return "Item Pickup";
    case EditorObjectKind::Goal:
        return "Level Goal";
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
        case EditorObjectKind::PressurePlate:
        case EditorObjectKind::StaticProp:
        case EditorObjectKind::ItemPickup:
        case EditorObjectKind::Goal:
            return "Level file record limit reached.";
        case EditorObjectKind::DynamicBox:
        case EditorObjectKind::Door:
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
    const std::vector<EditorSelection>& additionalSelections,
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
    case LevelEditorRequest::AddPressurePlate:
        return worldCenterPlacement ? AddPressurePlateAt(workingCopy, placementAnchor)
                                    : AddPressurePlate(workingCopy, placementAnchor);
    case LevelEditorRequest::AddDoor:
        return worldCenterPlacement ? AddDoorAt(workingCopy, placementAnchor)
                                    : AddDoor(workingCopy, placementAnchor);
    case LevelEditorRequest::AddItemPickup:
        return worldCenterPlacement ? AddItemPickupAt(workingCopy, placementAnchor)
                                    : AddItemPickup(workingCopy, placementAnchor);
    case LevelEditorRequest::AddGoal:
        return worldCenterPlacement ? AddGoalAt(workingCopy, placementAnchor)
                                    : AddGoal(workingCopy, placementAnchor);
    case LevelEditorRequest::AddStaticProp:
        return worldCenterPlacement
            ? AddStaticPropAt(workingCopy, placementAnchor, staticPropIdentity)
            : AddStaticProp(workingCopy, placementAnchor, staticPropIdentity);
    case LevelEditorRequest::DuplicateSelected:
        return DuplicateSelectionSet(workingCopy, selection, additionalSelections);
    case LevelEditorRequest::DeleteSelected:
        return DeleteSelectionSet(workingCopy, selection, additionalSelections);
    default:
        break;
    }
    LifecycleEditResult result{};
    result.status = LifecycleEditStatus::UnsupportedType;
    result.selection = selection;
    return result;
}

void SetSuccessMessage(
    LevelEditorState& state,
    LevelEditorRequest request,
    EditorObjectKind kind,
    bool multiSelected)
{
    if (multiSelected)
    {
        if (request == LevelEditorRequest::DuplicateSelected)
        {
            state.lastMessage = "Selection duplicated.";
            return;
        }
        if (request == LevelEditorRequest::DeleteSelected)
        {
            state.lastMessage = "Selection deleted.";
            return;
        }
    }
    const char* label = LifecycleCategoryLabel(kind);
    switch (request)
    {
    case LevelEditorRequest::AddPlatform:
    case LevelEditorRequest::AddCheckpoint:
    case LevelEditorRequest::AddHazard:
    case LevelEditorRequest::AddCollectible:
    case LevelEditorRequest::AddDynamicBox:
    case LevelEditorRequest::AddPressurePlate:
    case LevelEditorRequest::AddDoor:
    case LevelEditorRequest::AddItemPickup:
    case LevelEditorRequest::AddGoal:
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
    case LevelEditorRequest::AddPressurePlate:
    case LevelEditorRequest::AddDoor:
    case LevelEditorRequest::AddItemPickup:
    case LevelEditorRequest::AddGoal:
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
    case EditorObjectKind::PressurePlate:
        return LevelEditorRequest::AddPressurePlate;
    case EditorObjectKind::Door:
        return LevelEditorRequest::AddDoor;
    case EditorObjectKind::ItemPickup:
        return LevelEditorRequest::AddItemPickup;
    case EditorObjectKind::Goal:
        return LevelEditorRequest::AddGoal;
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
    std::string_view staticPropIdentity,
    const std::vector<EditorSelection>& additionalSelections)
{
    switch (request)
    {
    case LevelEditorRequest::AddPlatform:
    case LevelEditorRequest::AddCheckpoint:
    case LevelEditorRequest::AddHazard:
    case LevelEditorRequest::AddCollectible:
    case LevelEditorRequest::AddDynamicBox:
    case LevelEditorRequest::AddPressurePlate:
        return CanAddLifecycleObject(
            authoringAvailable, workingCopy, AddKindForRequest(request), gizmoDragging);
    case LevelEditorRequest::AddDoor:
        return CanAddLifecycleObject(
            authoringAvailable, workingCopy, AddKindForRequest(request), gizmoDragging);
    case LevelEditorRequest::AddItemPickup:
        return CanAddLifecycleObject(
            authoringAvailable, workingCopy, AddKindForRequest(request), gizmoDragging);
    case LevelEditorRequest::AddGoal:
        return CanAddLifecycleObject(
            authoringAvailable, workingCopy, AddKindForRequest(request), gizmoDragging);
    case LevelEditorRequest::AddStaticProp:
        return CanAddLifecycleObject(
                   authoringAvailable, workingCopy, EditorObjectKind::StaticProp, gizmoDragging)
            && world::StaticPropIdentityIsValid(staticPropIdentity);
    case LevelEditorRequest::DuplicateSelected:
        return CanDuplicateSelected(
            authoringAvailable, workingCopy, selection, additionalSelections, gizmoDragging);
    case LevelEditorRequest::DeleteSelected:
        return CanDeleteSelected(
            authoringAvailable, workingCopy, selection, additionalSelections, gizmoDragging);
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
    const std::vector<EditorSelection> previousAdditional = state.additionalSelections;
    const CategoryStructuralPending previousPending = state.structuralPending;
    const StructuralIndexMap previousMap = state.structuralMap;
    const bool multiSelected =
        EditorSelectionSetIsMulti(state.selection, state.additionalSelections);
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
            staticPropIdentity,
            state.additionalSelections))
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
        else if (request == LevelEditorRequest::DuplicateSelected)
        {
            const char* reason = DuplicateSelectedDisableReason(
                authoringAvailable,
                state.workingCopy,
                previousSelection,
                state.gizmo.dragging,
                previousAdditional);
            state.lastMessage = reason != nullptr
                ? reason
                : RejectionMessage(LifecycleEditStatus::InvalidSelection, previousSelection.kind);
        }
        else if (request == LevelEditorRequest::DeleteSelected)
        {
            const char* reason = DeleteSelectedDisableReason(
                authoringAvailable,
                state.workingCopy,
                previousSelection,
                state.gizmo.dragging,
                previousAdditional);
            if (reason != nullptr
                && std::string_view(reason)
                    == "Platform is referenced by checkpoint/goal support metadata.")
            {
                state.lastMessage =
                    "Delete blocked: Platform is referenced by level support metadata.";
            }
            else
            {
                state.lastMessage = reason != nullptr
                    ? reason
                    : RejectionMessage(
                          LifecycleEditStatus::InvalidSelection, previousSelection.kind);
            }
        }
        else if (CategoryAtCountLimit(state.workingCopy, affectedKind)
            && (request == LevelEditorRequest::AddPlatform
                || request == LevelEditorRequest::AddCheckpoint
                || request == LevelEditorRequest::AddHazard
                || request == LevelEditorRequest::AddCollectible
                || request == LevelEditorRequest::AddDynamicBox
                || request == LevelEditorRequest::AddPressurePlate
                || request == LevelEditorRequest::AddDoor
                || request == LevelEditorRequest::AddItemPickup
                || request == LevelEditorRequest::AddGoal
                || request == LevelEditorRequest::AddStaticProp))
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
        previousAdditional,
        request,
        placementAnchor,
        worldCenterPlacement,
        staticPropIdentity);
    if (!result.succeeded)
    {
        state.selection = previousSelection;
        state.additionalSelections = previousAdditional;
        state.structuralPending = previousPending;
        state.structuralMap = previousMap;
        ResetLevelActionStatuses(state);
        state.lastMessage = RejectionMessage(result.status, affectedKind);
        RefreshLevelEditorDerivedFlags(state, activeLevel);
        return true;
    }

    state.selection = result.selection;
    state.additionalSelections = result.additionalSelections;
    SanitizeEditorSelectionSet(state.selection, state.additionalSelections);

    if (request == LevelEditorRequest::DeleteSelected)
    {
        const std::vector<EditorSelection>& deleted =
            result.appliedPlan.empty()
                ? std::vector<EditorSelection>{previousSelection}
                : result.appliedPlan;
        for (const EditorSelection& item : deleted)
        {
            MarkCategoryStructuralPending(state.structuralPending, item.kind);
            ApplyLifecycleToStructuralMap(state.structuralMap, item.kind, true, item.index);
        }
    }
    else if (!result.appliedPlan.empty())
    {
        for (const EditorSelection& item : result.appliedPlan)
        {
            MarkCategoryStructuralPending(state.structuralPending, item.kind);
        }
    }
    else
    {
        MarkCategoryStructuralPending(state.structuralPending, affectedKind);
        ApplyLifecycleToStructuralMap(state.structuralMap, affectedKind, false, 0);
    }

    ClearGizmoInteraction(state.gizmo);
    ResetLevelActionStatuses(state);
    SetSuccessMessage(state, request, affectedKind, multiSelected);
    RefreshLevelEditorDerivedFlags(state, activeLevel);
    return true;
}
}
