#pragma once

// Presentation view-models for Inventory and Equipment (Milestone 100).
// Resolve ItemDefinition for display. Missing definitions stay explicit.

#include "gameplay/Equipment.h"
#include "gameplay/EquipmentSlot.h"
#include "gameplay/GameplayDefinition.h"
#include "gameplay/Inventory.h"
#include "gameplay/InventoryUi.h"

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace gameplay
{
struct InventorySlotView
{
    std::string identity;
    std::string displayName;
    int quantity = 0;
    std::string iconTextureIdentity;
    bool missingDefinition = false;
    bool canEquip = false;
};

struct EquipmentSlotView
{
    EquipmentSlot slot = EquipmentSlot::Head;
    std::string slotName;
    bool occupied = false;
    std::string identity;
    std::string displayName;
    std::string iconTextureIdentity;
    bool missingDefinition = false;
};

struct InventoryEquipmentView
{
    std::vector<InventorySlotView> stacks;
    std::array<EquipmentSlotView, kEquipmentSlotCount> equipment{};
    int selectedStackIndex = -1;
    bool equipmentFocus = false;
    EquipmentSlot selectedSlot = EquipmentSlot::Head;
};

inline InventorySlotView MakeInventorySlotView(
    const InventoryEntry& entry,
    const GameplayDefinitionRegistry& registry)
{
    InventorySlotView view;
    view.identity = entry.itemId;
    view.quantity = entry.quantity;
    const GameplayReferenceResolution resolution = registry.Resolve(
        GameplayDefinitionReference{entry.itemId},
        GameplayDefinitionCategory::Item);
    if (resolution.status != GameplayReferenceStatus::Resolved || resolution.definition == nullptr)
    {
        view.displayName = entry.itemId;
        view.missingDefinition = true;
        return view;
    }
    view.displayName = resolution.definition->item.displayName.empty()
        ? entry.itemId
        : resolution.definition->item.displayName;
    view.iconTextureIdentity = resolution.definition->item.iconTextureIdentity;
    view.canEquip = resolution.definition->item.type == ItemType::Equipment
        && resolution.definition->item.equipmentSlot.has_value();
    return view;
}

inline EquipmentSlotView MakeEquipmentSlotView(
    EquipmentSlot slot,
    std::string_view identity,
    const GameplayDefinitionRegistry& registry)
{
    EquipmentSlotView view;
    view.slot = slot;
    view.slotName = std::string(EquipmentSlotName(slot));
    if (identity.empty())
    {
        view.displayName = "Empty";
        return view;
    }
    view.occupied = true;
    view.identity.assign(identity);
    const GameplayReferenceResolution resolution = registry.Resolve(
        GameplayDefinitionReference{view.identity},
        GameplayDefinitionCategory::Item);
    if (resolution.status != GameplayReferenceStatus::Resolved || resolution.definition == nullptr)
    {
        view.displayName = view.identity;
        view.missingDefinition = true;
        return view;
    }
    view.displayName = resolution.definition->item.displayName.empty()
        ? view.identity
        : resolution.definition->item.displayName;
    view.iconTextureIdentity = resolution.definition->item.iconTextureIdentity;
    return view;
}

inline InventoryEquipmentView BuildInventoryEquipmentView(
    const Inventory& inventory,
    const Equipment& equipment,
    const InventoryUiState& ui,
    const GameplayDefinitionRegistry& registry)
{
    InventoryEquipmentView view;
    view.selectedStackIndex = ui.selectedStackIndex;
    view.equipmentFocus = ui.equipmentFocus;
    view.selectedSlot = ui.selectedSlot;
    const std::span<const InventoryEntry> entries = inventory.Entries();
    view.stacks.reserve(entries.size());
    for (const InventoryEntry& entry : entries)
    {
        view.stacks.push_back(MakeInventorySlotView(entry, registry));
    }
    for (std::size_t index = 0; index < kEquipmentSlotCount; ++index)
    {
        const EquipmentSlot slot = kEquipmentSlots[index];
        view.equipment[index] = MakeEquipmentSlotView(slot, equipment.GetEquipped(slot), registry);
    }
    return view;
}
}
