#include "gameplay/PlayerCharacterStats.h"

#include <cmath>
#include <string>

namespace gameplay
{
namespace
{
constexpr std::array<float, kGameplayStatCount> kLegacyPlayerBaseStats = {
    0.0f, // MaxHealth remains owned by the existing health system.
    6.0f, // MoveSpeed
    8.0f, // JumpStrength
    1.0f, // GravityScale applied to Player's legacy gravity acceleration.
    0.0f, // AttackPower has no gameplay consumer yet.
    0.0f, // Defense has no gameplay consumer yet.
    0.0f, // InteractionRange has no gameplay consumer yet.
};
}

PlayerCharacterStats CalculatePlayerCharacterStats(
    std::string_view characterIdentity,
    const GameplayDefinitionRegistry& registry,
    const Equipment& equipment)
{
    PlayerCharacterStats result;
    result.characterIdentity = std::string(characterIdentity);

    for (std::size_t index = 0; index < kGameplayStatCount; ++index)
    {
        result.values[index].base = kLegacyPlayerBaseStats[index];
    }

    const GameplayReferenceResolution character = registry.Resolve(
        GameplayDefinitionReference{result.characterIdentity},
        GameplayDefinitionCategory::Character);
    result.characterResolution = character.status;
    if (character.status == GameplayReferenceStatus::Resolved && character.definition != nullptr)
    {
        for (std::size_t index = 0; index < kGameplayStatCount; ++index)
        {
            if (character.definition->character.hasBaseStat[index])
            {
                result.values[index].base = character.definition->character.baseStatValue[index];
            }
        }
    }

    // Typed slot order makes accumulation deterministic and independent of UI order.
    for (const EquipmentSlot slot : kEquipmentSlots)
    {
        const std::string_view identity = equipment.GetEquipped(slot);
        if (identity.empty())
        {
            continue;
        }
        const GameplayReferenceResolution item = registry.Resolve(
            GameplayDefinitionReference{std::string(identity)},
            GameplayDefinitionCategory::Item);
        if (item.status != GameplayReferenceStatus::Resolved || item.definition == nullptr
            || item.definition->item.type != ItemType::Equipment)
        {
            continue;
        }
        for (const GameplayStatModifier& modifier : item.definition->item.modifiers)
        {
            const std::size_t index = static_cast<std::size_t>(modifier.stat);
            if (index >= kGameplayStatCount || !std::isfinite(modifier.addend))
            {
                continue;
            }
            const float accumulated = result.values[index].equipmentAdditive + modifier.addend;
            if (std::isfinite(accumulated))
            {
                result.values[index].equipmentAdditive = accumulated;
            }
        }
    }

    for (RuntimeCharacterStat& stat : result.values)
    {
        const float effective = stat.base + stat.equipmentAdditive;
        stat.effective = std::isfinite(effective) ? effective : stat.base;
    }
    return result;
}

PlayerMovementParameters ResolvePlayerMovementParameters(const PlayerCharacterStats& stats)
{
    PlayerMovementParameters result;
    const float moveSpeed = stats.Get(GameplayStatId::MoveSpeed).effective;
    const float jumpStrength = stats.Get(GameplayStatId::JumpStrength).effective;
    const float gravityScale = stats.Get(GameplayStatId::GravityScale).effective;
    if (std::isfinite(moveSpeed) && moveSpeed >= 0.0f) result.maxMoveSpeed = moveSpeed;
    if (std::isfinite(jumpStrength) && jumpStrength >= 0.0f) result.jumpSpeed = jumpStrength;
    if (std::isfinite(gravityScale) && gravityScale >= 0.0f) result.gravityScale = gravityScale;
    return result;
}
}
