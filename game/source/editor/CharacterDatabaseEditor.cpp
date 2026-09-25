#include "editor/CharacterDatabaseEditor.h"
#include "editor/LevelEditor.h"

#if defined(PLATFORMER_ENABLE_DEBUG_UI) && defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
#include "editor/AuthoringPaths.h"
#include "editor/EditorLayout.h"
#include "editor/EditorLayoutUi.h"
#include "imgui.h"
#endif

#include <cstdio>

namespace editor
{
#if defined(PLATFORMER_ENABLE_DEBUG_UI) && defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
void DrawCharacterDatabaseEditor(
    CharacterDatabaseEditorState& state, const LevelEditorViewContext& view,
    const assets::StaticModelCatalog& modelCatalog, bool authoringAvailable, bool* open)
{
    if (open == nullptr || !*open) return;
    ApplyKnownEditorWindowPlacement(kCharacterDatabaseWindowName, view.viewportWidth,
        view.viewportHeight, view.forceDefaultLayout);
    if (!ImGui::Begin(kCharacterDatabaseWindowName, open)) { ImGui::End(); return; }
    const std::filesystem::path path = AuthoringSourcePath(gameplay::kGameplayDefinitionsLogicalPath);
    if (!state.loaded && authoringAvailable && !path.empty())
        ApplyLoadedCharacterDatabase(state, gameplay::LoadGameplayDefinitionsFile(path));
    ImGui::TextUnformatted("Reusable Character definitions and typed locomotion animation bindings.");
    ImGui::Text("Status: %s%s", state.dirty ? "Dirty - " : "", state.statusMessage.c_str());
    ImGui::BeginDisabled(!authoringAvailable || !state.loaded || !state.dirty);
    if (ImGui::Button("Save")) TrySaveCharacterDatabase(state, path, authoringAvailable);
    ImGui::EndDisabled(); ImGui::SameLine();
    if (ImGui::Button("Reload")) TryReloadCharacterDatabase(state, path, authoringAvailable, false);
    if (state.reloadConfirmOpen) { ImGui::OpenPopup("Discard Character edits?"); state.reloadConfirmOpen = false; }
    if (ImGui::BeginPopupModal("Discard Character edits?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (ImGui::Button("Discard and Reload")) { TryReloadCharacterDatabase(state, path, authoringAvailable, true); ImGui::CloseCurrentPopup(); }
        ImGui::SameLine(); if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup(); ImGui::EndPopup();
    }
    char search[128]; std::snprintf(search, sizeof(search), "%s", state.searchFilter.c_str());
    if (ImGui::InputText("Search Characters", search, sizeof(search))) state.searchFilter = search;
    char createName[gameplay::kMaxGameplayDefinitionNameLength + 1];
    std::snprintf(createName, sizeof(createName), "%s", state.createName.c_str());
    if (ImGui::InputText("characters/<name>", createName, sizeof(createName))) state.createName = createName;
    ImGui::SameLine();
    if (ImGui::Button("Create"))
    {
        std::string created;
        const auto status = TryCreateCharacterDefinition(state.working, state.createName, created);
        if (status == CharacterDefinitionEditStatus::Changed)
        {
            TrySelectCharacterDefinition(state, created);
            state.createName.clear();
            RefreshCharacterDatabaseDirty(state);
            state.statusMessage = "Created " + created;
        }
        else state.statusMessage = "Create rejected";
    }
    const auto filtered = FilterCharacterDatabaseIdentities(state.working, state.searchFilter);
    ImGui::BeginChild("##characterList", ImVec2(230, 0), true);
    for (const auto& identity : filtered)
    {
        const auto* definition = state.working.Find(identity);
        if (ImGui::Selectable(identity.c_str(), state.selectedIdentity == identity))
            TrySelectCharacterDefinition(state, identity);
        if (definition != nullptr) ImGui::TextDisabled("%s", definition->character.displayName.c_str());
    }
    ImGui::EndChild(); ImGui::SameLine(); ImGui::BeginChild("##characterInspector", ImVec2(0, 0), true);
    auto* selected = state.working.FindMutable(state.selectedIdentity);
    if (selected == nullptr || selected->category != gameplay::GameplayDefinitionCategory::Character)
        ImGui::TextUnformatted("Select or create a Character.");
    else
    {
        ImGui::Text("Identity: %s", selected->identity.c_str());
        char rename[gameplay::kMaxGameplayDefinitionNameLength + 1];
        std::snprintf(rename, sizeof(rename), "%s", state.renameName.c_str());
        if (ImGui::InputText("Rename leaf", rename, sizeof(rename))) state.renameName = rename;
        ImGui::SameLine(); if (ImGui::Button("Rename"))
        {
            const auto status = TryRenameSelectedCharacterIdentity(state, state.renameName);
            state.statusMessage = status == CharacterDefinitionEditStatus::Changed ? "Renamed" : "Rename rejected";
            selected = state.working.FindMutable(state.selectedIdentity);
        }
        char displayName[gameplay::kMaxCharacterDisplayNameLength + 1];
        std::snprintf(displayName, sizeof(displayName), "%s", selected->character.displayName.c_str());
        if (ImGui::InputText("Display Name", displayName, sizeof(displayName)))
        { selected->character.displayName = displayName; RefreshCharacterDatabaseDirty(state); }
        char description[gameplay::kMaxCharacterDescriptionLength + 1];
        std::snprintf(description, sizeof(description), "%s", selected->character.description.c_str());
        if (ImGui::InputTextMultiline("Description", description, sizeof(description), ImVec2(-1, 55)))
        { selected->character.description = description; RefreshCharacterDatabaseDirty(state); }
        int type = static_cast<int>(selected->character.type);
        const char* typeNames[] = {"Player", "Enemy", "NPC", "Animal"};
        if (ImGui::Combo("Character Type", &type, typeNames, IM_ARRAYSIZE(typeNames)))
        { selected->character.type = static_cast<gameplay::CharacterType>(type); RefreshCharacterDatabaseDirty(state); }
        ImGui::Separator(); ImGui::TextUnformatted("World Model");
        if (selected->character.worldModelIdentity.empty()) ImGui::TextUnformatted("None");
        else
        {
            ImGui::TextWrapped("%s", selected->character.worldModelIdentity.c_str());
            if (modelCatalog.Find(selected->character.worldModelIdentity) == nullptr)
                ImGui::TextColored(ImVec4(.92f,.72f,.28f,1), "Missing (authored reference preserved)");
        }
        if (ImGui::BeginCombo("Select Model", selected->character.worldModelIdentity.empty() ? "None" : selected->character.worldModelIdentity.c_str()))
        {
            if (ImGui::Selectable("None")) { TryClearCharacterWorldModel(*selected); RefreshCharacterDatabaseDirty(state); }
            for (const auto& model : modelCatalog.Entries()) if (ImGui::Selectable(model.displayName.c_str()))
            { TryAssignCharacterWorldModel(*selected, model.canonicalIdentity); RefreshCharacterDatabaseDirty(state); }
            ImGui::EndCombo();
        }
        ImGui::Separator(); ImGui::TextUnformatted("Locomotion Animations");
        const char* labels[] = {"Idle Clip", "Move Clip", "Jump Clip"};
        auto slots = {CharacterAnimationSlot::Idle, CharacterAnimationSlot::Move,
            CharacterAnimationSlot::Jump};
        int slotIndex = 0;
        for (CharacterAnimationSlot slot : slots)
        {
            std::string* value = slot == CharacterAnimationSlot::Idle
                ? &selected->character.animations.idle : slot == CharacterAnimationSlot::Move
                ? &selected->character.animations.move : &selected->character.animations.jump;
            char clip[gameplay::kMaxCharacterAnimationClipNameLength + 1]{};
            std::snprintf(clip, sizeof(clip), "%s", value->c_str());
            if (ImGui::InputText(labels[slotIndex], clip, sizeof(clip)))
            {
                if (clip[0] == '\0') TryClearCharacterAnimation(*selected, slot);
                else (void)TryAssignCharacterAnimation(*selected, slot, clip);
                RefreshCharacterDatabaseDirty(state);
            }
            ImGui::SameLine();
            ImGui::TextDisabled("%s", value->empty() ? "None" : "Authored (validated at runtime)");
            ++slotIndex;
        }
        ImGui::Separator(); ImGui::TextUnformatted("Base Stats (authored only)");
        for (std::size_t index = 0; index < gameplay::kGameplayStatCount; ++index)
        {
            bool enabled = selected->character.hasBaseStat[index];
            ImGui::PushID(static_cast<int>(index));
            if (ImGui::Checkbox(gameplay::kGameplayStatNames[index].data(), &enabled))
            { selected->character.hasBaseStat[index] = enabled; RefreshCharacterDatabaseDirty(state); }
            if (enabled) { ImGui::SameLine(); float value = selected->character.baseStatValue[index];
                if (ImGui::InputFloat("##value", &value, 0, 0, "%.6g"))
                { selected->character.baseStatValue[index] = value; RefreshCharacterDatabaseDirty(state); } }
            ImGui::PopID();
        }
        if (ImGui::Button("Delete Character")) { TryDeleteSelectedCharacterDefinition(state); selected = nullptr; }
    }
    ImGui::EndChild(); ImGui::End();
}
#else
void DrawCharacterDatabaseEditor(CharacterDatabaseEditorState&, const LevelEditorViewContext&,
    const assets::StaticModelCatalog&, bool, bool*) {}
#endif
}
