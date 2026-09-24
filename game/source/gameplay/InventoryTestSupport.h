#pragma once

// Shared test fixtures for Inventory/Equipment/Item Pickup (Milestone 100).
// Not shipped.

#include "gameplay/EquipmentSlot.h"
#include "gameplay/GameplayDefinition.h"
#include "gameplay/ItemDefinition.h"

#include <optional>
#include <string>
#include <string_view>

namespace gameplay
{
inline GameplayDefinition MakeTestItemDefinition(
    std::string_view identity,
    ItemType type,
    bool stackable,
    int maxStack,
    std::optional<EquipmentSlot> slot = std::nullopt)
{
    GameplayDefinition definition;
    definition.identity = std::string(identity);
    definition.category = GameplayDefinitionCategory::Item;
    definition.item = MakeDefaultItemDefinition(identity);
    definition.item.type = type;
    definition.item.stackable = stackable;
    definition.item.maxStack = maxStack;
    definition.item.equipmentSlot = slot;
    return definition;
}

inline bool RegisterTestItem(
    GameplayDefinitionRegistry& registry,
    std::string_view identity,
    ItemType type,
    bool stackable,
    int maxStack,
    std::optional<EquipmentSlot> slot = std::nullopt)
{
    return registry.Register(MakeTestItemDefinition(identity, type, stackable, maxStack, slot)).status
        == RegisterGameplayDefinitionStatus::Registered;
}

inline GameplayDefinitionRegistry MakeStandardTestItemRegistry()
{
    GameplayDefinitionRegistry registry;
    RegisterTestItem(registry, "items/master_key", ItemType::Key, false, 1);
    RegisterTestItem(registry, "items/health_potion", ItemType::Consumable, false, 1);
    RegisterTestItem(registry, "items/bau", ItemType::Generic, true, 4);
    RegisterTestItem(registry, "items/coin", ItemType::Generic, true, 99);
    RegisterTestItem(registry, "items/battery", ItemType::Generic, true, 99);
    RegisterTestItem(registry, "items/card", ItemType::Key, false, 1);
    RegisterTestItem(registry, "items/red_key", ItemType::Key, false, 1);
    RegisterTestItem(
        registry, "items/helmet", ItemType::Equipment, false, 1, EquipmentSlot::Head);
    RegisterTestItem(
        registry, "items/armor", ItemType::Equipment, false, 1, EquipmentSlot::Body);
    RegisterTestItem(
        registry, "items/sword", ItemType::Equipment, false, 1, EquipmentSlot::MainHand);
    RegisterTestItem(
        registry, "items/shield", ItemType::Equipment, false, 1, EquipmentSlot::OffHand);
    RegisterTestItem(
        registry, "items/ring", ItemType::Equipment, false, 1, EquipmentSlot::Accessory);
    return registry;
}

inline bool AddTestItem(
    Inventory& inventory,
    const GameplayDefinitionRegistry& registry,
    std::string_view identity,
    int quantity)
{
    return inventory.TryAdd(identity, quantity, registry) == InventoryMutationStatus::Ok;
}
}
