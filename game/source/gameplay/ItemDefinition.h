#pragma once

// Typed Item payload for M98 Item-category gameplay definitions (Milestone 99).
// Authored identity remains items/<name>. Item Type is classification only.
// Modifiers are authored here; M102 applies those on equipped Items to Player stats.

#include "assets/RuntimePng.h"
#include "gameplay/EquipmentSlot.h"
#include "gameplay/GameplayIdentity.h"
#include "gameplay/GameplayStat.h"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gameplay
{
enum class ItemType
{
    Generic = 0,
    Consumable,
    Equipment,
    Key,
    Quest,
};

inline constexpr std::size_t kItemTypeCount = 5;
inline constexpr int kMinItemMaxStack = 1;
inline constexpr int kMaxItemMaxStack = 99;
inline constexpr std::size_t kMaxItemDisplayNameLength = 64;
inline constexpr std::size_t kMaxItemDescriptionLength = 200;
inline constexpr std::size_t kMaxItemModifiers = 32;

inline constexpr std::array<std::string_view, kItemTypeCount> kItemTypeNames = {
    "Generic",
    "Consumable",
    "Equipment",
    "Key",
    "Quest",
};

inline std::string_view ItemTypeName(ItemType type)
{
    const auto index = static_cast<std::size_t>(type);
    if (index >= kItemTypeCount)
    {
        return {};
    }
    return kItemTypeNames[index];
}

inline std::optional<ItemType> ItemTypeFromName(std::string_view name)
{
    for (std::size_t index = 0; index < kItemTypeCount; ++index)
    {
        if (name == kItemTypeNames[index])
        {
            return static_cast<ItemType>(index);
        }
    }
    return std::nullopt;
}

inline bool ItemAuthoredTextCharIsAllowed(char character)
{
    const unsigned char byte = static_cast<unsigned char>(character);
    return byte >= 32 && byte <= 126 && character != '"' && character != '\\';
}

inline bool IsValidItemDisplayName(std::string_view text)
{
    if (text.empty() || text.size() > kMaxItemDisplayNameLength)
    {
        return false;
    }
    if (text.front() == ' ' || text.back() == ' ')
    {
        return false;
    }
    for (const char character : text)
    {
        if (!ItemAuthoredTextCharIsAllowed(character))
        {
            return false;
        }
    }
    return true;
}

inline bool IsValidItemDescription(std::string_view text)
{
    if (text.size() > kMaxItemDescriptionLength)
    {
        return false;
    }
    for (const char character : text)
    {
        if (!ItemAuthoredTextCharIsAllowed(character))
        {
            return false;
        }
    }
    return true;
}

inline bool IsValidItemStack(bool stackable, int maxStack)
{
    if (maxStack < kMinItemMaxStack || maxStack > kMaxItemMaxStack)
    {
        return false;
    }
    if (!stackable && maxStack != kMinItemMaxStack)
    {
        return false;
    }
    return true;
}

// Same structural models/<file>.glb rule as Terrain vegetation identities.
// Catalog presence is not required; a missing asset keeps this identity.
inline bool IsValidItemWorldModelIdentity(std::string_view identity)
{
    constexpr std::string_view kPrefix = "models/";
    constexpr std::string_view kExtension = ".glb";
    if (!identity.starts_with(kPrefix) || identity.find('\\') != std::string_view::npos)
    {
        return false;
    }
    const std::string_view fileName = identity.substr(kPrefix.size());
    if (fileName.size() <= kExtension.size() || !fileName.ends_with(kExtension)
        || fileName.find('/') != std::string_view::npos)
    {
        return false;
    }
    const std::string_view stem = fileName.substr(0, fileName.size() - kExtension.size());
    if (stem.empty() || stem.front() == '.' || stem.front() == ' ' || stem.back() == ' '
        || stem.back() == '.')
    {
        return false;
    }
    for (const char character : fileName)
    {
        const unsigned char byte = static_cast<unsigned char>(character);
        if (byte < 32 || character == ':' || character == '*' || character == '?'
            || character == '"' || character == '<' || character == '>' || character == '|')
        {
            return false;
        }
    }
    return true;
}

inline bool IsValidItemIconTextureIdentity(std::string_view identity)
{
    return assets::RuntimePngIdentityIsValid(identity);
}

inline std::string DefaultItemDisplayName(std::string_view identity)
{
    ParsedGameplayIdentity parsed;
    if (!TryParseGameplayIdentity(identity, parsed)
        || parsed.category != GameplayDefinitionCategory::Item)
    {
        return {};
    }
    return parsed.name;
}

struct ItemDefinition
{
    std::string displayName;
    std::string description;
    ItemType type = ItemType::Generic;
    bool stackable = false;
    int maxStack = kMinItemMaxStack;
    // Present only when type == Equipment. Empty for every other type.
    std::optional<EquipmentSlot> equipmentSlot;
    std::string worldModelIdentity;
    std::string iconTextureIdentity;
    std::vector<GameplayStatModifier> modifiers;
};

inline ItemDefinition MakeDefaultItemDefinition(std::string_view identity)
{
    ItemDefinition item;
    item.displayName = DefaultItemDisplayName(identity);
    item.description.clear();
    item.type = ItemType::Generic;
    item.stackable = false;
    item.maxStack = kMinItemMaxStack;
    item.equipmentSlot.reset();
    item.worldModelIdentity.clear();
    item.iconTextureIdentity.clear();
    item.modifiers.clear();
    return item;
}

enum class ValidateItemStatus
{
    Valid,
    InvalidDisplayName,
    InvalidDescription,
    InvalidStack,
    InvalidWorldModel,
    InvalidIcon,
    InvalidModifier,
    TooManyModifiers,
    MissingEquipmentSlot,
    UnexpectedEquipmentSlot,
};

inline const char* ValidateItemStatusName(ValidateItemStatus status)
{
    switch (status)
    {
    case ValidateItemStatus::Valid:
        return "Valid";
    case ValidateItemStatus::InvalidDisplayName:
        return "InvalidDisplayName";
    case ValidateItemStatus::InvalidDescription:
        return "InvalidDescription";
    case ValidateItemStatus::InvalidStack:
        return "InvalidStack";
    case ValidateItemStatus::InvalidWorldModel:
        return "InvalidWorldModel";
    case ValidateItemStatus::InvalidIcon:
        return "InvalidIcon";
    case ValidateItemStatus::InvalidModifier:
        return "InvalidModifier";
    case ValidateItemStatus::TooManyModifiers:
        return "TooManyModifiers";
    case ValidateItemStatus::MissingEquipmentSlot:
        return "MissingEquipmentSlot";
    case ValidateItemStatus::UnexpectedEquipmentSlot:
        return "UnexpectedEquipmentSlot";
    }
    return "InvalidDisplayName";
}

inline ValidateItemStatus ValidateItemDefinition(const ItemDefinition& item)
{
    if (!IsValidItemDisplayName(item.displayName))
    {
        return ValidateItemStatus::InvalidDisplayName;
    }
    if (!IsValidItemDescription(item.description))
    {
        return ValidateItemStatus::InvalidDescription;
    }
    if (!IsValidItemStack(item.stackable, item.maxStack))
    {
        return ValidateItemStatus::InvalidStack;
    }
    if (item.type == ItemType::Equipment)
    {
        if (!item.equipmentSlot.has_value() || !IsValidEquipmentSlot(*item.equipmentSlot))
        {
            return ValidateItemStatus::MissingEquipmentSlot;
        }
    }
    else if (item.equipmentSlot.has_value())
    {
        return ValidateItemStatus::UnexpectedEquipmentSlot;
    }
    if (!item.worldModelIdentity.empty() && !IsValidItemWorldModelIdentity(item.worldModelIdentity))
    {
        return ValidateItemStatus::InvalidWorldModel;
    }
    if (!item.iconTextureIdentity.empty() && !IsValidItemIconTextureIdentity(item.iconTextureIdentity))
    {
        return ValidateItemStatus::InvalidIcon;
    }
    if (item.modifiers.size() > kMaxItemModifiers)
    {
        return ValidateItemStatus::TooManyModifiers;
    }
    for (const GameplayStatModifier& modifier : item.modifiers)
    {
        if (GameplayStatName(modifier.stat).empty() || !IsValidGameplayStatAddend(modifier.addend))
        {
            return ValidateItemStatus::InvalidModifier;
        }
    }
    return ValidateItemStatus::Valid;
}

inline bool ItemDefinitionsEqual(const ItemDefinition& a, const ItemDefinition& b)
{
    if (a.displayName != b.displayName || a.description != b.description || a.type != b.type
        || a.stackable != b.stackable || a.maxStack != b.maxStack
        || a.equipmentSlot != b.equipmentSlot
        || a.worldModelIdentity != b.worldModelIdentity
        || a.iconTextureIdentity != b.iconTextureIdentity
        || a.modifiers.size() != b.modifiers.size())
    {
        return false;
    }
    for (std::size_t index = 0; index < a.modifiers.size(); ++index)
    {
        if (a.modifiers[index].stat != b.modifiers[index].stat
            || a.modifiers[index].addend != b.modifiers[index].addend)
        {
            return false;
        }
    }
    return true;
}
}
