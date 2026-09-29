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
    gameplay::PlayerCharacterStats& playerCharacterStats,
    gameplay::RuntimeHealth& playerHealth, render::LevelCharacters& activeCharacters,
    [[maybe_unused]] const std::function<void(gameplay::DirectDamage)>& damagePlayer)
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
        playerCharacterStats = gameplay::CalculatePlayerCharacterStats(
            gameplay::kDefaultPlayerCharacterIdentity, gameplayDefinitions, equipment);
        playerHealth.SetMaximum(playerCharacterStats.Get(gameplay::GameplayStatId::MaxHealth).effective);
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
        if (ImGui::Begin("M114 Runtime Health"))
        {
            ImGui::TextUnformatted("Direct resolved amounts. Defeated characters cannot heal; reset restores Alive.");
            static float amount = 25.0f;
            ImGui::InputFloat("Amount", &amount);
            const auto drawHealth = [&](gameplay::RuntimeHealth& health, bool specialPlayer = false) {
                ImGui::Text("Health %.3f / %.3f / %s", health.Current(), health.Maximum(),
                    health.Defeated() ? "Defeated" : "Alive");
                ImGui::Text("Hit feedback %.3f s", health.DamageFeedbackRemaining());
                ImGui::Text("Attack: %s / %.3f s", health.AttackActive() ? "Active" : "Inactive", health.AttackRemaining());
                ImGui::Text("Hit reaction: %s / %s / %.3f s", health.HitReactionAvailable() ? "Resolved" : "Unavailable",
                    health.HitReactionActive() ? "Active" : "Inactive", health.HitReactionRemaining());
                // F2 freezes the production death delay and cannot exit during death.
                ImGui::BeginDisabled(specialPlayer && levelEditorState.active);
                if (ImGui::Button("Damage"))
                {
                    if (specialPlayer) damagePlayer({amount});
                    else health.ApplyDamage({amount});
                }
                ImGui::EndDisabled();
                ImGui::SameLine();
                if (ImGui::Button("Heal")) health.ApplyHealing({amount});
            };
            ImGui::PushID("Player");
            ImGui::TextUnformatted("Player / special session runtime");
            drawHealth(playerHealth, true);
            ImGui::TextWrapped("Attack resolution: %s", snapshot.playerAttackDiagnostic);
            if (levelEditorState.active) ImGui::TextUnformatted("Player Damage requires exiting F2 (death delay is paused in editor).");
            ImGui::PopID();
            const auto drawActors = [&](auto actors, const char* type) {
                ImGui::PushID(type);
                for (auto& actor : actors)
                {
                    ImGui::PushID(static_cast<int>(actor.sourcePlacementIndex));
                    ImGui::Separator();
                    ImGui::Text("%s %llu / placement %zu / %s", type,
                        static_cast<unsigned long long>(actor.handle), actor.sourcePlacementIndex,
                        actor.origin.definitionIdentity.c_str());
                    drawHealth(actor.health);
                    if (ImGui::Button("Attack")) actor.instance->RequestAttack();
                    ImGui::TextWrapped("Attack resolution: %s", actor.instance->AttackDiagnostic().c_str());
                    ImGui::TextWrapped("Hit reaction resolution: %s", actor.instance->HitReactionDiagnostic().c_str());
                    ImGui::PopID();
                }
                ImGui::PopID();
            };
            drawActors(activeCharacters.Npcs(), "NPC");
            drawActors(activeCharacters.Enemies(), "Enemy");
        }
        ImGui::End();
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
        if (ImGui::Begin("M113 Enemy Runtime"))
        {
            ImGui::TextUnformatted("Transient active state; never saved. No collision/navigation.");
            if (levelEditorView.levelCharacters != nullptr)
            {
                const auto enemies = levelEditorView.levelCharacters->Enemies();
                ImGui::Text("Active Enemys: %zu", enemies.size());
                for (const auto& enemy : enemies)
                {
                    ImGui::Separator();
                    ImGui::Text("Enemy %llu / placement %zu / CharacterInstance %llu",
                        static_cast<unsigned long long>(enemy.handle), enemy.sourcePlacementIndex,
                        static_cast<unsigned long long>(enemy.instance->Handle()));
                    ImGui::Text("%s / Type Enemy / Patrol %s / %s", enemy.origin.definitionIdentity.c_str(),
                        enemy.origin.patrolEnabled ? "Enabled" : "Disabled",
                        enemy.locomotion == gameplay::EnemyLocomotionState::Move ? "Move" : "Idle");
                    ImGui::Text("Origin %.3f %.3f %.3f", enemy.origin.position.x, enemy.origin.position.y, enemy.origin.position.z);
                    ImGui::Text("Current %.3f %.3f %.3f / direction %d", enemy.position.x, enemy.position.y, enemy.position.z, enemy.direction);
                    ImGui::Text("Min %.3f %.3f %.3f / Max %.3f %.3f %.3f", enemy.endpointMin.x, enemy.endpointMin.y,
                        enemy.endpointMin.z, enemy.endpointMax.x, enemy.endpointMax.y, enemy.endpointMax.z);
                    ImGui::Text("Presentation %s", enemy.instance->IsStatic() ? "Static"
                        : enemy.instance->Mode() == render::CharacterInstanceMode::Exact ? "Exact"
                        : enemy.instance->Mode() == render::CharacterInstanceMode::Retargeted ? "Retargeted" : "Unavailable");
                    ImGui::TextWrapped("%s; %s", enemy.diagnostic, enemy.instance->Diagnostic().c_str());
                }
            }
        }
        ImGui::End();
    }
#else
    (void)playerHealth;
    (void)activeCharacters;
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
