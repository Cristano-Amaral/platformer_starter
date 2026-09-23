#pragma once

// In-memory gameplay-definition registry (Milestone 98).
// Lookup is by textual identity. Insertion order is enumeration order only.
// A resolved pointer is valid until the next mutating call on that registry.

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
enum class SetGameplayStatStatus
{
    Set,
    Duplicate,
    InvalidValue,
};

struct GameplayDefinition
{
    std::string identity;
    GameplayDefinitionCategory category = GameplayDefinitionCategory::Item;
    std::array<bool, kGameplayStatCount> hasStat{};
    std::array<float, kGameplayStatCount> statValue{};
};

inline bool GameplayDefinitionHasStat(const GameplayDefinition& definition, GameplayStatId id)
{
    const auto index = static_cast<std::size_t>(id);
    return index < kGameplayStatCount && definition.hasStat[index];
}

inline std::optional<float> GameplayDefinitionStat(
    const GameplayDefinition& definition,
    GameplayStatId id)
{
    if (!GameplayDefinitionHasStat(definition, id))
    {
        return std::nullopt;
    }
    return definition.statValue[static_cast<std::size_t>(id)];
}

// One slot per stat. A second assignment is Duplicate and does not overwrite.
SetGameplayStatStatus TrySetGameplayStat(
    GameplayDefinition& definition,
    GameplayStatId id,
    float value);

enum class RegisterGameplayDefinitionStatus
{
    Registered,
    DuplicateIdentity,
    MalformedIdentity,
    CategoryMismatch,
    InvalidStat,
};

struct RegisterGameplayDefinitionResult
{
    RegisterGameplayDefinitionStatus status = RegisterGameplayDefinitionStatus::MalformedIdentity;
    std::string error;
};

inline const char* RegisterGameplayDefinitionStatusName(RegisterGameplayDefinitionStatus status)
{
    switch (status)
    {
    case RegisterGameplayDefinitionStatus::Registered:
        return "Registered";
    case RegisterGameplayDefinitionStatus::DuplicateIdentity:
        return "DuplicateIdentity";
    case RegisterGameplayDefinitionStatus::MalformedIdentity:
        return "MalformedIdentity";
    case RegisterGameplayDefinitionStatus::CategoryMismatch:
        return "CategoryMismatch";
    case RegisterGameplayDefinitionStatus::InvalidStat:
        return "InvalidStat";
    }
    return "MalformedIdentity";
}

// Empty identity is the explicit None reference. A non-empty identity is kept
// even when it is malformed, so resolution can report that failure.
struct GameplayDefinitionReference
{
    std::string identity;

    bool IsNone() const
    {
        return identity.empty();
    }
};

enum class GameplayReferenceStatus
{
    None,
    Resolved,
    Missing,
    CategoryMismatch,
    Malformed,
};

struct GameplayReferenceResolution
{
    GameplayReferenceStatus status = GameplayReferenceStatus::Malformed;
    const GameplayDefinition* definition = nullptr;
    std::optional<std::size_t> index;
};

inline const char* GameplayReferenceStatusName(GameplayReferenceStatus status)
{
    switch (status)
    {
    case GameplayReferenceStatus::None:
        return "None";
    case GameplayReferenceStatus::Resolved:
        return "Resolved";
    case GameplayReferenceStatus::Missing:
        return "Missing";
    case GameplayReferenceStatus::CategoryMismatch:
        return "CategoryMismatch";
    case GameplayReferenceStatus::Malformed:
        return "Malformed";
    }
    return "Malformed";
}

class GameplayDefinitionRegistry
{
public:
    RegisterGameplayDefinitionResult Register(const GameplayDefinition& definition);
    void Clear();

    std::size_t Count() const;
    const std::vector<GameplayDefinition>& Definitions() const;

    // Exact identity match. Missing and malformed identities return nullptr.
    const GameplayDefinition* Find(std::string_view identity) const;
    std::optional<std::size_t> FindIndex(std::string_view identity) const;

    // expectedCategory, when set, must match the identity prefix and the stored
    // category. Mismatch does not return another definition. Resolution does
    // not modify `reference`.
    GameplayReferenceResolution Resolve(
        const GameplayDefinitionReference& reference,
        std::optional<GameplayDefinitionCategory> expectedCategory) const;

private:
    std::vector<GameplayDefinition> definitions;
};
}
