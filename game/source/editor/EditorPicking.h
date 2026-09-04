#pragma once

// CPU editor picking. Identity is EditorSelection, never a Jolt BodyID.

#include "core/Vec3.h"
#include "editor/EditorSelection.h"
#include "render/CameraView.h"
#include "world/CollectibleWorld.h"
#include "world/LevelDefinition.h"

#include <array>
#include <cstddef>

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
    core::Vec3 dynamicBoxCenter{};
    core::Vec3 dynamicBoxSize{};
};

struct PickingProxy
{
    EditorSelection selection{};
    core::Vec3 center{};
    core::Vec3 size{};
    float rotationZDegrees = 0.0f;
};

inline constexpr int kMaxPickingProxies = 24;
inline constexpr float kCollectiblePickingSize = world::kCollectibleVisualSize;

struct EditorPickingSet
{
    std::array<PickingProxy, kMaxPickingProxies> proxies{};
    int count = 0;
};

struct EditorHighlightRequest
{
    bool visible = false;
    core::Vec3 center{};
    core::Vec3 size{};
    float rotationZDegrees = 0.0f;
};

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
}
