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
    workspace.showObjectPalette = true;
    workspace.showQuickToolbar = true;
}

void ToggleEditorWorkspaceMetrics(EditorWorkspaceState& workspace)
{
    workspace.showMetrics = !workspace.showMetrics;
}

bool AllEditorPanelsVisible(const EditorWorkspaceState& workspace)
{
    return workspace.showMetrics && workspace.showHierarchy && workspace.showInspector
        && workspace.showLevelEditor && workspace.showToolOutput && workspace.showObjectPalette
        && workspace.showQuickToolbar;
}

float ResolveEditorMenuBarHeight(float lastMenuBarHeight)
{
    return lastMenuBarHeight > 0.0f ? lastMenuBarHeight : kEditorMainMenuBarNominalHeight;
}

float ResolveEditorToolbarHeight(float lastToolbarHeight)
{
    return lastToolbarHeight > 0.0f ? lastToolbarHeight : kEditorQuickToolbarNominalHeight;
}

float LiveEditorChromeHeight(
    bool editorActive,
    float lastMenuBarHeight,
    float lastToolbarHeight,
    bool toolbarVisible)
{
    if (!editorActive)
    {
        return 0.0f;
    }

    float chrome = ResolveEditorMenuBarHeight(lastMenuBarHeight);
    if (toolbarVisible)
    {
        chrome += ResolveEditorToolbarHeight(lastToolbarHeight);
    }
    return chrome;
}

EditorContentViewport MakeEditorContentViewport(
    float windowWidth,
    float windowHeight,
    float chromeTopHeight)
{
    EditorContentViewport viewport{};
    const float width = windowWidth > 1.0f ? windowWidth : 1.0f;
    const float height = windowHeight > 1.0f ? windowHeight : 1.0f;
    const float chrome = chromeTopHeight > 0.0f ? chromeTopHeight : 0.0f;
    viewport.x = 0.0f;
    viewport.y = chrome < height ? chrome : 0.0f;
    viewport.width = width;
    viewport.height = height > viewport.y ? height - viewport.y : 1.0f;
    return viewport;
}

void MapWindowMouseToContent(
    float windowMouseX,
    float windowMouseY,
    const EditorContentViewport& viewport,
    float& localX,
    float& localY)
{
    localX = windowMouseX - viewport.x;
    localY = windowMouseY - viewport.y;
}

bool PointInEditorContentViewport(
    float windowX,
    float windowY,
    const EditorContentViewport& viewport)
{
    return windowX >= viewport.x && windowX < viewport.x + viewport.width && windowY >= viewport.y
        && windowY < viewport.y + viewport.height;
}

bool EditorViewportPointerBlocked(
    float windowMouseX,
    float windowMouseY,
    const EditorContentViewport& viewport,
    bool imguiWantsMouse)
{
    return imguiWantsMouse || !PointInEditorContentViewport(windowMouseX, windowMouseY, viewport);
}

float OrientationWidgetMenuBarTopInset(float menuBarHeight)
{
    if (!(menuBarHeight > 0.0f))
    {
        return 0.0f;
    }
    return menuBarHeight + kOrientationWidgetMenuBarGap;
}

float OrientationWidgetLiveExtraTopInset(
    bool editorActive,
    float lastMenuBarHeight,
    float lastToolbarHeight,
    bool toolbarVisible)
{
    if (!editorActive)
    {
        return 0.0f;
    }
    return LiveEditorChromeHeight(
               editorActive, lastMenuBarHeight, lastToolbarHeight, toolbarVisible)
        + kOrientationWidgetMenuBarGap;
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

bool CanReloadRuntimeLevel(
    bool authoringAvailable,
    bool modified,
    bool toolRunnerRunning,
    bool cookStageReloadPending)
{
    return authoringAvailable && !modified && !toolRunnerRunning && !cookStageReloadPending;
}

bool CanStartCookStageReload(
    bool authoringAvailable,
    bool modified,
    bool toolRunnerRunning,
    bool workflowPending)
{
    return CanReloadRuntimeLevel(
        authoringAvailable, modified, toolRunnerRunning, workflowPending);
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
