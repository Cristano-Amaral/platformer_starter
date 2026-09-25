#include "gameplay/PlayerCharacterStats.h"

#include <cmath>
#include <cstdio>
#include <string_view>

namespace
{
int gFailures = 0;

void Expect(bool condition, std::string_view message)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL: %.*s\n", static_cast<int>(message.size()), message.data());
        ++gFailures;
    }
}

bool Near(float a, float b) { return std::fabs(a - b) < 0.0001f; }

gameplay::GameplayDefinition MakeCharacter()
{
    gameplay::GameplayDefinition definition;
    definition.identity = "characters/player";
    definition.category = gameplay::GameplayDefinitionCategory::Character;
    definition.character = gameplay::MakeDefaultCharacterDefinition(definition.identity);
    definition.character.type = gameplay::CharacterType::Player;
    for (std::size_t index = 0; index < gameplay::kGameplayStatCount; ++index)
    {
        definition.character.hasBaseStat[index] = true;
        definition.character.baseStatValue[index] = static_cast<float>(index + 1);
    }
    return definition;
}

gameplay::GameplayDefinition MakeEquipment(
    std::string_view identity,
    gameplay::EquipmentSlot slot,
    gameplay::GameplayStatId stat,
    float addend)
{
    gameplay::GameplayDefinition definition;
    definition.identity = std::string(identity);
    definition.category = gameplay::GameplayDefinitionCategory::Item;
    definition.item = gameplay::MakeDefaultItemDefinition(identity);
    definition.item.type = gameplay::ItemType::Equipment;
    definition.item.equipmentSlot = slot;
    definition.item.modifiers.push_back({stat, addend});
    return definition;
}

void Register(gameplay::GameplayDefinitionRegistry& registry, const gameplay::GameplayDefinition& definition)
{
    Expect(
        registry.Register(definition).status == gameplay::RegisterGameplayDefinitionStatus::Registered,
        "fixture registers");
}
}

