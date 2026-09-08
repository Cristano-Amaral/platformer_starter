#include "editor/EditorPlacement.h"

#include "editor/LevelEditor.h"

namespace editor
{
const char* PlacementModeName(PlacementMode mode)
{
    switch (mode)
    {
    case PlacementMode::Platform:
        return "Platform";
    case PlacementMode::Checkpoint:
        return "Checkpoint";
    case PlacementMode::Hazard:
        return "Hazard";
    case PlacementMode::Collectible:
        return "Collectible";
    case PlacementMode::DynamicBox:
        return "Dynamic Box";
    default:
        return "None";
    }
}

EditorObjectKind KindFromPlacementMode(PlacementMode mode)
{
    switch (mode)
    {
    case PlacementMode::Platform:
        return EditorObjectKind::ElevatedPlatform;
    case PlacementMode::Checkpoint:
        return EditorObjectKind::Checkpoint;
    case PlacementMode::Hazard:
        return EditorObjectKind::Hazard;
    case PlacementMode::Collectible:
        return EditorObjectKind::Collectible;
    case PlacementMode::DynamicBox:
        return EditorObjectKind::DynamicBox;
    default:
        return EditorObjectKind::None;
    }
}

PlacementMode PlacementModeFromKind(EditorObjectKind kind)
{
    switch (kind)
    {
    case EditorObjectKind::ElevatedPlatform:
        return PlacementMode::Platform;
    case EditorObjectKind::Checkpoint:
        return PlacementMode::Checkpoint;
    case EditorObjectKind::Hazard:
        return PlacementMode::Hazard;
    case EditorObjectKind::Collectible:
        return PlacementMode::Collectible;
    case EditorObjectKind::DynamicBox:
        return PlacementMode::DynamicBox;
    default:
        return PlacementMode::None;
    }
}

LevelEditorRequest PlacementAddRequest(PlacementMode mode)
{
    switch (mode)
    {
    case PlacementMode::Platform:
        return LevelEditorRequest::AddPlatform;
    case PlacementMode::Checkpoint:
        return LevelEditorRequest::AddCheckpoint;
    case PlacementMode::Hazard:
        return LevelEditorRequest::AddHazard;
    case PlacementMode::Collectible:
        return LevelEditorRequest::AddCollectible;
    case PlacementMode::DynamicBox:
        return LevelEditorRequest::AddDynamicBox;
    default:
        return LevelEditorRequest::None;
    }
}

core::Vec3 DefaultPlacementSize(PlacementMode mode)
{
    switch (mode)
    {
    case PlacementMode::Platform:
        return kDefaultAddedPlatformSize;
    case PlacementMode::Checkpoint:
        return kDefaultAddedCheckpointSize;
    case PlacementMode::Hazard:
        return kDefaultAddedHazardSize;
    case PlacementMode::Collectible:
        return kDefaultAddedCollectibleSize;
    case PlacementMode::DynamicBox:
        return world::kDefaultDynamicBoxSize;
    default:
        return {};
    }
}

core::Vec3 DefaultPlacementOffset(PlacementMode mode)
{
    switch (mode)
    {
    case PlacementMode::Platform:
        return kDefaultAddedPlatformOffset;
    case PlacementMode::Checkpoint:
        return kDefaultAddedCheckpointOffset;
    case PlacementMode::Hazard:
        return kDefaultAddedHazardOffset;
    case PlacementMode::Collectible:
        return kDefaultAddedCollectibleOffset;
    case PlacementMode::DynamicBox:
        return kDefaultAddedDynamicBoxOffset;
    default:
        return {};
    }
}

bool IsEligiblePlacementSurface(EditorObjectKind kind)
{
    switch (kind)
    {
    case EditorObjectKind::Ground:
    case EditorObjectKind::ElevatedPlatform:
    case EditorObjectKind::Slope:
        return true;
    default:
        return false;
    }
}

PlacementSurfaceHit FindPlacementSurfaceHit(Ray3 ray, const EditorPickingSet& activeSet)
{
    PlacementSurfaceHit best{};
    float bestDistance = 0.0f;
    bool found = false;
    for (const PickingProxy& proxy : activeSet.proxies)
    {
        if (!IsEligiblePlacementSurface(proxy.selection.kind))
        {
            continue;
        }
        const RayHit hit = proxy.rotationZDegrees != 0.0f
            ? IntersectRayOrientedAabb(
                  ray, proxy.center, proxy.size, proxy.rotationZDegrees)
            : IntersectRayAabb(ray, proxy.center, proxy.size);
        if (!hit.hit)
        {
            continue;
        }
        if (!found || hit.distance < bestDistance)
        {
            found = true;
            bestDistance = hit.distance;
            best.hit = true;
            best.distance = hit.distance;
            best.point = {
                ray.origin.x + ray.direction.x * hit.distance,
                ray.origin.y + ray.direction.y * hit.distance,
                ray.origin.z + ray.direction.z * hit.distance};
        }
    }
    return best;
}

core::Vec3 MakePlacementFallbackCenter(core::Vec3 cameraAnchor)
{
    return cameraAnchor;
}

core::Vec3 MakePlacedObjectCenter(
    PlacementMode mode,
    core::Vec3 contactOrFallback,
    bool sitOnSurface)
{
    const core::Vec3 offset = DefaultPlacementOffset(mode);
    core::Vec3 center{
        contactOrFallback.x + offset.x,
        contactOrFallback.y + offset.y,
        contactOrFallback.z + offset.z};
    if (sitOnSurface)
    {
        center.y += DefaultPlacementSize(mode).y * 0.5f;
    }
    return center;
}

PlacementCandidate MakePlacementCandidate(PlacementMode mode, core::Vec3 worldCenter)
{
    PlacementCandidate candidate{};
    if (!PlacementModeIsActive(mode))
    {
        return candidate;
    }
    candidate.visible = true;
    candidate.mode = mode;
    candidate.kind = KindFromPlacementMode(mode);
    candidate.center = worldCenter;
    candidate.size = DefaultPlacementSize(mode);
    switch (mode)
    {
    case PlacementMode::Checkpoint:
        candidate.checkpoint.center = worldCenter;
        candidate.checkpoint.size = kDefaultAddedCheckpointSize;
        candidate.checkpoint.respawnPosition = {
            worldCenter.x + kDefaultAddedCheckpointRespawnOffset.x,
            worldCenter.y + kDefaultAddedCheckpointRespawnOffset.y,
            worldCenter.z + kDefaultAddedCheckpointRespawnOffset.z};
        break;
    case PlacementMode::Hazard:
        candidate.hazard.center = worldCenter;
        candidate.hazard.size = kDefaultAddedHazardSize;
        break;
    case PlacementMode::Collectible:
        candidate.collectible.center = worldCenter;
        candidate.collectible.size = kDefaultAddedCollectibleSize;
        break;
    default:
        break;
    }
    return candidate;
}

PlacementCandidate ResolvePlacementCandidate(
    PlacementMode mode,
    Ray3 ray,
    const EditorPickingSet& activeSet,
    core::Vec3 fallbackAnchor)
{
    if (!PlacementModeIsActive(mode))
    {
        return {};
    }
    const PlacementSurfaceHit surface = FindPlacementSurfaceHit(ray, activeSet);
    const core::Vec3 source =
        surface.hit ? surface.point : MakePlacementFallbackCenter(fallbackAnchor);
    PlacementCandidate candidate =
        MakePlacementCandidate(mode, MakePlacedObjectCenter(mode, source, surface.hit));
    candidate.source = surface.hit ? PlacementCandidateSource::SurfaceHit
                                   : PlacementCandidateSource::CameraFallback;
    return candidate;
}

const char* PlacementStopHintText()
{
    return "Esc or click again to stop";
}

const char* PlacementViewportActionHintText()
{
    return "LMB place | Esc cancel";
}

bool PlacementInteractionClaimsPointer(
    bool mouseCaptured,
    bool lookHeld,
    bool widgetConsumedPointer,
    bool gizmoConsumedPointer,
    bool gizmoDragging,
    bool gizmoHoveredOnPress)
{
    return mouseCaptured || lookHeld || widgetConsumedPointer || gizmoConsumedPointer
        || gizmoDragging || gizmoHoveredOnPress;
}

void UpdatePlacementPointerBlock(
    bool& blocked,
    bool selectPressed,
    bool selectHeld,
    bool selectReleased,
    bool claimed)
{
    if (claimed && (selectPressed || selectHeld || selectReleased))
    {
        blocked = true;
    }
}

bool ShouldConfirmPlacement(
    PlacementMode mode,
    bool selectPressed,
    bool mouseCaptured,
    bool lookHeld,
    bool widgetConsumedPointer,
    bool gizmoConsumedPointer,
    bool pointerBlocked,
    bool canAdd)
{
    return PlacementModeIsActive(mode) && selectPressed && !mouseCaptured && !lookHeld
        && !widgetConsumedPointer && !gizmoConsumedPointer && !pointerBlocked && canAdd;
}

bool ShouldCancelPlacementMode(
    PlacementMode mode,
    bool escapePressed,
    bool imguiWantsKeyboard)
{
    return PlacementModeIsActive(mode) && escapePressed && !imguiWantsKeyboard;
}
}
