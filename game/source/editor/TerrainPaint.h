#pragma once

// Milestone 90: transient Development Editor Terrain Paint tool state.
// Not Level Format, not Dirty by itself, and never serialized.

#include "editor/EditorPicking.h"
#include "editor/EditorSelection.h"
#include "editor/TerrainSculpt.h"
#include "world/LevelDefinition.h"
#include "world/TerrainPaint.h"

#include <string>

namespace editor
{
struct TerrainPaintState
{
    bool mode = false;
    int selectedLayer = 0;
    float radius = world::kDefaultTerrainPaintRadius;
    float strength = world::kDefaultTerrainPaintStrength;
    world::TerrainPaintStroke stroke{};
    bool previewHit = false;
    core::Vec3 previewPoint{};
};

inline void SanitizeTerrainPaintState(TerrainPaintState& paint, const world::TerrainSpec& terrain)
{
    paint.radius = world::SanitizeTerrainPaintRadius(paint.radius);
    paint.strength = world::SanitizeTerrainPaintStrength(paint.strength);
    const int layerCount = world::TerrainMaterialLayerCount(terrain);
    if (paint.selectedLayer < 0 || paint.selectedLayer >= layerCount)
    {
        paint.selectedLayer = 0;
    }
}

inline bool TrySelectTerrainPaintLayer(
    TerrainPaintState& paint,
    int layer,
    const world::TerrainSpec& terrain)
{
    const int layerCount = world::TerrainMaterialLayerCount(terrain);
    if (layer < 0 || layer >= layerCount)
    {
        return false;
    }
    if (paint.selectedLayer == layer)
    {
        return false;
    }
    paint.selectedLayer = layer;
    world::EndTerrainPaintStroke(paint.stroke);
    return true;
}

// After removing extra layer `removedIndex`, compact the selected index so a
// surviving layer stays selected. Removing the selected extra layer returns
// to layer 0. Layer 0 cannot be removed.
inline void RemapTerrainPaintLayerAfterRemove(TerrainPaintState& paint, int removedIndex)
{
    if (removedIndex <= 0)
    {
        return;
    }
    if (paint.selectedLayer == removedIndex)
    {
        paint.selectedLayer = 0;
        world::EndTerrainPaintStroke(paint.stroke);
        return;
    }
    if (paint.selectedLayer > removedIndex)
    {
        paint.selectedLayer -= 1;
        world::EndTerrainPaintStroke(paint.stroke);
    }
}

inline bool TerrainPaintHasAuthoringSurface(const world::LevelDefinition& workingCopy)
{
    return TerrainSculptHasAuthoringSurface(workingCopy);
}

inline bool TerrainPaintInteractionIsActive(
    const TerrainPaintState& paint,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection)
{
    return paint.mode && selection.kind == EditorObjectKind::Terrain
        && TerrainPaintHasAuthoringSurface(workingCopy);
}

inline void EndTerrainPaintStroke(TerrainPaintState& paint)
{
    world::EndTerrainPaintStroke(paint.stroke);
}

inline void ResetTerrainPaintState(TerrainPaintState& paint)
{
    paint = {};
}

inline void ReconcileTerrainPaintState(
    TerrainPaintState& paint,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection)
{
    if (!workingCopy.hasTerrain || selection.kind != EditorObjectKind::Terrain)
    {
        paint.mode = false;
        EndTerrainPaintStroke(paint);
        paint.previewHit = false;
        paint.previewPoint = {};
        return;
    }
    SanitizeTerrainPaintState(paint, workingCopy.terrain);
    if (!TerrainPaintInteractionIsActive(paint, workingCopy, selection))
    {
        EndTerrainPaintStroke(paint);
    }
}

inline void ExitTerrainPaintMode(TerrainPaintState& paint)
{
    paint.mode = false;
    EndTerrainPaintStroke(paint);
    paint.previewHit = false;
    paint.previewPoint = {};
}

struct TerrainPaintFrameResult
{
    bool mutatedWorkingCopy = false;
};

inline TerrainPaintFrameResult TickTerrainPaint(
    TerrainPaintState& paint,
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
    TerrainPaintFrameResult result{};
    ReconcileTerrainPaintState(paint, workingCopy, selection);
    paint.previewHit = false;
    paint.previewPoint = {};
    if (!TerrainPaintInteractionIsActive(paint, workingCopy, selection))
    {
        return result;
    }

    const bool pointerFree = !mouseCaptured && !lookHeld && !widgetConsumedPointer;
    core::Vec3 hit{};
    const bool hasHit = pointerFree && PickWorkingCopyTerrainSculptHit(workingCopy, ray, hit);
    if (hasHit)
    {
        paint.previewHit = true;
        paint.previewPoint = hit;
    }

    if (!pointerFree)
    {
        if (selectReleased || !selectHeld)
        {
            EndTerrainPaintStroke(paint);
        }
        return result;
    }

    world::TerrainPaintStampRequest request{};
    request.layer = paint.selectedLayer;
    request.radius = paint.radius;
    request.strength = paint.strength;

    if (selectPressed)
    {
        if (hasHit)
        {
            request.centerX = hit.x;
            request.centerZ = hit.z;
            result.mutatedWorkingCopy =
                world::BeginTerrainPaintStroke(paint.stroke, workingCopy.terrain, request);
        }
    }
    else if (paint.stroke.active && selectHeld && hasHit)
    {
        result.mutatedWorkingCopy = world::ContinueTerrainPaintStroke(
            paint.stroke, workingCopy.terrain, request, hit.x, hit.z);
    }

    if (selectReleased || !selectHeld)
    {
        EndTerrainPaintStroke(paint);
    }
    return result;
}

inline bool ShouldCancelTerrainPaintMode(
    const TerrainPaintState& paint,
    bool escapePressed,
    bool imguiWantsKeyboard)
{
    return paint.mode && escapePressed && !imguiWantsKeyboard;
}

inline std::string FormatTerrainPaintHudName(const TerrainPaintState& paint)
{
    return "Layer " + std::to_string(paint.selectedLayer);
}

inline const char* TerrainPaintViewportHintText()
{
    return "LMB paint | Esc exit";
}
}
