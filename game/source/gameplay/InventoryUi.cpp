#include "gameplay/InventoryUi.h"

#include <cstddef>
#include <span>

namespace gameplay
{
namespace
{
void SelectFirstInventoryStack(InventoryUiState& ui, const Inventory& inventory)
{
    const std::span<const InventoryEntry> entries = inventory.Entries();
    if (entries.empty())
    {
        ui.selectedStackIndex = -1;
        ui.selectedItemId.clear();
        ui.focus = InventoryUiFocusKind::EquipmentSlot;
        ui.equipmentFocus = true;
        ui.selectedSlot = EquipmentSlot::Head;
        return;
    }
    ui.focus = InventoryUiFocusKind::ItemStack;
    ui.equipmentFocus = false;
    ui.selectedStackIndex = 0;
    ui.selectedItemId = entries.front().itemId;
}

int SlotOrder(EquipmentSlot slot)
{
    return static_cast<int>(slot);
}
}

int FindInventoryUiSelectionIndex(
    const Inventory& inventory,
    std::string_view selectedItemId,
    int preferredIndex)
{
    if (selectedItemId.empty())
    {
        return -1;
    }
    const std::span<const InventoryEntry> entries = inventory.Entries();
    if (preferredIndex >= 0 && preferredIndex < static_cast<int>(entries.size())
        && entries[static_cast<std::size_t>(preferredIndex)].itemId == selectedItemId)
    {
        return preferredIndex;
    }
    for (std::size_t index = 0; index < entries.size(); ++index)
    {
        if (entries[index].itemId == selectedItemId)
        {
            return static_cast<int>(index);
        }
    }
    return -1;
}

void RepairInventorySelection(InventoryUiState& ui, const Inventory& inventory)
{
    const std::span<const InventoryEntry> entries = inventory.Entries();
    if (ui.equipmentFocus || ui.focus == InventoryUiFocusKind::EquipmentSlot)
    {
        ui.focus = InventoryUiFocusKind::EquipmentSlot;
        ui.equipmentFocus = true;
        if (!IsValidEquipmentSlot(ui.selectedSlot))
        {
            ui.selectedSlot = EquipmentSlot::Head;
        }
        if (entries.empty())
        {
            ui.selectedStackIndex = -1;
            ui.selectedItemId.clear();
        }
        return;
    }

    if (entries.empty())
    {
        SelectFirstInventoryStack(ui, inventory);
        return;
    }

    const int index = FindInventoryUiSelectionIndex(inventory, ui.selectedItemId, ui.selectedStackIndex);
    if (index >= 0)
    {
        ui.selectedStackIndex = index;
        ui.selectedItemId = entries[static_cast<std::size_t>(index)].itemId;
        ui.focus = InventoryUiFocusKind::ItemStack;
        ui.equipmentFocus = false;
        return;
    }
    ui.selectedStackIndex = 0;
    ui.selectedItemId = entries.front().itemId;
    ui.focus = InventoryUiFocusKind::ItemStack;
    ui.equipmentFocus = false;
}

void OpenInventoryUi(InventoryUiState& ui, const Inventory& inventory)
{
    ui.open = true;
    if (!ui.equipmentFocus)
    {
        ui.focus = InventoryUiFocusKind::ItemStack;
    }
    RepairInventorySelection(ui, inventory);
}

void CloseInventoryUi(InventoryUiState& ui)
{
    ui.open = false;
}

void NavigateInventorySelection(InventoryUiState& ui, const Inventory& inventory, int step)
{
    RepairInventorySelection(ui, inventory);
    if (step == 0)
    {
        return;
    }

    const std::span<const InventoryEntry> entries = inventory.Entries();
    const int stackCount = static_cast<int>(entries.size());
    const int equipmentCount = static_cast<int>(kEquipmentSlotCount);
    const int total = stackCount + equipmentCount;
    if (total <= 0)
    {
        return;
    }

    int cursor = 0;
    if (ui.equipmentFocus || ui.focus == InventoryUiFocusKind::EquipmentSlot)
    {
        cursor = stackCount + SlotOrder(ui.selectedSlot);
    }
    else
    {
        cursor = ui.selectedStackIndex < 0 ? 0 : ui.selectedStackIndex;
    }

    cursor += step;
    while (cursor < 0)
    {
        cursor += total;
    }
    cursor %= total;

    if (cursor < stackCount)
    {
        ui.focus = InventoryUiFocusKind::ItemStack;
        ui.equipmentFocus = false;
        ui.selectedStackIndex = cursor;
        ui.selectedItemId = entries[static_cast<std::size_t>(cursor)].itemId;
        return;
    }

    ui.focus = InventoryUiFocusKind::EquipmentSlot;
    ui.equipmentFocus = true;
    ui.selectedSlot = static_cast<EquipmentSlot>(cursor - stackCount);
    if (stackCount > 0 && ui.selectedStackIndex >= 0 && ui.selectedStackIndex < stackCount)
    {
        ui.selectedItemId = entries[static_cast<std::size_t>(ui.selectedStackIndex)].itemId;
    }
}

InventoryUiInputAction HandleInventoryUiInput(
    InventoryUiState& ui,
    Inventory& inventory,
    Equipment& equipment,
    const GameplayDefinitionRegistry& registry,
    const input::InputState& input)
{
    if (ui.open)
    {
        RepairInventorySelection(ui, inventory);
        if (input.cancelPressed || input.toggleInventoryPressed)
        {
            CloseInventoryUi(ui);
            return InventoryUiInputAction::Close;
        }
        if (input.inventoryPreviousPressed)
        {
            NavigateInventorySelection(ui, inventory, -1);
        }
        else if (input.inventoryNextPressed)
        {
            NavigateInventorySelection(ui, inventory, 1);
        }
        if (input.grabDropPressed)
        {
            if (ui.equipmentFocus || ui.focus == InventoryUiFocusKind::EquipmentSlot)
            {
                const EquipmentTransactionStatus status =
                    equipment.Unequip(inventory, ui.selectedSlot, registry);
                RepairInventorySelection(ui, inventory);
                return status == EquipmentTransactionStatus::Ok
                    ? InventoryUiInputAction::Unequip
                    : InventoryUiInputAction::None;
            }
            if (ui.selectedStackIndex >= 0)
            {
                const std::span<const InventoryEntry> entries = inventory.Entries();
                if (ui.selectedStackIndex < static_cast<int>(entries.size()))
                {
                    const std::string identity = entries[static_cast<std::size_t>(ui.selectedStackIndex)].itemId;
                    const EquipmentTransactionStatus status =
                        equipment.Equip(inventory, identity, registry);
                    RepairInventorySelection(ui, inventory);
                    return status == EquipmentTransactionStatus::Ok
                        ? InventoryUiInputAction::Equip
                        : InventoryUiInputAction::None;
                }
            }
        }
        return InventoryUiInputAction::None;
    }

    if (input.toggleInventoryPressed)
    {
        OpenInventoryUi(ui, inventory);
        return InventoryUiInputAction::Open;
    }
    return InventoryUiInputAction::None;
}

void ApplyInventoryUiLifecycle(
    InventoryUiState& ui,
    InventoryLifecycleEvent event,
    const Inventory& inventory)
{
    switch (event)
    {
    case InventoryLifecycleEvent::NewRun:
    case InventoryLifecycleEvent::RestartRun:
    case InventoryLifecycleEvent::ApplyCommittedLevel:
        CloseInventoryUi(ui);
        ui.selectedItemId.clear();
        ui.selectedStackIndex = -1;
        ui.equipmentFocus = false;
        ui.focus = InventoryUiFocusKind::ItemStack;
        ui.selectedSlot = EquipmentSlot::Head;
        break;
    case InventoryLifecycleEvent::CheckpointRespawn:
    case InventoryLifecycleEvent::PhysicsWorldRebuild:
        RepairInventorySelection(ui, inventory);
        break;
    case InventoryLifecycleEvent::LevelTransition:
        CloseInventoryUi(ui);
        RepairInventorySelection(ui, inventory);
        break;
    }
}
}
