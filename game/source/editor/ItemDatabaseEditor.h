#pragma once

// Development Item Database editor seams (Milestone 99). Standalone dirty/save/
// reload over definitions.gameplay. Not the Level Editor workingCopy lifecycle,
// not Inventory, and not a generic database framework.

#include "assets/SourceTextureCatalog.h"
#include "assets/StaticModelCatalog.h"
#include "editor/TerrainPickerCard.h"
#include "gameplay/GameplayDefinition.h"
#include "gameplay/GameplayDefinitionFile.h"
#include "gameplay/ItemDefinition.h"

#include <string>
#include <string_view>
#include <vector>

namespace editor
{
enum class ItemDatabaseLoadStatus
{
    NotAttempted,
    Loaded,
    Missing,
    Invalid,
    Error,
};

enum class ItemDatabaseSaveStatus
{
    NotAttempted,
    Saved,
    Invalid,
    Error,
};

enum class ItemDatabaseReloadStatus
{
    NotAttempted,
    Reloaded,
    Rejected,
    Missing,
    Invalid,
    Error,
};

inline const char* ItemDatabaseLoadStatusName(ItemDatabaseLoadStatus status)
{
    switch (status)
    {
    case ItemDatabaseLoadStatus::NotAttempted:
        return "NotAttempted";
    case ItemDatabaseLoadStatus::Loaded:
        return "Loaded";
    case ItemDatabaseLoadStatus::Missing:
        return "Missing";
    case ItemDatabaseLoadStatus::Invalid:
        return "Invalid";
    case ItemDatabaseLoadStatus::Error:
        return "Error";
    }
    return "Error";
}

inline const char* ItemDatabaseSaveStatusName(ItemDatabaseSaveStatus status)
{
    switch (status)
    {
    case ItemDatabaseSaveStatus::NotAttempted:
        return "NotAttempted";
    case ItemDatabaseSaveStatus::Saved:
        return "Saved";
    case ItemDatabaseSaveStatus::Invalid:
        return "Invalid";
    case ItemDatabaseSaveStatus::Error:
        return "Error";
    }
    return "Error";
}

inline const char* ItemDatabaseReloadStatusName(ItemDatabaseReloadStatus status)
{
    switch (status)
    {
    case ItemDatabaseReloadStatus::NotAttempted:
        return "NotAttempted";
    case ItemDatabaseReloadStatus::Reloaded:
        return "Reloaded";
    case ItemDatabaseReloadStatus::Rejected:
        return "Rejected";
    case ItemDatabaseReloadStatus::Missing:
        return "Missing";
    case ItemDatabaseReloadStatus::Invalid:
        return "Invalid";
    case ItemDatabaseReloadStatus::Error:
        return "Error";
    }
    return "Error";
}

enum class ItemAssetPickerKind
{
    WorldModel,
    IconTexture,
};

struct ItemAssetPickerItem
{
    std::string canonicalIdentity;
    std::string displayName;
};

struct ItemAssetPickerState
{
    ItemAssetPickerKind kind = ItemAssetPickerKind::WorldModel;
    bool open = false;
    bool pointerLock = false;
    std::string filter;
};

struct ItemDatabaseEditorState
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
    bool createError = false;
    bool renameError = false;
    bool deleteConfirmOpen = false;
    bool reloadConfirmOpen = false;
    ItemDatabaseLoadStatus lastLoadStatus = ItemDatabaseLoadStatus::NotAttempted;
    ItemDatabaseSaveStatus lastSaveStatus = ItemDatabaseSaveStatus::NotAttempted;
    ItemDatabaseReloadStatus lastReloadStatus = ItemDatabaseReloadStatus::NotAttempted;
    ItemAssetPickerState modelPicker{};
    ItemAssetPickerState iconPicker{};
};

