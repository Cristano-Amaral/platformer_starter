#pragma once

// Authored Pressure Plate (Milestone 52 / 53 / 57.1). Axis-aligned trigger
// region only. Runtime Active/Inactive is derived from overlap and is never
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
    // Legacy M52/M53 defaults: Dynamic Boxes activate, Player does not,
    // gameplay visual is drawn.
    bool activateByDynamicBox = true;
    bool activateByPlayer = false;
    bool visibleInGameplay = true;
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

// Generic AABB overlap against the authored plate volume.
inline bool PressurePlateOverlapsVolume(
    const PressurePlateSpec& plate,
    core::Vec3 volumeCenter,
    core::Vec3 volumeSize)
{
    return PressurePlateSpecIsValid(plate)
        && AabbOverlaps(plate.center, plate.size, volumeCenter, volumeSize);
}

// Production Dynamic Box overlap rule: authored plate AABB vs current box AABB.
inline bool PressurePlateOverlapsBox(
    const PressurePlateSpec& plate,
    core::Vec3 boxCenter,
    core::Vec3 boxSize)
{
    return PressurePlateOverlapsVolume(plate, boxCenter, boxSize);
}

// Player overlap uses the CharacterVirtual capsule AABB supplied by
// PhysicsWorld (radius 0.4, total height 1.6 from feet — the live collision
// shape, which currently matches kPlayerVisualSize). Not a rendered-only cube.
inline bool PressurePlateOverlapsPlayerVolume(
    const PressurePlateSpec& plate,
    core::Vec3 playerCollisionCenter,
    core::Vec3 playerCollisionSize)
{
    return PressurePlateOverlapsVolume(plate, playerCollisionCenter, playerCollisionSize);
}
}
