#pragma once

// Milestone 58.3 / 58.4 / 59: gameplay Item Pickup target presentation.
// Consumes the existing M55 target result. Does not search for targets, load
// assets, or change collection. Available in Debug/Development/Release.

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

inline constexpr unsigned char kItemPickupTargetGoldRed = 255;
inline constexpr unsigned char kItemPickupTargetGoldGreen = 220;
inline constexpr unsigned char kItemPickupTargetGoldBlue = 72;

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
    float highlightIntensity = 0.0f;
    float highlightGoldAmount = 0.0f;
    unsigned char highlightRed = 255;
    unsigned char highlightGreen = 255;
    unsigned char highlightBlue = 255;
    unsigned char highlightAlpha = 0;
};

inline unsigned char ItemPickupTargetHighlightAlpha(float intensity)
{
    if (!world::ItemPickupTargetHighlightIntensityIsValid(intensity) || !(intensity > 0.0f))
    {
        return 0;
    }
    const long rounded = std::lround(static_cast<double>(intensity) * 255.0);
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

// RGB of the extra target-highlight pass. Gold Amount 0 keeps white
// (original albedo * 1). Gold Amount 1 is the existing M58.3 gold.
// This is not an alpha multiplier.
inline unsigned char ItemPickupTargetHighlightChannel(
    unsigned char from,
    unsigned char to,
    float goldAmount)
{
    const float t = goldAmount < 0.0f ? 0.0f : (goldAmount > 1.0f ? 1.0f : goldAmount);
    return static_cast<unsigned char>(std::lround(
        static_cast<float>(from) + (static_cast<float>(to) - static_cast<float>(from)) * t));
}

inline void ItemPickupTargetHighlightTint(
    float goldAmount,
    unsigned char& red,
    unsigned char& green,
    unsigned char& blue)
{
    if (!world::ItemPickupTargetHighlightGoldAmountIsValid(goldAmount))
    {
        red = 255;
        green = 255;
        blue = 255;
        return;
    }
    red = ItemPickupTargetHighlightChannel(255, kItemPickupTargetGoldRed, goldAmount);
    green = ItemPickupTargetHighlightChannel(255, kItemPickupTargetGoldGreen, goldAmount);
    blue = ItemPickupTargetHighlightChannel(255, kItemPickupTargetGoldBlue, goldAmount);
}

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
// recompute targeting here. elapsedSeconds is the existing run timer used
// only for visual-only idle bob/spin attachment.
inline ItemPickupTargetPresentation MakeItemPickupTargetPresentation(
    const world::ItemPickupSpec& pickup,
    bool isGameplayTarget,
    bool collected,
    bool haveLoadedLocalBounds,
    core::Vec3 loadedMin,
    core::Vec3 loadedMax,
    double elapsedSeconds = 0.0)
{
    ItemPickupTargetPresentation result{};
    if (collected || !world::ItemPickupSpecIsValid(pickup))
    {
        return result;
    }

    result.modelBacked = !pickup.modelIdentity.empty();
    if (result.modelBacked)
    {
        result.visual = world::ItemPickupPresentedVisualProp(pickup, elapsedSeconds);
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
        result.visual.position.y += world::ItemPickupIdleBobOffsetY(pickup, elapsedSeconds);
        result.visual.rotationDegrees = {
            0.0f, world::ItemPickupIdleSpinYDegrees(pickup, elapsedSeconds), 0.0f};
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
    result.highlightIntensity = pickup.targetHighlightIntensity;
    result.highlightGoldAmount = pickup.targetHighlightGoldAmount;
    result.highlightAlpha = ItemPickupTargetHighlightAlpha(pickup.targetHighlightIntensity);
    ItemPickupTargetHighlightTint(
        pickup.targetHighlightGoldAmount,
        result.highlightRed,
        result.highlightGreen,
        result.highlightBlue);
    if (result.modelBacked)
    {
        result.drawModelHighlight = result.highlightAlpha > 0;
    }
    else
    {
        result.drawFallbackHighlight = result.highlightAlpha > 0;
    }
    return result;
}
}
