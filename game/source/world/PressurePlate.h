#pragma once

// Authored Pressure Plate (Milestone 52). Axis-aligned trigger region only.
// Runtime Active/Inactive is derived from Dynamic Box overlap and is never
// stored on this spec or in Level Format.

#include "core/Vec3.h"
#include "world/RespawnWorld.h"

#include <cmath>
#include <cstddef>

namespace world
{
// Matches editor::kMinAuthoredBoxExtent so Resize and parse share one floor.
inline constexpr float kMinPressurePlateExtent = 0.12f;
inline constexpr core::Vec3 kDefaultPressurePlateSize{2.0f, 0.2f, 2.0f};
inline constexpr int kLevel01PressurePlateCount = 0;

// Authored Door index into LevelDefinition.doors. Not a BodyID or pointer.
// -1 means no linked Door. Validated against the Door collection, never
// silently retargeted.
inline constexpr int kNoLinkedDoor = -1;

struct PressurePlateSpec
{
    core::Vec3 center{};
    core::Vec3 size{};
    int linkedDoorIndex = kNoLinkedDoor;
};

inline bool PressurePlateSizeIsValid(core::Vec3 size)
{
    return std::isfinite(size.x) && std::isfinite(size.y) && std::isfinite(size.z)
        && size.x >= kMinPressurePlateExtent && size.y >= kMinPressurePlateExtent
        && size.z >= kMinPressurePlateExtent;
}

inline bool PressurePlateCenterIsValid(core::Vec3 center)
{
    return std::isfinite(center.x) && std::isfinite(center.y) && std::isfinite(center.z);
}

inline bool PressurePlateSpecIsValid(const PressurePlateSpec& spec)
{
    return PressurePlateCenterIsValid(spec.center) && PressurePlateSizeIsValid(spec.size);
}

inline bool PressurePlateDoorLinkIsValid(int linkedDoorIndex, std::size_t doorCount)
{
    if (linkedDoorIndex == kNoLinkedDoor)
    {
        return true;
    }
    return linkedDoorIndex >= 0 && static_cast<std::size_t>(linkedDoorIndex) < doorCount;
}

// Production overlap rule: authored plate AABB vs current Dynamic Box AABB.
// Player, Static Props, and other authored categories are never arguments.
inline bool PressurePlateOverlapsBox(
    const PressurePlateSpec& plate,
    core::Vec3 boxCenter,
    core::Vec3 boxSize)
{
    return PressurePlateSpecIsValid(plate) && AabbOverlaps(plate.center, plate.size, boxCenter, boxSize);
}
}
