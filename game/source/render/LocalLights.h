#pragma once

// Milestone 85.3: CPU packing and attenuation for repeatable Point/Spot Lights.
// Fixed forward-renderer cap. Deterministic overflow. No local-light shadows.

#include "core/Vec3.h"
#include "world/LocalLight.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace render
{
inline constexpr int kMaxActiveLocalLights = 8;
inline constexpr int kLocalLightTypePoint = 1;
inline constexpr int kLocalLightTypeSpot = 2;
inline constexpr float kLocalLightAttenuationEpsilon = 1.0e-8f;

struct PackedLocalLight
{
    int type = 0;
    core::Vec3 position{};
    core::Vec3 direction{0.0f, -1.0f, 0.0f};
    core::Vec3 color{1.0f, 1.0f, 1.0f};
    float intensity = 0.0f;
    float range = 0.0f;
    float innerCos = 1.0f;
    float outerCos = 0.0f;
};

struct PackedLocalLights
{
    std::array<PackedLocalLight, kMaxActiveLocalLights> lights{};
    int count = 0;
};

inline float DegreesToRadians(float degrees)
{
    return degrees * (3.14159265f / 180.0f);
}

inline float CosineFromConeDegrees(float degrees)
{
    if (!std::isfinite(degrees))
    {
        return 0.0f;
    }
    return std::cos(DegreesToRadians(degrees));
}

inline float LocalLightDistanceAttenuation(float distance, float range)
{
    if (!std::isfinite(distance) || !std::isfinite(range) || !(range > 0.0f)
        || distance >= range)
    {
        return 0.0f;
    }
    if (distance <= 0.0f)
    {
        return 1.0f;
    }
    const float normalized = 1.0f - (distance / range);
    return normalized * normalized;
}

inline float LocalLightSpotAngularAttenuation(
    core::Vec3 lightToFragment,
    core::Vec3 spotDirection,
    float innerCos,
    float outerCos)
{
    auto lengthSq = [](core::Vec3 value) {
        return value.x * value.x + value.y * value.y + value.z * value.z;
    };
    auto normalizeOr = [&](core::Vec3 value, core::Vec3 fallback) {
        const float sq = lengthSq(value);
        if (!std::isfinite(sq) || sq < kLocalLightAttenuationEpsilon)
        {
            return fallback;
        }
        const float inv = 1.0f / std::sqrt(sq);
        return core::Vec3{value.x * inv, value.y * inv, value.z * inv};
    };

    const core::Vec3 toFrag = normalizeOr(lightToFragment, spotDirection);
    const core::Vec3 axis = normalizeOr(spotDirection, {0.0f, -1.0f, 0.0f});
    const float cosTheta = toFrag.x * axis.x + toFrag.y * axis.y + toFrag.z * axis.z;
    if (!std::isfinite(cosTheta) || !std::isfinite(innerCos) || !std::isfinite(outerCos))
    {
        return 0.0f;
    }
    if (cosTheta <= outerCos)
    {
        return 0.0f;
    }
    if (cosTheta >= innerCos)
    {
        return 1.0f;
    }
    const float denom = innerCos - outerCos;
    if (!(denom > 1.0e-5f))
    {
        return 1.0f;
    }
    return (cosTheta - outerCos) / denom;
}

inline core::Vec3 LocalLightDiffuseContribution(
    core::Vec3 albedo,
    core::Vec3 normal,
    core::Vec3 fragmentPosition,
    const PackedLocalLight& light)
{
    auto finiteVec = [](core::Vec3 value) {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    };
    auto lengthSq = [](core::Vec3 value) {
        return value.x * value.x + value.y * value.y + value.z * value.z;
    };
    if (light.type != kLocalLightTypePoint && light.type != kLocalLightTypeSpot)
    {
        return {};
    }
    if (!finiteVec(albedo) || !finiteVec(normal) || !finiteVec(fragmentPosition)
        || !finiteVec(light.position) || !finiteVec(light.color)
        || !std::isfinite(light.intensity) || !std::isfinite(light.range))
    {
        return {};
    }
    if (!(light.intensity > 0.0f) || !(light.range > 0.0f))
    {
        return {};
    }

    const core::Vec3 lightToFrag{
        fragmentPosition.x - light.position.x,
        fragmentPosition.y - light.position.y,
        fragmentPosition.z - light.position.z};
    const float distanceSq = lengthSq(lightToFrag);
    if (!std::isfinite(distanceSq))
    {
        return {};
    }
    const float distance = std::sqrt(distanceSq);
    const float distanceAtt = LocalLightDistanceAttenuation(distance, light.range);
    if (!(distanceAtt > 0.0f))
    {
        return {};
    }

    float angular = 1.0f;
    if (light.type == kLocalLightTypeSpot)
    {
        angular = LocalLightSpotAngularAttenuation(
            lightToFrag, light.direction, light.innerCos, light.outerCos);
        if (!(angular > 0.0f))
        {
            return {};
        }
    }

    core::Vec3 toLight{
        light.position.x - fragmentPosition.x,
        light.position.y - fragmentPosition.y,
        light.position.z - fragmentPosition.z};
    const float toLightSq = lengthSq(toLight);
    if (!(toLightSq > kLocalLightAttenuationEpsilon))
    {
        return {};
    }
    const float invToLight = 1.0f / std::sqrt(toLightSq);
    toLight = {toLight.x * invToLight, toLight.y * invToLight, toLight.z * invToLight};
    const float normalSq = lengthSq(normal);
    if (!(normalSq > kLocalLightAttenuationEpsilon))
    {
        return {};
    }
    const float invNormal = 1.0f / std::sqrt(normalSq);
    const core::Vec3 unitNormal{
        normal.x * invNormal, normal.y * invNormal, normal.z * invNormal};
    float nDotL =
        unitNormal.x * toLight.x + unitNormal.y * toLight.y + unitNormal.z * toLight.z;
    if (!std::isfinite(nDotL) || nDotL < 0.0f)
    {
        nDotL = 0.0f;
    }
    const float scale = light.intensity * nDotL * distanceAtt * angular;
    return {albedo.x * light.color.x * scale,
            albedo.y * light.color.y * scale,
            albedo.z * light.color.z * scale};
}

inline PackedLocalLight PackPointLight(const world::PointLightSpec& authored)
{
    PackedLocalLight packed{};
    packed.type = kLocalLightTypePoint;
    packed.position = authored.position;
    packed.color = authored.color;
    packed.intensity = authored.intensity;
    packed.range = authored.range;
    packed.direction = {0.0f, -1.0f, 0.0f};
    packed.innerCos = 1.0f;
    packed.outerCos = 0.0f;
    return packed;
}

inline PackedLocalLight PackSpotLight(const world::SpotLightSpec& authored)
{
    PackedLocalLight packed{};
    packed.type = kLocalLightTypeSpot;
    packed.position = authored.position;
    packed.direction = world::CanonicalSpotLightDirection(authored.direction);
    packed.color = authored.color;
    packed.intensity = authored.intensity;
    packed.range = authored.range;
    packed.innerCos = CosineFromConeDegrees(authored.innerConeDegrees);
    packed.outerCos = CosineFromConeDegrees(authored.outerConeDegrees);
    return packed;
}

// Enabled Point Lights in authored array order, then enabled Spot Lights in
// authored array order. Disabled lights do not occupy a slot. Lights beyond
// kMaxActiveLocalLights are ignored. Optional effective-enabled masks replace
// authored Enabled for packing without mutating the specs; nullptr uses
// authored Enabled (editor preview / unlinked default).
inline PackedLocalLights PackAuthoredLocalLights(
    const std::vector<world::PointLightSpec>& pointLights,
    const std::vector<world::SpotLightSpec>& spotLights,
    const std::uint8_t* pointEffectiveEnabled = nullptr,
    std::size_t pointEffectiveCount = 0,
    const std::uint8_t* spotEffectiveEnabled = nullptr,
    std::size_t spotEffectiveCount = 0)
{
    PackedLocalLights packed{};
    auto tryAdd = [&](const PackedLocalLight& light) {
        if (packed.count >= kMaxActiveLocalLights)
        {
            return;
        }
        packed.lights[static_cast<std::size_t>(packed.count)] = light;
        packed.count += 1;
    };

    auto pointIsEnabled = [&](std::size_t index, const world::PointLightSpec& light) {
        if (pointEffectiveEnabled != nullptr && index < pointEffectiveCount)
        {
            return pointEffectiveEnabled[index] != 0;
        }
        return light.enabled;
    };
    auto spotIsEnabled = [&](std::size_t index, const world::SpotLightSpec& light) {
        if (spotEffectiveEnabled != nullptr && index < spotEffectiveCount)
        {
            return spotEffectiveEnabled[index] != 0;
        }
        return light.enabled;
    };

    for (std::size_t index = 0; index < pointLights.size(); ++index)
    {
        if (packed.count >= kMaxActiveLocalLights)
        {
            break;
        }
        const world::PointLightSpec& light = pointLights[index];
        if (!pointIsEnabled(index, light) || !world::PointLightIsValid(light))
        {
            continue;
        }
        tryAdd(PackPointLight(light));
    }
    for (std::size_t index = 0; index < spotLights.size(); ++index)
    {
        if (packed.count >= kMaxActiveLocalLights)
        {
            break;
        }
        const world::SpotLightSpec& light = spotLights[index];
        if (!spotIsEnabled(index, light) || !world::SpotLightIsValid(light))
        {
            continue;
        }
        tryAdd(PackSpotLight(light));
    }
    return packed;
}
}
