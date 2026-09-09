#pragma once

// Milestone 50: transient Static Prop viewport placement. Not Level Format,
// not a second prop type, and not Object Palette camera-fallback placement.
// Confirmation reuses M49 AddStaticPropAt.

#include "editor/EditorPlacement.h"
#include "editor/StaticPropTransform.h"
#include "world/StaticProp.h"

#include <string>
#include <string_view>

namespace editor
{
inline constexpr const char* kContentBrowserAddStaticPropLabel = "Add Static Prop";
inline constexpr const char* kContentBrowserPlaceStaticPropLabel = "Place Static Prop";

struct StaticPropPlacementState
{
    bool active = false;
    std::string modelIdentity;
};

struct StaticPropPlacementPreview
{
    bool valid = false;
    std::string modelIdentity;
    core::Vec3 position{};
    core::Vec3 rotationDegrees = world::kDefaultStaticPropRotationDegrees;
    core::Vec3 scale = world::kDefaultStaticPropScale;
};

inline bool StaticPropPlacementIsActive(const StaticPropPlacementState& state)
{
    return state.active && world::StaticPropIdentityIsValid(state.modelIdentity);
}

inline bool CanStartStaticPropPlacement(std::string_view identity)
{
    return world::StaticPropIdentityIsValid(identity);
}

inline void CancelStaticPropPlacement(StaticPropPlacementState& state)
{
    state.active = false;
    state.modelIdentity.clear();
}

inline void StartStaticPropPlacement(StaticPropPlacementState& state, std::string_view identity)
{
    if (!CanStartStaticPropPlacement(identity))
    {
        return;
    }
    state.active = true;
    state.modelIdentity = std::string(identity);
}

// Behavior A: a new valid Content Browser identity updates the pending asset.
// Empty/invalid selection does not mix identities; the consumed placement
// identity stays until cancel or another valid selection.
inline void SyncStaticPropPlacementIdentityFromBrowser(
    StaticPropPlacementState& state,
    std::string_view selectedIdentity)
{
    if (!state.active)
    {
        return;
    }
    if (world::StaticPropIdentityIsValid(selectedIdentity))
    {
        state.modelIdentity = std::string(selectedIdentity);
    }
}

inline void CancelAllEditorPlacement(
    PlacementMode& paletteMode,
    bool& pointerBlocked,
    StaticPropPlacementState& staticProp)
{
    CancelPlacementSession(paletteMode, pointerBlocked);
    CancelStaticPropPlacement(staticProp);
}

inline bool EditorViewportPlacementIsActive(
    PlacementMode paletteMode,
    const StaticPropPlacementState& staticProp)
{
    return PlacementModeIsActive(paletteMode) || StaticPropPlacementIsActive(staticProp);
}

enum class PlaceStaticPropClickResult
{
    Ignored,
    Started,
    UpdatedIdentity,
    Cancelled,
};

// Content Browser Place Static Prop. Same identity again toggles off (Esc
// equivalent). A different valid identity updates the pending asset.
inline PlaceStaticPropClickResult ApplyPlaceStaticPropClick(
    StaticPropPlacementState& placement,
    PlacementMode& paletteMode,
    bool& pointerBlocked,
    std::string_view identity)
{
    if (!CanStartStaticPropPlacement(identity))
    {
        return PlaceStaticPropClickResult::Ignored;
    }
    if (StaticPropPlacementIsActive(placement) && placement.modelIdentity == identity)
    {
        CancelStaticPropPlacement(placement);
        return PlaceStaticPropClickResult::Cancelled;
    }
    CancelPlacementSession(paletteMode, pointerBlocked);
    const bool alreadyActive = placement.active;
    StartStaticPropPlacement(placement, identity);
    return alreadyActive ? PlaceStaticPropClickResult::UpdatedIdentity
                         : PlaceStaticPropClickResult::Started;
}

inline core::Vec3 MakeStaticPropPlacementPosition(
    core::Vec3 surfaceHit,
    core::Vec3 localMin,
    core::Vec3 localMax)
{
    world::StaticPropSpec spec{};
    spec.position = surfaceHit;
    spec.rotationDegrees = world::kDefaultStaticPropRotationDegrees;
    spec.scale = world::kDefaultStaticPropScale;
    core::Vec3 center{};
    core::Vec3 size{};
    StaticPropWorldAabb(spec, localMin, localMax, center, size);
    spec.position.y += surfaceHit.y - (center.y - size.y * 0.5f);
    return spec.position;
}

inline world::StaticPropSpec MakeStaticPropSpecFromPreview(const StaticPropPlacementPreview& preview)
{
    world::StaticPropSpec spec{};
    spec.modelIdentity = preview.modelIdentity;
    spec.position = preview.position;
    spec.rotationDegrees = preview.rotationDegrees;
    spec.scale = preview.scale;
    return spec;
}

inline bool StaticPropPlacementPreviewShouldDraw(const StaticPropPlacementPreview& preview)
{
    return preview.valid && world::StaticPropIdentityIsValid(preview.modelIdentity);
}

StaticPropPlacementPreview ResolveStaticPropPlacementPreview(
    const StaticPropPlacementState& state,
    Ray3 ray,
    const EditorPickingSet& pickingSet,
    core::Vec3 localMin,
    core::Vec3 localMax);

bool ShouldConfirmStaticPropPlacement(
    const StaticPropPlacementState& state,
    bool previewValid,
    bool selectPressed,
    bool mouseCaptured,
    bool lookHeld,
    bool widgetConsumedPointer,
    bool gizmoConsumedPointer,
    bool pointerBlocked,
    bool canAdd);

bool ShouldCancelStaticPropPlacement(
    const StaticPropPlacementState& state,
    bool escapePressed,
    bool imguiWantsKeyboard);

const char* StaticPropPlacementHudName();
}