int main()
{
    gameplay::GameplayDefinitionRegistry registry;
    Register(registry, MakeCharacter());
    const auto head = MakeEquipment(
        "items/head", gameplay::EquipmentSlot::Head, gameplay::GameplayStatId::MoveSpeed, 2.0f);
    auto hand = MakeEquipment(
        "items/hand", gameplay::EquipmentSlot::MainHand, gameplay::GameplayStatId::MoveSpeed, 3.0f);
    hand.item.modifiers.push_back({gameplay::GameplayStatId::JumpStrength, 4.0f});
    hand.item.modifiers.push_back({gameplay::GameplayStatId::GravityScale, -0.5f});
    Register(registry, head);
    Register(registry, hand);

    gameplay::Inventory inventory;
    gameplay::Equipment equipment;
    Expect(
        inventory.TryAdd("items/head", 1, registry) == gameplay::InventoryMutationStatus::Ok,
        "head enters inventory");
    auto stats = gameplay::CalculatePlayerCharacterStats("characters/player", registry, equipment);
    Expect(stats.characterResolution == gameplay::GameplayReferenceStatus::Resolved, "player resolves");
    for (std::size_t index = 0; index < gameplay::kGameplayStatCount; ++index)
    {
        Expect(Near(stats.values[index].base, static_cast<float>(index + 1)), "all seven bases consumed");
        Expect(Near(stats.values[index].effective, stats.values[index].base), "inventory-only has no effect");
    }

    Expect(
        equipment.Equip(inventory, "items/head", registry) == gameplay::EquipmentTransactionStatus::Ok,
        "one item equips");
    stats = gameplay::CalculatePlayerCharacterStats("characters/player", registry, equipment);
    Expect(Near(stats.Get(gameplay::GameplayStatId::MoveSpeed).equipmentAdditive, 2.0f), "one modifier");
    Expect(Near(stats.Get(gameplay::GameplayStatId::MoveSpeed).effective, 4.0f), "base plus one modifier");

    Expect(inventory.TryAdd("items/hand", 1, registry) == gameplay::InventoryMutationStatus::Ok, "hand enters inventory");
    Expect(
        equipment.Equip(inventory, "items/hand", registry) == gameplay::EquipmentTransactionStatus::Ok,
        "second item equips");
    stats = gameplay::CalculatePlayerCharacterStats("characters/player", registry, equipment);
    Expect(Near(stats.Get(gameplay::GameplayStatId::MoveSpeed).equipmentAdditive, 5.0f), "same stat accumulates");
    Expect(Near(stats.Get(gameplay::GameplayStatId::JumpStrength).effective, 7.0f), "jump modifier calculates");
    Expect(Near(stats.Get(gameplay::GameplayStatId::GravityScale).effective, 3.5f), "gravity modifier calculates");
    const gameplay::PlayerMovementParameters movement = gameplay::ResolvePlayerMovementParameters(stats);
    Expect(Near(movement.maxMoveSpeed, 7.0f), "MoveSpeed drives runtime parameter");
    Expect(Near(movement.jumpSpeed, 7.0f), "JumpStrength drives runtime parameter");
    Expect(Near(movement.gravityScale, 3.5f), "GravityScale drives runtime parameter");

    Expect(
        equipment.Unequip(inventory, gameplay::EquipmentSlot::Head, registry)
            == gameplay::EquipmentTransactionStatus::Ok,
        "unequip succeeds");
    stats = gameplay::CalculatePlayerCharacterStats("characters/player", registry, equipment);
    Expect(Near(stats.Get(gameplay::GameplayStatId::MoveSpeed).effective, 5.0f), "unequip removes contribution");

    gameplay::Inventory reverseInventory;
    gameplay::Equipment reverseEquipment;
    Expect(reverseInventory.TryAdd("items/hand", 1, registry) == gameplay::InventoryMutationStatus::Ok, "reverse hand add");
    Expect(reverseInventory.TryAdd("items/head", 1, registry) == gameplay::InventoryMutationStatus::Ok, "reverse head add");
    Expect(reverseEquipment.Equip(reverseInventory, "items/hand", registry) == gameplay::EquipmentTransactionStatus::Ok, "reverse hand equip");
    Expect(reverseEquipment.Equip(reverseInventory, "items/head", registry) == gameplay::EquipmentTransactionStatus::Ok, "reverse head equip");
    const auto deterministic = gameplay::CalculatePlayerCharacterStats("characters/player", registry, reverseEquipment);
    Expect(Near(deterministic.Get(gameplay::GameplayStatId::MoveSpeed).effective, 7.0f), "slot calculation deterministic");

    Expect(
        gameplay::CalculatePlayerCharacterStats("characters/missing", registry, equipment).characterResolution
            == gameplay::GameplayReferenceStatus::Missing,
        "missing Player reference diagnosed");
    const auto malformed = gameplay::CalculatePlayerCharacterStats("not-an-identity", registry, equipment);
    Expect(malformed.characterResolution == gameplay::GameplayReferenceStatus::Malformed, "malformed Player reference diagnosed");
    Expect(Near(malformed.Get(gameplay::GameplayStatId::MoveSpeed).base, 6.0f), "malformed Player uses legacy fallback");
    Expect(
        gameplay::CalculatePlayerCharacterStats("items/head", registry, equipment).characterResolution
            == gameplay::GameplayReferenceStatus::CategoryMismatch,
        "category-mismatched Player reference diagnosed");

    registry.Remove("items/hand");
    stats = gameplay::CalculatePlayerCharacterStats("characters/player", registry, equipment);
    Expect(Near(stats.Get(gameplay::GameplayStatId::MoveSpeed).equipmentAdditive, 0.0f), "missing equipped item ignored safely");
    Expect(Near(stats.Get(gameplay::GameplayStatId::Defense).effective, 6.0f), "unrelated stat remains intact");

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d PlayerCharacterStatsTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("PlayerCharacterStatsTest passed\n");
    return 0;
}
