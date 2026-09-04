#include "ui/debug/DebugUi.h"

namespace ui
{
void DebugUi::Initialize()
{
    backend.Initialize();
    panelVisible = true;
}

void DebugUi::Shutdown()
{
    backend.Shutdown();
}

editor::LevelEditorRequest DebugUi::Draw(
    const DebugMetricsSnapshot& snapshot,
    editor::LevelEditorState& levelEditorState,
    const world::LevelDefinition& level,
    const editor::LevelEditorViewContext& levelEditorView)
{
    if (backend.ConsumeTogglePressed())
    {
        panelVisible = !panelVisible;
    }

    backend.BeginFrame();
    if (panelVisible)
    {
        DrawDebugMetrics(snapshot);
    }

    // Hierarchy / Inspector / Level Editor are their own windows, independent
    // of the F1 metrics panel, and driven by the F2 editor toggle.
    editor::LevelEditorRequest request = editor::LevelEditorRequest::None;
    if (levelEditorState.active)
    {
        request = editor::DrawLevelEditor(levelEditorState, level, levelEditorView);
    }
    backend.EndFrame();
    return request;
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