inline bool GameplayDefinitionsEqual(
    const gameplay::GameplayDefinition& a,
    const gameplay::GameplayDefinition& b)
{
    if (a.identity != b.identity || a.category != b.category)
    {
        return false;
    }
    for (std::size_t index = 0; index < gameplay::kGameplayStatCount; ++index)
    {
        if (a.hasStat[index] != b.hasStat[index])
        {
            return false;
        }
        if (a.hasStat[index] && a.statValue[index] != b.statValue[index])
        {
            return false;
        }
    }
    if (a.category == gameplay::GameplayDefinitionCategory::Item)
    {
        return gameplay::ItemDefinitionsEqual(a.item, b.item);
    }
    return true;
}

inline bool GameplayRegistriesEqual(
    const gameplay::GameplayDefinitionRegistry& a,
    const gameplay::GameplayDefinitionRegistry& b)
{
    if (a.Count() != b.Count())
    {
        return false;
    }
    for (std::size_t index = 0; index < a.Count(); ++index)
    {
        if (!GameplayDefinitionsEqual(a.Definitions()[index], b.Definitions()[index]))
        {
            return false;
        }
    }
    return true;
}

inline void RefreshItemDatabaseDirty(ItemDatabaseEditorState& state)
{
    state.dirty = !GameplayRegistriesEqual(state.working, state.baseline);
}

inline gameplay::GameplayDefinitionRegistry CloneGameplayDefinitionRegistry(
    const gameplay::GameplayDefinitionRegistry& source)
{
    gameplay::GameplayDefinitionRegistry copy;
    for (const gameplay::GameplayDefinition& definition : source.Definitions())
    {
        copy.Register(definition);
    }
    return copy;
}

inline ItemDatabaseLoadStatus ItemDatabaseLoadStatusFromFile(
    gameplay::LoadGameplayDefinitionsStatus status)
{
    switch (status)
    {
    case gameplay::LoadGameplayDefinitionsStatus::Loaded:
        return ItemDatabaseLoadStatus::Loaded;
    case gameplay::LoadGameplayDefinitionsStatus::Missing:
        return ItemDatabaseLoadStatus::Missing;
    case gameplay::LoadGameplayDefinitionsStatus::Invalid:
        return ItemDatabaseLoadStatus::Invalid;
    case gameplay::LoadGameplayDefinitionsStatus::Error:
        return ItemDatabaseLoadStatus::Error;
    }
    return ItemDatabaseLoadStatus::Error;
}

inline bool ApplyLoadedItemDatabase(
    ItemDatabaseEditorState& state,
    const gameplay::ParseGameplayDefinitionsResult& parsed)
{
    state.lastLoadStatus = ItemDatabaseLoadStatusFromFile(parsed.status);
    if (parsed.status != gameplay::LoadGameplayDefinitionsStatus::Loaded)
    {
        if (parsed.error.empty())
        {
            state.statusMessage = gameplay::LoadGameplayDefinitionsStatusName(parsed.status);
        }
        else if (parsed.errorLine > 0)
        {
            state.statusMessage =
                "line " + std::to_string(parsed.errorLine) + ": " + parsed.error;
        }
        else
        {
            state.statusMessage = parsed.error;
        }
        return false;
    }

    state.working = CloneGameplayDefinitionRegistry(parsed.registry);
    state.baseline = CloneGameplayDefinitionRegistry(parsed.registry);
    state.dirty = false;
    state.loaded = true;
    state.createError = false;
    state.renameError = false;
    state.deleteConfirmOpen = false;
    state.reloadConfirmOpen = false;
    state.modelPicker = {};
    state.iconPicker = {};
    state.statusMessage = "Loaded";
    if (!state.selectedIdentity.empty() && state.working.Find(state.selectedIdentity) == nullptr)
    {
        state.selectedIdentity.clear();
    }
    if (state.selectedIdentity.empty())
    {
        for (const gameplay::GameplayDefinition& definition : state.working.Definitions())
        {
            if (definition.category == gameplay::GameplayDefinitionCategory::Item)
            {
                state.selectedIdentity = definition.identity;
                gameplay::ParsedGameplayIdentity identityParsed;
                if (gameplay::TryParseGameplayIdentity(definition.identity, identityParsed))
                {
                    state.renameName = identityParsed.name;
                }
                break;
            }
        }
    }
    return true;
}

