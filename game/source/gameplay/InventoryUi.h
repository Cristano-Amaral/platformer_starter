#pragma once

// Player-facing Inventory UI v1 (Milestone 56). Transient presentation only.
// gameplay::Inventory remains the single source of truth for contents.
// No ImGui, no item-use, no second Inventory storage, no GUIDs.

#include "gameplay/Inventory.h"
#include "input/InputState.h"

#include <string>
#include <string_view>

namespace gameplay
{
inline constexpr bool kPlayerInventoryUiEnabled = true;

struct InventoryUiState
{
    bool open = false;
    // Stable itemId, never a pointer/span/index into Inventory storage.
    std::string selectedItemId;
};

inline bool InventoryUiPausesSimulation(const InventoryUiState& ui)
{
    return ui.open;
}

// True for the whole frame that Inventory is or was open, so closing Tab/Esc
// cannot leak the same-frame E / movement / jump into gameplay.
inline bool InventoryUiBlocksGameplay(bool wasOpen, bool isOpen)
{
    return wasOpen || isOpen;
}

void RepairInventorySelection(InventoryUiState& ui, const Inventory& inventory);
void OpenInventoryUi(InventoryUiState& ui, const Inventory& inventory);
void CloseInventoryUi(InventoryUiState& ui);
void NavigateInventorySelection(InventoryUiState& ui, const Inventory& inventory, int step);
void HandleInventoryUiInput(
    InventoryUiState& ui,
    const Inventory& inventory,
    const input::InputState& input);
void ApplyInventoryUiLifecycle(
    InventoryUiState& ui,
    InventoryLifecycleEvent event,
    const Inventory& inventory);

int FindInventoryUiSelectionIndex(
    const Inventory& inventory,
    std::string_view selectedItemId);
}
