#pragma once

// Milestone 58.2: Development editor selection ghost for model-backed
// Static Props and Item Pickups. Visual feedback only: the workingCopy
// authored transform is the sole authority. Not a selection renderer,
// Transform component, or second preview transform.

#include "editor/EditorSelection.h"
#include "editor/StaticPropTransform.h"
#include "world/ItemPickup.h"
#include "world/LevelDefinition.h"
#include "world/StaticProp.h"

namespace editor
{
struct SelectedModelGhostRequest
{
    bool visible = false;
    bool demotePrimaryBox = false;
    bool drawOrientedBounds = false;
    world::StaticPropSpec visual{};
    core::Vec3 localMin = kStaticPropDefaultLocalMin;
    core::Vec3 localMax = kStaticPropDefaultLocalMax;
};

inline bool IsModelBackedSelection(
    EditorSelection selection,
    const world::LevelDefinition& workingCopy)
{
    if (selection.kind == EditorObjectKind::StaticProp
        && selection.index < workingCopy.staticProps.size())
    {
        return true;
    }
    if (selection.kind == EditorObjectKind::ItemPickup
        && selection.index < workingCopy.itemPickups.size())
    {
        return !workingCopy.itemPickups[selection.index].modelIdentity.empty();
    }
    return false;
}

// Ghost transform is copied from workingCopy. Callers must not invent a
// previewPosition / previewRotation / previewScale authority.
inline SelectedModelGhostRequest MakeSelectedModelGhostRequest(
    EditorSelection workingSelection,
    const world::LevelDefinition& workingCopy)
{
    SelectedModelGhostRequest request{};
    if (workingSelection.kind == EditorObjectKind::StaticProp
        && workingSelection.index < workingCopy.staticProps.size())
    {
        const world::StaticPropSpec& prop = workingCopy.staticProps[workingSelection.index];
        if (!world::StaticPropTransformIsValid(prop))
        {
            return request;
        }
        request.visible = true;
        request.demotePrimaryBox = true;
        request.drawOrientedBounds = true;
        request.visual = prop;
        return request;
    }
    if (workingSelection.kind == EditorObjectKind::ItemPickup
        && workingSelection.index < workingCopy.itemPickups.size())
    {
        const world::ItemPickupSpec& pickup = workingCopy.itemPickups[workingSelection.index];
        if (pickup.modelIdentity.empty())
        {
            return request;
        }
        const world::StaticPropSpec visual = world::ItemPickupVisualProp(pickup);
        if (!world::StaticPropTransformIsValid(visual))
        {
            return request;
        }
        request.visible = true;
        request.demotePrimaryBox = true;
        request.drawOrientedBounds = true;
        request.visual = visual;
        return request;
    }
    return request;
}

inline void ApplyLoadedLocalBoundsToGhost(
    SelectedModelGhostRequest& request,
    bool haveLoadedBounds,
    core::Vec3 loadedMin,
    core::Vec3 loadedMax)
{
    if (!request.visible || !haveLoadedBounds)
    {
        return;
    }
    request.localMin = loadedMin;
    request.localMax = loadedMax;
}
}
