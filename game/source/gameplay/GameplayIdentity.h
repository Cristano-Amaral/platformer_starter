#pragma once

// Stable authored identity for gameplay definitions (Milestone 98).
// The textual identity is the durable key. Runtime indices are not serialized.
// This is not an M54 Inventory itemId and not a Level `id` token.
// Character roles (Player / Enemy / NPC / Animal) are not part of the identity.

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace gameplay
{
inline constexpr std::string_view kGameplayItemIdentityPrefix = "items";
inline constexpr std::string_view kGameplayCharacterIdentityPrefix = "characters";
inline constexpr std::size_t kMaxGameplayDefinitionNameLength = 32;
// "characters/" (11) + name (32).
inline constexpr std::size_t kMaxGameplayIdentityLength = 43;

enum class GameplayDefinitionCategory
{
    Item,
    Character,
};

inline std::string_view GameplayDefinitionCategoryName(GameplayDefinitionCategory category)
{
    switch (category)
    {
    case GameplayDefinitionCategory::Item:
        return "Item";
    case GameplayDefinitionCategory::Character:
        return "Character";
    }
    return {};
}

inline std::string_view GameplayDefinitionCategoryPrefix(GameplayDefinitionCategory category)
{
    switch (category)
    {
    case GameplayDefinitionCategory::Item:
        return kGameplayItemIdentityPrefix;
    case GameplayDefinitionCategory::Character:
        return kGameplayCharacterIdentityPrefix;
    }
    return {};
}

inline std::optional<GameplayDefinitionCategory> GameplayDefinitionCategoryFromPrefix(
    std::string_view prefix)
{
    if (prefix == kGameplayItemIdentityPrefix)
    {
        return GameplayDefinitionCategory::Item;
    }
    if (prefix == kGameplayCharacterIdentityPrefix)
    {
        return GameplayDefinitionCategory::Character;
    }
    return std::nullopt;
}

// name = [a-z][a-z0-9_]{0,31}. No case folding and no hyphen.
inline bool IsValidGameplayDefinitionName(std::string_view name)
{
    if (name.empty() || name.size() > kMaxGameplayDefinitionNameLength)
    {
        return false;
    }
    const char first = name.front();
    if (first < 'a' || first > 'z')
    {
        return false;
    }
    for (const char character : name)
    {
        const bool ok = (character >= 'a' && character <= 'z')
            || (character >= '0' && character <= '9') || character == '_';
        if (!ok)
        {
            return false;
        }
    }
    return true;
}

struct ParsedGameplayIdentity
{
    std::string text;
    GameplayDefinitionCategory category = GameplayDefinitionCategory::Item;
    std::string name;
};

// Grammar: ("items" | "characters") "/" name
// Exactly one slash. The token is copied unchanged on success.
// Failure leaves `out` untouched and does not rewrite the token.
inline bool TryParseGameplayIdentity(std::string_view token, ParsedGameplayIdentity& out)
{
    if (token.empty() || token.size() > kMaxGameplayIdentityLength)
    {
        return false;
    }
    const std::size_t slash = token.find('/');
    if (slash == std::string_view::npos || token.find('/', slash + 1) != std::string_view::npos)
    {
        return false;
    }
    const std::string_view prefix = token.substr(0, slash);
    const std::string_view name = token.substr(slash + 1);
    const std::optional<GameplayDefinitionCategory> category =
        GameplayDefinitionCategoryFromPrefix(prefix);
    if (!category.has_value() || !IsValidGameplayDefinitionName(name))
    {
        return false;
    }

    ParsedGameplayIdentity parsed;
    parsed.text.assign(token);
    parsed.category = *category;
    parsed.name.assign(name);
    out = std::move(parsed);
    return true;
}
}
