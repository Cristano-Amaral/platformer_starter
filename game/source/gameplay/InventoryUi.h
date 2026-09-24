#pragma once

// Player-facing Inventory UI v1 (Milestone 56 / 100). Transient presentation
// only. gameplay::Inventory remains the source of truth for contents.
// No ImGui, no item-use, no second Inventory storage, no GUIDs.

#include "gameplay/Equipment.h"
#include "gameplay/EquipmentSlot.h"
#include "gameplay/GameplayDefinition.h"
#include "gameplay/Inventory.h"
#include "input/InputState.h"

#include <string>
#include <string_view>

namespace gameplay
{
inline constexpr bool kPlayerInventoryUiEnabled = true;

enum class InventoryUiFocusKind
{
    ItemStack,
    EquipmentSlot,
};

struct InventoryUiState
{
    bool open = false;
    InventoryUiFocusKind focus = InventoryUiFocusKind::ItemStack;
    int selectedStackIndex = -1;
    // Stable identity for the selected stack, never a pointer into storage.
    std::string selectedItemId;
    EquipmentSlot selectedSlot = EquipmentSlot::Head;
    bool equipmentFocus = false;
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

enum class InventoryUiInputAction
{
    None,
    Open,
    Close,
    Equip,
    Unequip,
};

InventoryUiInputAction HandleInventoryUiInput(
    InventoryUiState& ui,
    Inventory& inventory,
    Equipment& equipment,
    const GameplayDefinitionRegistry& registry,
    const input::InputState& input);
void ApplyInventoryUiLifecycle(
    InventoryUiState& ui,
    InventoryLifecycleEvent event,
    const Inventory& inventory);

int FindInventoryUiSelectionIndex(
    const Inventory& inventory,
    std::string_view selectedItemId,
    int preferredIndex = -1);
}
