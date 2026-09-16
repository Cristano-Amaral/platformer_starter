#pragma once

// Milestone 62: Item Pickup collection HUD notification. Runtime-only, not
// authored, not serialized, not a generic toast/notification/event system.
// Spawn only after the existing TryCollectItemPickup path succeeds.

#include "gameplay/Inventory.h"
#include "world/ItemPickup.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string_view>

namespace gameplay
{
inline constexpr int kItemPickupCollectionHudCapacity = 4;
inline constexpr float kItemPickupCollectionHudLifetimeSeconds = 2.0f;
inline constexpr float kItemPickupCollectionHudHoldSeconds = 1.5f;
inline constexpr float kItemPickupCollectionHudFadeSeconds =
    kItemPickupCollectionHudLifetimeSeconds - kItemPickupCollectionHudHoldSeconds;
inline constexpr std::size_t kItemPickupCollectionHudTextCapacity = 64;

struct ItemPickupCollectionHudEntry
{
    char itemId[kMaxItemIdLength + 1]{};
    int quantity = 0;
    float ageSeconds = 0.0f;
    bool active = false;
};

struct ItemPickupCollectionHudState
{
    ItemPickupCollectionHudEntry entries[kItemPickupCollectionHudCapacity]{};
    int count = 0;
    int emittedCount = 0;
};

inline void ClearItemPickupCollectionHud(ItemPickupCollectionHudState& state)
{
    state = ItemPickupCollectionHudState{};
}

inline int ActiveItemPickupCollectionHudCount(const ItemPickupCollectionHudState& state)
{
    return state.count;
}

inline void FormatItemPickupCollectionHudText(
    char* buffer,
    std::size_t bufferSize,
    std::string_view itemId,
    int quantity)
{
    if (buffer == nullptr || bufferSize == 0)
    {
        return;
    }
    const int itemLength = static_cast<int>(itemId.size());
    std::snprintf(
        buffer,
        bufferSize,
        "Picked Up %.*s x%d",
        itemLength,
        itemId.data(),
        quantity);
}

inline void CopyItemPickupCollectionHudItemId(
    ItemPickupCollectionHudEntry& entry,
    std::string_view itemId)
{
    std::size_t copied = 0;
    const std::size_t limit = itemId.size() < kMaxItemIdLength ? itemId.size() : kMaxItemIdLength;
    for (; copied < limit; ++copied)
    {
        entry.itemId[copied] = itemId[copied];
    }
    entry.itemId[copied] = '\0';
}

inline void DropOldestItemPickupCollectionHud(ItemPickupCollectionHudState& state)
{
    if (state.count <= 0)
    {
        return;
    }
    for (int index = 0; index < state.count - 1; ++index)
    {
        state.entries[index] = state.entries[index + 1];
    }
    --state.count;
    state.entries[state.count] = ItemPickupCollectionHudEntry{};
}

inline void CompactItemPickupCollectionHud(ItemPickupCollectionHudState& state)
{
    int write = 0;
    for (int read = 0; read < state.count; ++read)
    {
        if (!state.entries[read].active)
        {
            continue;
        }
        if (write != read)
        {
            state.entries[write] = state.entries[read];
        }
        ++write;
    }
    for (int index = write; index < state.count; ++index)
    {
        state.entries[index] = ItemPickupCollectionHudEntry{};
    }
    state.count = write;
}

inline void UpdateItemPickupCollectionHud(
    ItemPickupCollectionHudState& state,
    float deltaSeconds)
{
    if (!(deltaSeconds > 0.0f) || state.count <= 0)
    {
        return;
    }
    for (int index = 0; index < state.count; ++index)
    {
        ItemPickupCollectionHudEntry& entry = state.entries[index];
        if (!entry.active)
        {
            continue;
        }
        entry.ageSeconds += deltaSeconds;
        if (entry.ageSeconds >= kItemPickupCollectionHudLifetimeSeconds)
        {
            entry = ItemPickupCollectionHudEntry{};
        }
    }
    CompactItemPickupCollectionHud(state);
}

inline bool SpawnItemPickupCollectionHud(
    ItemPickupCollectionHudState& state,
    std::string_view itemId,
    int quantity)
{
    if (state.count >= kItemPickupCollectionHudCapacity)
    {
        DropOldestItemPickupCollectionHud(state);
    }
    ItemPickupCollectionHudEntry& entry = state.entries[state.count];
    entry = ItemPickupCollectionHudEntry{};
    CopyItemPickupCollectionHudItemId(entry, itemId);
    entry.quantity = quantity;
    entry.ageSeconds = 0.0f;
    entry.active = true;
    ++state.count;
    ++state.emittedCount;
    return true;
}

inline bool SpawnItemPickupCollectionHud(
    ItemPickupCollectionHudState& state,
    const world::ItemPickupSpec& collectedPickup)
{
    return SpawnItemPickupCollectionHud(state, collectedPickup.itemId, collectedPickup.quantity);
}

inline float ItemPickupCollectionHudFadeAmount(const ItemPickupCollectionHudEntry& entry)
{
    if (!entry.active)
    {
        return 1.0f;
    }
    if (entry.ageSeconds <= kItemPickupCollectionHudHoldSeconds)
    {
        return 0.0f;
    }
    constexpr float fadeSeconds = kItemPickupCollectionHudFadeSeconds;
    static_assert(fadeSeconds > 0.0f, "fade window must be positive");
    const float fade =
        (entry.ageSeconds - kItemPickupCollectionHudHoldSeconds) / fadeSeconds;
    if (fade < 0.0f)
    {
        return 0.0f;
    }
    if (fade > 1.0f)
    {
        return 1.0f;
    }
    return fade;
}

inline unsigned char ItemPickupCollectionHudAlpha(const ItemPickupCollectionHudEntry& entry)
{
    const float remain = 1.0f - ItemPickupCollectionHudFadeAmount(entry);
    const long rounded = std::lround(static_cast<double>(remain) * 255.0);
    if (rounded <= 0)
    {
        return 0;
    }
    if (rounded >= 255)
    {
        return 255;
    }
    return static_cast<unsigned char>(rounded);
}
}
