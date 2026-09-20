#pragma once

// Milestone 85.3: repeatable authored Point and Spot Lights.
// Presentation lighting. M85.4 may name them as Pressure Plate targets via
// typed indices. Not Directional Light, not a GUID/reference framework.

#include "core/Vec3.h"
#include "world/LevelEnvironment.h"

#include <cmath>

namespace world
{
inline constexpr core::Vec3 kDefaultPointLightColor{1.0f, 0.95f, 0.85f};
inline constexpr float kDefaultPointLightIntensity = 1.5f;
inline constexpr float kDefaultPointLightRange = 8.0f;
inline constexpr core::Vec3 kDefaultSpotLightColor{1.0f, 0.95f, 0.85f};
inline constexpr float kDefaultSpotLightIntensity = 1.5f;
inline constexpr float kDefaultSpotLightRange = 8.0f;
inline constexpr core::Vec3 kDefaultSpotLightDirection{0.0f, -1.0f, 0.0f};
inline constexpr float kDefaultSpotInnerConeDegrees = 20.0f;
inline constexpr float kDefaultSpotOuterConeDegrees = 35.0f;

inline constexpr float kMaxAuthoredLocalLightIntensity = kMaxAuthoredDirectionalIntensity;
inline constexpr float kMinAuthoredLocalLightRange = 0.1f;
inline constexpr float kMaxAuthoredLocalLightRange = 64.0f;
inline constexpr float kMinSpotInnerConeDegrees = 0.0f;
inline constexpr float kMaxSpotOuterConeDegrees = 89.0f;
inline constexpr float kMinSpotConeSeparationDegrees = 0.5f;
inline constexpr float kMinSpotDirectionLength = kMinDirectionalRayLength;
inline constexpr core::Vec3 kFallbackSpotLightDirection{0.0f, -1.0f, 0.0f};

struct PointLightSpec
{
    core::Vec3 position{};
    core::Vec3 color = kDefaultPointLightColor;
    float intensity = kDefaultPointLightIntensity;
    float range = kDefaultPointLightRange;
    bool enabled = true;
};

struct SpotLightSpec
{
    core::Vec3 position{};
    core::Vec3 direction = kDefaultSpotLightDirection;
    core::Vec3 color = kDefaultSpotLightColor;
    float intensity = kDefaultSpotLightIntensity;
    float range = kDefaultSpotLightRange;
    float innerConeDegrees = kDefaultSpotInnerConeDegrees;
    float outerConeDegrees = kDefaultSpotOuterConeDegrees;
    bool enabled = true;
};

inline bool LocalLightComponentFinite(float value)
{
    return std::isfinite(value);
}

inline bool LocalLightVecFinite(core::Vec3 value)
{
    return LocalLightComponentFinite(value.x) && LocalLightComponentFinite(value.y)
        && LocalLightComponentFinite(value.z);
}

inline bool LocalLightColorIsValid(core::Vec3 color)
{
    return LevelEnvironmentColorIsValid(color);
}

inline bool LocalLightIntensityIsValid(float intensity)
{
    return LevelEnvironmentIntensityIsValid(intensity, kMaxAuthoredLocalLightIntensity);
}

inline bool LocalLightRangeIsValid(float range)
{
    return LocalLightComponentFinite(range) && range >= kMinAuthoredLocalLightRange
        && range <= kMaxAuthoredLocalLightRange;
}

inline bool LocalLightPositionIsValid(core::Vec3 position)
{
    return LocalLightVecFinite(position);
}

inline float LocalLightLengthSquared(core::Vec3 value)
{
    return value.x * value.x + value.y * value.y + value.z * value.z;
}

inline core::Vec3 CanonicalSpotLightDirection(core::Vec3 direction)
{
    if (!LocalLightVecFinite(direction))
    {
        return kFallbackSpotLightDirection;
    }
    const float lengthSq = LocalLightLengthSquared(direction);
    if (lengthSq < kMinSpotDirectionLength * kMinSpotDirectionLength)
    {
        return kFallbackSpotLightDirection;
    }
    const float invLength = 1.0f / std::sqrt(lengthSq);
    return {direction.x * invLength, direction.y * invLength, direction.z * invLength};
}

inline bool SpotLightDirectionIsValid(core::Vec3 direction)
{
    if (!LocalLightVecFinite(direction))
    {
        return false;
    }
    return LocalLightLengthSquared(direction)
        >= kMinSpotDirectionLength * kMinSpotDirectionLength;
}

inline bool SpotConeAnglesAreValid(float innerConeDegrees, float outerConeDegrees)
{
    if (!LocalLightComponentFinite(innerConeDegrees) || !LocalLightComponentFinite(outerConeDegrees))
    {
        return false;
    }
    if (innerConeDegrees < kMinSpotInnerConeDegrees || outerConeDegrees > kMaxSpotOuterConeDegrees)
    {
        return false;
    }
    return outerConeDegrees - innerConeDegrees >= kMinSpotConeSeparationDegrees;
}

inline bool PointLightIsValid(const PointLightSpec& light)
{
    return LocalLightPositionIsValid(light.position) && LocalLightColorIsValid(light.color)
        && LocalLightIntensityIsValid(light.intensity) && LocalLightRangeIsValid(light.range);
}

inline bool SpotLightIsValid(const SpotLightSpec& light)
{
    return LocalLightPositionIsValid(light.position) && SpotLightDirectionIsValid(light.direction)
        && LocalLightColorIsValid(light.color) && LocalLightIntensityIsValid(light.intensity)
        && LocalLightRangeIsValid(light.range)
        && SpotConeAnglesAreValid(light.innerConeDegrees, light.outerConeDegrees);
}

inline PointLightSpec MakeDefaultPointLight(core::Vec3 position)
{
    PointLightSpec light{};
    light.position = position;
    return light;
}

inline SpotLightSpec MakeDefaultSpotLight(core::Vec3 position)
{
    SpotLightSpec light{};
    light.position = position;
    light.direction = CanonicalSpotLightDirection(light.direction);
    return light;
}

inline void CanonicalizeSpotLight(SpotLightSpec& light)
{
    light.direction = CanonicalSpotLightDirection(light.direction);
}

inline bool PointLightEqual(const PointLightSpec& a, const PointLightSpec& b)
{
    return a.position.x == b.position.x && a.position.y == b.position.y
        && a.position.z == b.position.z && a.color.x == b.color.x && a.color.y == b.color.y
        && a.color.z == b.color.z && a.intensity == b.intensity && a.range == b.range
        && a.enabled == b.enabled;
}

inline bool SpotLightEqual(const SpotLightSpec& a, const SpotLightSpec& b)
{
    return a.position.x == b.position.x && a.position.y == b.position.y
        && a.position.z == b.position.z && a.direction.x == b.direction.x
        && a.direction.y == b.direction.y && a.direction.z == b.direction.z
        && a.color.x == b.color.x && a.color.y == b.color.y && a.color.z == b.color.z
        && a.intensity == b.intensity && a.range == b.range
        && a.innerConeDegrees == b.innerConeDegrees && a.outerConeDegrees == b.outerConeDegrees
        && a.enabled == b.enabled;
}

inline float ClampLocalLightIntensity(float intensity)
{
    if (!LocalLightComponentFinite(intensity) || intensity < 0.0f)
    {
        return 0.0f;
    }
    if (intensity > kMaxAuthoredLocalLightIntensity)
    {
        return kMaxAuthoredLocalLightIntensity;
    }
    return intensity;
}

inline float ClampLocalLightRange(float range)
{
    if (!LocalLightComponentFinite(range) || range < kMinAuthoredLocalLightRange)
    {
        return kMinAuthoredLocalLightRange;
    }
    if (range > kMaxAuthoredLocalLightRange)
    {
        return kMaxAuthoredLocalLightRange;
    }
    return range;
}

inline void ClampSpotConeAngles(float& innerConeDegrees, float& outerConeDegrees)
{
    if (!LocalLightComponentFinite(innerConeDegrees))
    {
        innerConeDegrees = kDefaultSpotInnerConeDegrees;
    }
    if (!LocalLightComponentFinite(outerConeDegrees))
    {
        outerConeDegrees = kDefaultSpotOuterConeDegrees;
    }
    if (innerConeDegrees < kMinSpotInnerConeDegrees)
    {
        innerConeDegrees = kMinSpotInnerConeDegrees;
    }
    if (outerConeDegrees > kMaxSpotOuterConeDegrees)
    {
        outerConeDegrees = kMaxSpotOuterConeDegrees;
    }
    if (outerConeDegrees - innerConeDegrees < kMinSpotConeSeparationDegrees)
    {
        const float mid = 0.5f * (innerConeDegrees + outerConeDegrees);
        innerConeDegrees = mid - 0.5f * kMinSpotConeSeparationDegrees;
        outerConeDegrees = mid + 0.5f * kMinSpotConeSeparationDegrees;
        if (innerConeDegrees < kMinSpotInnerConeDegrees)
        {
            innerConeDegrees = kMinSpotInnerConeDegrees;
            outerConeDegrees = innerConeDegrees + kMinSpotConeSeparationDegrees;
        }
        if (outerConeDegrees > kMaxSpotOuterConeDegrees)
        {
            outerConeDegrees = kMaxSpotOuterConeDegrees;
            innerConeDegrees = outerConeDegrees - kMinSpotConeSeparationDegrees;
        }
    }
}
}
