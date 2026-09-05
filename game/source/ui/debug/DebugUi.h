#pragma once

#include "editor/LevelEditor.h"
#include "editor/EditorToolRunner.h"
#include "ui/debug/DebugMetrics.h"
#include "ui/debug/DebugUiBackend.h"

namespace ui
{
class DebugUi
{
public:
    void Initialize();
    void Shutdown();
    void SaveEditorLayout();

    // The editor state and the active level are owned by Application. The
    // debug UI only hosts the panel and returns whatever action the user
    // requested, so Application performs the rebuild/save itself.
    editor::LevelEditorRequest Draw(
        const DebugMetricsSnapshot& snapshot,
        editor::LevelEditorState& levelEditorState,
        const world::LevelDefinition& level,
        const editor::LevelEditorViewContext& levelEditorView,
        editor::EditorToolRunner& toolRunner);

    // True while an ImGui field owns the keyboard, so Application can ignore
    // the editor toggle while the user is typing a value.
    bool WantsKeyboardCapture() const;
    bool WantsMouseCapture() const;

private:
    DebugUiBackend backend;
    bool recoveredMetricsLayout = false;
    bool recoveredEditorWindowsLayout = false;
};
}