inline char ItemDatabaseAsciiLower(char character)
{
    return (character >= 'A' && character <= 'Z')
        ? static_cast<char>(character - 'A' + 'a')
        : character;
}

inline bool ItemDatabaseQueryMatches(std::string_view haystack, std::string_view query)
{
    if (query.empty())
    {
        return true;
    }
    if (query.size() > haystack.size())
    {
        return false;
    }
    for (std::size_t start = 0; start + query.size() <= haystack.size(); ++start)
    {
        bool match = true;
        for (std::size_t index = 0; index < query.size(); ++index)
        {
            if (ItemDatabaseAsciiLower(haystack[start + index])
                != ItemDatabaseAsciiLower(query[index]))
            {
                match = false;
                break;
            }
        }
        if (match)
        {
            return true;
        }
    }
    return false;
}

inline bool ItemDatabaseSearchMatches(
    const gameplay::GameplayDefinition& definition,
    std::string_view query)
{
    if (definition.category != gameplay::GameplayDefinitionCategory::Item)
    {
        return false;
    }
    return ItemDatabaseQueryMatches(definition.identity, query)
        || ItemDatabaseQueryMatches(definition.item.displayName, query);
}

inline std::vector<std::string> FilterItemDatabaseIdentities(
    const gameplay::GameplayDefinitionRegistry& registry,
    std::string_view query)
{
    std::vector<std::string> identities;
    for (const gameplay::GameplayDefinition& definition : registry.Definitions())
    {
        if (ItemDatabaseSearchMatches(definition, query))
        {
            identities.push_back(definition.identity);
        }
    }
    return identities;
}

struct ItemDefinitionPickerRow
{
    std::string identity;
    std::string displayName;
};

inline std::vector<ItemDefinitionPickerRow> CollectItemDefinitionPickerRows(
    const gameplay::GameplayDefinitionRegistry& registry,
    std::string_view query)
{
    std::vector<ItemDefinitionPickerRow> rows;
    for (const gameplay::GameplayDefinition& definition : registry.Definitions())
    {
        if (definition.category != gameplay::GameplayDefinitionCategory::Item)
        {
            continue;
        }
        if (!ItemDatabaseSearchMatches(definition, query))
        {
            continue;
        }
        ItemDefinitionPickerRow row;
        row.identity = definition.identity;
        row.displayName = definition.item.displayName.empty()
            ? definition.identity
            : definition.item.displayName;
        rows.push_back(std::move(row));
    }
    return rows;
}

inline const char* ItemDatabaseSearchStatusText(
    const gameplay::GameplayDefinitionRegistry& registry,
    const std::vector<std::string>& filtered)
{
    std::size_t itemCount = 0;
    for (const gameplay::GameplayDefinition& definition : registry.Definitions())
    {
        if (definition.category == gameplay::GameplayDefinitionCategory::Item)
        {
            ++itemCount;
        }
    }
    if (itemCount == 0)
    {
        return "No Item definitions.";
    }
    if (filtered.empty())
    {
        return "No Items match this search.";
    }
    return "";
}

enum class CreateItemDefinitionStatus
{
    Created,
    MalformedIdentity,
    DuplicateIdentity,
    CategoryMismatch,
};

inline const char* CreateItemDefinitionStatusName(CreateItemDefinitionStatus status)
{
    switch (status)
    {
    case CreateItemDefinitionStatus::Created:
        return "Created";
    case CreateItemDefinitionStatus::MalformedIdentity:
        return "MalformedIdentity";
    case CreateItemDefinitionStatus::DuplicateIdentity:
        return "DuplicateIdentity";
    case CreateItemDefinitionStatus::CategoryMismatch:
        return "CategoryMismatch";
    }
    return "MalformedIdentity";
}

