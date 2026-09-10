#include "gameplay/InventoryUi.h"

#include <cstddef>
#include <span>

namespace gameplay
{
int FindInventoryUiSelectionIndex(
    const Inventory& inventory,
    std::string_view selectedItemId)
{
    if (selectedItemId.empty())
    {
        return -1;
    }
    const std::span<const InventoryEntry> entries = inventory.Entries();
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
    if (entries.empty())
    {
        ui.selectedItemId.clear();
        return;
    }
    if (FindInventoryUiSelectionIndex(inventory, ui.selectedItemId) >= 0)
    {
        return;
    }
    ui.selectedItemId = entries.front().itemId;
}

void OpenInventoryUi(InventoryUiState& ui, const Inventory& inventory)
{
    ui.open = true;
    RepairInventorySelection(ui, inventory);
}

void CloseInventoryUi(InventoryUiState& ui)
{
    ui.open = false;
}

void NavigateInventorySelection(InventoryUiState& ui, const Inventory& inventory, int step)
{
    RepairInventorySelection(ui, inventory);
    const std::span<const InventoryEntry> entries = inventory.Entries();
    if (entries.empty() || step == 0)
    {
        return;
    }
    const int count = static_cast<int>(entries.size());
    int index = FindInventoryUiSelectionIndex(inventory, ui.selectedItemId);
    if (index < 0)
    {
        return;
    }
    index += step;
    if (index < 0)
    {
        index = count - 1;
    }
    else if (index >= count)
    {
        index = 0;
    }
    ui.selectedItemId = entries[static_cast<std::size_t>(index)].itemId;
}

void HandleInventoryUiInput(
    InventoryUiState& ui,
    const Inventory& inventory,
    const input::InputState& input)
{
    if (ui.open)
    {
        RepairInventorySelection(ui, inventory);
        if (input.cancelPressed || input.toggleInventoryPressed)
        {
            CloseInventoryUi(ui);
            return;
        }
        if (input.inventoryPreviousPressed)
        {
            NavigateInventorySelection(ui, inventory, -1);
        }
        else if (input.inventoryNextPressed)
        {
            NavigateInventorySelection(ui, inventory, 1);
        }
        return;
    }

    if (input.toggleInventoryPressed)
    {
        OpenInventoryUi(ui, inventory);
    }
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
        break;
    case InventoryLifecycleEvent::CheckpointRespawn:
    case InventoryLifecycleEvent::PhysicsWorldRebuild:
        RepairInventorySelection(ui, inventory);
        break;
    }
}
}
