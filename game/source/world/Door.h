#pragma once

// Authored Door (Milestone 53 / 57 / 57.1). Closed pose + size + +Y open
// distance. Optional requiredItemId is authored; runtime locked/unlocked is
// never stored on this spec.

#include "core/Vec3.h"
#include "gameplay/Inventory.h"

#include <cmath>
#include <string>
#include <string_view>

namespace world
{
inline constexpr float kMinDoorExtent = 0.12f;
inline constexpr core::Vec3 kDefaultDoorSize{1.2f, 3.0f, 2.4f};
inline constexpr float kDefaultDoorOpenDistance = 3.2f;
inline constexpr float kMinDoorOpenDistance = 0.12f;
inline constexpr float kMaxDoorOpenDistance = 20.0f;
inline constexpr int kLevel01DoorCount = 0;

// M57 trailing token "1" maps to this Inventory itemId. Canonical M57.1
// writer emits the itemId itself, not "1".
inline constexpr std::string_view kM57LegacyRequiredKeyItemId = "key";

struct DoorSpec
{
    core::Vec3 center{};
    core::Vec3 size{};
    float openDistance = 0.0f;
    // Empty: no Inventory requirement (M53 motion). Non-empty: the Door
    // starts each applied run locked until TryRemove(this, 1) unlocks it.
    // Logical Inventory itemId only — never a Pickup index, pointer, or GUID.
    std::string requiredItemId{};
};

inline bool DoorSizeIsValid(core::Vec3 size)
{
    return std::isfinite(size.x) && std::isfinite(size.y) && std::isfinite(size.z)
        && size.x >= kMinDoorExtent && size.y >= kMinDoorExtent && size.z >= kMinDoorExtent;
}

inline bool DoorCenterIsValid(core::Vec3 center)
{
    return std::isfinite(center.x) && std::isfinite(center.y) && std::isfinite(center.z);
}

inline bool DoorOpenDistanceIsValid(float openDistance)
{
    return std::isfinite(openDistance) && openDistance >= kMinDoorOpenDistance
        && openDistance <= kMaxDoorOpenDistance;
}

inline bool DoorRequiredItemIdIsValid(std::string_view requiredItemId)
{
    return requiredItemId.empty() || gameplay::IsValidItemId(requiredItemId);
}

inline bool DoorRequiresInventoryItem(const DoorSpec& spec)
{
    return !spec.requiredItemId.empty();
}

inline bool DoorSpecIsValid(const DoorSpec& spec)
{
    return DoorCenterIsValid(spec.center) && DoorSizeIsValid(spec.size)
        && DoorOpenDistanceIsValid(spec.openDistance)
        && DoorRequiredItemIdIsValid(spec.requiredItemId);
}

inline core::Vec3 DoorOpenCenter(const DoorSpec& spec)
{
    return {spec.center.x, spec.center.y + spec.openDistance, spec.center.z};
}

inline core::Vec3 DoorCenterAtFraction(const DoorSpec& spec, float openFraction)
{
    const float clamped = openFraction < 0.0f ? 0.0f : (openFraction > 1.0f ? 1.0f : openFraction);
    return {spec.center.x, spec.center.y + spec.openDistance * clamped, spec.center.z};
}
}