inline CreateItemDefinitionStatus TryCreateItemDefinition(
    gameplay::GameplayDefinitionRegistry& registry,
    std::string_view nameOrIdentity,
    std::string& createdIdentity)
{
    createdIdentity.clear();
    std::string identity;
    gameplay::ParsedGameplayIdentity parsed;
    if (gameplay::TryParseGameplayIdentity(nameOrIdentity, parsed))
    {
        if (parsed.category != gameplay::GameplayDefinitionCategory::Item)
        {
            return CreateItemDefinitionStatus::CategoryMismatch;
        }
        identity = parsed.text;
    }
    else if (gameplay::IsValidGameplayDefinitionName(nameOrIdentity))
    {
        identity = gameplay::MakeGameplayIdentity(
            gameplay::GameplayDefinitionCategory::Item, nameOrIdentity);
    }
    else
    {
        return CreateItemDefinitionStatus::MalformedIdentity;
    }

    gameplay::GameplayDefinition definition;
    definition.identity = identity;
    definition.category = gameplay::GameplayDefinitionCategory::Item;
    definition.item = gameplay::MakeDefaultItemDefinition(identity);
    const gameplay::RegisterGameplayDefinitionResult registered = registry.Register(definition);
    if (registered.status == gameplay::RegisterGameplayDefinitionStatus::DuplicateIdentity)
    {
        return CreateItemDefinitionStatus::DuplicateIdentity;
    }
    if (registered.status != gameplay::RegisterGameplayDefinitionStatus::Registered)
    {
        return CreateItemDefinitionStatus::MalformedIdentity;
    }
    createdIdentity = identity;
    return CreateItemDefinitionStatus::Created;
}

inline bool TrySelectItemDefinition(ItemDatabaseEditorState& state, std::string_view identity)
{
    const gameplay::GameplayDefinition* definition = state.working.Find(identity);
    if (definition == nullptr || definition->category != gameplay::GameplayDefinitionCategory::Item)
    {
        return false;
    }
    state.selectedIdentity.assign(identity);
    gameplay::ParsedGameplayIdentity parsed;
    if (gameplay::TryParseGameplayIdentity(definition->identity, parsed))
    {
        state.renameName = parsed.name;
    }
    state.renameError = false;
    return true;
}

inline bool TryDeleteSelectedItemDefinition(ItemDatabaseEditorState& state)
{
    if (state.selectedIdentity.empty())
    {
        return false;
    }
    const gameplay::GameplayDefinition* definition = state.working.Find(state.selectedIdentity);
    if (definition == nullptr || definition->category != gameplay::GameplayDefinitionCategory::Item)
    {
        return false;
    }
    const std::string removed = state.selectedIdentity;
    if (!state.working.Remove(removed))
    {
        return false;
    }
    state.selectedIdentity.clear();
    state.renameName.clear();
    state.deleteConfirmOpen = false;
    for (const gameplay::GameplayDefinition& remaining : state.working.Definitions())
    {
        if (remaining.category == gameplay::GameplayDefinitionCategory::Item)
        {
            TrySelectItemDefinition(state, remaining.identity);
            break;
        }
    }
    RefreshItemDatabaseDirty(state);
    return true;
}

enum class RenameItemDefinitionStatus
{
    Renamed,
    Unchanged,
    MalformedIdentity,
    DuplicateIdentity,
    Missing,
};

