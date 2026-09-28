#pragma once

#include "editor/LevelEditor.h"
#include "editor/EditorToolRunner.h"
#include "gameplay/Equipment.h"
#include "gameplay/GameplayDefinition.h"
#include "gameplay/Inventory.h"
#include "gameplay/PlayerCharacterStats.h"
#include "ui/debug/DebugMetrics.h"
#include "ui/debug/DebugUiBackend.h"
#include "ui/debug/CharacterInstanceHarness.h"

#include "gameplay/RuntimeHealth.h"
namespace render { class LevelCharacters; }

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
        editor::EditorToolRunner& toolRunner,
        bool cookStageReloadPending,
        gameplay::Inventory& inventory,
        gameplay::Equipment& equipment,
        const gameplay::GameplayDefinitionRegistry& gameplayDefinitions,
        gameplay::PlayerCharacterStats& playerCharacterStats,
        gameplay::RuntimeHealth& playerHealth, render::LevelCharacters& activeCharacters);

    // True while an ImGui field owns the keyboard, so Application can ignore
    // the editor toggle while the user is typing a value.
    bool WantsKeyboardCapture() const;
    bool WantsMouseCapture() const;
    bool WantsTextInput() const;
#if defined(GAME_DEVELOPMENT)
    CharacterInstanceHarness characterInstances;
#endif

private:
    DebugUiBackend backend;
    bool recoveredMetricsLayout = false;
    bool recoveredEditorWindowsLayout = false;
};
}
