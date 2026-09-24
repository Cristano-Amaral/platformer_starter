#include "gameplay/Equipment.h"

#include "gameplay/ItemDefinition.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace gameplay
{
namespace
{
std::size_t SlotIndex(EquipmentSlot slot)
{
    return static_cast<std::size_t>(slot);
}

EquipmentTransactionStatus ResolveEquipmentItem(
    std::string_view identity,
    const GameplayDefinitionRegistry& registry,
    const GameplayDefinition*& definition)
{
    definition = nullptr;
    ParsedGameplayIdentity parsed;
    if (!TryParseGameplayIdentity(identity, parsed))
    {
        return EquipmentTransactionStatus::MalformedIdentity;
    }
    if (parsed.category != GameplayDefinitionCategory::Item)
    {
        return EquipmentTransactionStatus::WrongCategory;
    }

    const GameplayReferenceResolution resolution = registry.Resolve(
        GameplayDefinitionReference{std::string(identity)},
        GameplayDefinitionCategory::Item);
    if (resolution.status == GameplayReferenceStatus::CategoryMismatch)
    {
        return EquipmentTransactionStatus::WrongCategory;
    }
    if (resolution.status != GameplayReferenceStatus::Resolved || resolution.definition == nullptr)
    {
        return EquipmentTransactionStatus::MissingDefinition;
    }
    if (resolution.definition->item.type != ItemType::Equipment)
    {
        return EquipmentTransactionStatus::NotEquipment;
    }
    if (!resolution.definition->item.equipmentSlot.has_value()
        || !IsValidEquipmentSlot(*resolution.definition->item.equipmentSlot))
    {
        return EquipmentTransactionStatus::SlotMismatch;
    }
    definition = resolution.definition;
    return EquipmentTransactionStatus::Ok;
}
}

bool Equipment::CanEquip(std::string_view identity, const GameplayDefinitionRegistry& registry) const
{
    const GameplayDefinition* definition = nullptr;
    return ResolveEquipmentItem(identity, registry, definition) == EquipmentTransactionStatus::Ok;
}

EquipmentTransactionStatus Equipment::Equip(
    Inventory& inventory,
    std::string_view identity,
    const GameplayDefinitionRegistry& registry)
{
    const GameplayDefinition* definition = nullptr;
    const EquipmentTransactionStatus resolved = ResolveEquipmentItem(identity, registry, definition);
    if (resolved != EquipmentTransactionStatus::Ok || definition == nullptr)
    {
        return resolved;
    }

    const EquipmentSlot slot = *definition->item.equipmentSlot;
    if (!inventory.Has(identity, 1))
    {
        return EquipmentTransactionStatus::MissingInventoryItem;
    }

    const std::string incoming(identity);
    const std::string outgoing{GetEquipped(slot)};
    if (!inventory.TryRemove(incoming, 1))
    {
        return EquipmentTransactionStatus::MissingInventoryItem;
    }

    if (!outgoing.empty())
    {
        if (inventory.TryAdd(outgoing, 1, registry) != InventoryMutationStatus::Ok)
        {
            (void)inventory.TryAdd(incoming, 1, registry);
            return EquipmentTransactionStatus::InventoryRejected;
        }
    }

    slots[SlotIndex(slot)] = incoming;
    return EquipmentTransactionStatus::Ok;
}

EquipmentTransactionStatus Equipment::Unequip(
    Inventory& inventory,
    EquipmentSlot slot,
    const GameplayDefinitionRegistry& registry)
{
    if (!IsValidEquipmentSlot(slot))
    {
        return EquipmentTransactionStatus::SlotMismatch;
    }
    const std::string identity{GetEquipped(slot)};
    if (identity.empty())
    {
        return EquipmentTransactionStatus::EmptySlot;
    }
    if (inventory.TryAdd(identity, 1, registry) != InventoryMutationStatus::Ok)
    {
        return EquipmentTransactionStatus::InventoryRejected;
    }
    slots[SlotIndex(slot)].clear();
    return EquipmentTransactionStatus::Ok;
}

std::string_view Equipment::GetEquipped(EquipmentSlot slot) const
{
    if (!IsValidEquipmentSlot(slot))
    {
        return {};
    }
    return slots[SlotIndex(slot)];
}

void Equipment::Clear()
{
    for (std::string& slot : slots)
    {
        slot.clear();
    }
}

void ApplyEquipmentLifecycle(Equipment& equipment, InventoryLifecycleEvent event)
{
    switch (event)
    {
    case InventoryLifecycleEvent::NewRun:
    case InventoryLifecycleEvent::RestartRun:
    case InventoryLifecycleEvent::ApplyCommittedLevel:
        equipment.Clear();
        break;
    case InventoryLifecycleEvent::CheckpointRespawn:
    case InventoryLifecycleEvent::PhysicsWorldRebuild:
    case InventoryLifecycleEvent::LevelTransition:
        break;
    }
}
}
