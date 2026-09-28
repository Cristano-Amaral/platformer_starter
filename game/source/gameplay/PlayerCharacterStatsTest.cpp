#include "gameplay/PlayerCharacterStats.h"

#include "gameplay/CharacterHealth.h"
#include <limits>
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
    gameplay::RuntimeHealth flash(100);
    flash.ApplyDamage({10});
    Expect(flash.DamageFeedbackActive(), "accepted positive reduction starts feedback");
    flash.AdvanceDamageFeedback(0.125f);
    flash.ApplyDamage({10});
    Expect(flash.DamageFeedbackRemaining() == gameplay::kDamageFeedbackSeconds, "hit restarts bounded window");
    flash.AdvanceDamageFeedback(gameplay::kDamageFeedbackSeconds);
    Expect(!flash.DamageFeedbackActive(), "feedback expires");
    flash.ApplyHealing({1});
    flash.SetMaximum(120);
    for (float invalid : {0.0f, -1.0f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()})
        flash.ApplyDamage({invalid});
    Expect(!flash.DamageFeedbackActive(), "heal maximum sync and invalid damage never flash");
    flash.ApplyDamage({1000});
    flash.AdvanceDamageFeedback(gameplay::kDamageFeedbackSeconds);
    flash.ApplyDamage({1});
    Expect(flash.Defeated() && !flash.DamageFeedbackActive(), "zero applied damage cannot refresh defeated feedback");
    flash.Reset();
    Expect(!flash.DamageFeedbackActive() && !flash.Defeated(), "reset clears feedback");
    // M114 bounded operations, including float extremes and invalid inputs.
    gameplay::RuntimeHealth health(80.5f);
    Expect(health.Current() == 80.5f && health.Maximum() == 80.5f && !health.Depleted(), "health starts at maximum");
    auto operation = health.ApplyDamage({20.25f});
    Expect(operation.accepted && operation.before == 80.5f && operation.after == 60.25f
        && operation.applied == 20.25f, "typed damage reports actual reduction");
    for (float invalid : {0.0f, -1.0f, std::numeric_limits<float>::infinity(),
            -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()})
    {
        Expect(!health.ApplyDamage({invalid}).accepted && !health.ApplyHealing({invalid}).accepted
            && health.Current() == 60.25f, "invalid direct operations are deterministic no-ops");
        gameplay::RuntimeHealth fallback(invalid);
        Expect(fallback.Current() == gameplay::kFallbackMaxHealth, "invalid maximum uses bounded fallback");
    }
    operation = health.ApplyDamage({std::numeric_limits<float>::max()});
    Expect(operation.applied == 60.25f && health.Current() == 0 && health.Depleted(), "damage clamps to depleted");
    Expect(health.ApplyDamage({1}).applied == 0, "depleted damage cannot underflow");
    operation = health.ApplyHealing({std::numeric_limits<float>::max()});
    Expect(!operation.accepted && health.Current() == 0 && health.Defeated(), "healing cannot revive defeated health");
    health.Reset();
    Expect(!health.Defeated(), "reset restores Alive");
    health.ApplyDamage({1});
    operation = health.ApplyHealing({std::numeric_limits<float>::max()});
    Expect(operation.applied == 1 && health.Current() == 80.5f, "Alive healing clamps at maximum");
    Expect(health.ApplyHealing({1}).applied == 0, "full health cannot overflow");
    health.SetMaximum(120);
    Expect(health.Current() == 80.5f, "increased maximum does not refill");
    health.SetMaximum(40);
    Expect(health.Current() == 40, "decreased maximum clamps");
    health.ApplyDamage({10}); health.Reset();
    Expect(health.Current() == 40, "reset fills current maximum");
    health.SetMaximum(std::numeric_limits<float>::quiet_NaN());
    Expect(health.Maximum() == gameplay::kFallbackMaxHealth && health.Current() == 40,
        "invalid maximum uses fallback without refilling existing current health");
    gameplay::RuntimeHealth extreme(std::numeric_limits<float>::max());
    extreme.ApplyDamage({std::numeric_limits<float>::max() / 2});
    extreme.ApplyHealing({std::numeric_limits<float>::max()});
    Expect(std::isfinite(extreme.Current()) && extreme.Current() == extreme.Maximum(), "large finite healing never stores infinity");
    gameplay::CharacterDefinition absent;
    Expect(gameplay::ResolveCharacterMaxHealth(absent) == gameplay::kFallbackMaxHealth, "missing Character maximum uses shared fallback");
    absent.hasBaseStat[0] = true;
    absent.baseStatValue[0] = 0;
    Expect(gameplay::ResolveCharacterMaxHealth(absent) == gameplay::kFallbackMaxHealth, "authored zero uses shared fallback");

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

    const auto healthItem = MakeEquipment("items/health", gameplay::EquipmentSlot::Head,
        gameplay::GameplayStatId::MaxHealth, 30.5f);
    Register(registry, healthItem);
    gameplay::Equipment healthEquipment;
    gameplay::Inventory healthInventory;
    gameplay::RuntimeHealth playerHealth(gameplay::CalculatePlayerCharacterStats(
        "characters/player", registry, healthEquipment).Get(gameplay::GameplayStatId::MaxHealth).effective);
    Expect(playerHealth.Maximum() == 1, "Player health consumes existing effective MaxHealth");
    playerHealth.ApplyDamage({0.5f});
    healthInventory.TryAdd("items/health", 1, registry);
    Expect(healthEquipment.Equip(healthInventory, "items/health", registry) == gameplay::EquipmentTransactionStatus::Ok,
        "health modifier equips through real inventory authority");
    playerHealth.SetMaximum(gameplay::CalculatePlayerCharacterStats("characters/player", registry, healthEquipment)
        .Get(gameplay::GameplayStatId::MaxHealth).effective);
    Expect(playerHealth.Maximum() == 31.5f && playerHealth.Current() == 0.5f, "equipment increases maximum without free refill");
    playerHealth.ApplyHealing({20});
    healthEquipment.Unequip(healthInventory, gameplay::EquipmentSlot::Head, registry);
    playerHealth.SetMaximum(gameplay::CalculatePlayerCharacterStats("characters/player", registry, healthEquipment)
        .Get(gameplay::GameplayStatId::MaxHealth).effective);
    Expect(playerHealth.Maximum() == 1 && playerHealth.Current() == 1, "unequip immediately clamps current to effective maximum");
    Expect(gameplay::CalculatePlayerCharacterStats("characters/missing", registry, healthEquipment)
        .Get(gameplay::GameplayStatId::MaxHealth).effective == 0, "M102 missing MaxHealth base remains zero");
    gameplay::RuntimeHealth missingPlayerHealth(gameplay::CalculatePlayerCharacterStats(
        "characters/missing", registry, healthEquipment).Get(gameplay::GameplayStatId::MaxHealth).effective);
    Expect(missingPlayerHealth.Maximum() == gameplay::kFallbackMaxHealth, "unusable effective Player maximum uses shared legacy health fallback");

    healthEquipment.Equip(healthInventory, "items/health", registry);
    const auto missingEquipped = gameplay::CalculatePlayerCharacterStats("characters/missing", registry, healthEquipment);
    Expect(missingEquipped.Get(gameplay::GameplayStatId::MaxHealth).effective == 30.5f
        && gameplay::RuntimeHealth(missingEquipped.Get(gameplay::GameplayStatId::MaxHealth).effective).Maximum() == 30.5f,
        "missing Player base preserves M102 additive behavior and consumes positive effective maximum directly");

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
