#pragma once

// Milestone 34: world-space translation gizmo math, state, and live tick.
// No rotation, scale, snapping, or generic transform framework.

#include "core/Vec3.h"
#include "editor/EditorPicking.h"
#include "editor/EditorSelection.h"
#include "render/CameraView.h"
#include "world/LevelDefinition.h"

#include <cstddef>

namespace editor
{
enum class EditorAxis
{
    None,
    X,
    Y,
    Z,
};

struct GizmoInteractionState
{
    EditorAxis hovered = EditorAxis::None;
    EditorAxis active = EditorAxis::None;
    bool dragging = false;
    EditorSelection dragTarget{};
    core::Vec3 dragStartPosition{};
    float dragStartAxisParameter = 0.0f;
};

struct GizmoDrawRequest
{
    bool visible = false;
    core::Vec3 origin{};
    float axisLength = 1.0f;
    EditorAxis hovered = EditorAxis::None;
    EditorAxis active = EditorAxis::None;
};

struct EditorPendingTransformPreview
{
    bool visible = false;
    core::Vec3 center{};
    core::Vec3 size{};
};

// Visual shaft is thin; hit radius is larger so axes are clickable without
// swallowing the whole scene. Units are fractions of axisLength.
inline constexpr float kGizmoViewHeightFraction = 0.22f;
inline constexpr float kGizmoMinWorldLength = 0.75f;
inline constexpr float kGizmoMaxWorldLength = 24.0f;
inline constexpr float kGizmoVisualRadiusFraction = 0.03f;
inline constexpr float kGizmoHitRadiusFraction = 0.09f;
// Shared origin is not pickable so a ray through the hub cannot match all axes.
inline constexpr float kGizmoPickHubSkipFraction = 0.12f;
// |axis × viewForward| below this uses closest-points instead of a drag plane.
inline constexpr float kGizmoParallelEpsilon = 0.05f;

const char* EditorAxisName(EditorAxis axis);
core::Vec3 EditorAxisDirection(EditorAxis axis);

void ClearGizmoInteraction(GizmoInteractionState& state);

bool IsGizmoSelection(EditorSelection selection);

// Mutable authored position in workingCopy. Null for Camera and all M33
// read-only kinds. Does not allocate.
core::Vec3* GetEditablePosition(
    world::LevelDefinition& level,
    EditorSelection selection);
const core::Vec3* GetEditablePosition(
    const world::LevelDefinition& level,
    EditorSelection selection);

// Working-copy box used by the pending preview (spawn uses kPlayerVisualSize).
bool GetGizmoPreviewBox(
    const world::LevelDefinition& workingCopy,
    EditorSelection selection,
    core::Vec3& center,
    core::Vec3& size);

float GizmoWorldLength(const render::CameraView& view, core::Vec3 origin);
float GizmoVisualRadius(float axisLength);
float GizmoHitRadius(float axisLength);

GizmoDrawRequest MakeGizmoDrawRequest(
    EditorSelection selection,
    const world::LevelDefinition& workingCopy,
    const render::CameraView& view,
    const GizmoInteractionState& interaction);

bool AuthoredGeometryDiffers(
    const world::LevelDefinition& active,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection);

EditorPendingTransformPreview MakePendingTransformPreview(
    EditorSelection selection,
    const world::LevelDefinition& active,
    const world::LevelDefinition& workingCopy);

EditorAxis PickGizmoHandle(
    Ray3 ray,
    core::Vec3 origin,
    float axisLength,
    float hitRadius);

bool BeginGizmoDrag(
    GizmoInteractionState& state,
    EditorSelection selection,
    EditorAxis axis,
    core::Vec3 workingPosition,
    Ray3 mouseRay,
    const render::CameraView& view);

// Absolute position from drag start + axis delta. Does not accumulate
// frame-to-frame. Returns start position if the ray is unusable.
core::Vec3 GizmoDragPosition(
    const GizmoInteractionState& state,
    Ray3 mouseRay,
    const render::CameraView& view);

void EndGizmoDrag(GizmoInteractionState& state);

// Live per-frame interaction. Uses the tested pick/drag helpers. Returns true
// when this frame's LMB must not also world-pick (active drag, drag start, or
// drag end).
bool UpdateGizmoInteraction(
    GizmoInteractionState& state,
    EditorSelection currentSelection,
    world::LevelDefinition& workingCopy,
    const render::CameraView& view,
    Ray3 mouseRay,
    bool imguiWantsMouse,
    bool lookHeld,
    bool selectPressed,
    bool selectHeld,
    bool selectReleased);

bool IsFiniteVec3(core::Vec3 value);
}