inline RenameItemDefinitionStatus TryRenameSelectedItemIdentity(
    ItemDatabaseEditorState& state,
    std::string_view newName)
{
    gameplay::GameplayDefinition* current = state.working.FindMutable(state.selectedIdentity);
    if (current == nullptr || current->category != gameplay::GameplayDefinitionCategory::Item)
    {
        return RenameItemDefinitionStatus::Missing;
    }

    const std::string identity =
        gameplay::MakeGameplayIdentity(gameplay::GameplayDefinitionCategory::Item, newName);
    if (identity.empty())
    {
        return RenameItemDefinitionStatus::MalformedIdentity;
    }
    if (identity == current->identity)
    {
        return RenameItemDefinitionStatus::Unchanged;
    }
    if (state.working.Find(identity) != nullptr)
    {
        return RenameItemDefinitionStatus::DuplicateIdentity;
    }

    gameplay::GameplayDefinition backup = *current;
    gameplay::GameplayDefinition renamed = backup;
    renamed.identity = identity;
    if (renamed.item.displayName == gameplay::DefaultItemDisplayName(backup.identity))
    {
        renamed.item.displayName = gameplay::DefaultItemDisplayName(identity);
    }
    if (!state.working.Remove(backup.identity))
    {
        return RenameItemDefinitionStatus::Missing;
    }
    const gameplay::RegisterGameplayDefinitionResult registered = state.working.Register(renamed);
    if (registered.status != gameplay::RegisterGameplayDefinitionStatus::Registered)
    {
        (void)state.working.Register(backup);
        return RenameItemDefinitionStatus::MalformedIdentity;
    }
    state.selectedIdentity = identity;
    state.renameName.assign(newName);
    RefreshItemDatabaseDirty(state);
    return RenameItemDefinitionStatus::Renamed;
}

inline bool ItemDatabaseCanSave(const ItemDatabaseEditorState& state, bool authoringAvailable)
{
    return authoringAvailable && state.loaded && state.dirty;
}

inline bool ItemDatabaseCanReload(const ItemDatabaseEditorState& state, bool authoringAvailable)
{
    return authoringAvailable && (state.loaded || !state.dirty);
}

inline bool ItemDatabaseShouldWarnBeforeReload(const ItemDatabaseEditorState& state)
{
    return state.dirty;
}

inline std::string ValidateItemDatabaseForSave(const gameplay::GameplayDefinitionRegistry& registry)
{
    const gameplay::WriteGameplayDefinitionsResult written =
        gameplay::WriteGameplayDefinitionsText(registry);
    if (!written.ok)
    {
        return written.error.empty() ? std::string("invalid catalog") : written.error;
    }
    return {};
}

inline ItemDatabaseSaveStatus TrySaveItemDatabase(
    ItemDatabaseEditorState& state,
    const std::filesystem::path& path,
    bool authoringAvailable)
{
    state.lastSaveStatus = ItemDatabaseSaveStatus::NotAttempted;
    if (!authoringAvailable)
    {
        state.lastSaveStatus = ItemDatabaseSaveStatus::Error;
        state.statusMessage = "authoring unavailable";
        return state.lastSaveStatus;
    }
    if (!state.loaded)
    {
        state.lastSaveStatus = ItemDatabaseSaveStatus::Error;
        state.statusMessage = "Save blocked until a valid database is loaded";
        return state.lastSaveStatus;
    }
    const std::string validation = ValidateItemDatabaseForSave(state.working);
    if (!validation.empty())
    {
        state.lastSaveStatus = ItemDatabaseSaveStatus::Invalid;
        state.statusMessage = validation;
        return state.lastSaveStatus;
    }
    const gameplay::SaveGameplayDefinitionsResult saved =
        gameplay::SaveGameplayDefinitionsFile(path, state.working);
    if (!saved.ok)
    {
        state.lastSaveStatus = ItemDatabaseSaveStatus::Error;
        state.statusMessage = saved.error.empty() ? std::string("save failed") : saved.error;
        return state.lastSaveStatus;
    }
    state.baseline = CloneGameplayDefinitionRegistry(state.working);
    state.dirty = false;
    state.lastSaveStatus = ItemDatabaseSaveStatus::Saved;
    state.statusMessage = "Saved";
    return state.lastSaveStatus;
}

