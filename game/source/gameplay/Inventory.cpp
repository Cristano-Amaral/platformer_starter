#include "gameplay/Inventory.h"

#include "gameplay/ItemDefinition.h"

#include <algorithm>
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

int EffectiveMaxStack(const ItemDefinition& item)
{
    if (!item.stackable)
    {
        return kMinItemMaxStack;
    }
    return item.maxStack;
}

std::size_t FirstStackIndex(const std::vector<InventoryEntry>& entries, std::string_view itemId)
{
    std::size_t index = 0;
    while (index < entries.size() && entries[index].itemId < itemId)
    {
        ++index;
    }
    return index;
}

std::size_t StackCountForIdentity(
    const std::vector<InventoryEntry>& entries,
    std::size_t first,
    std::string_view itemId)
{
    std::size_t count = 0;
    while (first + count < entries.size() && entries[first + count].itemId == itemId)
    {
        ++count;
    }
    return count;
}

const GameplayDefinition* ResolveInventoryItem(
    std::string_view itemId,
    const GameplayDefinitionRegistry& registry,
    InventoryMutationStatus& status)
{
    ParsedGameplayIdentity parsed;
    if (!TryParseGameplayIdentity(itemId, parsed))
    {
        status = InventoryMutationStatus::MalformedIdentity;
        return nullptr;
    }
    if (parsed.category != GameplayDefinitionCategory::Item)
    {
        status = InventoryMutationStatus::WrongCategory;
        return nullptr;
    }

    const GameplayReferenceResolution resolution = registry.Resolve(
        GameplayDefinitionReference{std::string(itemId)},
        GameplayDefinitionCategory::Item);
    if (resolution.status == GameplayReferenceStatus::CategoryMismatch)
    {
        status = InventoryMutationStatus::WrongCategory;
        return nullptr;
    }
    if (resolution.status != GameplayReferenceStatus::Resolved || resolution.definition == nullptr)
    {
        status = InventoryMutationStatus::MissingDefinition;
        return nullptr;
    }
    if (ValidateItemDefinition(resolution.definition->item) != ValidateItemStatus::Valid)
    {
        status = InventoryMutationStatus::InvalidDefinition;
        return nullptr;
    }
    status = InventoryMutationStatus::Ok;
    return resolution.definition;
}
}

int Inventory::GetQuantity(std::string_view itemId) const
{
    ParsedGameplayIdentity parsed;
    if (!TryParseGameplayIdentity(itemId, parsed)
        || parsed.category != GameplayDefinitionCategory::Item)
    {
        return 0;
    }
    int total = 0;
    const std::size_t first = FirstStackIndex(entries, itemId);
    const std::size_t count = StackCountForIdentity(entries, first, itemId);
    for (std::size_t index = 0; index < count; ++index)
    {
        total += entries[first + index].quantity;
    }
    return total;
}

bool Inventory::Has(std::string_view itemId, int quantity) const
{
    if (!QuantityInRange(quantity))
    {
        return false;
    }
    return GetQuantity(itemId) >= quantity;
}

InventoryMutationStatus Inventory::TryAdd(
    std::string_view itemId,
    int quantity,
    const GameplayDefinitionRegistry& registry)
{
    if (!QuantityInRange(quantity))
    {
        return InventoryMutationStatus::InvalidQuantity;
    }

    InventoryMutationStatus status = InventoryMutationStatus::MalformedIdentity;
    const GameplayDefinition* definition = ResolveInventoryItem(itemId, registry, status);
    if (definition == nullptr)
    {
        return status;
    }

    const std::string identity(itemId);
    const int current = GetQuantity(identity);
    if (current > kMaxItemQuantity - quantity)
    {
        return InventoryMutationStatus::QuantityOverflow;
    }

    const int maxStack = EffectiveMaxStack(definition->item);
    std::vector<InventoryEntry> next = entries;
    const std::size_t first = FirstStackIndex(next, identity);
    std::size_t count = StackCountForIdentity(next, first, identity);
    int remaining = quantity;

    for (std::size_t index = 0; index < count && remaining > 0; ++index)
    {
        InventoryEntry& stack = next[first + index];
        const int room = maxStack - stack.quantity;
        if (room <= 0)
        {
            continue;
        }
        const int added = remaining < room ? remaining : room;
        stack.quantity += added;
        remaining -= added;
    }

    std::size_t insertAt = first + count;
    while (remaining > 0)
    {
        const int added = remaining < maxStack ? remaining : maxStack;
        next.insert(
            next.begin() + static_cast<std::ptrdiff_t>(insertAt),
            InventoryEntry{identity, added});
        remaining -= added;
        ++insertAt;
    }

    entries = std::move(next);
    return InventoryMutationStatus::Ok;
}

bool Inventory::TryRemove(std::string_view itemId, int quantity)
{
    if (!QuantityInRange(quantity))
    {
        return false;
    }
    ParsedGameplayIdentity parsed;
    if (!TryParseGameplayIdentity(itemId, parsed)
        || parsed.category != GameplayDefinitionCategory::Item)
    {
        return false;
    }
    if (GetQuantity(itemId) < quantity)
    {
        return false;
    }

    std::vector<InventoryEntry> next = entries;
    const std::size_t first = FirstStackIndex(next, itemId);
    std::size_t count = StackCountForIdentity(next, first, itemId);
    int remaining = quantity;
    for (std::size_t index = 0; index < count && remaining > 0; ++index)
    {
        InventoryEntry& stack = next[first + index];
        if (stack.quantity > remaining)
        {
            stack.quantity -= remaining;
            remaining = 0;
            break;
        }
        remaining -= stack.quantity;
        stack.quantity = 0;
    }

    next.erase(
        std::remove_if(
            next.begin(),
            next.end(),
            [](const InventoryEntry& entry) { return entry.quantity <= 0; }),
        next.end());
    entries = std::move(next);
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
    case InventoryLifecycleEvent::LevelTransition:
        break;
    }
}
}
