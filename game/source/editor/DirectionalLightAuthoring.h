#pragma once

// Milestone 85.1/85.2 editor-only Directional Light visualization/manipulation.
// The displayed position and scale are authoring cosmetics, not light physics.
// Rotate still edits the authored ray direction.

#include "core/Vec3.h"
#include "world/LevelEnvironment.h"

#include <cmath>

namespace editor
{
inline constexpr core::Vec3 kDirectionalLightAuthoringAnchor{0.0f, 8.0f, 0.0f};
inline constexpr core::Vec3 kDirectionalLightAuthoringPickSize{0.85f, 0.85f, 0.85f};
inline constexpr float kDirectionalLightAuthoringDefaultScale = 1.0f;
inline constexpr float kMinDirectionalLightVisualizationScale = 0.25f;
inline constexpr float kMaxDirectionalLightVisualizationScale = 8.0f;

struct DirectionalLightVisualization
{
    core::Vec3 anchor = kDirectionalLightAuthoringAnchor;
    float scale = kDirectionalLightAuthoringDefaultScale;
};

inline bool DirectionalLightVisualizationAnchorIsFinite(core::Vec3 anchor)
{
    return std::isfinite(anchor.x) && std::isfinite(anchor.y) && std::isfinite(anchor.z);
}

inline float ClampDirectionalLightVisualizationScale(float scale)
{
    if (!std::isfinite(scale))
    {
        return kDirectionalLightAuthoringDefaultScale;
    }
    if (scale < kMinDirectionalLightVisualizationScale)
    {
        return kMinDirectionalLightVisualizationScale;
    }
    if (scale > kMaxDirectionalLightVisualizationScale)
    {
        return kMaxDirectionalLightVisualizationScale;
    }
    return scale;
}

inline void CanonicalizeDirectionalLightVisualization(DirectionalLightVisualization& visualization)
{
    if (!DirectionalLightVisualizationAnchorIsFinite(visualization.anchor))
    {
        visualization.anchor = kDirectionalLightAuthoringAnchor;
    }
    visualization.scale = ClampDirectionalLightVisualizationScale(visualization.scale);
}

inline void ResetDirectionalLightVisualization(DirectionalLightVisualization& visualization)
{
    visualization = {};
}

inline core::Vec3 DirectionalLightAuthoringAnchor()
{
    return kDirectionalLightAuthoringAnchor;
}

inline core::Vec3 DirectionalLightVisualizationAnchor(
    const DirectionalLightVisualization* visualization)
{
    if (visualization == nullptr)
    {
        return kDirectionalLightAuthoringAnchor;
    }
    if (!DirectionalLightVisualizationAnchorIsFinite(visualization->anchor))
    {
        return kDirectionalLightAuthoringAnchor;
    }
    return visualization->anchor;
}

inline core::Vec3 ScaledDirectionalLightPickSize(float scale)
{
    const float clamped = ClampDirectionalLightVisualizationScale(scale);
    return {
        kDirectionalLightAuthoringPickSize.x * clamped,
        kDirectionalLightAuthoringPickSize.y * clamped,
        kDirectionalLightAuthoringPickSize.z * clamped};
}

inline core::Vec3 DirectionalLightVisualizationPickExtents(
    const DirectionalLightVisualization* visualization)
{
    const float scale = visualization == nullptr
        ? kDirectionalLightAuthoringDefaultScale
        : visualization->scale;
    return ScaledDirectionalLightPickSize(scale);
}

inline core::Vec3 RotateAuthoredDirectionalRay(core::Vec3 rayDirection, core::Vec3 axis, float degrees)
{
    rayDirection = world::CanonicalLevelDirectionalRay(rayDirection);
    if (!std::isfinite(degrees))
    {
        return rayDirection;
    }
    const float axisLengthSq =
        axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
    if (!(axisLengthSq > 1.0e-10f))
    {
        return rayDirection;
    }
    const float invAxis = 1.0f / std::sqrt(axisLengthSq);
    const core::Vec3 unitAxis{axis.x * invAxis, axis.y * invAxis, axis.z * invAxis};
    const float radians = degrees * (3.14159265f / 180.0f);
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    const core::Vec3 axisCrossRay{
        unitAxis.y * rayDirection.z - unitAxis.z * rayDirection.y,
        unitAxis.z * rayDirection.x - unitAxis.x * rayDirection.z,
        unitAxis.x * rayDirection.y - unitAxis.y * rayDirection.x};
    const float axisDotRay =
        unitAxis.x * rayDirection.x + unitAxis.y * rayDirection.y + unitAxis.z * rayDirection.z;
    const core::Vec3 rotated{
        rayDirection.x * cosine + axisCrossRay.x * sine + unitAxis.x * axisDotRay * (1.0f - cosine),
        rayDirection.y * cosine + axisCrossRay.y * sine + unitAxis.y * axisDotRay * (1.0f - cosine),
        rayDirection.z * cosine + axisCrossRay.z * sine + unitAxis.z * axisDotRay * (1.0f - cosine)};
    return world::CanonicalLevelDirectionalRay(rotated);
}

inline bool TryCommitAuthoredDirectionalRay(core::Vec3 proposed, core::Vec3& current)
{
    if (!world::LevelDirectionalRayIsValid(proposed))
    {
        return false;
    }
    current = world::CanonicalLevelDirectionalRay(proposed);
    return true;
}
}
