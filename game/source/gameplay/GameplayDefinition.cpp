#include "gameplay/GameplayDefinition.h"

#include <cstddef>

namespace gameplay
{
SetGameplayStatStatus TrySetGameplayStat(
    GameplayDefinition& definition,
    GameplayStatId id,
    float value)
{
    if (definition.category == GameplayDefinitionCategory::Animation)
        return SetGameplayStatStatus::InvalidValue;
    const auto index = static_cast<std::size_t>(id);
    if (index >= kGameplayStatCount || !IsValidGameplayStatValue(value))
    {
        return SetGameplayStatStatus::InvalidValue;
    }
    auto& hasStat = definition.category == GameplayDefinitionCategory::Character
        ? definition.character.hasBaseStat : definition.hasStat;
    auto& statValue = definition.category == GameplayDefinitionCategory::Character
        ? definition.character.baseStatValue : definition.statValue;
    if (hasStat[index])
    {
        return SetGameplayStatStatus::Duplicate;
    }
    hasStat[index] = true;
    statValue[index] = value;
    return SetGameplayStatStatus::Set;
}

RegisterGameplayDefinitionResult GameplayDefinitionRegistry::Register(
    const GameplayDefinition& definition)
{
    RegisterGameplayDefinitionResult result;
    ParsedGameplayIdentity parsed;
    if (!TryParseGameplayIdentity(definition.identity, parsed))
    {
        result.status = RegisterGameplayDefinitionStatus::MalformedIdentity;
        result.error = "malformed identity";
        return result;
    }
    if (parsed.category != definition.category || parsed.text != definition.identity)
    {
        result.status = RegisterGameplayDefinitionStatus::CategoryMismatch;
        result.error = "identity category does not match definition category";
        return result;
    }
    for (std::size_t index = 0; index < kGameplayStatCount; ++index)
    {
        const std::optional<float> value = GameplayDefinitionStat(
            definition, static_cast<GameplayStatId>(index));
        if (value.has_value() && !IsValidGameplayStatValue(*value))
        {
            result.status = RegisterGameplayDefinitionStatus::InvalidStat;
            result.error = "invalid stat value";
            return result;
        }
    }
    if (parsed.category == GameplayDefinitionCategory::Item)
    {
        const ValidateItemStatus itemStatus = ValidateItemDefinition(definition.item);
        if (itemStatus != ValidateItemStatus::Valid)
        {
            result.status = RegisterGameplayDefinitionStatus::InvalidItem;
            result.error = ValidateItemStatusName(itemStatus);
            return result;
        }
    }
    else if (parsed.category == GameplayDefinitionCategory::Character)
    {
        const ValidateCharacterStatus characterStatus = ValidateCharacterDefinition(definition.character);
        if (characterStatus != ValidateCharacterStatus::Valid)
        {
            result.status = RegisterGameplayDefinitionStatus::InvalidCharacter;
            result.error = ValidateCharacterStatusName(characterStatus);
            return result;
        }
    }
    else if (!ValidateAnimationDefinition(definition.animation))
    {
        result.status = RegisterGameplayDefinitionStatus::InvalidAnimation;
        result.error = "invalid animation";
        return result;
    }
    if (Find(parsed.text) != nullptr)
    {
        result.status = RegisterGameplayDefinitionStatus::DuplicateIdentity;
        result.error = "duplicate identity";
        return result;
    }

    definitions.push_back(definition);
    result.status = RegisterGameplayDefinitionStatus::Registered;
    result.error.clear();
    return result;
}

bool GameplayDefinitionRegistry::Remove(std::string_view identity)
{
    const std::optional<std::size_t> index = FindIndex(identity);
    if (!index.has_value())
    {
        return false;
    }
    definitions.erase(definitions.begin() + static_cast<std::ptrdiff_t>(*index));
    return true;
}

void GameplayDefinitionRegistry::Clear()
{
    definitions.clear();
}

std::size_t GameplayDefinitionRegistry::Count() const
{
    return definitions.size();
}

const std::vector<GameplayDefinition>& GameplayDefinitionRegistry::Definitions() const
{
    return definitions;
}

const GameplayDefinition* GameplayDefinitionRegistry::Find(std::string_view identity) const
{
    const std::optional<std::size_t> index = FindIndex(identity);
    if (!index.has_value())
    {
        return nullptr;
    }
    return &definitions[*index];
}

GameplayDefinition* GameplayDefinitionRegistry::FindMutable(std::string_view identity)
{
    const std::optional<std::size_t> index = FindIndex(identity);
    if (!index.has_value())
    {
        return nullptr;
    }
    return &definitions[*index];
}

std::optional<std::size_t> GameplayDefinitionRegistry::FindIndex(std::string_view identity) const
{
    if (identity.empty())
    {
        return std::nullopt;
    }
    for (std::size_t index = 0; index < definitions.size(); ++index)
    {
        if (definitions[index].identity == identity)
        {
            return index;
        }
    }
    return std::nullopt;
}

GameplayReferenceResolution GameplayDefinitionRegistry::Resolve(
    const GameplayDefinitionReference& reference,
    std::optional<GameplayDefinitionCategory> expectedCategory) const
{
    GameplayReferenceResolution resolution;
    if (reference.identity.empty())
    {
        resolution.status = GameplayReferenceStatus::None;
        return resolution;
    }

    ParsedGameplayIdentity parsed;
    if (!TryParseGameplayIdentity(reference.identity, parsed))
    {
        resolution.status = GameplayReferenceStatus::Malformed;
        return resolution;
    }
    if (expectedCategory.has_value() && parsed.category != *expectedCategory)
    {
        resolution.status = GameplayReferenceStatus::CategoryMismatch;
        return resolution;
    }

    const std::optional<std::size_t> index = FindIndex(parsed.text);
    if (!index.has_value())
    {
        resolution.status = GameplayReferenceStatus::Missing;
        return resolution;
    }
    const GameplayDefinition& definition = definitions[*index];
    if (definition.category != parsed.category
        || (expectedCategory.has_value() && definition.category != *expectedCategory))
    {
        resolution.status = GameplayReferenceStatus::CategoryMismatch;
        return resolution;
    }

    resolution.status = GameplayReferenceStatus::Resolved;
    resolution.definition = &definition;
    resolution.index = index;
    return resolution;
}
}
