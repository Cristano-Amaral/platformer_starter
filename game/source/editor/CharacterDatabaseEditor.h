#pragma once

#include "assets/StaticModelCatalog.h"
#include "editor/ItemDatabaseEditor.h"
#include "gameplay/CharacterDefinition.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace editor
{
struct CharacterDatabaseEditorState
{
    gameplay::GameplayDefinitionRegistry working{};
    gameplay::GameplayDefinitionRegistry baseline{};
    std::string selectedIdentity;
    std::string searchFilter;
    std::string createName;
    std::string renameName;
    std::string statusMessage;
    bool dirty = false;
    bool loaded = false;
    bool reloadConfirmOpen = false;
};

inline void RefreshCharacterDatabaseDirty(CharacterDatabaseEditorState& state)
{
    state.dirty = !GameplayRegistriesEqual(state.working, state.baseline);
}

inline bool ApplyLoadedCharacterDatabase(
    CharacterDatabaseEditorState& state, const gameplay::ParseGameplayDefinitionsResult& parsed)
{
    if (parsed.status != gameplay::LoadGameplayDefinitionsStatus::Loaded)
    {
        state.statusMessage = parsed.errorLine > 0
            ? "line " + std::to_string(parsed.errorLine) + ": " + parsed.error
            : (parsed.error.empty() ? gameplay::LoadGameplayDefinitionsStatusName(parsed.status) : parsed.error);
        return false;
    }
    state.working = CloneGameplayDefinitionRegistry(parsed.registry);
    state.baseline = CloneGameplayDefinitionRegistry(parsed.registry);
    state.loaded = true;
    state.dirty = false;
    state.reloadConfirmOpen = false;
    state.statusMessage = "Loaded";
    if (!state.selectedIdentity.empty() && state.working.Find(state.selectedIdentity) == nullptr)
        state.selectedIdentity.clear();
    if (state.selectedIdentity.empty())
    {
        for (const auto& definition : state.working.Definitions())
        {
            if (definition.category == gameplay::GameplayDefinitionCategory::Character)
            {
                state.selectedIdentity = definition.identity;
                gameplay::ParsedGameplayIdentity parsedIdentity;
                if (gameplay::TryParseGameplayIdentity(definition.identity, parsedIdentity))
                    state.renameName = parsedIdentity.name;
                break;
            }
        }
    }
    return true;
}

inline bool CharacterDatabaseSearchMatches(
    const gameplay::GameplayDefinition& definition, std::string_view query)
{
    return definition.category == gameplay::GameplayDefinitionCategory::Character
        && (ItemDatabaseQueryMatches(definition.identity, query)
            || ItemDatabaseQueryMatches(definition.character.displayName, query));
}

inline std::vector<std::string> FilterCharacterDatabaseIdentities(
    const gameplay::GameplayDefinitionRegistry& registry, std::string_view query)
{
    std::vector<std::string> result;
    for (const auto& definition : registry.Definitions())
        if (CharacterDatabaseSearchMatches(definition, query)) result.push_back(definition.identity);
    return result;
}

enum class CharacterDefinitionEditStatus
{
    Changed, Unchanged, MalformedIdentity, DuplicateIdentity, CategoryMismatch, Missing,
};

inline CharacterDefinitionEditStatus TryCreateCharacterDefinition(
    gameplay::GameplayDefinitionRegistry& registry,
    std::string_view nameOrIdentity,
    std::string& createdIdentity)
{
    createdIdentity.clear();
    gameplay::ParsedGameplayIdentity parsed;
    std::string identity;
    if (gameplay::TryParseGameplayIdentity(nameOrIdentity, parsed))
    {
        if (parsed.category != gameplay::GameplayDefinitionCategory::Character)
            return CharacterDefinitionEditStatus::CategoryMismatch;
        identity = parsed.text;
    }
    else if (gameplay::IsValidGameplayDefinitionName(nameOrIdentity))
        identity = gameplay::MakeGameplayIdentity(gameplay::GameplayDefinitionCategory::Character, nameOrIdentity);
    else return CharacterDefinitionEditStatus::MalformedIdentity;

    gameplay::GameplayDefinition definition;
    definition.identity = identity;
    definition.category = gameplay::GameplayDefinitionCategory::Character;
    definition.character = gameplay::MakeDefaultCharacterDefinition(identity);
    const auto registered = registry.Register(definition);
    if (registered.status == gameplay::RegisterGameplayDefinitionStatus::DuplicateIdentity)
        return CharacterDefinitionEditStatus::DuplicateIdentity;
    if (registered.status != gameplay::RegisterGameplayDefinitionStatus::Registered)
        return CharacterDefinitionEditStatus::MalformedIdentity;
    createdIdentity = identity;
    return CharacterDefinitionEditStatus::Changed;
}

inline bool TrySelectCharacterDefinition(CharacterDatabaseEditorState& state, std::string_view identity)
{
    const auto* definition = state.working.Find(identity);
    if (definition == nullptr || definition->category != gameplay::GameplayDefinitionCategory::Character)
        return false;
    state.selectedIdentity.assign(identity);
    gameplay::ParsedGameplayIdentity parsed;
    if (gameplay::TryParseGameplayIdentity(identity, parsed)) state.renameName = parsed.name;
    return true;
}

inline CharacterDefinitionEditStatus TryRenameSelectedCharacterIdentity(
    CharacterDatabaseEditorState& state, std::string_view newName)
{
    auto* current = state.working.FindMutable(state.selectedIdentity);
    if (current == nullptr || current->category != gameplay::GameplayDefinitionCategory::Character)
        return CharacterDefinitionEditStatus::Missing;
    const std::string identity = gameplay::MakeGameplayIdentity(
        gameplay::GameplayDefinitionCategory::Character, newName);
    if (identity.empty()) return CharacterDefinitionEditStatus::MalformedIdentity;
    if (identity == current->identity) return CharacterDefinitionEditStatus::Unchanged;
    if (state.working.Find(identity) != nullptr) return CharacterDefinitionEditStatus::DuplicateIdentity;
    gameplay::GameplayDefinition old = *current;
    gameplay::GameplayDefinition renamed = old;
    renamed.identity = identity;
    if (renamed.character.displayName == gameplay::DefaultCharacterDisplayName(old.identity))
        renamed.character.displayName = gameplay::DefaultCharacterDisplayName(identity);
    state.working.Remove(old.identity);
    if (state.working.Register(renamed).status != gameplay::RegisterGameplayDefinitionStatus::Registered)
    {
        (void)state.working.Register(old);
        return CharacterDefinitionEditStatus::MalformedIdentity;
    }
    state.selectedIdentity = identity;
    state.renameName.assign(newName);
    RefreshCharacterDatabaseDirty(state);
    return CharacterDefinitionEditStatus::Changed;
}

inline bool TryDeleteSelectedCharacterDefinition(CharacterDatabaseEditorState& state)
{
    const auto* selected = state.working.Find(state.selectedIdentity);
    if (selected == nullptr || selected->category != gameplay::GameplayDefinitionCategory::Character
        || !state.working.Remove(state.selectedIdentity)) return false;
    state.selectedIdentity.clear();
    state.renameName.clear();
    for (const auto& definition : state.working.Definitions())
        if (definition.category == gameplay::GameplayDefinitionCategory::Character)
        { TrySelectCharacterDefinition(state, definition.identity); break; }
    RefreshCharacterDatabaseDirty(state);
    return true;
}

inline bool TryAssignCharacterWorldModel(gameplay::GameplayDefinition& definition, std::string_view identity)
{
    if (definition.category != gameplay::GameplayDefinitionCategory::Character
        || !gameplay::IsValidItemWorldModelIdentity(identity)) return false;
    definition.character.worldModelIdentity.assign(identity);
    return true;
}

inline bool TryClearCharacterWorldModel(gameplay::GameplayDefinition& definition)
{
    if (definition.category != gameplay::GameplayDefinitionCategory::Character) return false;
    definition.character.worldModelIdentity.clear();
    return true;
}

inline ItemDatabaseSaveStatus TrySaveCharacterDatabase(
    CharacterDatabaseEditorState& state, const std::filesystem::path& path, bool authoringAvailable)
{
    if (!authoringAvailable || !state.loaded) return ItemDatabaseSaveStatus::Error;
    const auto saved = gameplay::SaveGameplayDefinitionsFile(path, state.working);
    if (!saved.ok) { state.statusMessage = saved.error; return ItemDatabaseSaveStatus::Invalid; }
    state.baseline = CloneGameplayDefinitionRegistry(state.working);
    state.dirty = false;
    state.statusMessage = "Saved";
    return ItemDatabaseSaveStatus::Saved;
}

inline ItemDatabaseReloadStatus TryReloadCharacterDatabase(
    CharacterDatabaseEditorState& state, const std::filesystem::path& path,
    bool authoringAvailable, bool confirmDiscard)
{
    if (!authoringAvailable) return ItemDatabaseReloadStatus::Error;
    if (state.dirty && !confirmDiscard)
    {
        state.reloadConfirmOpen = true;
        return ItemDatabaseReloadStatus::Rejected;
    }
    const auto parsed = gameplay::LoadGameplayDefinitionsFile(path);
    if (!ApplyLoadedCharacterDatabase(state, parsed))
        return parsed.status == gameplay::LoadGameplayDefinitionsStatus::Missing
            ? ItemDatabaseReloadStatus::Missing : ItemDatabaseReloadStatus::Invalid;
    state.statusMessage = "Reloaded";
    return ItemDatabaseReloadStatus::Reloaded;
}

struct LevelEditorViewContext;
void DrawCharacterDatabaseEditor(
    CharacterDatabaseEditorState& state,
    const LevelEditorViewContext& view,
    const assets::StaticModelCatalog& modelCatalog,
    bool authoringAvailable,
    bool* open);
}
