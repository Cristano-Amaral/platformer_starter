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
    const char* runtimeLevelPath)
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

    // The Level Editor panel is its own window driven by the editor toggle,
    // independent of the F1 metrics panel.
    editor::LevelEditorRequest request = editor::LevelEditorRequest::None;
    if (levelEditorState.active)
    {
        request = editor::DrawLevelEditor(levelEditorState, level, runtimeLevelPath);
    }
    backend.EndFrame();
    return request;
}

bool DebugUi::WantsKeyboardCapture() const
{
    return backend.WantsKeyboardCapture();
}
}
