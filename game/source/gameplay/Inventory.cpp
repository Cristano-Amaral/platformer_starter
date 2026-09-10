#include "gameplay/Inventory.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace gameplay
{
namespace
{
bool QuantityInRange(int quantity)
{
    return quantity >= 1 && quantity <= kMaxItemQuantity;
}

// Insertion point in the lexicographically sorted unique-id vector.
std::size_t LowerBoundIndex(
    const std::vector<InventoryEntry>& entries,
    std::string_view itemId)
{
    std::size_t index = 0;
    while (index < entries.size() && entries[index].itemId < itemId)
    {
        ++index;
    }
    return index;
}
}

int Inventory::GetQuantity(std::string_view itemId) const
{
    if (!IsValidItemId(itemId))
    {
        return 0;
    }
    const std::size_t index = LowerBoundIndex(entries, itemId);
    if (index == entries.size() || entries[index].itemId != itemId)
    {
        return 0;
    }
    return entries[index].quantity;
}

bool Inventory::Has(std::string_view itemId, int quantity) const
{
    if (!IsValidItemId(itemId) || !QuantityInRange(quantity))
    {
        return false;
    }
    return GetQuantity(itemId) >= quantity;
}

bool Inventory::TryAdd(std::string_view itemId, int quantity)
{
    if (!IsValidItemId(itemId) || !QuantityInRange(quantity))
    {
        return false;
    }
    const std::size_t index = LowerBoundIndex(entries, itemId);
    if (index == entries.size() || entries[index].itemId != itemId)
    {
        entries.insert(entries.begin() + static_cast<std::ptrdiff_t>(index),
            InventoryEntry{std::string(itemId), quantity});
        return true;
    }
    if (entries[index].quantity > kMaxItemQuantity - quantity)
    {
        return false;
    }
    entries[index].quantity += quantity;
    return true;
}

bool Inventory::TryRemove(std::string_view itemId, int quantity)
{
    if (!IsValidItemId(itemId) || !QuantityInRange(quantity))
    {
        return false;
    }
    const std::size_t index = LowerBoundIndex(entries, itemId);
    if (index == entries.size() || entries[index].itemId != itemId)
    {
        return false;
    }
    if (entries[index].quantity < quantity)
    {
        return false;
    }
    if (entries[index].quantity == quantity)
    {
        entries.erase(entries.begin() + static_cast<std::ptrdiff_t>(index));
        return true;
    }
    entries[index].quantity -= quantity;
    return true;
}

void Inventory::Clear()
{
    entries.clear();
}

std::span<const InventoryEntry> Inventory::Entries() const
{
    return entries;
}

void ApplyInventoryLifecycle(Inventory& inventory, InventoryLifecycleEvent event)
{
    switch (event)
    {
    case InventoryLifecycleEvent::NewRun:
    case InventoryLifecycleEvent::RestartRun:
    case InventoryLifecycleEvent::ApplyCommittedLevel:
        inventory.Clear();
        break;
    case InventoryLifecycleEvent::CheckpointRespawn:
    case InventoryLifecycleEvent::PhysicsWorldRebuild:
        break;
    }
}
}
