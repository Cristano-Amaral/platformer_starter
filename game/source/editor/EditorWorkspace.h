#pragma once

// Milestone 36 workspace visibility plus Milestone 43 Quick Toolbar chrome.
// showToolOutput owns the Development Tool Output window. showContentBrowser
// owns the Development Content Browser. showModelPreview owns the Development
// Model Preview. showQuickToolbar is Development authoring chrome, not a
// floating panel. Debug keeps the M36 editor without Build, without the Quick
// Toolbar, without the Content Browser, and without Model Preview.

#include "editor/EditorGizmo.h"

namespace editor
{
struct EditorWorkspaceState
{
    bool showMetrics = true;
    bool showHierarchy = true;
    bool showInspector = true;
    bool showLevelEditor = true;
    bool showToolOutput = true;
    bool showObjectPalette = true;
    bool showContentBrowser = true;
    bool showModelPreview = true;
    bool showQuickToolbar = true;
};

inline constexpr float kEditorMainMenuBarNominalHeight = 24.0f;
inline constexpr float kEditorQuickToolbarNominalHeight = 28.0f;
inline constexpr float kOrientationWidgetMenuBarGap = 8.0f;

// Shared editor content/viewport geometry. Window origin is top-left.
// When F2 is on, 3D content starts below the menu bar and, if visible,
// the Quick Toolbar. This is not a docking framework.
struct EditorContentViewport
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

void ResetEditorWorkspaceVisibility(EditorWorkspaceState& workspace);
void ToggleEditorWorkspaceMetrics(EditorWorkspaceState& workspace);
bool AllEditorPanelsVisible(const EditorWorkspaceState& workspace);

float ResolveEditorMenuBarHeight(float lastMenuBarHeight);
float ResolveEditorToolbarHeight(float lastToolbarHeight);

// Menu bar plus optional toolbar. 0 when the editor is inactive.
float LiveEditorChromeHeight(
    bool editorActive,
    float lastMenuBarHeight,
    float lastToolbarHeight,
    bool toolbarVisible);

EditorContentViewport MakeEditorContentViewport(
    float windowWidth,
    float windowHeight,
    float chromeTopHeight);

void MapWindowMouseToContent(
    float windowMouseX,
    float windowMouseY,
    const EditorContentViewport& viewport,
    float& localX,
    float& localY);

bool PointInEditorContentViewport(
    float windowX,
    float windowY,
    const EditorContentViewport& viewport);

// True when ImGui wants the pointer or the cursor is outside the 3D content.
// Uses the caller's current-frame capture flag; do not pass a previous frame.
bool EditorViewportPointerBlocked(
    float windowMouseX,
    float windowMouseY,
    const EditorContentViewport& viewport,
    bool imguiWantsMouse);

// Extra top inset for MakeOrientationWidgetLayout when a main menu bar is live.
float OrientationWidgetMenuBarTopInset(float menuBarHeight);

// Live Application path: 0 when F2 is off. When F2 is on, uses last frame's
// menu-bar height, or kEditorMainMenuBarNominalHeight before the first bar.
// Toolbar height is added only while the Quick Toolbar is visible.
float OrientationWidgetLiveExtraTopInset(
    bool editorActive,
    float lastMenuBarHeight,
    float lastToolbarHeight = 0.0f,
    bool toolbarVisible = false);

// workingCopyValid is in-memory LevelDefinition validity (finite values, size,
// FOV) — not source-authoring availability. Debug may Apply and must not Save.
bool CanApplyPreview(bool modified, bool workingCopyValid);
bool CanRevertWorkingCopy(bool modified);
bool CanSaveLevelSource(bool authoringAvailable, bool modified);
// Policy A: Development-only, rejected while unapplied working-copy edits
// exist, rejected while EditorToolRunner.IsRunning(), and rejected while a
// Cook, Stage & Reload workflow is pending (transition window). Dirty does
// not block. toolRunnerRunning is the runner's IsRunning(), not a second flag.
bool CanReloadRuntimeLevel(
    bool authoringAvailable,
    bool modified,
    bool toolRunnerRunning,
    bool cookStageReloadPending);

// Same guards as a convenience Cook, Stage & Reload start. Dirty is not a
// parameter and does not block.
bool CanStartCookStageReload(
    bool authoringAvailable,
    bool modified,
    bool toolRunnerRunning,
    bool workflowPending);

// Returns false and leaves mode unchanged while a gizmo drag is active.
bool TrySetEditorTransformMode(
    EditorTransformMode& mode,
    bool gizmoDragging,
    EditorTransformMode requested);
}
