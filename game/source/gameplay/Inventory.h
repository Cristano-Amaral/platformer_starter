#pragma once

// Application-owned per-run player inventory (Milestone 54).
// Runtime gameplay state only: never authored LevelDefinition data, never
// serialized in Level Format v1, and never a physics or editor concern.
//
// itemId is a canonical identity token, not an ItemDefinition / catalog /
// asset. There is no world pickup in M54.

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
    std::string itemId;
    int quantity = 0;
};

// Canonical itemId:
//   length 1..kMaxItemIdLength
//   first character [a-z]
//   remaining characters [a-z0-9_-]
// Case-sensitive; no folding. Uppercase is rejected, not canonicalized.
bool IsValidItemId(std::string_view itemId);

enum class InventoryLifecycleEvent
{
    NewRun,
    CheckpointRespawn,
    RestartRun,
    PhysicsWorldRebuild,
    ApplyCommittedLevel,
};

class Inventory
{
public:
    int GetQuantity(std::string_view itemId) const;
    bool Has(std::string_view itemId, int quantity) const;
    bool TryAdd(std::string_view itemId, int quantity);
    bool TryRemove(std::string_view itemId, int quantity);
    void Clear();
    // Sorted unique entries. Do not retain the span across a mutation.
    std::span<const InventoryEntry> Entries() const;

private:
    std::vector<InventoryEntry> entries;
};

// Single lifecycle policy used by Application. PhysicsWorld must not call this.
void ApplyInventoryLifecycle(Inventory& inventory, InventoryLifecycleEvent event);
}
