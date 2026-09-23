#pragma once

// Focused typed gameplay-stat vocabulary (Milestone 98).
// Serialized and display names are the same canonical token.
// Not a property bag, and not the live Player tuning constants.

#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

namespace gameplay
{
enum class GameplayStatId
{
    MaxHealth = 0,
    MoveSpeed,
    JumpStrength,
    GravityScale,
    AttackPower,
    Defense,
    InteractionRange,
};

inline constexpr std::size_t kGameplayStatCount = 7;

inline constexpr std::array<std::string_view, kGameplayStatCount> kGameplayStatNames = {
    "MaxHealth",
    "MoveSpeed",
    "JumpStrength",
    "GravityScale",
    "AttackPower",
    "Defense",
    "InteractionRange",
};

inline std::string_view GameplayStatName(GameplayStatId id)
{
    const auto index = static_cast<std::size_t>(id);
    if (index >= kGameplayStatCount)
    {
        return {};
    }
    return kGameplayStatNames[index];
}

inline std::optional<GameplayStatId> GameplayStatIdFromName(std::string_view name)
{
    for (std::size_t index = 0; index < kGameplayStatCount; ++index)
    {
        if (name == kGameplayStatNames[index])
        {
            return static_cast<GameplayStatId>(index);
        }
    }
    return std::nullopt;
}

// Authored base stats are finite and non-negative. Zero is allowed.
// Values are not clamped.
inline bool IsValidGameplayStatValue(float value)
{
    return std::isfinite(value) && value >= 0.0f;
}

// Modifier addends are finite and may be negative. They are not authored base stats.
inline bool IsValidGameplayStatAddend(float value)
{
    return std::isfinite(value);
}

struct GameplayStatModifier
{
    GameplayStatId stat = GameplayStatId::MaxHealth;
    float addend = 0.0f;
};

struct GameplayStatEvaluation
{
    bool valid = false;
    float effective = 0.0f;
};

// effective = base + matching equipment addends + matching temporary addends.
// Spans are applied left to right, equipment first. Other stats in the spans
// are ignored. A non-finite base, addend, or running sum is invalid.
inline GameplayStatEvaluation EvaluateGameplayStat(
    float base,
    GameplayStatId stat,
    std::span<const GameplayStatModifier> equipmentModifiers,
    std::span<const GameplayStatModifier> temporaryModifiers)
{
    GameplayStatEvaluation result;
    if (!std::isfinite(base))
    {
        return result;
    }

    float sum = base;
    const auto accumulate = [&](std::span<const GameplayStatModifier> modifiers) {
        for (const GameplayStatModifier& modifier : modifiers)
        {
            if (modifier.stat != stat)
            {
                continue;
            }
            if (!std::isfinite(modifier.addend))
            {
                return false;
            }
            sum += modifier.addend;
            if (!std::isfinite(sum))
            {
                return false;
            }
        }
        return true;
    };

    if (!accumulate(equipmentModifiers) || !accumulate(temporaryModifiers))
    {
        return result;
    }
    result.valid = true;
    result.effective = sum;
    return result;
}
}
