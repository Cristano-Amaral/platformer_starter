#pragma once

// Typed player Equipment state (Milestone 100). Owns Item identities only.
// M102 resolves modifiers from these identities; Equipment still owns no metadata.

#include "gameplay/EquipmentSlot.h"
#include "gameplay/GameplayDefinition.h"
#include "gameplay/Inventory.h"

#include <array>
#include <string>
#include <string_view>

namespace gameplay
{
enum class EquipmentTransactionStatus
{
    Ok,
    MalformedIdentity,
    MissingDefinition,
    WrongCategory,
    NotEquipment,
    SlotMismatch,
    MissingInventoryItem,
    InventoryRejected,
    EmptySlot,
};

inline const char* EquipmentTransactionStatusName(EquipmentTransactionStatus status)
{
    switch (status)
    {
    case EquipmentTransactionStatus::Ok:
        return "Ok";
    case EquipmentTransactionStatus::MalformedIdentity:
        return "MalformedIdentity";
    case EquipmentTransactionStatus::MissingDefinition:
        return "MissingDefinition";
    case EquipmentTransactionStatus::WrongCategory:
        return "WrongCategory";
    case EquipmentTransactionStatus::NotEquipment:
        return "NotEquipment";
    case EquipmentTransactionStatus::SlotMismatch:
        return "SlotMismatch";
    case EquipmentTransactionStatus::MissingInventoryItem:
        return "MissingInventoryItem";
    case EquipmentTransactionStatus::InventoryRejected:
        return "InventoryRejected";
    case EquipmentTransactionStatus::EmptySlot:
        return "EmptySlot";
    }
    return "MalformedIdentity";
}

class Equipment
{
public:
    bool CanEquip(std::string_view identity, const GameplayDefinitionRegistry& registry) const;
    EquipmentTransactionStatus Equip(
        Inventory& inventory,
        std::string_view identity,
        const GameplayDefinitionRegistry& registry);
    EquipmentTransactionStatus Unequip(
        Inventory& inventory,
        EquipmentSlot slot,
        const GameplayDefinitionRegistry& registry);
    std::string_view GetEquipped(EquipmentSlot slot) const;
    void Clear();

private:
    std::array<std::string, kEquipmentSlotCount> slots{};
};

void ApplyEquipmentLifecycle(Equipment& equipment, InventoryLifecycleEvent event);
}