inline ItemDatabaseReloadStatus TryReloadItemDatabase(
    ItemDatabaseEditorState& state,
    const std::filesystem::path& path,
    bool authoringAvailable,
    bool confirmDiscard)
{
    state.lastReloadStatus = ItemDatabaseReloadStatus::NotAttempted;
    if (!authoringAvailable)
    {
        state.lastReloadStatus = ItemDatabaseReloadStatus::Error;
        state.statusMessage = "authoring unavailable";
        return state.lastReloadStatus;
    }
    if (state.dirty && !confirmDiscard)
    {
        state.reloadConfirmOpen = true;
        state.lastReloadStatus = ItemDatabaseReloadStatus::Rejected;
        state.statusMessage = "Reload would discard unsaved Item Database edits";
        return state.lastReloadStatus;
    }

    const gameplay::ParseGameplayDefinitionsResult parsed =
        gameplay::LoadGameplayDefinitionsFile(path);
    if (!ApplyLoadedItemDatabase(state, parsed))
    {
        if (parsed.status == gameplay::LoadGameplayDefinitionsStatus::Missing)
        {
            state.lastReloadStatus = ItemDatabaseReloadStatus::Missing;
        }
        else if (parsed.status == gameplay::LoadGameplayDefinitionsStatus::Invalid)
        {
            state.lastReloadStatus = ItemDatabaseReloadStatus::Invalid;
        }
        else
        {
            state.lastReloadStatus = ItemDatabaseReloadStatus::Error;
        }
        return state.lastReloadStatus;
    }
    state.lastReloadStatus = ItemDatabaseReloadStatus::Reloaded;
    state.statusMessage = "Reloaded";
    return state.lastReloadStatus;
}

inline void NoteItemAssetPickerOpen(ItemAssetPickerState& picker, bool open)
{
    picker.open = open;
    if (open)
    {
        picker.pointerLock = true;
    }
}

inline bool ItemAssetPickerBlocksPointer(const ItemAssetPickerState& picker)
{
    return picker.open || picker.pointerLock;
}

inline bool ItemDatabaseUiBlocksPointer(const ItemDatabaseEditorState& state)
{
    return ItemAssetPickerBlocksPointer(state.modelPicker)
        || ItemAssetPickerBlocksPointer(state.iconPicker);
}

inline void TickItemAssetPickerPointerLock(ItemAssetPickerState& picker, bool pointerHeld)
{
    if (picker.open)
    {
        picker.pointerLock = true;
        return;
    }
    if (!pointerHeld)
    {
        picker.pointerLock = false;
    }
}

inline void TickItemDatabasePickerPointerLocks(ItemDatabaseEditorState& state, bool pointerHeld)
{
    TickItemAssetPickerPointerLock(state.modelPicker, pointerHeld);
    TickItemAssetPickerPointerLock(state.iconPicker, pointerHeld);
}

inline bool ItemAssetPickerOpensWithZeroResults(std::size_t catalogCount)
{
    (void)catalogCount;
    return true;
}

inline const char* ItemAssetPickerEmptyStatusText(ItemAssetPickerKind kind, bool catalogEmpty)
{
    if (catalogEmpty)
    {
        return kind == ItemAssetPickerKind::WorldModel
            ? "No compatible static .glb models are available."
            : "No compatible PNG textures are available.";
    }
    return kind == ItemAssetPickerKind::WorldModel
        ? "No compatible static models match this search."
        : "No compatible textures match this search.";
}

inline std::vector<ItemAssetPickerItem> CollectItemWorldModelPickerItems(
    const assets::StaticModelCatalog& catalog)
{
    std::vector<ItemAssetPickerItem> items;
    items.reserve(catalog.Count());
    for (const assets::StaticModelCatalogEntry& entry : catalog.Entries())
    {
        ItemAssetPickerItem item{};
        item.canonicalIdentity = entry.canonicalIdentity;
        item.displayName = entry.displayName;
        items.push_back(std::move(item));
    }
    return items;
}

inline std::vector<ItemAssetPickerItem> CollectItemIconTexturePickerItems(
    const assets::SourceTextureCatalog& catalog)
{
    std::vector<ItemAssetPickerItem> items;
    items.reserve(catalog.Count());
    for (const assets::SourceTextureCatalogEntry& entry : catalog.Entries())
    {
        ItemAssetPickerItem item{};
        item.canonicalIdentity = entry.canonicalIdentity;
        item.displayName = entry.displayName;
        items.push_back(std::move(item));
    }
    return items;
}

