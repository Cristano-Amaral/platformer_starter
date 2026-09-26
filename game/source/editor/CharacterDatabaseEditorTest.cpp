#include "editor/CharacterDatabaseEditor.h"

#include <cstdio>
#include <filesystem>
#include <fstream>

namespace
{
int failures = 0;
void Expect(bool condition, const char* name)
{
    if (!condition) { std::fprintf(stderr, "FAIL %s\n", name); ++failures; }
}
}

int main()
{
    using gameplay::GameplayDefinitionCategory;
    using gameplay::GameplayStatId;
    for (std::size_t index = 0; index < gameplay::kCharacterTypeCount; ++index)
    {
        const auto type = static_cast<gameplay::CharacterType>(index);
        Expect(gameplay::CharacterTypeFromName(gameplay::CharacterTypeName(type)) == type,
            "Character Type round-trip");
    }

    editor::CharacterDatabaseEditorState state;
    const auto fixture = gameplay::LoadGameplayDefinitionsFile(
        PLATFORMER_GAMEPLAY_DEFINITIONS_SOURCE_PATH);
    Expect(editor::ApplyLoadedCharacterDatabase(state, fixture), "load production definitions");
    Expect(state.working.Find("items/master_key") != nullptr, "Items preserved in shared registry");

    std::string created;
    Expect(editor::TryCreateCharacterDefinition(state.working, "hero", created)
            == editor::CharacterDefinitionEditStatus::Changed,
        "create Character");
    Expect(created == "characters/hero", "create uses Character prefix");
    Expect(editor::TryCreateCharacterDefinition(state.working, "characters/hero", created)
            == editor::CharacterDefinitionEditStatus::DuplicateIdentity,
        "duplicate rejected");
    Expect(editor::TryCreateCharacterDefinition(state.working, "items/not_character", created)
            == editor::CharacterDefinitionEditStatus::CategoryMismatch,
        "category mismatch rejected");
    Expect(editor::TryCreateCharacterDefinition(state.working, "Bad Name", created)
            == editor::CharacterDefinitionEditStatus::MalformedIdentity,
        "malformed identity rejected");

    editor::TrySelectCharacterDefinition(state, "characters/hero");
    auto* hero = state.working.FindMutable("characters/hero");
    hero->character.displayName = "Hero";
    hero->character.description = "Authored character";
    hero->character.type = gameplay::CharacterType::Player;
    Expect(editor::TryAssignCharacterWorldModel(
        *hero, "models/Character Model With Spaces.glb"), "assign path-safe model");
    Expect(editor::TryAssignCharacterAnimation(
        *hero, editor::CharacterAnimationSlot::Idle, "Idle"), "assign Idle clip");
    Expect(editor::TryAssignCharacterAnimation(
        *hero, editor::CharacterAnimationSlot::Move, "Run Forward"), "assign Move clip");
    Expect(editor::TryAssignCharacterAnimation(
        *hero, editor::CharacterAnimationSlot::Jump, "Jump"), "assign Jump clip");
    Expect(editor::TryAssignCharacterAnimationAsset(
        *hero, editor::CharacterAnimationSlot::Idle, "animations/humanoid_idle"), "assign Idle asset");
    Expect(editor::TryAssignCharacterAnimationAsset(
        *hero, editor::CharacterAnimationSlot::Move, "animations/humanoid_move"), "assign Move asset");
    Expect(editor::TryAssignCharacterAnimationAsset(
        *hero, editor::CharacterAnimationSlot::Jump, "animations/humanoid_jump"), "assign Jump asset");
    Expect(gameplay::TrySetGameplayStat(*hero, GameplayStatId::MaxHealth, 125.0f)
            == gameplay::SetGameplayStatStatus::Set, "typed base stat");
    Expect(gameplay::TrySetGameplayStat(*hero, GameplayStatId::MoveSpeed, 7.5f)
            == gameplay::SetGameplayStatStatus::Set, "second typed base stat");
    editor::RefreshCharacterDatabaseDirty(state);
    Expect(state.dirty, "edits are dirty");
    Expect(editor::FilterCharacterDatabaseIdentities(state.working, "HERO").size() == 1,
        "search identity and display name");
    Expect(editor::TryRenameSelectedCharacterIdentity(state, "main_hero")
            == editor::CharacterDefinitionEditStatus::Changed, "rename Character");
    Expect(editor::TryRenameSelectedCharacterIdentity(state, "guard")
            == editor::CharacterDefinitionEditStatus::DuplicateIdentity, "duplicate rename rejected");

    const std::filesystem::path temp = std::filesystem::temp_directory_path()
        / "platformer_character_database_test.gameplay";
    Expect(editor::TrySaveCharacterDatabase(state, temp, true)
            == editor::ItemDatabaseSaveStatus::Saved, "save Character Database");
    const auto saved = gameplay::LoadGameplayDefinitionsFile(temp);
    Expect(saved.status == gameplay::LoadGameplayDefinitionsStatus::Loaded, "reload saved file");
    const auto* again = saved.registry.Find("characters/main_hero");
    Expect(again != nullptr && again->character.type == gameplay::CharacterType::Player,
        "typed Character persisted");
    Expect(again != nullptr && again->character.worldModelIdentity
            == "models/Character Model With Spaces.glb", "missing model identity preserved");
    Expect(again != nullptr && again->character.animations.idle == "Idle"
            && again->character.animations.move == "Run Forward"
            && again->character.animations.jump == "Jump", "typed animation bindings persisted");
    Expect(again != nullptr && again->character.animations.idleAsset == "animations/humanoid_idle"
            && again->character.animations.moveAsset == "animations/humanoid_move"
            && again->character.animations.jumpAsset == "animations/humanoid_jump",
        "typed reusable animation bindings persisted");
    gameplay::GameplayDefinition clearProbe = *again;
    Expect(editor::TryClearCharacterAnimationAsset(clearProbe, editor::CharacterAnimationSlot::Idle)
            && clearProbe.character.animations.idleAsset.empty(), "clear reusable binding");
    Expect(again != nullptr && gameplay::GameplayDefinitionStat(*again, GameplayStatId::MaxHealth) == 125.0f,
        "typed base stat persisted");
    const auto written = gameplay::WriteGameplayDefinitionsText(saved.registry);
    Expect(written.text.find("world_model \"models/Character Model With Spaces.glb\"")
            != std::string::npos, "spaced model is quoted");
    Expect(written.text.find("item_type Generic") != std::string::npos,
        "Item fields preserved");

    // Production-equivalent authority path: editor working copy -> Save ->
    // persisted file -> active registry. Saving does not directly alias or
    // mutate the active registry used by gameplay.
    gameplay::GameplayDefinitionRegistry active =
        editor::CloneGameplayDefinitionRegistry(fixture.registry);
    const std::filesystem::path propagationTemp = std::filesystem::temp_directory_path()
        / "platformer_character_database_propagation_test.gameplay";
    editor::CharacterDatabaseEditorState propagation;
    Expect(editor::ApplyLoadedCharacterDatabase(propagation, fixture),
        "propagation fixture load");
    auto* workingPlayer = propagation.working.FindMutable("characters/player");
    const auto* activePlayerBefore = active.Find("characters/player");
    Expect(workingPlayer != nullptr && activePlayerBefore != nullptr
            && activePlayerBefore->character.animations.idleAsset
                == "animations/humanoid_idle",
        "active Player initially uses reusable Idle");
    Expect(workingPlayer != nullptr
            && editor::TryClearCharacterAnimationAsset(
                *workingPlayer, editor::CharacterAnimationSlot::Idle)
            && editor::TryClearCharacterAnimationAsset(
                *workingPlayer, editor::CharacterAnimationSlot::Move)
            && editor::TryClearCharacterAnimationAsset(
                *workingPlayer, editor::CharacterAnimationSlot::Jump)
            && editor::TryAssignCharacterWorldModel(*workingPlayer, "models/test_static.glb"),
        "author propagation changes in working copy");
    editor::RefreshCharacterDatabaseDirty(propagation);
    Expect(active.Find("characters/player")->character.worldModelIdentity == "models/player.glb"
            && active.Find("characters/player")->character.animations.idleAsset
                == "animations/humanoid_idle",
        "working copy remains separate from active registry");
    Expect(editor::TrySaveCharacterDatabase(propagation, propagationTemp, true)
            == editor::ItemDatabaseSaveStatus::Saved,
        "save propagation changes");
    Expect(active.Find("characters/player")->character.animations.idleAsset
            == "animations/humanoid_idle",
        "Save alone does not mutate active registry");
    const auto promoted = gameplay::LoadGameplayDefinitionsFile(propagationTemp);
    Expect(promoted.status == gameplay::LoadGameplayDefinitionsStatus::Loaded,
        "load persisted definitions for promotion");
    active = editor::CloneGameplayDefinitionRegistry(promoted.registry);
    const auto* activePlayerAfter = active.Find("characters/player");
    Expect(activePlayerAfter != nullptr
            && activePlayerAfter->character.worldModelIdentity == "models/test_static.glb",
        "promotion updates active Player World Model");
    Expect(activePlayerAfter != nullptr
            && activePlayerAfter->character.animations.idleAsset.empty()
            && activePlayerAfter->character.animations.moveAsset.empty()
            && activePlayerAfter->character.animations.jumpAsset.empty(),
        "promotion clears active Idle Move Jump reusable bindings");
    Expect(activePlayerAfter != nullptr
            && activePlayerAfter->character.animations.idle == "Idle",
        "None reusable binding preserves embedded fallback");

    workingPlayer = propagation.working.FindMutable("characters/player");
    Expect(workingPlayer != nullptr
            && editor::TryAssignCharacterAnimationAsset(
                *workingPlayer, editor::CharacterAnimationSlot::Idle,
                "animations/humanoid_move")
            && editor::TryAssignCharacterAnimationAsset(
                *workingPlayer, editor::CharacterAnimationSlot::Move,
                "animations/humanoid_jump")
            && editor::TryAssignCharacterAnimationAsset(
                *workingPlayer, editor::CharacterAnimationSlot::Jump,
                "animations/humanoid_idle"),
        "replace reusable bindings");
    editor::RefreshCharacterDatabaseDirty(propagation);
    Expect(editor::TrySaveCharacterDatabase(propagation, propagationTemp, true)
            == editor::ItemDatabaseSaveStatus::Saved,
        "save replacement reusable bindings");
    Expect(editor::TryReloadCharacterDatabase(propagation, propagationTemp, true, false)
            == editor::ItemDatabaseReloadStatus::Reloaded,
        "reload replacement reusable bindings");
    const auto* reloadedPlayer = propagation.working.Find("characters/player");
    Expect(reloadedPlayer != nullptr
            && reloadedPlayer->character.animations.idleAsset == "animations/humanoid_move"
            && reloadedPlayer->character.animations.moveAsset == "animations/humanoid_jump"
            && reloadedPlayer->character.animations.jumpAsset == "animations/humanoid_idle",
        "replacement bindings survive Save and Reload");
    Expect(gameplay::ParseGameplayDefinitionsText(
        "PLATFORMER_GAMEPLAY_DEFINITIONS\ndefinition items/x\ncharacter_type NPC\n").status
            == gameplay::LoadGameplayDefinitionsStatus::Invalid,
        "Character-only field rejected on Item");
    Expect(gameplay::ParseGameplayDefinitionsText(
        "PLATFORMER_GAMEPLAY_DEFINITIONS\ndefinition characters/x\nitem_type Generic\n").status
            == gameplay::LoadGameplayDefinitionsStatus::Invalid,
        "Item-only field rejected on Character");

    editor::CharacterDatabaseEditorState fresh;
    Expect(editor::ApplyLoadedCharacterDatabase(fresh, saved), "fresh editor load");
    Expect(editor::TryDeleteSelectedCharacterDefinition(fresh), "delete selected Character");
    editor::RefreshCharacterDatabaseDirty(fresh);
    Expect(editor::TryReloadCharacterDatabase(fresh, temp, true, true)
            == editor::ItemDatabaseReloadStatus::Reloaded, "Reload restores persisted state");
    Expect(fresh.working.Find("characters/main_hero") != nullptr, "Reload restored deleted Character");
    std::error_code ignored; std::filesystem::remove(temp, ignored);
    std::filesystem::remove(propagationTemp, ignored);

    if (failures != 0) return 1;
    std::printf("CharacterDatabaseEditor tests passed.\n");
    return 0;
}
