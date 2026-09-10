#pragma once

// Authored Item Pickup (Milestone 55). World acquisition only: position,
// M54 itemId, quantity, optional Static Model identity. Runtime collected
// state is never stored here or in Level Format.

#include "core/Vec3.h"
#include "gameplay/Inventory.h"
#include "world/StaticProp.h"

#include <cmath>
#include <string>
#include <string_view>

namespace world
{
inline constexpr int kLevel01ItemPickupCount = 0;
inline constexpr float kItemPickupVisualSize = 0.5f;
inline constexpr core::Vec3 kItemPickupVisualExtents{
    kItemPickupVisualSize,
    kItemPickupVisualSize,
    kItemPickupVisualSize};
inline constexpr int kDefaultItemPickupQuantity = 1;
inline constexpr std::string_view kDefaultItemPickupId = "key";

struct ItemPickupSpec
{
    core::Vec3 position{};
    std::string itemId{kDefaultItemPickupId};
    int quantity = kDefaultItemPickupQuantity;
    // Empty: primitive fallback. Non-empty: canonical models/<file>.glb.
    std::string modelIdentity;
};

inline bool ItemPickupPositionIsValid(core::Vec3 position)
{
    return std::isfinite(position.x) && std::isfinite(position.y) && std::isfinite(position.z);
}

inline bool ItemPickupQuantityIsValid(int quantity)
{
    return quantity >= 1 && quantity <= gameplay::kMaxItemQuantity;
}

inline bool ItemPickupModelIdentityIsValid(std::string_view identity)
{
    return identity.empty() || StaticPropIdentityIsValid(identity);
}

inline bool ItemPickupSpecIsValid(const ItemPickupSpec& spec)
{
    return ItemPickupPositionIsValid(spec.position) && gameplay::IsValidItemId(spec.itemId)
        && ItemPickupQuantityIsValid(spec.quantity)
        && ItemPickupModelIdentityIsValid(spec.modelIdentity);
}
}
