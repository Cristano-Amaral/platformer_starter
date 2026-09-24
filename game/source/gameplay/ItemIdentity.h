#pragma once

// Durable Item identity is items/<name> (Milestone 98/99/100).
// Legacy M54 short itemId tokens and the M57 numeric door token "1" are
// accepted only through this explicit compatibility table. There is no
// heuristic/fuzzy matching and no second Item catalog.

#include "gameplay/GameplayIdentity.h"
#include "gameplay/Inventory.h"

#include <string>
#include <string_view>

namespace gameplay
{
inline constexpr std::string_view kCanonicalKeyItemIdentity = "items/master_key";
inline constexpr std::string_view kLegacyKeyItemIdToken = "key";
inline constexpr std::string_view kLegacyNumericKeyItemToken = "1";

enum class AuthoredItemReferenceStatus
{
    Resolved,
    LegacyMapped,
    Malformed,
    WrongCategory,
    UnmappedLegacy,
};

inline const char* AuthoredItemReferenceStatusName(AuthoredItemReferenceStatus status)
{
    switch (status)
    {
    case AuthoredItemReferenceStatus::Resolved:
        return "Resolved";
    case AuthoredItemReferenceStatus::LegacyMapped:
        return "LegacyMapped";
    case AuthoredItemReferenceStatus::Malformed:
        return "Malformed";
    case AuthoredItemReferenceStatus::WrongCategory:
        return "WrongCategory";
    case AuthoredItemReferenceStatus::UnmappedLegacy:
        return "UnmappedLegacy";
    }
    return "Malformed";
}

struct AuthoredItemReferenceResolution
{
    AuthoredItemReferenceStatus status = AuthoredItemReferenceStatus::Malformed;
    std::string identity;
};

inline bool IsValidInventoryItemIdentity(std::string_view identity)
{
    ParsedGameplayIdentity parsed;
    return TryParseGameplayIdentity(identity, parsed)
        && parsed.category == GameplayDefinitionCategory::Item;
}

// Explicit compatibility only:
//   key -> items/master_key
//   1   -> items/master_key
// Unknown M54 tokens and other numerics are UnmappedLegacy, not guessed.
inline bool TryMapLegacyItemToken(std::string_view token, std::string& outIdentity)
{
    if (token == kLegacyKeyItemIdToken || token == kLegacyNumericKeyItemToken)
    {
        outIdentity = std::string(kCanonicalKeyItemIdentity);
        return true;
    }
    return false;
}

// Resolve an authored pickup/door Item token to items/<name>.
// characters/... is WrongCategory. Bare unknown tokens are UnmappedLegacy when
// they look like M54 itemId or a numeric token, otherwise Malformed.
inline AuthoredItemReferenceResolution ResolveAuthoredItemReference(std::string_view token)
{
    AuthoredItemReferenceResolution resolution;
    ParsedGameplayIdentity parsed;
    if (TryParseGameplayIdentity(token, parsed))
    {
        if (parsed.category != GameplayDefinitionCategory::Item)
        {
            resolution.status = AuthoredItemReferenceStatus::WrongCategory;
            return resolution;
        }
        resolution.status = AuthoredItemReferenceStatus::Resolved;
        resolution.identity = parsed.text;
        return resolution;
    }

    std::string mapped;
    if (TryMapLegacyItemToken(token, mapped))
    {
        resolution.status = AuthoredItemReferenceStatus::LegacyMapped;
        resolution.identity = std::move(mapped);
        return resolution;
    }

    if (IsValidItemId(token) || (token.size() == 1 && token.front() >= '0' && token.front() <= '9'))
    {
        resolution.status = AuthoredItemReferenceStatus::UnmappedLegacy;
        return resolution;
    }

    resolution.status = AuthoredItemReferenceStatus::Malformed;
    return resolution;
}
}
