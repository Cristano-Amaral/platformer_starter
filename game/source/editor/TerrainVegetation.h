#pragma once

// Milestone 93: transient Development Editor Terrain vegetation brush.
// Not Level Format, not Dirty by itself, and never serialized.

#include "editor/EditorPicking.h"
#include "editor/EditorSelection.h"
#include "editor/TerrainSculpt.h"
#include "world/LevelDefinition.h"
#include "world/TerrainVegetation.h"

#include <string>

namespace editor
{
struct TerrainVegetationState
{
    bool mode = false;
    world::TerrainVegetationBrushOperation operation = world::TerrainVegetationBrushOperation::Paint;
    int selectedEntry = 0;
    float radius = world::kDefaultTerrainVegetationRadius;
    world::TerrainVegetationStroke stroke{};
    bool previewHit = false;
    core::Vec3 previewPoint{};
};

inline bool TerrainVegetationOperationIsValid(world::TerrainVegetationBrushOperation operation)
{
    return operation == world::TerrainVegetationBrushOperation::Paint
        || operation == world::TerrainVegetationBrushOperation::Erase;
}

inline void SanitizeTerrainVegetationState(
    TerrainVegetationState& vegetation,
    const world::TerrainSpec& terrain)
{
    vegetation.radius = world::SanitizeTerrainVegetationRadius(vegetation.radius);
    if (!TerrainVegetationOperationIsValid(vegetation.operation))
    {
        vegetation.operation = world::TerrainVegetationBrushOperation::Paint;
    }
    const int entryCount = static_cast<int>(terrain.vegetationEntries.size());
    if (entryCount <= 0)
    {
        vegetation.selectedEntry = 0;
        return;
    }
    if (vegetation.selectedEntry < 0 || vegetation.selectedEntry >= entryCount)
    {
        vegetation.selectedEntry = 0;
    }
}

inline bool TerrainVegetationHasAuthoringSurface(const world::LevelDefinition& workingCopy)
{
    return workingCopy.hasTerrain && workingCopy.terrain.enabled
        && world::TerrainSpecIsValid(workingCopy.terrain);
}

inline bool TerrainVegetationInteractionIsActive(
    const TerrainVegetationState& vegetation,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection)
{
    return vegetation.mode && selection.kind == EditorObjectKind::Terrain
        && TerrainVegetationHasAuthoringSurface(workingCopy);
}

inline void ResetTerrainVegetationState(TerrainVegetationState& vegetation)
{
    vegetation = {};
}

inline void EndTerrainVegetationStroke(TerrainVegetationState& vegetation)
{
    world::EndTerrainVegetationStroke(vegetation.stroke);
}

inline void RemapTerrainVegetationSelectionAfterRemove(
    TerrainVegetationState& vegetation,
    int removedIndex)
{
    if (vegetation.selectedEntry == removedIndex)
    {
        vegetation.selectedEntry = 0;
    }
    else if (vegetation.selectedEntry > removedIndex)
    {
        --vegetation.selectedEntry;
    }
}

inline void ReconcileTerrainVegetationState(
    TerrainVegetationState& vegetation,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection)
{
    if (!workingCopy.hasTerrain)
    {
        vegetation.mode = false;
        EndTerrainVegetationStroke(vegetation);
        vegetation.previewHit = false;
        vegetation.previewPoint = {};
        vegetation.selectedEntry = 0;
        return;
    }
    SanitizeTerrainVegetationState(vegetation, workingCopy.terrain);
    if (selection.kind != EditorObjectKind::Terrain)
    {
        vegetation.mode = false;
        EndTerrainVegetationStroke(vegetation);
        vegetation.previewHit = false;
        vegetation.previewPoint = {};
        return;
    }
    if (!TerrainVegetationInteractionIsActive(vegetation, workingCopy, selection))
    {
        EndTerrainVegetationStroke(vegetation);
    }
}

struct TerrainVegetationFrameResult
{
    bool mutatedWorkingCopy = false;
};

inline TerrainVegetationFrameResult TickTerrainVegetation(
    TerrainVegetationState& vegetation,
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
    TerrainVegetationFrameResult result{};
    ReconcileTerrainVegetationState(vegetation, workingCopy, selection);
    vegetation.previewHit = false;
    vegetation.previewPoint = {};
    if (!TerrainVegetationInteractionIsActive(vegetation, workingCopy, selection))
    {
        return result;
    }

    const bool pointerFree = !mouseCaptured && !lookHeld && !widgetConsumedPointer;
    core::Vec3 hit{};
    const bool hasHit = pointerFree && PickWorkingCopyTerrainSculptHit(workingCopy, ray, hit);
    if (hasHit)
    {
        vegetation.previewHit = true;
        vegetation.previewPoint = hit;
    }

    if (!pointerFree)
    {
        if (selectReleased || !selectHeld)
        {
            EndTerrainVegetationStroke(vegetation);
        }
        return result;
    }

    world::TerrainVegetationStampRequest request{};
    request.operation = vegetation.operation;
    request.entryIndex = vegetation.selectedEntry;
    request.radius = vegetation.radius;

    if (selectPressed)
    {
        if (hasHit)
        {
            request.centerX = hit.x;
            request.centerZ = hit.z;
            result.mutatedWorkingCopy = world::BeginTerrainVegetationStroke(
                vegetation.stroke, workingCopy.terrain, request);
        }
    }
    else if (vegetation.stroke.active && selectHeld && hasHit)
    {
        request.centerX = hit.x;
        request.centerZ = hit.z;
        result.mutatedWorkingCopy = world::ContinueTerrainVegetationStroke(
            vegetation.stroke, workingCopy.terrain, request, hit.x, hit.z);
    }

    if (selectReleased || !selectHeld)
    {
        EndTerrainVegetationStroke(vegetation);
    }
    return result;
}

inline bool ShouldCancelTerrainVegetationMode(
    const TerrainVegetationState& vegetation,
    bool escapePressed,
    bool imguiWantsKeyboard)
{
    return vegetation.mode && escapePressed && !imguiWantsKeyboard;
}

inline std::string FormatTerrainVegetationHudName(const TerrainVegetationState& vegetation)
{
    const char* operation =
        vegetation.operation == world::TerrainVegetationBrushOperation::Erase ? "Erase" : "Paint";
    return std::string(operation) + " " + std::to_string(vegetation.selectedEntry);
}

inline const char* TerrainVegetationViewportHintText(const TerrainVegetationState& vegetation)
{
    return vegetation.operation == world::TerrainVegetationBrushOperation::Erase
        ? "LMB erase selected | Esc exit"
        : "LMB paint selected | Esc exit";
}
}
