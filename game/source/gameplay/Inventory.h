#pragma once

// Application-owned per-run player inventory (Milestone 54 / 100).
// Runtime gameplay state only: never authored LevelDefinition data, never
// serialized in Level Format v1, and never a physics or editor concern.
//
// Stack identity is the M98/M99 textual ItemDefinition identity items/<name>.
// Authored metadata lives on ItemDefinition, not on Inventory stacks.
// Legacy M54 short itemId tokens are not a second Item authority.

#include "gameplay/GameplayDefinition.h"
#include "gameplay/GameplayIdentity.h"

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gameplay
{
inline constexpr std::size_t kMaxItemIdLength = 32;
inline constexpr int kMaxItemQuantity = 1'000'000;

// Development inspection UI may compile only when authoring tools are on
// (Development). Production Inventory remains available in every config.
#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
inline constexpr bool kInventoryDevelopmentHarnessEnabled = true;
#else
inline constexpr bool kInventoryDevelopmentHarnessEnabled = false;
#endif

struct InventoryEntry
{
    // Durable Item identity: items/<name>.
    std::string itemId;
    int quantity = 0;
};

// Legacy M54 itemId grammar, retained only to recognize compatibility tokens.
// Inventory add/remove requires items/<name>.
inline bool IsValidItemId(std::string_view itemId)
{
    if (itemId.empty() || itemId.size() > kMaxItemIdLength)
    {
        return false;
    }
    const char first = itemId.front();
    if (first < 'a' || first > 'z')
    {
        return false;
    }
    for (const char character : itemId)
    {
        const bool ok = (character >= 'a' && character <= 'z')
            || (character >= '0' && character <= '9') || character == '_' || character == '-';
        if (!ok)
        {
            return false;
        }
    }
    return true;
}

enum class InventoryLifecycleEvent
{
    NewRun,
    CheckpointRespawn,
    RestartRun,
    PhysicsWorldRebuild,
    ApplyCommittedLevel,
    LevelTransition,
};

enum class InventoryMutationStatus
{
    Ok,
    InvalidQuantity,
    QuantityOverflow,
    MalformedIdentity,
    MissingDefinition,
    WrongCategory,
    InvalidDefinition,
};

inline const char* InventoryMutationStatusName(InventoryMutationStatus status)
{
    switch (status)
    {
    case InventoryMutationStatus::Ok:
        return "Ok";
    case InventoryMutationStatus::InvalidQuantity:
        return "InvalidQuantity";
    case InventoryMutationStatus::QuantityOverflow:
        return "QuantityOverflow";
    case InventoryMutationStatus::MalformedIdentity:
        return "MalformedIdentity";
    case InventoryMutationStatus::MissingDefinition:
        return "MissingDefinition";
    case InventoryMutationStatus::WrongCategory:
        return "WrongCategory";
    case InventoryMutationStatus::InvalidDefinition:
        return "InvalidDefinition";
    }
    return "MalformedIdentity";
}

class Inventory
{
public:
    int GetQuantity(std::string_view itemId) const;
    bool Has(std::string_view itemId, int quantity) const;
    InventoryMutationStatus TryAdd(
        std::string_view itemId,
        int quantity,
        const GameplayDefinitionRegistry& registry);
    bool TryRemove(std::string_view itemId, int quantity);
    void Clear();
    // Stacks grouped by identity (lexicographic), fill order within identity.
    // Do not retain the span across a mutation.
    std::span<const InventoryEntry> Entries() const;

private:
    std::vector<InventoryEntry> entries;
};

// Single lifecycle policy used by Application. PhysicsWorld must not call this.
void ApplyInventoryLifecycle(Inventory& inventory, InventoryLifecycleEvent event);
}
