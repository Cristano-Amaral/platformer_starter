#pragma once

// Focused Player runtime projection of authored Character stats plus equipped
// Item addends. This is derived session state, not authored data or a general
// Character instance framework.

#include "gameplay/Equipment.h"
#include "gameplay/GameplayDefinition.h"
#include "gameplay/GameplayStat.h"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace gameplay
{
inline constexpr std::string_view kDefaultPlayerCharacterIdentity = "characters/player";

struct RuntimeCharacterStat
{
    float base = 0.0f;
    float equipmentAdditive = 0.0f;
    float effective = 0.0f;
};

struct PlayerCharacterStats
{
    std::string characterIdentity{ kDefaultPlayerCharacterIdentity };
    GameplayReferenceStatus characterResolution = GameplayReferenceStatus::Missing;
    std::array<RuntimeCharacterStat, kGameplayStatCount> values{};

    const RuntimeCharacterStat& Get(GameplayStatId stat) const
    {
        return values[static_cast<std::size_t>(stat)];
    }
};

struct PlayerMovementParameters
{
    float maxMoveSpeed = 6.0f;
    float jumpSpeed = 8.0f;
    float gravityScale = 1.0f;
};

PlayerCharacterStats CalculatePlayerCharacterStats(
    std::string_view characterIdentity,
    const GameplayDefinitionRegistry& registry,
    const Equipment& equipment);

PlayerMovementParameters ResolvePlayerMovementParameters(const PlayerCharacterStats& stats);
}
