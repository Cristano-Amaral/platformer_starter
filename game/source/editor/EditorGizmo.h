#pragma once

// Milestone 34 translation gizmo + Milestone 35 resize math/live handles.
// No rotation, generic Transform scale, snapping, or transform framework.

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

enum class EditorTransformMode
{
    Translate,
    Resize,
};

struct ResizeHandlePick
{
    EditorAxis axis = EditorAxis::None;
    // +1 = positive-extent cube, -1 = negative-extent cube. Both edit total size.
    int sign = 1;
};

struct GizmoInteractionState
{
    EditorAxis hovered = EditorAxis::None;
    EditorAxis active = EditorAxis::None;
    bool dragging = false;
    EditorSelection dragTarget{};
    core::Vec3 dragStartPosition{};
    float dragStartAxisParameter = 0.0f;
    // Resize-only. Translation ignores these.
    core::Vec3 dragStartSize{};
    int dragHandleSign = 1;
    int hoveredSign = 1;
};

struct GizmoDrawRequest
{
    bool visible = false;
    core::Vec3 origin{};
    float axisLength = 1.0f;
    EditorAxis hovered = EditorAxis::None;
    EditorAxis active = EditorAxis::None;
    // Resize-only.  +1 / -1 identifies the cube, not the whole axis.
    int hoveredSign = 1;
    int activeSign = 1;
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
// Center-preserving: 1 world unit of +/- handle travel changes total size by 2.
inline constexpr float kResizeSizeFromAxisDelta = 2.0f;
// Jolt BoxShape default convex radius is 0.05, so half-extent must stay above
// that. 0.12 full-size is still smaller than any Level 01 authored box (0.4+).
inline constexpr float kMinAuthoredBoxExtent = 0.12f;
inline constexpr float kResizeHandleHitFraction = 0.10f;

const char* EditorAxisName(EditorAxis axis);
core::Vec3 EditorAxisDirection(EditorAxis axis);

void ClearGizmoInteraction(GizmoInteractionState& state);

bool IsGizmoSelection(EditorSelection selection);
bool IsResizeSelection(EditorSelection selection);

// Mutable authored position in workingCopy. Null for Camera and all M33
// read-only kinds. Does not allocate.
core::Vec3* GetEditablePosition(
    world::LevelDefinition& level,
    EditorSelection selection);
const core::Vec3* GetEditablePosition(
    const world::LevelDefinition& level,
    EditorSelection selection);

// Mutable authored box size. Ground and Elevated Platform 0..5 only.
core::Vec3* GetEditableSize(
    world::LevelDefinition& level,
    EditorSelection selection);
const core::Vec3* GetEditableSize(
    const world::LevelDefinition& level,
    EditorSelection selection);

float ClampAuthoredBoxExtent(float value);
core::Vec3 ClampAuthoredBoxSize(core::Vec3 size);

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

GizmoDrawRequest MakeResizeGizmoDrawRequest(
    EditorSelection selection,
    const world::LevelDefinition& workingCopy,
    const render::CameraView& view,
    const GizmoInteractionState& interaction);

ResizeHandlePick PickResizeHandle(
    Ray3 ray,
    core::Vec3 origin,
    float axisLength,
    float hitRadius);

bool BeginResizeDrag(
    GizmoInteractionState& state,
    EditorSelection selection,
    EditorAxis axis,
    int handleSign,
    core::Vec3 workingPosition,
    core::Vec3 workingSize,
    Ray3 mouseRay,
    const render::CameraView& view);

// Center-preserving. Only the active axis component changes.
core::Vec3 GizmoResizeSize(
    const GizmoInteractionState& state,
    Ray3 mouseRay,
    const render::CameraView& view);

// Live resize interaction. Returns true when this frame's LMB must not also
// world-pick (active drag, drag start, or drag end).
bool UpdateResizeInteraction(
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
