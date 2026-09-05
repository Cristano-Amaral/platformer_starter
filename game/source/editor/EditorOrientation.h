#pragma once

// Milestone 35: screen-space world-axis orientation widget.
// Not the world translation/resize gizmo. Not serialized. Not gameplay camera.

#include "core/Vec3.h"
#include "editor/EditorCamera.h"

namespace editor
{
enum class CanonicalEditorView
{
    None,
    Right, // from +X, looking -X
    Left,  // from -X, looking +X
    Top,   // from +Y, looking -Y
    Front, // from +Z, looking -Z (yaw 0)
    Back,  // from -Z, looking +Z
};

struct OrientationWidgetAxes
{
    // Widget-space XY: +x right, +y up. Z is camera-forward depth (unused for hit).
    core::Vec3 x{};
    core::Vec3 y{};
    core::Vec3 z{};
};

struct OrientationWidgetLayout
{
    float originX = 872.0f;
    float originY = 100.0f;
    float radius = 36.0f;
    float hitRadius = 12.0f;
};

// Viewport overlay, not ImGui layout. Matches the default Inspector column
// width math (340 / 28% / 240) without tracking the live Inspector window.
inline constexpr float kOrientationWidgetRadius = 36.0f;
inline constexpr float kOrientationWidgetHitRadius = 12.0f;
inline constexpr float kOrientationWidgetTopMargin = 64.0f;
inline constexpr float kOrientationWidgetInspectorGap = 24.0f;
inline constexpr float kOrientationWidgetEdgeMargin = 8.0f;
inline constexpr float kOrientationWidgetDefaultPanelWidth = 340.0f;
inline constexpr float kOrientationWidgetMinPanelWidth = 240.0f;
inline constexpr float kOrientationWidgetPanelWidthFraction = 0.28f;
inline constexpr float kOrientationWidgetNarrowShiftY = 84.0f;

// Upper-right of the unobstructed 3D view: left of the default Inspector
// column, below the top chrome. Not persisted. Not bound to live ImGui pose.
OrientationWidgetLayout MakeOrientationWidgetLayout(float viewportWidth, float viewportHeight);

OrientationWidgetAxes ProjectOrientationWidgetAxes(const EditorCamera& camera);

// yaw/pitch only. Position, FOV, and movementSpeed are unchanged.
void ApplyCanonicalEditorView(EditorCamera& camera, CanonicalEditorView view);

CanonicalEditorView PickOrientationWidget(
    float mouseX,
    float mouseY,
    const OrientationWidgetLayout& layout,
    const OrientationWidgetAxes& axes);

const char* CanonicalEditorViewName(CanonicalEditorView view);
}
