#pragma once

// Milestone 58.3: gameplay Item Pickup target presentation. Consumes the
// existing M55 target result. Does not search for targets, load assets, or
// change collection. Available in Debug/Development/Release.

#include "core/Vec3.h"
#include "world/ItemPickup.h"
#include "world/StaticProp.h"

#include <cmath>

namespace render
{
// Same local unit cube M58.2 uses when loaded model bounds are unavailable.
inline constexpr core::Vec3 kItemPickupModelFallbackLocalMin{-0.5f, -0.5f, -0.5f};
inline constexpr core::Vec3 kItemPickupModelFallbackLocalMax{0.5f, 0.5f, 0.5f};

inline constexpr core::Vec3 kItemPickupPrimitiveLocalMin{
    -world::kItemPickupVisualSize * 0.5f,
    -world::kItemPickupVisualSize * 0.5f,
    -world::kItemPickupVisualSize * 0.5f};
inline constexpr core::Vec3 kItemPickupPrimitiveLocalMax{
    world::kItemPickupVisualSize * 0.5f,
    world::kItemPickupVisualSize * 0.5f,
    world::kItemPickupVisualSize * 0.5f};

struct ItemPickupTargetPresentation
{
    bool drawHud = false;
    bool drawModelHighlight = false;
    bool drawFallbackHighlight = false;
    bool drawInteractionBounds = false;
    bool modelBacked = false;
    world::StaticPropSpec visual{};
    core::Vec3 localMin = kItemPickupModelFallbackLocalMin;
    core::Vec3 localMax = kItemPickupModelFallbackLocalMax;
    core::Vec3 boundsCorners[8]{};
};

namespace item_pickup_highlight_detail
{
inline constexpr float kDegreesToRadians = 3.14159265f / 180.0f;

inline core::Vec3 ScaleAxes(core::Vec3 value, core::Vec3 scale)
{
    return {value.x * scale.x, value.y * scale.y, value.z * scale.z};
}

inline core::Vec3 RotateX(core::Vec3 value, float degrees)
{
    const float radians = degrees * kDegreesToRadians;
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    return {
        value.x,
        value.y * cosine - value.z * sine,
        value.y * sine + value.z * cosine};
}

inline core::Vec3 RotateY(core::Vec3 value, float degrees)
{
    const float radians = degrees * kDegreesToRadians;
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    return {
        value.x * cosine + value.z * sine,
        value.y,
        -value.x * sine + value.z * cosine};
}

inline core::Vec3 RotateZ(core::Vec3 value, float degrees)
{
    const float radians = degrees * kDegreesToRadians;
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    return {
        value.x * cosine - value.y * sine,
        value.x * sine + value.y * cosine,
        value.z};
}

// Rx then Ry then Rz. Matches DrawPropTransform / M58.2 StaticPropWorldFromLocal.
inline core::Vec3 RotateEulerXYZ(core::Vec3 value, core::Vec3 degrees)
{
    return RotateZ(RotateY(RotateX(value, degrees.x), degrees.y), degrees.z);
}

inline core::Vec3 WorldFromLocal(const world::StaticPropSpec& spec, core::Vec3 local)
{
    return spec.position + RotateEulerXYZ(ScaleAxes(local, spec.scale), spec.rotationDegrees);
}

inline void FillWorldCorners(
    const world::StaticPropSpec& spec,
    core::Vec3 localMin,
    core::Vec3 localMax,
    core::Vec3 outCorners[8])
{
    outCorners[0] = WorldFromLocal(spec, {localMin.x, localMin.y, localMin.z});
    outCorners[1] = WorldFromLocal(spec, {localMax.x, localMin.y, localMin.z});
    outCorners[2] = WorldFromLocal(spec, {localMin.x, localMax.y, localMin.z});
    outCorners[3] = WorldFromLocal(spec, {localMax.x, localMax.y, localMin.z});
    outCorners[4] = WorldFromLocal(spec, {localMin.x, localMin.y, localMax.z});
    outCorners[5] = WorldFromLocal(spec, {localMax.x, localMin.y, localMax.z});
    outCorners[6] = WorldFromLocal(spec, {localMin.x, localMax.y, localMax.z});
    outCorners[7] = WorldFromLocal(spec, {localMax.x, localMax.y, localMax.z});
}
}

// isGameplayTarget is the current M55 index comparison. Callers must not
// recompute targeting here.
inline ItemPickupTargetPresentation MakeItemPickupTargetPresentation(
    const world::ItemPickupSpec& pickup,
    bool isGameplayTarget,
    bool collected,
    bool haveLoadedLocalBounds,
    core::Vec3 loadedMin,
    core::Vec3 loadedMax)
{
    ItemPickupTargetPresentation result{};
    if (collected || !world::ItemPickupSpecIsValid(pickup))
    {
        return result;
    }

    result.modelBacked = !pickup.modelIdentity.empty();
    if (result.modelBacked)
    {
        result.visual = world::ItemPickupVisualProp(pickup);
        result.localMin = kItemPickupModelFallbackLocalMin;
        result.localMax = kItemPickupModelFallbackLocalMax;
        if (haveLoadedLocalBounds)
        {
            result.localMin = loadedMin;
            result.localMax = loadedMax;
        }
    }
    else
    {
        result.visual.position = pickup.position;
        result.visual.rotationDegrees = {};
        result.visual.scale = {1.0f, 1.0f, 1.0f};
        result.localMin = kItemPickupPrimitiveLocalMin;
        result.localMax = kItemPickupPrimitiveLocalMax;
    }
    item_pickup_highlight_detail::FillWorldCorners(
        result.visual, result.localMin, result.localMax, result.boundsCorners);

    if (!isGameplayTarget)
    {
        return result;
    }

    result.drawHud = true;
    result.drawInteractionBounds = pickup.showInteractionBounds;
    if (result.modelBacked)
    {
        result.drawModelHighlight = true;
    }
    else
    {
        result.drawFallbackHighlight = true;
    }
    return result;
}
}
