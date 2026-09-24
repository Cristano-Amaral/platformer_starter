#include "editor/ItemDatabaseEditor.h"
#include "gameplay/GameplayDefinitionFile.h"
#include "gameplay/ItemDefinition.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

namespace
{
int gFailures = 0;

void Expect(bool condition, const char* name)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++gFailures;
    }
}

std::filesystem::path MakeTempDir()
{
    std::error_code error;
    const std::filesystem::path root =
        std::filesystem::temp_directory_path(error) / "platformer_item_database_test";
    std::filesystem::remove_all(root, error);
    std::filesystem::create_directories(root, error);
    return root;
}

void RemoveTree(const std::filesystem::path& root)
{
    std::error_code error;
    std::filesystem::remove_all(root, error);
}

std::string ReadFile(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        return {};
    }
    return std::string(
        (std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
}
}

int main()
{
    using gameplay::GameplayStatId;

    {
        const std::vector<editor::ItemAssetPickerItem> items = {
            {"models/crate.glb", "Crate"},
            {"models/alpha_box.glb", "Alpha Box"},
        };
        Expect(editor::FilterItemAssetPickerItems(items, "").size() == 2, "empty search keeps catalog");
        Expect(editor::FilterItemAssetPickerItems(items, "CRATE").size() == 1, "item search is case-insensitive");
        Expect(editor::FilterItemAssetPickerItems(items, "no-such").empty(), "unmatched picker search");
        Expect(editor::ItemAssetPickerOpensWithZeroResults(0), "picker opens with zero catalog results");
        Expect(
            std::string(editor::ItemAssetPickerEmptyStatusText(editor::ItemAssetPickerKind::WorldModel, true))
                    .find("available")
                != std::string::npos,
            "empty model catalog status");
        Expect(
            std::string(editor::ItemAssetPickerEmptyStatusText(editor::ItemAssetPickerKind::IconTexture, false))
                    .find("match")
                != std::string::npos,
            "empty filtered texture status");
    }

    {
        Expect(
            editor::ItemAssetPickerCardShouldActivate(true, 10.0f, 10.0f, 0.0f, 0.0f, 64.0f, 16.0f),
            "card thumbnail hit");
        Expect(
            editor::ItemAssetPickerCardShouldActivate(true, 10.0f, 70.0f, 0.0f, 0.0f, 64.0f, 16.0f),
            "card name-band hit");
        Expect(
            !editor::ItemAssetPickerCardShouldActivate(true, 80.0f, 10.0f, 0.0f, 0.0f, 64.0f, 16.0f),
            "outside card width");
        Expect(
            !editor::ItemAssetPickerCardShouldActivate(false, 10.0f, 10.0f, 0.0f, 0.0f, 64.0f, 16.0f),
            "no press does not activate");
    }

    {
        editor::ItemAssetPickerState picker{};
        Expect(!editor::ItemAssetPickerBlocksPointer(picker), "picker starts closed");
        Expect(editor::ItemAssetPickerOpensWithZeroResults(0), "open request ignores result count");
        editor::NoteItemAssetPickerOpen(picker, true);
        Expect(editor::ItemAssetPickerBlocksPointer(picker), "open picker captures pointer");
        Expect(picker.pointerLock, "open latches pointer lock");
        editor::NoteItemAssetPickerOpen(picker, false);
        Expect(picker.pointerLock, "close keeps lock until release");
        editor::TickItemAssetPickerPointerLock(picker, true);
        Expect(picker.pointerLock, "held mouse keeps lock");
        editor::TickItemAssetPickerPointerLock(picker, false);
        Expect(!picker.pointerLock, "release clears picker pointer lock");
        Expect(!editor::ItemAssetPickerBlocksPointer(picker), "closed picker does not capture");
    }

    {
        editor::ItemDatabaseEditorState state{};
        std::string created;
        Expect(
            editor::TryCreateItemDefinition(state.working, "characters/player", created)
                == editor::CreateItemDefinitionStatus::CategoryMismatch,
            "characters identity is not an Item");
        Expect(
            editor::TryCreateItemDefinition(state.working, "items/Master_Key", created)
                == editor::CreateItemDefinitionStatus::MalformedIdentity,
            "malformed create identity");
        Expect(
            editor::TryCreateItemDefinition(state.working, "temp_key", created)
                == editor::CreateItemDefinitionStatus::Created,
            "create from leaf name");
        Expect(created == "items/temp_key", "created identity uses items/ prefix");
        Expect(state.working.Find("items/temp_key") != nullptr, "created item is registered");
        Expect(
            editor::TryCreateItemDefinition(state.working, "items/temp_key", created)
                == editor::CreateItemDefinitionStatus::DuplicateIdentity,
            "duplicate create is rejected");
        Expect(state.working.Count() == 1, "duplicate create does not add a row");

        editor::TrySelectItemDefinition(state, "items/temp_key");
        Expect(state.selectedIdentity == "items/temp_key", "select by identity");
        gameplay::GameplayDefinition* item = state.working.FindMutable("items/temp_key");
        Expect(item != nullptr, "mutable selected item");
        item->item.displayName = "Temporary Key";
        const std::vector<std::string> hits = editor::FilterItemDatabaseIdentities(state.working, "temporary");
        Expect(hits.size() == 1 && hits[0] == "items/temp_key", "search matches display name");
        Expect(editor::FilterItemDatabaseIdentities(state.working, "TEMP_KEY").size() == 1,
            "search matches identity");
        Expect(editor::FilterItemDatabaseIdentities(state.working, "nope").empty(), "unmatched item search");

        Expect(editor::TryAssignItemWorldModel(*item, "models/crate.glb"), "assign world model");
        Expect(item->item.worldModelIdentity == "models/crate.glb", "world model stored");
        Expect(editor::TryClearItemWorldModel(*item), "clear world model");
        Expect(item->item.worldModelIdentity.empty(), "world model none");
        Expect(!editor::TryAssignItemWorldModel(*item, "textures/grass.png"), "texture is not a model");

        Expect(editor::TryAssignItemIconTexture(*item, "textures/grass.png"), "assign icon");
        Expect(item->item.iconTextureIdentity == "textures/grass.png", "icon stored");
        Expect(editor::TryClearItemIconTexture(*item), "clear icon");
        Expect(item->item.iconTextureIdentity.empty(), "icon none");

        Expect(editor::TryAddItemStatModifier(*item, GameplayStatId::Defense, 2.0f), "add modifier");
        Expect(editor::TryAddItemStatModifier(*item, GameplayStatId::Defense, 1.0f), "duplicate-stat modifier allowed");
        Expect(item->item.modifiers.size() == 2, "two modifiers");
        Expect(editor::TryRemoveItemStatModifier(*item, 0), "remove first modifier");
        Expect(item->item.modifiers.size() == 1 && item->item.modifiers[0].addend == 1.0f,
            "remaining modifier keeps authored order");

        Expect(editor::TryDeleteSelectedItemDefinition(state), "delete selected");
        Expect(state.working.Find("items/temp_key") == nullptr, "deleted item is gone");
        Expect(state.selectedIdentity.empty(), "selection clears when no items remain");
    }

    {
        const std::filesystem::path root = MakeTempDir();
        const std::filesystem::path path = root / "definitions.gameplay";
        editor::ItemDatabaseEditorState state{};
        const auto loaded = gameplay::LoadGameplayDefinitionsFile(
            PLATFORMER_GAMEPLAY_DEFINITIONS_SOURCE_PATH);
        Expect(loaded.status == gameplay::LoadGameplayDefinitionsStatus::Loaded, "fixture load for editor");
        Expect(editor::ApplyLoadedItemDatabase(state, loaded), "apply loaded catalog");
        Expect(!state.dirty, "fresh load is clean");
        Expect(state.working.Find("items/master_key") != nullptr, "fixture item visible");
        Expect(state.working.Find("characters/player") != nullptr, "characters preserved in working copy");

        std::string created;
        Expect(
            editor::TryCreateItemDefinition(state.working, "temp_acceptance_item", created)
                == editor::CreateItemDefinitionStatus::Created,
            "create temp item");
        editor::RefreshItemDatabaseDirty(state);
        Expect(state.dirty, "create marks dirty");
        gameplay::GameplayDefinition* item = state.working.FindMutable(created);
        Expect(item != nullptr, "created item exists");
        item->item.displayName = "Temp Item";
        item->item.description = "Disposable M99 item";
        item->item.type = gameplay::ItemType::Key;
        item->item.stackable = true;
        item->item.maxStack = 8;
        Expect(editor::TryAssignItemWorldModel(*item, "models/missing_item.glb"), "keep missing model identity");
        Expect(editor::TryAddItemStatModifier(*item, GameplayStatId::AttackPower, 3.0f), "authored modifier");
        editor::RefreshItemDatabaseDirty(state);

        Expect(
            editor::TryReloadItemDatabase(state, path, true, false)
                == editor::ItemDatabaseReloadStatus::Rejected,
            "reload without confirm keeps dirty edits");
        Expect(state.working.Find(created) != nullptr, "rejected reload leaves the new item");

        Expect(
            editor::TrySaveItemDatabase(state, path, true) == editor::ItemDatabaseSaveStatus::Saved,
            "save writes the catalog");
        Expect(!state.dirty, "saved catalog is clean");
        const auto reloaded = gameplay::LoadGameplayDefinitionsFile(path);
        Expect(reloaded.status == gameplay::LoadGameplayDefinitionsStatus::Loaded, "saved file loads");
        const gameplay::GameplayDefinition* savedItem = reloaded.registry.Find(created);
        Expect(savedItem != nullptr && savedItem->item.displayName == "Temp Item", "saved display name");
        Expect(savedItem->item.worldModelIdentity == "models/missing_item.glb", "missing model survived save");
        Expect(reloaded.registry.Find("characters/guard") != nullptr, "save keeps characters");

        item = state.working.FindMutable(created);
        item->item.displayName = "";
        editor::RefreshItemDatabaseDirty(state);
        const std::string beforeInvalid = ReadFile(path);
        Expect(
            editor::TrySaveItemDatabase(state, path, true) == editor::ItemDatabaseSaveStatus::Invalid,
            "invalid item is not persisted");
        Expect(ReadFile(path) == beforeInvalid, "failed save leaves previous file");
        item->item.displayName = "Temp Item";
        editor::RefreshItemDatabaseDirty(state);
        editor::TrySelectItemDefinition(state, created);
        Expect(editor::TryDeleteSelectedItemDefinition(state), "delete temp item");
        Expect(state.working.Find(created) == nullptr, "deleted from working copy");
        Expect(editor::TrySaveItemDatabase(state, path, true) == editor::ItemDatabaseSaveStatus::Saved,
            "save after delete");
        const auto afterDelete = gameplay::LoadGameplayDefinitionsFile(path);
        Expect(afterDelete.registry.Find(created) == nullptr, "deleted item is gone from file");
        Expect(afterDelete.registry.Find("items/master_key") != nullptr, "unrelated items remain");
        Expect(afterDelete.registry.Find("characters/player") != nullptr, "unrelated characters remain");

        editor::TryCreateItemDefinition(state.working, "another_temp", created);
        editor::RefreshItemDatabaseDirty(state);
        Expect(
            editor::TryReloadItemDatabase(state, path, true, true)
                == editor::ItemDatabaseReloadStatus::Reloaded,
            "confirmed reload discards dirty create");
        Expect(state.working.Find("items/another_temp") == nullptr, "reload restored saved catalog");

        RemoveTree(root);
    }

    {
        const std::filesystem::path root = MakeTempDir();
        const std::filesystem::path path = root / "definitions.gameplay";
        constexpr char kSpacedModel[] = "models/Chest by Quaternius - O72u4Drp8k.glb";
        editor::ItemDatabaseEditorState state{};
        const auto loaded = gameplay::LoadGameplayDefinitionsFile(
            PLATFORMER_GAMEPLAY_DEFINITIONS_SOURCE_PATH);
        Expect(loaded.status == gameplay::LoadGameplayDefinitionsStatus::Loaded,
            "production save starts from M98 fixtures");
        Expect(editor::ApplyLoadedItemDatabase(state, loaded), "apply M98 fixtures");
        Expect(state.working.Find("items/master_key") != nullptr, "fixture master_key present");
        Expect(state.working.Find("items/health_potion") != nullptr, "fixture health_potion present");
        Expect(state.working.Find("characters/player") != nullptr, "fixture player present");
        Expect(state.working.Find("characters/guard") != nullptr, "fixture guard present");

        gameplay::GameplayDefinition* potion = state.working.FindMutable("items/health_potion");
        Expect(potion != nullptr, "mutable health_potion");
        potion->item.displayName = "Health Potion";
        potion->item.description = "Restores a little health.";
        potion->item.type = gameplay::ItemType::Consumable;
        potion->item.stackable = true;
        potion->item.maxStack = 10;
        Expect(editor::TryAssignItemWorldModel(*potion, kSpacedModel), "assign spaced catalog model");
        Expect(editor::TryAssignItemIconTexture(*potion, "textures/missing_icon.png"), "assign icon");
        Expect(editor::TryAddItemStatModifier(*potion, GameplayStatId::MaxHealth, 15.0f),
            "assign typed modifier");
        editor::RefreshItemDatabaseDirty(state);
        Expect(state.dirty, "edited Item is dirty before Save");
        Expect(
            editor::TrySaveItemDatabase(state, path, true) == editor::ItemDatabaseSaveStatus::Saved,
            "production Save writes the catalog");
        Expect(!state.dirty, "successful Save clears dirty");
        Expect(ReadFile(path).find("world_model \"models/Chest by Quaternius - O72u4Drp8k.glb\"")
                != std::string::npos,
            "production Save quotes spaced world_model");

        state = {};
        Expect(state.working.Count() == 0 && !state.loaded, "in-memory state discarded");

        editor::ItemDatabaseEditorState fresh{};
        const auto fromDisk = gameplay::LoadGameplayDefinitionsFile(path);
        Expect(fromDisk.status == gameplay::LoadGameplayDefinitionsStatus::Loaded,
            "fresh process reader loads the saved file");
        Expect(fromDisk.error != "wrong field count", "saved spaced world_model is not wrong field count");
        Expect(editor::ApplyLoadedItemDatabase(fresh, fromDisk), "fresh editor applies disk catalog");
        const gameplay::GameplayDefinition* reloadedPotion = fresh.working.Find("items/health_potion");
        Expect(reloadedPotion != nullptr, "health_potion survived disk round-trip");
        Expect(reloadedPotion->item.displayName == "Health Potion", "display name with spaces survived");
        Expect(reloadedPotion->item.description == "Restores a little health.",
            "description with spaces survived");
        Expect(reloadedPotion->item.type == gameplay::ItemType::Consumable, "item type survived");
        Expect(reloadedPotion->item.stackable && reloadedPotion->item.maxStack == 10,
            "stack fields survived");
        Expect(reloadedPotion->item.worldModelIdentity == kSpacedModel,
            "spaced world model survived production Save");
        Expect(reloadedPotion->item.iconTextureIdentity == "textures/missing_icon.png",
            "icon identity survived");
        Expect(reloadedPotion->item.modifiers.size() == 1
                && reloadedPotion->item.modifiers[0].stat == GameplayStatId::MaxHealth
                && reloadedPotion->item.modifiers[0].addend == 15.0f,
            "typed modifier survived");
        Expect(gameplay::GameplayDefinitionStat(*reloadedPotion, GameplayStatId::MaxHealth) == 25.0f,
            "M98 potion base stat survived");
        Expect(fresh.working.Find("characters/player") != nullptr, "player survived Save");
        Expect(fresh.working.Find("characters/guard") != nullptr, "guard survived Save");
        Expect(fresh.working.Find("items/master_key") != nullptr, "master_key survived Save");
        gameplay::GameplayDefinitionReference valid;
        valid.identity = "items/health_potion";
        const auto resolved = fresh.working.Resolve(valid, gameplay::GameplayDefinitionCategory::Item);
        Expect(resolved.status == gameplay::GameplayReferenceStatus::Resolved,
            "fresh load still resolves authored identity");
        Expect(resolved.definition != nullptr && resolved.definition->identity == valid.identity,
            "resolved identity remains textual");

        Expect(
            editor::TryReloadItemDatabase(fresh, path, true, false)
                == editor::ItemDatabaseReloadStatus::Reloaded,
            "clean Reload reads the persisted file");

        RemoveTree(root);
    }

    {
        const std::filesystem::path root = MakeTempDir();
        const std::filesystem::path path = root / "definitions.gameplay";
        editor::ItemDatabaseEditorState failed{};
        const auto invalid = gameplay::ParseGameplayDefinitionsText(
            "PLATFORMER_GAMEPLAY_DEFINITIONS\n"
            "definition items/master_key\n"
            "world_model \"models/crate.glb\" leftover\n");
        Expect(invalid.status == gameplay::LoadGameplayDefinitionsStatus::Invalid, "malformed load");
        Expect(!editor::ApplyLoadedItemDatabase(failed, invalid), "failed load is not applied");
        Expect(!failed.loaded, "failed load does not mark the database loaded");
        Expect(failed.working.Count() == 0, "failed load does not fabricate a catalog");
        Expect(failed.statusMessage.find("line ") != std::string::npos, "load failure reports the line");
        Expect(failed.statusMessage.find("invalid world model") != std::string::npos,
            "load failure keeps an explicit parse error");
        Expect(!editor::ItemDatabaseCanSave(failed, true), "Save stays disabled after load failure");
        Expect(
            editor::TrySaveItemDatabase(failed, path, true) == editor::ItemDatabaseSaveStatus::Error,
            "Save is blocked until a valid database is loaded");
        Expect(!std::filesystem::exists(path), "blocked Save does not create a file");

        RemoveTree(root);
    }

    {
        assets::StaticModelCatalog models;
        assets::SourceTextureCatalog textures;
        Expect(editor::CollectItemWorldModelPickerItems(models).empty(), "empty model catalog collect");
        Expect(editor::CollectItemIconTexturePickerItems(textures).empty(), "empty texture catalog collect");
    }

    {
        const std::filesystem::path root = MakeTempDir();
        const std::filesystem::path path = root / "definitions.gameplay";
        editor::ItemDatabaseEditorState state{};
        gameplay::GameplayDefinition helmet;
        helmet.identity = "items/iron_helmet";
        helmet.category = gameplay::GameplayDefinitionCategory::Item;
        helmet.item = gameplay::MakeDefaultItemDefinition("items/iron_helmet");
        helmet.item.displayName = "Iron Helmet";
        helmet.item.type = gameplay::ItemType::Equipment;
        helmet.item.equipmentSlot = gameplay::EquipmentSlot::Head;
        Expect(state.working.Register(helmet).status
                == gameplay::RegisterGameplayDefinitionStatus::Registered,
            "register equipment item");
        state.loaded = true;
        editor::RefreshItemDatabaseDirty(state);
        Expect(editor::TrySaveItemDatabase(state, path, true) == editor::ItemDatabaseSaveStatus::Saved,
            "save equipment slot");
        const auto loaded = gameplay::LoadGameplayDefinitionsFile(path);
        Expect(loaded.status == gameplay::LoadGameplayDefinitionsStatus::Loaded, "reload equipment file");
        const gameplay::GameplayDefinition* again = loaded.registry.Find("items/iron_helmet");
        Expect(again != nullptr && again->item.type == gameplay::ItemType::Equipment
                && again->item.equipmentSlot == gameplay::EquipmentSlot::Head,
            "Item Database equipment_slot round-trip");
        RemoveTree(root);
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d ItemDatabaseEditor test(s) failed.\n", gFailures);
        return 1;
    }
    std::printf("ItemDatabaseEditor tests passed.\n");
    return 0;
}
