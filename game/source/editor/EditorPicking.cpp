#include "editor/EditorPicking.h"

#include "editor/EditorMath.h"
#include "world/RespawnWorld.h"

#include <cmath>
#include <limits>

namespace editor
{
namespace
{
constexpr float kParallelEpsilon = 1.0e-8f;
constexpr float kTieEpsilon = 1.0e-6f;

void AddProxy(
    EditorPickingSet& set,
    EditorObjectKind kind,
    std::size_t index,
    core::Vec3 center,
    core::Vec3 size,
    float rotationZDegrees)
{
    if (set.count >= kMaxPickingProxies)
    {
        return;
    }

    PickingProxy& proxy = set.proxies[static_cast<std::size_t>(set.count)];
    proxy.selection = {kind, index};
    proxy.center = center;
    proxy.size = size;
    proxy.rotationZDegrees = rotationZDegrees;
    ++set.count;
}

RayHit IntersectProxy(Ray3 ray, const PickingProxy& proxy)
{
    if (proxy.rotationZDegrees != 0.0f)
    {
        return IntersectRayOrientedAabb(
            ray, proxy.center, proxy.size, proxy.rotationZDegrees);
    }
    return IntersectRayAabb(ray, proxy.center, proxy.size);
}
}

RayHit IntersectRayAabb(Ray3 ray, core::Vec3 center, core::Vec3 size)
{
    RayHit result{};
    if (!(size.x > 0.0f) || !(size.y > 0.0f) || !(size.z > 0.0f))
    {
        return result;
    }

    const core::Vec3 half = Scale(size, 0.5f);
    const core::Vec3 minimum = Sub(center, half);
    const core::Vec3 maximum = center + half;
    const float origin[3] = {ray.origin.x, ray.origin.y, ray.origin.z};
    const float direction[3] = {ray.direction.x, ray.direction.y, ray.direction.z};
    const float boxMin[3] = {minimum.x, minimum.y, minimum.z};
    const float boxMax[3] = {maximum.x, maximum.y, maximum.z};

    float tMin = 0.0f;
    float tMax = std::numeric_limits<float>::infinity();

    for (int axis = 0; axis < 3; ++axis)
    {
        if (std::fabs(direction[axis]) <= kParallelEpsilon)
        {
            if (origin[axis] < boxMin[axis] || origin[axis] > boxMax[axis])
            {
                return result;
            }
            continue;
        }

        float t0 = (boxMin[axis] - origin[axis]) / direction[axis];
        float t1 = (boxMax[axis] - origin[axis]) / direction[axis];
        if (t0 > t1)
        {
            const float swap = t0;
            t0 = t1;
            t1 = swap;
        }
        if (t0 > tMin)
        {
            tMin = t0;
        }
        if (t1 < tMax)
        {
            tMax = t1;
        }
        if (tMin > tMax)
        {
            return result;
        }
    }

    if (tMax < 0.0f)
    {
        return result;
    }

    result.hit = true;
    result.distance = tMin >= 0.0f ? tMin : 0.0f;
    return result;
}

RayHit IntersectRayOrientedAabb(
    Ray3 ray,
    core::Vec3 center,
    core::Vec3 size,
    float rotationZDegrees)
{
    // Inverse-rotate the ray into the box local frame. The ray parameter t is
    // unchanged because rotation preserves length.
    const Ray3 local{
        RotateZ(Sub(ray.origin, center), -rotationZDegrees),
        RotateZ(ray.direction, -rotationZDegrees)};
    return IntersectRayAabb(local, {}, size);
}

Ray3 ScreenToWorldRay(
    const render::CameraView& view,
    float mouseX,
    float mouseY,
    float viewportWidth,
    float viewportHeight)
{
    Ray3 ray{};
    ray.origin = view.position;

    const core::Vec3 forward = NormalizeOr(Sub(view.target, view.position), {0.0f, 0.0f, -1.0f});
    const core::Vec3 right = NormalizeOr(Cross(forward, view.up), {1.0f, 0.0f, 0.0f});
    const core::Vec3 up = Cross(right, forward);

    const float width = viewportWidth > 0.0f ? viewportWidth : 1.0f;
    const float height = viewportHeight > 0.0f ? viewportHeight : 1.0f;
    const float aspect = width / height;
    const float fov = view.fieldOfViewY > 0.0f && view.fieldOfViewY < 180.0f ? view.fieldOfViewY
                                                                            : 40.0f;
    const float tanHalf = std::tan(0.5f * fov * kDegreesToRadians);
    const float ndcX = (2.0f * mouseX / width) - 1.0f;
    const float ndcY = 1.0f - (2.0f * mouseY / height);

    ray.direction = NormalizeOr(
        forward + Scale(right, ndcX * tanHalf * aspect) + Scale(up, ndcY * tanHalf),
        forward);
    return ray;
}

EditorPickingWorldState AuthoredPickingWorldState(const world::LevelDefinition& appliedLevel)
{
    EditorPickingWorldState state{};
    state.movingPlatformCenter = {
        appliedLevel.movingPlatform.startX,
        appliedLevel.movingPlatform.centerY,
        appliedLevel.movingPlatform.centerZ};
    state.movingPlatformSize = appliedLevel.movingPlatform.size;
    state.dynamicBoxCenter = appliedLevel.dynamicBox.center;
    state.dynamicBoxSize = appliedLevel.dynamicBox.size;
    return state;
}

EditorPickingSet BuildPickingSet(
    const world::LevelDefinition& appliedLevel,
    const EditorPickingWorldState& worldState)
{
    EditorPickingSet set{};

    // Hierarchy order, Camera omitted: it is a framing spec, not a placed object.
    AddProxy(
        set,
        EditorObjectKind::Spawn,
        0,
        appliedLevel.initialSpawnVisualCenter,
        world::kPlayerVisualSize,
        0.0f);
    AddProxy(
        set,
        EditorObjectKind::Ground,
        0,
        appliedLevel.ground.center,
        appliedLevel.ground.size,
        0.0f);
    for (std::size_t index = 0; index < appliedLevel.elevatedPlatforms.size(); ++index)
    {
        AddProxy(
            set,
            EditorObjectKind::ElevatedPlatform,
            index,
            appliedLevel.elevatedPlatforms[index].center,
            appliedLevel.elevatedPlatforms[index].size,
            0.0f);
    }
    for (std::size_t index = 0; index < appliedLevel.slopes.size(); ++index)
    {
        AddProxy(
            set,
            EditorObjectKind::Slope,
            index,
            appliedLevel.slopes[index].center,
            appliedLevel.slopes[index].size,
            appliedLevel.slopes[index].rotationZDegrees);
    }
    AddProxy(
        set,
        EditorObjectKind::MovingPlatform,
        0,
        worldState.movingPlatformCenter,
        worldState.movingPlatformSize,
        0.0f);
    for (std::size_t index = 0; index < appliedLevel.checkpoints.size(); ++index)
    {
        AddProxy(
            set,
            EditorObjectKind::Checkpoint,
            index,
            appliedLevel.checkpoints[index].center,
            appliedLevel.checkpoints[index].size,
            0.0f);
    }
    for (std::size_t index = 0; index < appliedLevel.hazards.size(); ++index)
    {
        AddProxy(
            set,
            EditorObjectKind::Hazard,
            index,
            appliedLevel.hazards[index].center,
            appliedLevel.hazards[index].size,
            0.0f);
    }
    for (std::size_t index = 0; index < appliedLevel.collectibles.size(); ++index)
    {
        AddProxy(
            set,
            EditorObjectKind::Collectible,
            index,
            appliedLevel.collectibles[index].center,
            {kCollectiblePickingSize, kCollectiblePickingSize, kCollectiblePickingSize},
            0.0f);
    }
    AddProxy(
        set, EditorObjectKind::Goal, 0, appliedLevel.goal.center, appliedLevel.goal.size, 0.0f);
    AddProxy(
        set,
        EditorObjectKind::DynamicBox,
        0,
        worldState.dynamicBoxCenter,
        worldState.dynamicBoxSize,
        0.0f);
    return set;
}

EditorSelection PickNearest(Ray3 ray, const EditorPickingSet& set)
{
    EditorSelection best = ClearSelection();
    float bestDistance = std::numeric_limits<float>::infinity();

    for (int index = 0; index < set.count; ++index)
    {
        const RayHit hit = IntersectProxy(ray, set.proxies[static_cast<std::size_t>(index)]);
        if (!hit.hit)
        {
            continue;
        }
        // Strict < so an exact tie keeps the earlier (hierarchy-order) proxy.
        if (hit.distance + kTieEpsilon < bestDistance)
        {
            bestDistance = hit.distance;
            best = set.proxies[static_cast<std::size_t>(index)].selection;
        }
    }

    return best;
}

EditorHighlightRequest MakeHighlightRequest(
    EditorSelection selection,
    const EditorPickingSet& set)
{
    EditorHighlightRequest request{};
    if (selection.kind == EditorObjectKind::None)
    {
        return request;
    }

    for (int index = 0; index < set.count; ++index)
    {
        const PickingProxy& proxy = set.proxies[static_cast<std::size_t>(index)];
        if (proxy.selection == selection)
        {
            request.visible = true;
            request.center = proxy.center;
            request.size = proxy.size;
            request.rotationZDegrees = proxy.rotationZDegrees;
            return request;
        }
    }
    return request;
}
}
