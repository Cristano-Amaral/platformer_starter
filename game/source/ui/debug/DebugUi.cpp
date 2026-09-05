#include "ui/debug/DebugUi.h"

namespace ui
{
void DebugUi::Initialize()
{
    backend.Initialize();
    recoveredMetricsLayout = false;
    recoveredEditorWindowsLayout = false;
}

void DebugUi::Shutdown()
{
    backend.Shutdown();
}

void DebugUi::SaveEditorLayout()
{
    backend.SaveIniSettings();
}

editor::LevelEditorRequest DebugUi::Draw(
    const DebugMetricsSnapshot& snapshot,
    editor::LevelEditorState& levelEditorState,
    const world::LevelDefinition& level,
    const editor::LevelEditorViewContext& levelEditorView)
{
    if (backend.ConsumeTogglePressed())
    {
        editor::ToggleEditorWorkspaceMetrics(levelEditorState.workspace);
    }

    editor::LevelEditorViewContext view = levelEditorView;
    view.forceDefaultLayout = levelEditorState.forceDefaultLayoutFrames > 0;

    backend.BeginFrame();

    editor::LevelEditorRequest menuRequest = editor::LevelEditorRequest::None;
    if (levelEditorState.active)
    {
        editor::RefreshLevelEditorDerivedFlags(levelEditorState, level);
        menuRequest = editor::DrawEditorMenuBar(levelEditorState, level, view);
        view.forceDefaultLayout = levelEditorState.forceDefaultLayoutFrames > 0;
    }

    if (levelEditorState.workspace.showMetrics)
    {
        const bool recoverMetrics = !recoveredMetricsLayout && !view.forceDefaultLayout;
        DrawDebugMetrics(
            snapshot,
            view.viewportWidth,
            view.viewportHeight,
            view.forceDefaultLayout,
            recoverMetrics,
            &levelEditorState.workspace.showMetrics);
        recoveredMetricsLayout = true;
    }

    // Hierarchy / Inspector / Level Editor are their own windows, independent
    // of the F1 metrics panel, and driven by the F2 editor toggle.
    editor::LevelEditorRequest panelRequest = editor::LevelEditorRequest::None;
    if (levelEditorState.active)
    {
        view.recoverOffscreenLayout =
            !recoveredEditorWindowsLayout && !view.forceDefaultLayout;
        panelRequest = editor::DrawLevelEditor(levelEditorState, level, view);
        recoveredEditorWindowsLayout = true;
    }
    if (levelEditorState.forceDefaultLayoutFrames > 0)
    {
        backend.SaveIniSettings();
        --levelEditorState.forceDefaultLayoutFrames;
    }
    backend.EndFrame();
    return panelRequest != editor::LevelEditorRequest::None ? panelRequest : menuRequest;
}

bool DebugUi::WantsKeyboardCapture() const
{
    return backend.WantsKeyboardCapture();
}

bool DebugUi::WantsMouseCapture() const
{
    return backend.WantsMouseCapture();
}
}
