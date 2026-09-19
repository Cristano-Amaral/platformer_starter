#pragma once

// Milestone 85.1: singleton Level-authored environment. Presentation lighting
// only. Not a light list, Component, Scene node, ECS, or Environment asset.
// A future gameplay activation seam may combine authoredEnabled with a
// transient override; M85.1 persists authoredEnabled only.

#include "core/Vec3.h"

#include <cmath>

namespace world
{
inline constexpr core::Vec3 kDefaultAmbientColor{1.0f, 1.0f, 1.0f};
inline constexpr float kDefaultAmbientIntensity = 0.34f;
inline constexpr core::Vec3 kDefaultDirectionalRayDirection{-0.42f, -1.0f, -0.38f};
inline constexpr core::Vec3 kDefaultDirectionalColor{1.0f, 0.96f, 0.88f};
inline constexpr float kDefaultDirectionalIntensity = 0.88f;
inline constexpr core::Vec3 kFallbackDirectionalRayDirection{0.0f, -1.0f, 0.0f};
inline constexpr float kMinDirectionalRayLength = 1.0e-5f;
inline constexpr float kMaxAuthoredAmbientIntensity = 2.0f;
inline constexpr float kMaxAuthoredDirectionalIntensity = 4.0f;

struct LevelEnvironment
{
    core::Vec3 ambientColor = kDefaultAmbientColor;
    float ambientIntensity = kDefaultAmbientIntensity;
    // Authored Directional Light Enabled. Effective enabled currently equals
    // this value; gameplay override is a future seam, not M85.1 behavior.
    bool directionalEnabled = true;
    core::Vec3 directionalRayDirection = kDefaultDirectionalRayDirection;
    core::Vec3 directionalColor = kDefaultDirectionalColor;
    float directionalIntensity = kDefaultDirectionalIntensity;
    bool directionalShadowsEnabled = true;
};

inline bool LevelEnvironmentComponentFinite(float value)
{
    return std::isfinite(value);
}

inline bool LevelEnvironmentVecFinite(core::Vec3 value)
{
    return LevelEnvironmentComponentFinite(value.x) && LevelEnvironmentComponentFinite(value.y)
        && LevelEnvironmentComponentFinite(value.z);
}

inline bool LevelEnvironmentColorIsValid(core::Vec3 color)
{
    if (!LevelEnvironmentVecFinite(color))
    {
        return false;
    }
    return color.x >= 0.0f && color.x <= 1.0f && color.y >= 0.0f && color.y <= 1.0f
        && color.z >= 0.0f && color.z <= 1.0f;
}

inline bool LevelEnvironmentIntensityIsValid(float intensity, float maxIntensity)
{
    return LevelEnvironmentComponentFinite(intensity) && intensity >= 0.0f
        && intensity <= maxIntensity;
}

inline float LevelEnvironmentLengthSquared(core::Vec3 value)
{
    return value.x * value.x + value.y * value.y + value.z * value.z;
}

inline core::Vec3 CanonicalLevelDirectionalRay(core::Vec3 rayDirection)
{
    if (!LevelEnvironmentVecFinite(rayDirection))
    {
        return kFallbackDirectionalRayDirection;
    }
    const float lengthSq = LevelEnvironmentLengthSquared(rayDirection);
    if (lengthSq < kMinDirectionalRayLength * kMinDirectionalRayLength)
    {
        return kFallbackDirectionalRayDirection;
    }
    const float invLength = 1.0f / std::sqrt(lengthSq);
    return {rayDirection.x * invLength, rayDirection.y * invLength, rayDirection.z * invLength};
}

inline bool LevelDirectionalRayIsValid(core::Vec3 rayDirection)
{
    if (!LevelEnvironmentVecFinite(rayDirection))
    {
        return false;
    }
    return LevelEnvironmentLengthSquared(rayDirection)
        >= kMinDirectionalRayLength * kMinDirectionalRayLength;
}

inline LevelEnvironment MakeDefaultLevelEnvironment()
{
    LevelEnvironment environment{};
    environment.directionalRayDirection =
        CanonicalLevelDirectionalRay(environment.directionalRayDirection);
    return environment;
}

inline bool LevelEnvironmentIsValid(const LevelEnvironment& environment)
{
    return LevelEnvironmentColorIsValid(environment.ambientColor)
        && LevelEnvironmentIntensityIsValid(
            environment.ambientIntensity, kMaxAuthoredAmbientIntensity)
        && LevelEnvironmentColorIsValid(environment.directionalColor)
        && LevelEnvironmentIntensityIsValid(
            environment.directionalIntensity, kMaxAuthoredDirectionalIntensity)
        && LevelDirectionalRayIsValid(environment.directionalRayDirection);
}

inline void CanonicalizeLevelEnvironment(LevelEnvironment& environment)
{
    environment.directionalRayDirection =
        CanonicalLevelDirectionalRay(environment.directionalRayDirection);
}

inline bool LevelEnvironmentEqual(const LevelEnvironment& a, const LevelEnvironment& b)
{
    return a.ambientColor.x == b.ambientColor.x && a.ambientColor.y == b.ambientColor.y
        && a.ambientColor.z == b.ambientColor.z && a.ambientIntensity == b.ambientIntensity
        && a.directionalEnabled == b.directionalEnabled
        && a.directionalRayDirection.x == b.directionalRayDirection.x
        && a.directionalRayDirection.y == b.directionalRayDirection.y
        && a.directionalRayDirection.z == b.directionalRayDirection.z
        && a.directionalColor.x == b.directionalColor.x
        && a.directionalColor.y == b.directionalColor.y
        && a.directionalColor.z == b.directionalColor.z
        && a.directionalIntensity == b.directionalIntensity
        && a.directionalShadowsEnabled == b.directionalShadowsEnabled;
}
}
