#pragma once

// Authored Pressure Plate (Milestone 52 / 53 / 57.1). Axis-aligned trigger
// region only. Runtime Active/Inactive is derived from overlap and is never
// stored on this spec or in Level Format.

#include "core/Vec3.h"
#include "world/RespawnWorld.h"

#include <cmath>
#include <cstddef>
#include <string_view>
#include <vector>

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

// M85.4: typed index into LevelDefinition.pointLights or .spotLights.
// Separate namespaces. Not a GUID, BodyID, Authoring Group, or Directional Light.
enum class LocalLightKind
{
    Point,
    Spot
};

inline constexpr std::string_view kPressurePlateLocalLightsMarker = "lights";
inline constexpr std::string_view kLocalLightKindPointToken = "point";
inline constexpr std::string_view kLocalLightKindSpotToken = "spot";

struct LocalLightTarget
{
    LocalLightKind kind = LocalLightKind::Point;
    int index = 0;
};

inline bool operator==(LocalLightTarget a, LocalLightTarget b)
{
    return a.kind == b.kind && a.index == b.index;
}

inline bool operator!=(LocalLightTarget a, LocalLightTarget b)
{
    return !(a == b);
}

inline const char* LocalLightKindToken(LocalLightKind kind)
{
    switch (kind)
    {
    case LocalLightKind::Point:
        return kLocalLightKindPointToken.data();
    case LocalLightKind::Spot:
        return kLocalLightKindSpotToken.data();
    }
    return "";
}

inline bool TryParseLocalLightKind(std::string_view token, LocalLightKind& kind)
{
    if (token == kLocalLightKindPointToken)
    {
        kind = LocalLightKind::Point;
        return true;
    }
    if (token == kLocalLightKindSpotToken)
    {
        kind = LocalLightKind::Spot;
        return true;
    }
    return false;
}

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
    // M85.2: optional control of the singleton Level Directional Light.
    // Independent of linkedDoorIndex. Default false keeps pre-M85.2 plates
    // from affecting lighting.
    bool controlsDirectionalLight = false;
    // M85.4: ordered unique Point/Spot targets. Empty means no local-light
    // control. Independent of linkedDoorIndex and controlsDirectionalLight.
    std::vector<LocalLightTarget> controlledLocalLights{};
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

inline bool LocalLightTargetIndexIsValid(
    LocalLightKind kind,
    int index,
    std::size_t pointCount,
    std::size_t spotCount)
{
    if (index < 0)
    {
        return false;
    }
    const std::size_t unsignedIndex = static_cast<std::size_t>(index);
    switch (kind)
    {
    case LocalLightKind::Point:
        return unsignedIndex < pointCount;
    case LocalLightKind::Spot:
        return unsignedIndex < spotCount;
    }
    return false;
}

inline bool LocalLightTargetIsValid(
    LocalLightTarget target,
    std::size_t pointCount,
    std::size_t spotCount)
{
    return LocalLightTargetIndexIsValid(target.kind, target.index, pointCount, spotCount);
}

inline bool PressurePlateHasLocalLightTarget(
    const PressurePlateSpec& plate,
    LocalLightTarget target)
{
    for (const LocalLightTarget& existing : plate.controlledLocalLights)
    {
        if (existing == target)
        {
            return true;
        }
    }
    return false;
}

inline bool PressurePlateLocalLightTargetsAreUnique(const PressurePlateSpec& plate)
{
    for (std::size_t index = 0; index < plate.controlledLocalLights.size(); ++index)
    {
        for (std::size_t later = index + 1; later < plate.controlledLocalLights.size(); ++later)
        {
            if (plate.controlledLocalLights[index] == plate.controlledLocalLights[later])
            {
                return false;
            }
        }
    }
    return true;
}

inline bool PressurePlateLocalLightTargetsAreValid(
    const PressurePlateSpec& plate,
    std::size_t pointCount,
    std::size_t spotCount)
{
    if (!PressurePlateLocalLightTargetsAreUnique(plate))
    {
        return false;
    }
    for (const LocalLightTarget& target : plate.controlledLocalLights)
    {
        if (!LocalLightTargetIsValid(target, pointCount, spotCount))
        {
            return false;
        }
    }
    return true;
}

// Drop targets of `kind` at deletedIndex; decrement later same-kind indices.
// The other kind is untouched. Order of surviving targets is preserved.
inline void RemapPressurePlateLocalLightTargetsAfterDelete(
    std::vector<PressurePlateSpec>& plates,
    LocalLightKind kind,
    std::size_t deletedIndex)
{
    const int deleted = static_cast<int>(deletedIndex);
    for (PressurePlateSpec& plate : plates)
    {
        std::vector<LocalLightTarget> surviving;
        surviving.reserve(plate.controlledLocalLights.size());
        for (LocalLightTarget target : plate.controlledLocalLights)
        {
            if (target.kind != kind)
            {
                surviving.push_back(target);
                continue;
            }
            if (target.index == deleted)
            {
                continue;
            }
            if (target.index > deleted)
            {
                --target.index;
            }
            surviving.push_back(target);
        }
        plate.controlledLocalLights = std::move(surviving);
    }
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
