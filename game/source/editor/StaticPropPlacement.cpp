#include "editor/StaticPropPlacement.h"

namespace editor
{
StaticPropPlacementPreview ResolveStaticPropPlacementPreview(
    const StaticPropPlacementState& state,
    Ray3 ray,
    const EditorPickingSet& pickingSet,
    core::Vec3 localMin,
    core::Vec3 localMax)
{
    StaticPropPlacementPreview preview{};
    if (!StaticPropPlacementIsActive(state))
    {
        return preview;
    }

    const PlacementSurfaceHit surface = FindPlacementSurfaceHit(ray, pickingSet);
    if (!surface.hit)
    {
        preview.modelIdentity = state.modelIdentity;
        return preview;
    }

    preview.valid = true;
    preview.modelIdentity = state.modelIdentity;
    preview.rotationDegrees = world::kDefaultStaticPropRotationDegrees;
    preview.scale = world::kDefaultStaticPropScale;
    preview.position = MakeStaticPropPlacementPosition(surface.point, localMin, localMax);
    return preview;
}

bool ShouldConfirmStaticPropPlacement(
    const StaticPropPlacementState& state,
    bool previewValid,
    bool selectPressed,
    bool mouseCaptured,
    bool lookHeld,
    bool widgetConsumedPointer,
    bool gizmoConsumedPointer,
    bool pointerBlocked,
    bool canAdd)
{
    return StaticPropPlacementIsActive(state) && previewValid && selectPressed && !mouseCaptured
        && !lookHeld && !widgetConsumedPointer && !gizmoConsumedPointer && !pointerBlocked
        && canAdd;
}

bool ShouldCancelStaticPropPlacement(
    const StaticPropPlacementState& state,
    bool escapePressed,
    bool imguiWantsKeyboard)
{
    return StaticPropPlacementIsActive(state) && escapePressed && !imguiWantsKeyboard;
}

const char* StaticPropPlacementHudName()
{
    return "Static Prop";
}
}