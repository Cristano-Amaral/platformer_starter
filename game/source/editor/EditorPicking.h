#pragma once

// CPU editor picking. Identity is EditorSelection, never a Jolt BodyID.

#include "core/Vec3.h"
#include "editor/EditorSelection.h"
#include "render/CameraView.h"
#include "world/CollectibleWorld.h"
#include "world/LevelDefinition.h"
#include "world/StaticProp.h"

#include <cstddef>
#include <vector>

namespace editor
{
struct Ray3
{
    core::Vec3 origin{};
    core::Vec3 direction{};
};

struct RayHit
{
    bool hit = false;
    float distance = 0.0f;
};

struct EditorPickingWorldState
{
    core::Vec3 movingPlatformCenter{};
    core::Vec3 movingPlatformSize{};
    std::vector<core::Vec3> dynamicBoxCenters{};
    std::vector<core::Vec3> dynamicBoxSizes{};
    std::vector<core::Vec3> doorCenters{};
    std::vector<core::Vec3> doorSizes{};
};

struct PickingProxy
{
    EditorSelection selection{};
    core::Vec3 center{};
    core::Vec3 size{};
    float rotationZDegrees = 0.0f;
    bool usesStaticPropTransform = false;
    world::StaticPropSpec staticProp{};
    core::Vec3 localMin{-0.5f, -0.5f, -0.5f};
    core::Vec3 localMax{0.5f, 0.5f, 0.5f};
};

inline constexpr float kCollectiblePickingSize = world::kCollectibleVisualSize;

struct EditorPickingSet
{
    std::vector<PickingProxy> proxies;
};

struct EditorHighlightRequest
{
    bool visible = false;
    core::Vec3 center{};
    core::Vec3 size{};
    float rotationZDegrees = 0.0f;
};

// Editor-only pending workingCopy pick volume. selection is a workingCopy
// type+index, never an active index and never a GUID.
struct PendingPickProxy
{
    EditorSelection selection{};
    core::Vec3 center{};
    core::Vec3 size{};
};

struct StructuralIndexMap;

RayHit IntersectRayAabb(Ray3 ray, core::Vec3 center, core::Vec3 size);
RayHit IntersectRayOrientedAabb(
    Ray3 ray,
    core::Vec3 center,
    core::Vec3 size,
    float rotationZDegrees);

Ray3 ScreenToWorldRay(
    const render::CameraView& view,
    float mouseX,
    float mouseY,
    float viewportWidth,
    float viewportHeight);

struct EditorContentViewport;

Ray3 ScreenToWorldRayFromWindow(
    const render::CameraView& view,
    float windowMouseX,
    float windowMouseY,
    const EditorContentViewport& viewport);

EditorPickingWorldState AuthoredPickingWorldState(const world::LevelDefinition& appliedLevel);

// Viewport proxies for the currently applied/rendered level, plus runtime
// poses for objects that have already moved. Do not pass a working copy:
// unapplied Inspector edits must not move pick/highlight ahead of the world.
EditorPickingSet BuildPickingSet(
    const world::LevelDefinition& appliedLevel,
    const EditorPickingWorldState& worldState);

// Nearest positive hit. Exact distance ties keep the earlier proxy, which is
// the stable hierarchy order BuildPickingSet uses. No hit returns None.
EditorSelection PickNearest(Ray3 ray, const EditorPickingSet& set);

EditorHighlightRequest MakeHighlightRequest(
    EditorSelection selection,
    const EditorPickingSet& set);

// Same-frame ImGui / look / gizmo gating used by Application. Pending and
// active viewport picks share this; gizmo drag stays higher priority.
inline bool ShouldAttemptEditorViewportPick(
    bool selectPressed,
    bool mouseCaptured,
    bool lookHeld,
    bool widgetConsumedPointer,
    bool gizmoConsumedPointer)
{
    return selectPressed && !mouseCaptured && !lookHeld && !widgetConsumedPointer
        && !gizmoConsumedPointer;
}

// Nearest positive pending AABB hit. Exact ties keep the earlier proxy.
// Pending-deleted objects are not in this list.
EditorSelection PickNearestPending(Ray3 ray, const std::vector<PendingPickProxy>& proxies);

// Priority: nearest pending workingCopy hit, else active-world pick mapped
// through StructuralIndexMap, else empty. Returns false when an active hit is
// pending-deleted (ignore; do not assign selection).
bool TryResolveEditorViewportPick(
    Ray3 ray,
    const EditorPickingSet& activeSet,
    const std::vector<PendingPickProxy>& pendingProxies,
    const StructuralIndexMap& map,
    EditorSelection& outWorkingSelection);
}
