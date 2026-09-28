#pragma once

#include "gameplay/CharacterDefinition.h"
#include "gameplay/RuntimeHealth.h"

namespace gameplay
{
// Placed actors use authored base stats only; Player uses the M102 effective path.
inline float ResolveCharacterMaxHealth(const CharacterDefinition& definition)
{
    const auto index = static_cast<std::size_t>(GameplayStatId::MaxHealth);
    return ResolveRuntimeMaxHealth(definition.hasBaseStat[index] ? definition.baseStatValue[index] : 0.0f);
}
}
