#pragma once

// Application-owned per-run Item Pickup availability. Not authored, not
// physics-owned, not serialized. Collection uses gameplay::Inventory::TryAdd.

#include "core/Vec3.h"
#include "gameplay/Inventory.h"
#include "world/ItemPickup.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace gameplay
{
inline constexpr int kNoItemPickupIndex = -1;

// Same world-scale targeting as M51 Dynamic Box grab.
inline constexpr float kItemPickupMaxDistance = 2.5f;
inline constexpr float kItemPickupMinFacingDot = 0.35f;
inline constexpr float kItemPickupMinDistance = 0.05f;
inline constexpr float kItemPickupLosBlockFraction = 0.98f;

struct ItemPickupRunState
{
    std::vector<std::uint8_t> collected{};
};

inline ItemPickupRunState MakeClearedItemPickupRunState(std::size_t count)
{
    ItemPickupRunState state{};
    state.collected.assign(count, 0);
    return state;
}

inline bool ItemPickupIsAvailable(const ItemPickupRunState& state, std::size_t index)
{
    return index < state.collected.size() && state.collected[index] == 0;
}

// Nearest in-range in-front available pickup. Lower session index wins when
// distances are within 1e-5. losBlocked[i] != 0 skips that index.
int FindItemPickupTargetIndex(
    core::Vec3 playerCenter,
    float facingX,
    std::span<const world::ItemPickupSpec> pickups,
    std::span<const std::uint8_t> collected,
    std::span<const std::uint8_t> losBlocked);

// TryAdd first. Only on success mark collected. No partial quantity.
bool TryCollectItemPickup(
    Inventory& inventory,
    ItemPickupRunState& runState,
    std::span<const world::ItemPickupSpec> pickups,
    int index);
}
