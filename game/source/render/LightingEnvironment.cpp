#include "render/LightingEnvironment.h"

#include <cmath>

namespace render
{
namespace
{
bool Finite(float value)
{
    return std::isfinite(value);
}

bool FiniteVec(core::Vec3 value)
{
    return Finite(value.x) && Finite(value.y) && Finite(value.z);
}

float Clamp(float value, float lo, float hi)
{
    if (value < lo)
    {
        return lo;
    }
    if (value > hi)
    {
        return hi;
    }
    return value;
}

core::Vec3 ClampColor(core::Vec3 color, core::Vec3 fallback)
{
    if (!FiniteVec(color))
    {
        return fallback;
    }
    return {
        Clamp(color.x, 0.0f, 1.0f),
        Clamp(color.y, 0.0f, 1.0f),
        Clamp(color.z, 0.0f, 1.0f)};
}

float LengthSquared(core::Vec3 value)
{
    return value.x * value.x + value.y * value.y + value.z * value.z;
}

core::Vec3 Scale(core::Vec3 value, float scalar)
{
    return {value.x * scalar, value.y * scalar, value.z * scalar};
}

core::Vec3 Sub(core::Vec3 a, core::Vec3 b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

core::Vec3 Cross(core::Vec3 a, core::Vec3 b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x};
}

core::Vec3 SnapToTexel(core::Vec3 value, float texel)
{
    if (!(texel > 0.0f) || !Finite(texel) || !FiniteVec(value))
    {
        return value;
    }
    return {
        std::round(value.x / texel) * texel,
        std::round(value.y / texel) * texel,
        std::round(value.z / texel) * texel};
}

core::Vec3 NormalizeVec(core::Vec3 value, core::Vec3 fallback)
{
    if (!FiniteVec(value))
    {
        return fallback;
    }
    const float lengthSq = LengthSquared(value);
    if (lengthSq < kMinDirectionalLightLength * kMinDirectionalLightLength)
    {
        return fallback;
    }
    return Scale(value, 1.0f / std::sqrt(lengthSq));
}
}

LightingEnvironment MakeDefaultLightingEnvironment()
{
    return ValidateLightingEnvironment(LightingEnvironment{});
}

core::Vec3 NormalizeDirectionalLight(core::Vec3 rayDirection)
{
    if (!FiniteVec(rayDirection))
    {
        return kFallbackDirectionalRayDirection;
    }
    const float lengthSq = LengthSquared(rayDirection);
    if (lengthSq < kMinDirectionalLightLength * kMinDirectionalLightLength)
    {
        return kFallbackDirectionalRayDirection;
    }
    return Scale(rayDirection, 1.0f / std::sqrt(lengthSq));
}

core::Vec3 DirectionalLightTowardSurface(core::Vec3 rayDirection)
{
    const core::Vec3 ray = NormalizeDirectionalLight(rayDirection);
    return {-ray.x, -ray.y, -ray.z};
}

core::Vec3 RotateDirectionalRay(core::Vec3 rayDirection, core::Vec3 axis, float degrees)
{
    const core::Vec3 ray = NormalizeDirectionalLight(rayDirection);
    if (!Finite(degrees))
    {
        return ray;
    }
    const core::Vec3 unitAxis = NormalizeVec(axis, {0.0f, 1.0f, 0.0f});
    const float radians = degrees * (3.14159265f / 180.0f);
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    const core::Vec3 axisCrossRay = Cross(unitAxis, ray);
    const float axisDotRay = unitAxis.x * ray.x + unitAxis.y * ray.y + unitAxis.z * ray.z;
    const core::Vec3 rotated{
        ray.x * cosine + axisCrossRay.x * sine + unitAxis.x * axisDotRay * (1.0f - cosine),
        ray.y * cosine + axisCrossRay.y * sine + unitAxis.y * axisDotRay * (1.0f - cosine),
        ray.z * cosine + axisCrossRay.z * sine + unitAxis.z * axisDotRay * (1.0f - cosine)};
    return NormalizeDirectionalLight(rotated);
}

int NormalizedShadowMapResolution(int resolution)
{
    int value = resolution;
    if (value < kMinShadowMapResolution)
    {
        value = kMinShadowMapResolution;
    }
    if (value > kMaxShadowMapResolution)
    {
        value = kMaxShadowMapResolution;
    }
    return value;
}

float NormalizedShadowBias(float bias)
{
    if (!Finite(bias))
    {
        return DirectionalShadowConfig{}.bias;
    }
    return Clamp(bias, 0.00005f, 0.05f);
}

LightingEnvironment ValidateLightingEnvironment(LightingEnvironment environment)
{
    const LightingEnvironment defaults{};
    environment.ambient.color = ClampColor(environment.ambient.color, defaults.ambient.color);
    environment.ambient.intensity = Finite(environment.ambient.intensity)
        ? Clamp(environment.ambient.intensity, 0.0f, kMaxAmbientIntensity)
        : defaults.ambient.intensity;

    environment.directional.color =
        ClampColor(environment.directional.color, defaults.directional.color);
    environment.directional.intensity = Finite(environment.directional.intensity)
        ? Clamp(environment.directional.intensity, 0.0f, kMaxLightIntensity)
        : defaults.directional.intensity;
    environment.directional.rayDirection =
        NormalizeDirectionalLight(environment.directional.rayDirection);

    environment.shadows.mapResolution =
        NormalizedShadowMapResolution(environment.shadows.mapResolution);
    environment.shadows.coverage = Finite(environment.shadows.coverage)
        ? Clamp(environment.shadows.coverage, 8.0f, 256.0f)
        : defaults.shadows.coverage;
    environment.shadows.nearPlane = Finite(environment.shadows.nearPlane)
        ? Clamp(environment.shadows.nearPlane, 0.05f, 40.0f)
        : defaults.shadows.nearPlane;
    environment.shadows.farPlane = Finite(environment.shadows.farPlane)
        ? Clamp(environment.shadows.farPlane, environment.shadows.nearPlane + 1.0f, 400.0f)
        : defaults.shadows.farPlane;
    environment.shadows.lightDistance = Finite(environment.shadows.lightDistance)
        ? Clamp(environment.shadows.lightDistance, 8.0f, 200.0f)
        : defaults.shadows.lightDistance;
    environment.shadows.bias = NormalizedShadowBias(environment.shadows.bias);
    if (!FiniteVec(environment.shadows.focus))
    {
        environment.shadows.focus = defaults.shadows.focus;
    }
    return environment;
}

LightingEnvironment MakeLightingEnvironmentFromAuthored(const world::LevelEnvironment& authored)
{
    LightingEnvironment environment = MakeDefaultLightingEnvironment();
    environment.ambient.color = authored.ambientColor;
    environment.ambient.intensity = authored.ambientIntensity;
    environment.directional.authoredEnabled = authored.directionalEnabled;
    environment.directional.rayDirection = authored.directionalRayDirection;
    environment.directional.color = authored.directionalColor;
    environment.directional.intensity = authored.directionalIntensity;
    environment.directional.shadowsEnabled = authored.directionalShadowsEnabled;
    return ValidateLightingEnvironment(environment);
}

bool EffectiveDirectionalEnabled(const LightingEnvironment& environment)
{
    return environment.directional.authoredEnabled;
}

bool DirectionalShadowsAreActive(const LightingEnvironment& environment)
{
    return EffectiveDirectionalEnabled(environment) && environment.directional.shadowsEnabled;
}

DirectionalLightView BuildDirectionalLightView(
    const DirectionalLight& light,
    const DirectionalShadowConfig& shadows)
{
    const LightingEnvironment validated = ValidateLightingEnvironment(LightingEnvironment{
        {},
        light,
        shadows});
    const core::Vec3 ray = validated.directional.rayDirection;
    const float texel =
        validated.shadows.coverage / static_cast<float>(validated.shadows.mapResolution);
    const core::Vec3 focus = SnapToTexel(validated.shadows.focus, texel);

    DirectionalLightView view{};
    view.target = focus;
    view.eye = Sub(focus, Scale(ray, validated.shadows.lightDistance));

    const core::Vec3 worldUp{0.0f, 1.0f, 0.0f};
    core::Vec3 right = Cross(worldUp, ray);
    if (LengthSquared(right) < 1.0e-6f)
    {
        right = Cross(core::Vec3{0.0f, 0.0f, 1.0f}, ray);
    }
    view.up = NormalizeVec(Cross(ray, right), worldUp);
    if (view.up.y < 0.0f)
    {
        view.up = Scale(view.up, -1.0f);
    }
    return view;
}

ShadowProjection BuildShadowProjection(const DirectionalShadowConfig& shadows)
{
    const DirectionalShadowConfig validated =
        ValidateLightingEnvironment(LightingEnvironment{{}, {}, shadows}).shadows;
    const float half = validated.coverage * 0.5f;
    ShadowProjection projection{};
    projection.left = -half;
    projection.right = half;
    projection.bottom = -half;
    projection.top = half;
    projection.nearPlane = validated.nearPlane;
    projection.farPlane = validated.farPlane;
    return projection;
}

bool ShouldCastDirectionalShadow(ShadowParticipant participant)
{
    switch (participant)
    {
    case ShadowParticipant::GreyboxWorld:
    case ShadowParticipant::MovingPlatform:
    case ShadowParticipant::Slope:
    case ShadowParticipant::DynamicBox:
    case ShadowParticipant::PressurePlateGameplay:
    case ShadowParticipant::Door:
    case ShadowParticipant::StaticProp:
    case ShadowParticipant::ItemPickup:
    case ShadowParticipant::Player:
    case ShadowParticipant::Hazard:
    case ShadowParticipant::Collectible:
    case ShadowParticipant::CheckpointMarker:
    case ShadowParticipant::LevelGoalMarker:
        return true;
    case ShadowParticipant::PressurePlateEditorGhost:
    case ShadowParticipant::LevelGoalEditorVolume:
    case ShadowParticipant::EditorGrid:
    case ShadowParticipant::EditorGizmo:
    case ShadowParticipant::EditorGhost:
    case ShadowParticipant::EditorHighlight:
    case ShadowParticipant::OverlayWires:
    case ShadowParticipant::Thumbnail:
    case ShadowParticipant::Preview:
        return false;
    }
    return false;
}

bool ShouldReceiveDirectionalShadow(ShadowParticipant participant)
{
    return ShouldCastDirectionalShadow(participant);
}
}