inline std::vector<ItemAssetPickerItem> FilterItemAssetPickerItems(
    const std::vector<ItemAssetPickerItem>& items,
    std::string_view query)
{
    std::vector<ItemAssetPickerItem> filtered;
    filtered.reserve(items.size());
    for (const ItemAssetPickerItem& item : items)
    {
        if (ItemDatabaseQueryMatches(item.canonicalIdentity, query)
            || ItemDatabaseQueryMatches(item.displayName, query))
        {
            filtered.push_back(item);
        }
    }
    return filtered;
}

inline bool TryAssignItemWorldModel(gameplay::GameplayDefinition& item, std::string_view identity)
{
    if (item.category != gameplay::GameplayDefinitionCategory::Item
        || !gameplay::IsValidItemWorldModelIdentity(identity))
    {
        return false;
    }
    item.item.worldModelIdentity.assign(identity);
    return true;
}

inline bool TryClearItemWorldModel(gameplay::GameplayDefinition& item)
{
    if (item.category != gameplay::GameplayDefinitionCategory::Item)
    {
        return false;
    }
    item.item.worldModelIdentity.clear();
    return true;
}

inline bool TryAssignItemIconTexture(gameplay::GameplayDefinition& item, std::string_view identity)
{
    if (item.category != gameplay::GameplayDefinitionCategory::Item
        || !gameplay::IsValidItemIconTextureIdentity(identity))
    {
        return false;
    }
    item.item.iconTextureIdentity.assign(identity);
    return true;
}

inline bool TryClearItemIconTexture(gameplay::GameplayDefinition& item)
{
    if (item.category != gameplay::GameplayDefinitionCategory::Item)
    {
        return false;
    }
    item.item.iconTextureIdentity.clear();
    return true;
}

inline bool TryAddItemStatModifier(
    gameplay::GameplayDefinition& item,
    gameplay::GameplayStatId stat,
    float addend)
{
    if (item.category != gameplay::GameplayDefinitionCategory::Item
        || !gameplay::IsValidGameplayStatAddend(addend)
        || item.item.modifiers.size() >= gameplay::kMaxItemModifiers)
    {
        return false;
    }
    item.item.modifiers.push_back(gameplay::GameplayStatModifier{stat, addend});
    return true;
}

inline bool TryRemoveItemStatModifier(gameplay::GameplayDefinition& item, std::size_t index)
{
    if (item.category != gameplay::GameplayDefinitionCategory::Item
        || index >= item.item.modifiers.size())
    {
        return false;
    }
    item.item.modifiers.erase(
        item.item.modifiers.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

inline bool ItemWorldModelIsCataloged(
    std::string_view identity,
    const std::vector<ItemAssetPickerItem>& catalogItems)
{
    for (const ItemAssetPickerItem& item : catalogItems)
    {
        if (item.canonicalIdentity == identity)
        {
            return true;
        }
    }
    return false;
}

inline bool ItemIconTextureIsCataloged(
    std::string_view identity,
    const std::vector<ItemAssetPickerItem>& catalogItems)
{
    return ItemWorldModelIsCataloged(identity, catalogItems);
}

inline const char* ItemMissingWorldModelStatusText()
{
    return "Referenced world model is missing from the catalog.";
}

inline const char* ItemMissingIconTextureStatusText()
{
    return "Referenced icon texture is missing from the catalog.";
}

// Reuses M95/M96 TerrainPickerCard geometry so thumbnail plus name band is one hit.
inline bool ItemAssetPickerCardShouldActivate(
    bool pointerPressed,
    float pointerX,
    float pointerY,
    float cardMinX,
    float cardMinY,
    float thumbSize,
    float textLineHeight)
{
    return TerrainPickerCardShouldActivate(
        pointerPressed, pointerX, pointerY, cardMinX, cardMinY, thumbSize, textLineHeight);
}

struct LevelEditorViewContext;

void DrawItemDatabaseEditor(
    ItemDatabaseEditorState& state,
    const LevelEditorViewContext& view,
    const assets::StaticModelCatalog& modelCatalog,
    const assets::SourceTextureCatalog& textureCatalog,
    bool authoringAvailable,
    bool* open);
}
