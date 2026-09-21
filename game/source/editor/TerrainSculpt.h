#pragma once

// Milestone 87: transient Development Editor Terrain Sculpt tool state.
// Not Level Format, not Dirty by itself, and never serialized.

#include "editor/EditorPicking.h"
#include "editor/EditorSelection.h"
#include "world/LevelDefinition.h"
#include "world/TerrainSculpt.h"

namespace editor
{
struct TerrainSculptState
{
    bool mode = false;
    world::TerrainSculptOperation operation = world::TerrainSculptOperation::Raise;
    float radius = world::kDefaultTerrainSculptRadius;
    float strength = world::kDefaultTerrainSculptStrength;
    world::TerrainSculptStroke stroke{};
    bool previewHit = false;
    core::Vec3 previewPoint{};
};

inline const char* TerrainSculptOperationName(world::TerrainSculptOperation operation)
{
    switch (operation)
    {
    case world::TerrainSculptOperation::Raise:
        return "Raise";
    case world::TerrainSculptOperation::Lower:
        return "Lower";
    case world::TerrainSculptOperation::Smooth:
        return "Smooth";
    case world::TerrainSculptOperation::Flatten:
        return "Flatten";
    }
    return "Raise";
}

inline bool TerrainSculptOperationIsValid(world::TerrainSculptOperation operation)
{
    return operation == world::TerrainSculptOperation::Raise
        || operation == world::TerrainSculptOperation::Lower
        || operation == world::TerrainSculptOperation::Smooth
        || operation == world::TerrainSculptOperation::Flatten;
}

inline void SanitizeTerrainSculptState(TerrainSculptState& sculpt)
{
    sculpt.radius = world::SanitizeTerrainSculptRadius(sculpt.radius);
    sculpt.strength = world::SanitizeTerrainSculptStrength(sculpt.strength);
    if (!TerrainSculptOperationIsValid(sculpt.operation))
    {
        sculpt.operation = world::TerrainSculptOperation::Raise;
    }
}

inline bool TerrainSculptHasAuthoringSurface(const world::LevelDefinition& workingCopy)
{
    return workingCopy.hasTerrain && workingCopy.terrain.enabled
        && world::TerrainSpecIsValid(workingCopy.terrain);
}

inline bool TerrainSculptInteractionIsActive(
    const TerrainSculptState& sculpt,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection)
{
    return sculpt.mode && selection.kind == EditorObjectKind::Terrain
        && TerrainSculptHasAuthoringSurface(workingCopy);
}

inline void ResetTerrainSculptState(TerrainSculptState& sculpt)
{
    sculpt = {};
}

inline void EndTerrainSculptStroke(TerrainSculptState& sculpt)
{
    world::EndTerrainSculptStroke(sculpt.stroke);
}

inline void ReconcileTerrainSculptState(
    TerrainSculptState& sculpt,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection)
{
    SanitizeTerrainSculptState(sculpt);
    if (!workingCopy.hasTerrain || selection.kind != EditorObjectKind::Terrain)
    {
        sculpt.mode = false;
        EndTerrainSculptStroke(sculpt);
        sculpt.previewHit = false;
        sculpt.previewPoint = {};
        return;
    }
    if (!TerrainSculptInteractionIsActive(sculpt, workingCopy, selection))
    {
        EndTerrainSculptStroke(sculpt);
    }
}

inline bool PickWorkingCopyTerrainSculptHit(
    const world::LevelDefinition& workingCopy,
    Ray3 ray,
    core::Vec3& outPoint)
{
    if (!TerrainSculptHasAuthoringSurface(workingCopy))
    {
        return false;
    }
    float distance = 0.0f;
    return world::IntersectRayTerrain(
        workingCopy.terrain, ray.origin, ray.direction, distance, outPoint);
}

struct TerrainSculptFrameResult
{
    bool mutatedWorkingCopy = false;
};

inline TerrainSculptFrameResult TickTerrainSculpt(
    TerrainSculptState& sculpt,
    world::LevelDefinition& workingCopy,
    EditorSelection selection,
    Ray3 ray,
    bool selectPressed,
    bool selectHeld,
    bool selectReleased,
    bool mouseCaptured,
    bool lookHeld,
    bool widgetConsumedPointer)
{
    TerrainSculptFrameResult result{};
    ReconcileTerrainSculptState(sculpt, workingCopy, selection);
    sculpt.previewHit = false;
    sculpt.previewPoint = {};
    if (!TerrainSculptInteractionIsActive(sculpt, workingCopy, selection))
    {
        return result;
    }

    const bool pointerFree = !mouseCaptured && !lookHeld && !widgetConsumedPointer;
    core::Vec3 hit{};
    const bool hasHit = pointerFree && PickWorkingCopyTerrainSculptHit(workingCopy, ray, hit);
    if (hasHit)
    {
        sculpt.previewHit = true;
        sculpt.previewPoint = hit;
    }

    if (!pointerFree)
    {
        if (selectReleased || !selectHeld)
        {
            EndTerrainSculptStroke(sculpt);
        }
        return result;
    }

    world::TerrainSculptStampRequest request{};
    request.operation = sculpt.operation;
    request.radius = sculpt.radius;
    request.strength = sculpt.strength;

    if (selectPressed)
    {
        if (hasHit)
        {
            request.centerX = hit.x;
            request.centerZ = hit.z;
            result.mutatedWorkingCopy = world::BeginTerrainSculptStroke(
                sculpt.stroke, workingCopy.terrain, request, hit.y);
        }
    }
    else if (sculpt.stroke.active && selectHeld && hasHit)
    {
        request.centerX = hit.x;
        request.centerZ = hit.z;
        result.mutatedWorkingCopy = world::ContinueTerrainSculptStroke(
            sculpt.stroke, workingCopy.terrain, request, hit.x, hit.z);
    }

    if (selectReleased || !selectHeld)
    {
        EndTerrainSculptStroke(sculpt);
    }
    return result;
}

inline bool ShouldCancelTerrainSculptMode(
    const TerrainSculptState& sculpt,
    bool escapePressed,
    bool imguiWantsKeyboard)
{
    return sculpt.mode && escapePressed && !imguiWantsKeyboard;
}

inline const char* TerrainSculptHudName(const TerrainSculptState& sculpt)
{
    return TerrainSculptOperationName(sculpt.operation);
}

inline const char* TerrainSculptViewportHintText()
{
    return "LMB sculpt | Esc exit";
}
}
