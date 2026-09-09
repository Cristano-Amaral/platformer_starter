#pragma once

// Milestone 42: Object Palette placement mode and candidate resolution.
// Transient editor state only. Does not mutate workingCopy until confirm
// reuses M41 Add. No tool framework, GUIDs, or physics raycasts.

#include "core/Vec3.h"
#include "editor/AuthoredObjectLifecycle.h"
#include "editor/EditorPicking.h"
#include "editor/EditorSelection.h"
#include "world/LevelDefinition.h"

namespace editor
{
enum class LevelEditorRequest;

enum class PlacementMode
{
    None,
    Platform,
    Checkpoint,
    Hazard,
    Collectible,
    DynamicBox,
    PressurePlate,
    Door,
};

// Transient, not authored, not Level Format.
enum class PlacementCandidateSource
{
    None,
    SurfaceHit,
    CameraFallback,
};

enum class PlacementPreviewStyle
{
    None,
    Surface,
    Fallback,
};

struct PlacementSurfaceHit
{
    bool hit = false;
    float distance = 0.0f;
    core::Vec3 point{};
};

struct PlacementCandidate
{
    bool visible = false;
    PlacementCandidateSource source = PlacementCandidateSource::None;
    PlacementMode mode = PlacementMode::None;
    EditorObjectKind kind = EditorObjectKind::None;
    core::Vec3 center{};
    core::Vec3 size{};
    world::CheckpointSpec checkpoint{};
    world::HazardSpec hazard{};
    world::CollectibleSpec collectible{};
};

inline bool PlacementModeIsActive(PlacementMode mode)
{
    return mode != PlacementMode::None;
}

inline void ClearPlacementMode(PlacementMode& mode)
{
    mode = PlacementMode::None;
}

inline void SetPlacementMode(PlacementMode& mode, PlacementMode requested)
{
    mode = requested;
}

// Palette entries are tool toggles. Same category again returns to None.
inline void ApplyPaletteCategoryClick(PlacementMode& mode, PlacementMode clicked)
{
    if (clicked == PlacementMode::None)
    {
        return;
    }
    if (mode == clicked)
    {
        mode = PlacementMode::None;
        return;
    }
    mode = clicked;
}

inline bool PaletteCategoryIsActive(PlacementMode mode, PlacementMode category)
{
    return PlacementModeIsActive(mode) && mode == category;
}

inline void ClearPlacementPointerBlock(bool& blocked)
{
    blocked = false;
}

inline void CancelPlacementSession(PlacementMode& mode, bool& pointerBlocked)
{
    mode = PlacementMode::None;
    pointerBlocked = false;
}

inline PlacementPreviewStyle StyleForCandidateSource(PlacementCandidateSource source)
{
    switch (source)
    {
    case PlacementCandidateSource::SurfaceHit:
        return PlacementPreviewStyle::Surface;
    case PlacementCandidateSource::CameraFallback:
        return PlacementPreviewStyle::Fallback;
    default:
        return PlacementPreviewStyle::None;
    }
}

inline bool PlacementCandidateFromSurface(const PlacementCandidate& candidate)
{
    return candidate.source == PlacementCandidateSource::SurfaceHit;
}

const char* PlacementModeName(PlacementMode mode);
EditorObjectKind KindFromPlacementMode(PlacementMode mode);
PlacementMode PlacementModeFromKind(EditorObjectKind kind);
LevelEditorRequest PlacementAddRequest(PlacementMode mode);

core::Vec3 DefaultPlacementSize(PlacementMode mode);
core::Vec3 DefaultPlacementOffset(PlacementMode mode);

bool IsEligiblePlacementSurface(EditorObjectKind kind);
PlacementSurfaceHit FindPlacementSurfaceHit(Ray3 ray, const EditorPickingSet& activeSet);

core::Vec3 MakePlacementFallbackCenter(core::Vec3 cameraAnchor);
core::Vec3 MakePlacedObjectCenter(
    PlacementMode mode,
    core::Vec3 contactOrFallback,
    bool sitOnSurface);

PlacementCandidate MakePlacementCandidate(PlacementMode mode, core::Vec3 worldCenter);
PlacementCandidate ResolvePlacementCandidate(
    PlacementMode mode,
    Ray3 ray,
    const EditorPickingSet& activeSet,
    core::Vec3 fallbackAnchor);

const char* PlacementStopHintText();
const char* PlacementViewportActionHintText();

bool PlacementInteractionClaimsPointer(
    bool mouseCaptured,
    bool lookHeld,
    bool widgetConsumedPointer,
    bool gizmoConsumedPointer,
    bool gizmoDragging,
    bool gizmoHoveredOnPress);

void UpdatePlacementPointerBlock(
    bool& blocked,
    bool selectPressed,
    bool selectHeld,
    bool selectReleased,
    bool claimed);

bool ShouldConfirmPlacement(
    PlacementMode mode,
    bool selectPressed,
    bool mouseCaptured,
    bool lookHeld,
    bool widgetConsumedPointer,
    bool gizmoConsumedPointer,
    bool pointerBlocked,
    bool canAdd);

bool ShouldCancelPlacementMode(
    PlacementMode mode,
    bool escapePressed,
    bool imguiWantsKeyboard);
}
