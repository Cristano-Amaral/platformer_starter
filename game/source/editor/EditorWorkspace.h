#pragma once

// Milestone 36: one authoritative editor-workspace visibility and
// action-enable policy. The F2 menu bar and the existing panels share this
// state; they are not a second implementation. Not authored, not persisted,
// not BEST.

#include "editor/EditorGizmo.h"

namespace editor
{
struct EditorWorkspaceState
{
    bool showMetrics = true;
    bool showHierarchy = true;
    bool showInspector = true;
    bool showLevelEditor = true;
};

inline constexpr float kEditorMainMenuBarNominalHeight = 24.0f;
inline constexpr float kOrientationWidgetMenuBarGap = 8.0f;

void ResetEditorWorkspaceVisibility(EditorWorkspaceState& workspace);
void ToggleEditorWorkspaceMetrics(EditorWorkspaceState& workspace);
bool AllEditorPanelsVisible(const EditorWorkspaceState& workspace);

// Extra top inset for MakeOrientationWidgetLayout when a main menu bar is live.
float OrientationWidgetMenuBarTopInset(float menuBarHeight);

// Live Application path: 0 when F2 is off. When F2 is on, uses last frame's
// menu-bar height, or kEditorMainMenuBarNominalHeight before the first bar.
float OrientationWidgetLiveExtraTopInset(bool editorActive, float lastMenuBarHeight);

// workingCopyValid is in-memory LevelDefinition validity (finite values, size,
// FOV) — not source-authoring availability. Debug may Apply and must not Save.
bool CanApplyPreview(bool modified, bool workingCopyValid);
bool CanRevertWorkingCopy(bool modified);
bool CanSaveLevelSource(bool authoringAvailable, bool modified);

// Returns false and leaves mode unchanged while a gizmo drag is active.
bool TrySetEditorTransformMode(
    EditorTransformMode& mode,
    bool gizmoDragging,
    EditorTransformMode requested);
}
