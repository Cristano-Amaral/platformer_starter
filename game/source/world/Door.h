#pragma once

// Authored Door (Milestone 53 / 57). Closed pose + size + +Y open distance.
// Optional requiresKey is authored; runtime locked/unlocked is never stored
// on this spec.

#include "core/Vec3.h"

#include <cmath>

namespace world
{
inline constexpr float kMinDoorExtent = 0.12f;
inline constexpr core::Vec3 kDefaultDoorSize{1.2f, 3.0f, 2.4f};
inline constexpr float kDefaultDoorOpenDistance = 3.2f;
inline constexpr float kMinDoorOpenDistance = 0.12f;
inline constexpr float kMaxDoorOpenDistance = 20.0f;
inline constexpr int kLevel01DoorCount = 0;

struct DoorSpec
{
    core::Vec3 center{};
    core::Vec3 size{};
    float openDistance = 0.0f;
    // Concrete M57 lock: when true, a new applied run starts locked until the
    // production Inventory item "key" unlocks that runtime Door. Default false
    // preserves exact M53 Pressure Plate motion.
    bool requiresKey = false;
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

inline bool DoorSpecIsValid(const DoorSpec& spec)
{
    return DoorCenterIsValid(spec.center) && DoorSizeIsValid(spec.size)
        && DoorOpenDistanceIsValid(spec.openDistance);
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
