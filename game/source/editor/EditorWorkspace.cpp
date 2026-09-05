#include "editor/EditorWorkspace.h"

namespace editor
{
void ResetEditorWorkspaceVisibility(EditorWorkspaceState& workspace)
{
    workspace.showMetrics = true;
    workspace.showHierarchy = true;
    workspace.showInspector = true;
    workspace.showLevelEditor = true;
    workspace.showToolOutput = true;
}

void ToggleEditorWorkspaceMetrics(EditorWorkspaceState& workspace)
{
    workspace.showMetrics = !workspace.showMetrics;
}

bool AllEditorPanelsVisible(const EditorWorkspaceState& workspace)
{
    return workspace.showMetrics && workspace.showHierarchy && workspace.showInspector
        && workspace.showLevelEditor && workspace.showToolOutput;
}

float OrientationWidgetMenuBarTopInset(float menuBarHeight)
{
    if (!(menuBarHeight > 0.0f))
    {
        return 0.0f;
    }
    return menuBarHeight + kOrientationWidgetMenuBarGap;
}

float OrientationWidgetLiveExtraTopInset(bool editorActive, float lastMenuBarHeight)
{
    if (!editorActive)
    {
        return 0.0f;
    }
    const float bar = lastMenuBarHeight > 0.0f
        ? lastMenuBarHeight
        : kEditorMainMenuBarNominalHeight;
    return OrientationWidgetMenuBarTopInset(bar);
}

bool CanApplyPreview(bool modified, bool workingCopyValid)
{
    return modified && workingCopyValid;
}

bool CanRevertWorkingCopy(bool modified)
{
    return modified;
}

bool CanSaveLevelSource(bool authoringAvailable, bool modified)
{
    return authoringAvailable && !modified;
}

bool TrySetEditorTransformMode(
    EditorTransformMode& mode,
    bool gizmoDragging,
    EditorTransformMode requested)
{
    if (gizmoDragging)
    {
        return false;
    }
    mode = requested;
    return true;
}
}
