#pragma once

// Narrow Static Prop world/local transform helpers. Not a generic Transform
// system. Euler XYZ degrees match renderer rlTranslate/rlRotate/rlScale.

#include "editor/EditorMath.h"
#include "editor/EditorPicking.h"
#include "world/ItemPickup.h"
#include "world/StaticProp.h"

#include <cmath>

namespace editor
{
inline constexpr core::Vec3 kStaticPropDefaultLocalMin{-0.5f, -0.5f, -0.5f};
inline constexpr core::Vec3 kStaticPropDefaultLocalMax{0.5f, 0.5f, 0.5f};

inline core::Vec3 ScaleAxes(core::Vec3 value, core::Vec3 scale)
{
    return {value.x * scale.x, value.y * scale.y, value.z * scale.z};
}

inline core::Vec3 InverseScaleAxes(core::Vec3 value, core::Vec3 scale)
{
    return {
        scale.x != 0.0f ? value.x / scale.x : 0.0f,
        scale.y != 0.0f ? value.y / scale.y : 0.0f,
        scale.z != 0.0f ? value.z / scale.z : 0.0f};
}

inline core::Vec3 StaticPropWorldFromLocal(
    const world::StaticPropSpec& spec,
    core::Vec3 local)
{
    return spec.position
        + RotateEulerXYZ(ScaleAxes(local, spec.scale), spec.rotationDegrees);
}

inline void StaticPropWorldCorners(
    const world::StaticPropSpec& spec,
    core::Vec3 localMin,
    core::Vec3 localMax,
    core::Vec3 outCorners[8])
{
    outCorners[0] = StaticPropWorldFromLocal(spec, {localMin.x, localMin.y, localMin.z});
    outCorners[1] = StaticPropWorldFromLocal(spec, {localMax.x, localMin.y, localMin.z});
    outCorners[2] = StaticPropWorldFromLocal(spec, {localMin.x, localMax.y, localMin.z});
    outCorners[3] = StaticPropWorldFromLocal(spec, {localMax.x, localMax.y, localMin.z});
    outCorners[4] = StaticPropWorldFromLocal(spec, {localMin.x, localMin.y, localMax.z});
    outCorners[5] = StaticPropWorldFromLocal(spec, {localMax.x, localMin.y, localMax.z});
    outCorners[6] = StaticPropWorldFromLocal(spec, {localMin.x, localMax.y, localMax.z});
    outCorners[7] = StaticPropWorldFromLocal(spec, {localMax.x, localMax.y, localMax.z});
}

inline void StaticPropWorldAabb(
    const world::StaticPropSpec& spec,
    core::Vec3 localMin,
    core::Vec3 localMax,
    core::Vec3& outCenter,
    core::Vec3& outSize)
{
    core::Vec3 corners[8]{};
    StaticPropWorldCorners(spec, localMin, localMax, corners);
    core::Vec3 minimum = corners[0];
    core::Vec3 maximum = corners[0];
    for (int index = 1; index < 8; ++index)
    {
        minimum.x = corners[index].x < minimum.x ? corners[index].x : minimum.x;
        minimum.y = corners[index].y < minimum.y ? corners[index].y : minimum.y;
        minimum.z = corners[index].z < minimum.z ? corners[index].z : minimum.z;
        maximum.x = corners[index].x > maximum.x ? corners[index].x : maximum.x;
        maximum.y = corners[index].y > maximum.y ? corners[index].y : maximum.y;
        maximum.z = corners[index].z > maximum.z ? corners[index].z : maximum.z;
    }
    outCenter = {
        (minimum.x + maximum.x) * 0.5f,
        (minimum.y + maximum.y) * 0.5f,
        (minimum.z + maximum.z) * 0.5f};
    outSize = {maximum.x - minimum.x, maximum.y - minimum.y, maximum.z - minimum.z};
    if (!(outSize.x > 0.0f))
    {
        outSize.x = 0.001f;
    }
    if (!(outSize.y > 0.0f))
    {
        outSize.y = 0.001f;
    }
    if (!(outSize.z > 0.0f))
    {
        outSize.z = 0.001f;
    }
}

inline RayHit IntersectRayStaticProp(
    Ray3 ray,
    const world::StaticPropSpec& spec,
    core::Vec3 localMin,
    core::Vec3 localMax)
{
    RayHit miss{};
    if (!world::StaticPropTransformIsValid(spec))
    {
        return miss;
    }
    const core::Vec3 localOrigin = InverseScaleAxes(
        InverseRotateEulerXYZ(Sub(ray.origin, spec.position), spec.rotationDegrees),
        spec.scale);
    const core::Vec3 localDirection = InverseScaleAxes(
        InverseRotateEulerXYZ(ray.direction, spec.rotationDegrees),
        spec.scale);
    const core::Vec3 size{
        localMax.x - localMin.x,
        localMax.y - localMin.y,
        localMax.z - localMin.z};
    const core::Vec3 center{
        (localMin.x + localMax.x) * 0.5f,
        (localMin.y + localMax.y) * 0.5f,
        (localMin.z + localMax.z) * 0.5f};
    return IntersectRayAabb({localOrigin, localDirection}, center, size);
}

// Editor visual bounds. Gameplay targeting still uses ItemPickupSpec::position.
inline void ItemPickupEditorBounds(
    const world::ItemPickupSpec& pickup,
    core::Vec3& outCenter,
    core::Vec3& outSize,
    core::Vec3 localMin = kStaticPropDefaultLocalMin,
    core::Vec3 localMax = kStaticPropDefaultLocalMax)
{
    if (pickup.modelIdentity.empty())
    {
        outCenter = pickup.position;
        outSize = world::kItemPickupVisualExtents;
        return;
    }
    StaticPropWorldAabb(
        world::ItemPickupVisualProp(pickup),
        localMin,
        localMax,
        outCenter,
        outSize);
}

inline void AssignLoadedStaticPropLocalBounds(
    core::Vec3 loadedMin,
    core::Vec3 loadedMax,
    core::Vec3& localMin,
    core::Vec3& localMax)
{
    localMin = loadedMin;
    localMax = loadedMax;
}

inline bool PointInsideAabb(core::Vec3 point, core::Vec3 center, core::Vec3 size)
{
    const float halfX = size.x * 0.5f;
    const float halfY = size.y * 0.5f;
    const float halfZ = size.z * 0.5f;
    return point.x >= center.x - halfX && point.x <= center.x + halfX
        && point.y >= center.y - halfY && point.y <= center.y + halfY
        && point.z >= center.z - halfZ && point.z <= center.z + halfZ;
}

inline bool StaticPropContainsWorldPoint(
    const world::StaticPropSpec& spec,
    core::Vec3 localMin,
    core::Vec3 localMax,
    core::Vec3 worldPoint)
{
    if (!world::StaticPropTransformIsValid(spec))
    {
        return false;
    }
    core::Vec3 center{};
    core::Vec3 size{};
    StaticPropWorldAabb(spec, localMin, localMax, center, size);
    return PointInsideAabb(worldPoint, center, size);
}

inline bool AuthoredLevelsProtectStaticPropIdentity(
    const world::LevelDefinition& workingCopy,
    const world::LevelDefinition& active,
    const world::LevelDefinition& savedSourceBaseline,
    std::string_view identity)
{
    return world::LevelReferencesStaticPropIdentity(workingCopy, identity)
        || world::LevelReferencesStaticPropIdentity(active, identity)
        || world::LevelReferencesStaticPropIdentity(savedSourceBaseline, identity);
}

// Why a Static Prop draws the placeholder cube instead of its model. Runtime
// rendering authority stays staged-only: this classifies, it never falls back
// to canonical source and never cooks or stages. The caller supplies the
// staged-file answer so the policy stays free of filesystem access.
enum class StaticPropAssetState
{
    Ok,
    InvalidIdentity,
    MissingStagedRuntimeModel,
};

inline StaticPropAssetState ClassifyStaticPropAsset(
    std::string_view identity,
    bool stagedRuntimeModelExists)
{
    if (!world::StaticPropIdentityIsValid(identity))
    {
        return StaticPropAssetState::InvalidIdentity;
    }
    if (!stagedRuntimeModelExists)
    {
        return StaticPropAssetState::MissingStagedRuntimeModel;
    }
    return StaticPropAssetState::Ok;
}

// nullptr when the staged model is usable, so callers can skip the line.
inline const char* StaticPropAssetStateMessage(StaticPropAssetState state)
{
    switch (state)
    {
    case StaticPropAssetState::InvalidIdentity:
        return "Invalid static model identity. Expected models/<file>.glb.";
    case StaticPropAssetState::MissingStagedRuntimeModel:
        return "Static model is not available in staged runtime assets, so the viewport draws a "
               "placeholder cube. Run Build > Cook & Stage.";
    case StaticPropAssetState::Ok:
        break;
    }
    return nullptr;
}

inline std::string StaticPropReferencedDeleteMessage(std::string_view identity)
{
    std::string message = "Cannot delete ";
    message.append(identity);
    message += ": it is referenced by one or more Static Props in the current authored level "
               "(working copy, applied world, or last saved source).";
    return message;
}
}
