#include "ui/debug/DebugUi.h"
#if defined(GAME_DEVELOPMENT)
#include "render/LevelCharacters.h"
#include <imgui.h>
#endif

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
#if defined(GAME_DEVELOPMENT)
    characterInstances.Shutdown();
#endif
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
    const editor::LevelEditorViewContext& levelEditorView,
    editor::EditorToolRunner& toolRunner,
    bool cookStageReloadPending,
    gameplay::Inventory& inventory,
    gameplay::Equipment& equipment,
    const gameplay::GameplayDefinitionRegistry& gameplayDefinitions,
    const gameplay::PlayerCharacterStats& playerCharacterStats)
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
        menuRequest = editor::DrawEditorMenuBar(
            levelEditorState, level, view, toolRunner, cookStageReloadPending);
        const editor::LevelEditorRequest toolbarRequest = editor::DrawEditorQuickToolbar(
            levelEditorState, level, toolRunner, cookStageReloadPending);
        if (menuRequest == editor::LevelEditorRequest::None)
        {
            menuRequest = toolbarRequest;
        }
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
            &levelEditorState.workspace.showMetrics,
            &inventory,
            &equipment,
            &gameplayDefinitions,
            &playerCharacterStats);
        recoveredMetricsLayout = true;
    }

    editor::DrawEditorToolOutput(levelEditorState, view, toolRunner);

    // Hierarchy / Inspector / Level Editor are their own windows, independent
    // of the F1 metrics panel, and driven by the F2 editor toggle.
    editor::LevelEditorRequest panelRequest = editor::LevelEditorRequest::None;
    if (levelEditorState.active)
    {
        view.recoverOffscreenLayout =
            !recoveredEditorWindowsLayout && !view.forceDefaultLayout;
        panelRequest = editor::DrawLevelEditor(
            levelEditorState, level, view, toolRunner, cookStageReloadPending);
        recoveredEditorWindowsLayout = true;
    }
    if (levelEditorState.forceDefaultLayoutFrames > 0)
    {
        backend.SaveIniSettings();
        --levelEditorState.forceDefaultLayoutFrames;
    }
#if defined(GAME_DEVELOPMENT)
    if (levelEditorState.workspace.showMetrics)
    {
        characterInstances.DrawControls();
        if (ImGui::Begin("M112 NPC Runtime"))
        {
            ImGui::TextUnformatted("Transient active state; never saved. No collision/navigation.");
            if (levelEditorView.levelCharacters != nullptr)
            {
                const auto npcs = levelEditorView.levelCharacters->Npcs();
                ImGui::Text("Active NPCs: %zu", npcs.size());
                for (const auto& npc : npcs)
                {
                    ImGui::Separator();
                    ImGui::Text("NPC %llu / placement %zu / CharacterInstance %llu",
                        static_cast<unsigned long long>(npc.handle), npc.sourcePlacementIndex,
                        static_cast<unsigned long long>(npc.instance->Handle()));
                    ImGui::Text("%s / Type NPC / Patrol %s / %s", npc.origin.definitionIdentity.c_str(),
                        npc.origin.patrolEnabled ? "Enabled" : "Disabled",
                        npc.locomotion == gameplay::NpcLocomotionState::Move ? "Move" : "Idle");
                    ImGui::Text("Origin %.3f %.3f %.3f", npc.origin.position.x, npc.origin.position.y, npc.origin.position.z);
                    ImGui::Text("Current %.3f %.3f %.3f / direction %d", npc.position.x, npc.position.y, npc.position.z, npc.direction);
                    ImGui::Text("Min %.3f %.3f %.3f / Max %.3f %.3f %.3f", npc.endpointMin.x, npc.endpointMin.y,
                        npc.endpointMin.z, npc.endpointMax.x, npc.endpointMax.y, npc.endpointMax.z);
                    ImGui::Text("Presentation %s", npc.instance->IsStatic() ? "Static"
                        : npc.instance->Mode() == render::CharacterInstanceMode::Exact ? "Exact"
                        : npc.instance->Mode() == render::CharacterInstanceMode::Retargeted ? "Retargeted" : "Unavailable");
                    ImGui::TextWrapped("%s; %s", npc.diagnostic, npc.instance->Diagnostic().c_str());
                }
            }
        }
        ImGui::End();
    }
#endif
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

bool DebugUi::WantsTextInput() const
{
    return backend.WantsTextInput();
}
}
