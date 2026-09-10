#pragma once

// Authored Item Pickup (Milestone 55 / 58 / 58.3 / 58.4). World acquisition:
// gameplay position, M54 itemId, quantity, optional Static Model identity, a
// per-instance visual transform, presentation-only interaction-bounds
// visibility, and gameplay target-highlight intensity. Runtime collected
// state is never stored here or in Level Format. Not an ItemDefinition or
// generic Transform.

#include "core/Vec3.h"
#include "gameplay/Inventory.h"
#include "world/StaticProp.h"

#include <algorithm>
#include <cmath>
#include <span>
#include <string>
#include <string_view>
#include <vector>

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
inline constexpr core::Vec3 kDefaultItemPickupVisualOffset{0.0f, 0.0f, 0.0f};
inline constexpr core::Vec3 kDefaultItemPickupVisualRotationDegrees{0.0f, 0.0f, 0.0f};
inline constexpr core::Vec3 kDefaultItemPickupVisualScale{1.0f, 1.0f, 1.0f};
inline constexpr bool kDefaultItemPickupShowInteractionBounds = true;
// Approximately M58.3 tint alpha 180/255. Presentation only.
inline constexpr float kDefaultItemPickupTargetHighlightIntensity = 0.70f;
inline constexpr float kMinItemPickupTargetHighlightIntensity = 0.0f;
inline constexpr float kMaxItemPickupTargetHighlightIntensity = 1.0f;
// Level Format v1 markers. Cannot collide with modelIdentity (models/<file>.glb).
inline constexpr std::string_view kItemPickupVisualKeyword = "visual";
inline constexpr std::string_view kItemPickupBoundsKeyword = "bounds";
inline constexpr std::string_view kItemPickupHighlightKeyword = "highlight";

struct ItemPickupSpec
{
    core::Vec3 position{};
    std::string itemId{kDefaultItemPickupId};
    int quantity = kDefaultItemPickupQuantity;
    // Empty: primitive fallback. Non-empty: canonical models/<file>.glb.
    std::string modelIdentity;
    core::Vec3 visualOffset{};
    core::Vec3 visualRotationDegrees{};
    core::Vec3 visualScale = kDefaultItemPickupVisualScale;
    // Presentation only. When this pickup is the current M55 target, draw
    // the interaction-bounds wire. Does not affect targeting or collection.
    bool showInteractionBounds = kDefaultItemPickupShowInteractionBounds;
    // Presentation only. Strength of the Gameplay golden target tint.
    // Does not affect targeting, HUD, collection, or showInteractionBounds.
    float targetHighlightIntensity = kDefaultItemPickupTargetHighlightIntensity;
};

inline bool ItemPickupPositionIsValid(core::Vec3 position)
{
    return std::isfinite(position.x) && std::isfinite(position.y) && std::isfinite(position.z);
}

inline bool ItemPickupVisualOffsetIsValid(core::Vec3 visualOffset)
{
    return std::isfinite(visualOffset.x) && std::isfinite(visualOffset.y)
        && std::isfinite(visualOffset.z);
}

inline bool ItemPickupVisualRotationIsValid(core::Vec3 visualRotationDegrees)
{
    return std::isfinite(visualRotationDegrees.x) && std::isfinite(visualRotationDegrees.y)
        && std::isfinite(visualRotationDegrees.z);
}

inline bool ItemPickupVisualScaleIsValid(core::Vec3 visualScale)
{
    return StaticPropScaleIsValid(visualScale);
}

inline bool ItemPickupQuantityIsValid(int quantity)
{
    return quantity >= 1 && quantity <= gameplay::kMaxItemQuantity;
}

inline bool ItemPickupModelIdentityIsValid(std::string_view identity)
{
    return identity.empty() || StaticPropIdentityIsValid(identity);
}

inline bool ItemPickupTargetHighlightIntensityIsValid(float intensity)
{
    return std::isfinite(intensity) && intensity >= kMinItemPickupTargetHighlightIntensity
        && intensity <= kMaxItemPickupTargetHighlightIntensity;
}

inline bool ItemPickupSpecIsValid(const ItemPickupSpec& spec)
{
    return ItemPickupPositionIsValid(spec.position) && gameplay::IsValidItemId(spec.itemId)
        && ItemPickupQuantityIsValid(spec.quantity)
        && ItemPickupModelIdentityIsValid(spec.modelIdentity)
        && ItemPickupVisualOffsetIsValid(spec.visualOffset)
        && ItemPickupVisualRotationIsValid(spec.visualRotationDegrees)
        && ItemPickupVisualScaleIsValid(spec.visualScale)
        && ItemPickupTargetHighlightIntensityIsValid(spec.targetHighlightIntensity);
}

// Rendered model origin. Gameplay targeting continues to use position.
inline core::Vec3 ItemPickupVisualPosition(const ItemPickupSpec& spec)
{
    return spec.position + spec.visualOffset;
}

// Narrow DrawProp/picking adapter. Not a Transform component.
inline StaticPropSpec ItemPickupVisualProp(const ItemPickupSpec& spec)
{
    StaticPropSpec visual{};
    visual.modelIdentity = spec.modelIdentity;
    visual.position = ItemPickupVisualPosition(spec);
    visual.rotationDegrees = spec.visualRotationDegrees;
    visual.scale = spec.visualScale;
    return visual;
}

// Unique deterministic itemId values currently authored by Item Pickups.
// Not an ItemCatalog. Invalid ids are skipped. Sorted for Inspector order.
inline std::vector<std::string> UniqueAuthoredPickupItemIds(
    std::span<const ItemPickupSpec> pickups)
{
    std::vector<std::string> ids;
    ids.reserve(pickups.size());
    for (const ItemPickupSpec& pickup : pickups)
    {
        if (!gameplay::IsValidItemId(pickup.itemId))
        {
            continue;
        }
        ids.push_back(pickup.itemId);
    }
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}
}
