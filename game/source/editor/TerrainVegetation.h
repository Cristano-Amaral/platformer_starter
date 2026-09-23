#pragma once

// Milestone 93: transient Development Editor Terrain vegetation brush.
// Milestone 95 adds editor-only palette/picker/preview state. Not Level
// Format, not Dirty by itself, and never serialized.

#include "editor/EditorPicking.h"
#include "editor/EditorSelection.h"
#include "editor/TerrainPickerCard.h"
#include "editor/TerrainSculpt.h"
#include "world/LevelDefinition.h"
#include "world/TerrainVegetation.h"

#include <string>
#include <string_view>
#include <vector>

namespace editor
{
struct TerrainVegetationPickerItem
{
    std::string canonicalIdentity;
    std::string displayName;
};

struct TerrainVegetationState
{
    bool mode = false;
    world::TerrainVegetationBrushOperation operation = world::TerrainVegetationBrushOperation::Paint;
    int selectedEntry = 0;
    float radius = world::kDefaultTerrainVegetationRadius;
    world::TerrainVegetationStroke stroke{};
    bool previewHit = false;
    core::Vec3 previewPoint{};
    // M95 popup/search. Transient editor UI only.
    bool pickerOpen = false;
    bool pickerPointerLock = false;
    std::string pickerFilter;
};

inline bool TerrainVegetationOperationIsValid(world::TerrainVegetationBrushOperation operation)
{
    return operation == world::TerrainVegetationBrushOperation::Paint
        || operation == world::TerrainVegetationBrushOperation::Erase;
}

inline bool TerrainVegetationPaletteIsEmpty(const world::TerrainSpec& terrain)
{
    return terrain.vegetationEntries.empty();
}

inline bool TerrainVegetationPaletteIsFull(const world::TerrainSpec& terrain)
{
    return static_cast<int>(terrain.vegetationEntries.size()) >= world::kMaxTerrainVegetationEntries;
}

inline bool TerrainVegetationHasSelectedEntry(
    const TerrainVegetationState& vegetation,
    const world::TerrainSpec& terrain)
{
    return vegetation.selectedEntry >= 0
        && vegetation.selectedEntry < static_cast<int>(terrain.vegetationEntries.size());
}

inline char TerrainVegetationAsciiLower(char ch)
{
    return (ch >= 'A' && ch <= 'Z') ? static_cast<char>(ch - 'A' + 'a') : ch;
}

inline bool TerrainVegetationPickerQueryMatches(std::string_view haystack, std::string_view query)
{
    if (query.empty())
    {
        return true;
    }
    if (query.size() > haystack.size())
    {
        return false;
    }
    for (std::size_t start = 0; start + query.size() <= haystack.size(); ++start)
    {
        bool match = true;
        for (std::size_t index = 0; index < query.size(); ++index)
        {
            if (TerrainVegetationAsciiLower(haystack[start + index])
                != TerrainVegetationAsciiLower(query[index]))
            {
                match = false;
                break;
            }
        }
        if (match)
        {
            return true;
        }
    }
    return false;
}

inline std::string TerrainVegetationModelFileName(std::string_view identity)
{
    constexpr std::string_view kPrefix = "models/";
    if (identity.starts_with(kPrefix) && identity.size() > kPrefix.size())
    {
        return std::string(identity.substr(kPrefix.size()));
    }
    return std::string(identity);
}

inline std::string TerrainVegetationModelDisplayName(
    std::string_view identity,
    const std::vector<TerrainVegetationPickerItem>& catalogItems)
{
    for (const TerrainVegetationPickerItem& item : catalogItems)
    {
        if (item.canonicalIdentity == identity)
        {
            return item.displayName.empty() ? TerrainVegetationModelFileName(identity)
                                            : item.displayName;
        }
    }
    return TerrainVegetationModelFileName(identity);
}

inline bool TerrainVegetationModelIsCataloged(
    std::string_view identity,
    const std::vector<TerrainVegetationPickerItem>& catalogItems)
{
    for (const TerrainVegetationPickerItem& item : catalogItems)
    {
        if (item.canonicalIdentity == identity)
        {
            return true;
        }
    }
    return false;
}

inline std::vector<TerrainVegetationPickerItem> FilterTerrainVegetationPickerItems(
    const std::vector<TerrainVegetationPickerItem>& items,
    std::string_view query)
{
    std::vector<TerrainVegetationPickerItem> filtered;
    filtered.reserve(items.size());
    for (const TerrainVegetationPickerItem& item : items)
    {
        if (TerrainVegetationPickerQueryMatches(item.canonicalIdentity, query)
            || TerrainVegetationPickerQueryMatches(item.displayName, query)
            || TerrainVegetationPickerQueryMatches(TerrainVegetationModelFileName(item.canonicalIdentity), query))
        {
            filtered.push_back(item);
        }
    }
    return filtered;
}

inline const char* TerrainVegetationPaletteStatusText(const world::TerrainSpec& terrain)
{
    if (TerrainVegetationPaletteIsEmpty(terrain))
    {
        return "Palette is empty. Add a compatible static model to paint.";
    }
    if (TerrainVegetationPaletteIsFull(terrain))
    {
        return "Palette limit reached (8 models).";
    }
    return "";
}

inline const char* TerrainVegetationPickerStatusText(
    const std::vector<TerrainVegetationPickerItem>& catalogItems,
    const std::vector<TerrainVegetationPickerItem>& filteredItems,
    bool paletteFull)
{
    if (paletteFull)
    {
        return "Palette limit reached (8 models).";
    }
    if (catalogItems.empty())
    {
        return "No compatible static .glb models are available.";
    }
    if (filteredItems.empty())
    {
        return "No compatible static models match this search.";
    }
    return "";
}

inline const char* TerrainVegetationMissingModelStatusText()
{
    return "Referenced model is missing from the catalog.";
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

inline bool TerrainVegetationUiBlocksPointer(const TerrainVegetationState& vegetation)
{
    return vegetation.pickerOpen || vegetation.pickerPointerLock;
}

inline bool TerrainVegetationPointerIsFree(
    const TerrainVegetationState& vegetation,
    bool mouseCaptured,
    bool lookHeld,
    bool widgetConsumedPointer)
{
    return !mouseCaptured && !lookHeld && !widgetConsumedPointer
        && !TerrainVegetationUiBlocksPointer(vegetation);
}

inline bool TerrainVegetationBrushPreviewShouldDraw(
    bool interactionActive,
    bool previewHit,
    const TerrainVegetationState& vegetation)
{
    return interactionActive && previewHit && !TerrainVegetationUiBlocksPointer(vegetation);
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

inline bool TrySelectTerrainVegetationEntry(
    TerrainVegetationState& vegetation,
    int entryIndex,
    const world::TerrainSpec& terrain)
{
    if (entryIndex < 0 || entryIndex >= static_cast<int>(terrain.vegetationEntries.size()))
    {
        return false;
    }
    if (vegetation.selectedEntry != entryIndex)
    {
        vegetation.selectedEntry = entryIndex;
        EndTerrainVegetationStroke(vegetation);
    }
    return true;
}

inline bool TryAddTerrainVegetationPaletteEntry(
    world::TerrainSpec& terrain,
    TerrainVegetationState& vegetation,
    std::string_view modelIdentity)
{
    if (!world::TryAddTerrainVegetationEntry(terrain, modelIdentity))
    {
        return false;
    }
    vegetation.selectedEntry = static_cast<int>(terrain.vegetationEntries.size()) - 1;
    vegetation.pickerOpen = false;
    EndTerrainVegetationStroke(vegetation);
    return true;
}

inline bool TryRemoveTerrainVegetationPaletteEntry(
    world::TerrainSpec& terrain,
    TerrainVegetationState& vegetation,
    int entryIndex)
{
    if (!world::TryRemoveTerrainVegetationEntry(terrain, entryIndex))
    {
        return false;
    }
    RemapTerrainVegetationSelectionAfterRemove(vegetation, entryIndex);
    EndTerrainVegetationStroke(vegetation);
    SanitizeTerrainVegetationState(vegetation, terrain);
    return true;
}

inline void NoteTerrainVegetationPickerOpen(TerrainVegetationState& vegetation, bool open)
{
    vegetation.pickerOpen = open;
    if (open)
    {
        vegetation.pickerPointerLock = true;
        EndTerrainVegetationStroke(vegetation);
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
        vegetation.pickerOpen = false;
        vegetation.pickerPointerLock = false;
        return;
    }
    SanitizeTerrainVegetationState(vegetation, workingCopy.terrain);
    if (selection.kind != EditorObjectKind::Terrain)
    {
        vegetation.mode = false;
        EndTerrainVegetationStroke(vegetation);
        vegetation.previewHit = false;
        vegetation.previewPoint = {};
        vegetation.pickerOpen = false;
        vegetation.pickerPointerLock = false;
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
    if (vegetation.pickerOpen)
    {
        vegetation.pickerPointerLock = true;
    }
    if (!TerrainVegetationInteractionIsActive(vegetation, workingCopy, selection))
    {
        if (selectReleased || !selectHeld)
        {
            if (!vegetation.pickerOpen)
            {
                vegetation.pickerPointerLock = false;
            }
        }
        return result;
    }

    const bool pointerFree =
        TerrainVegetationPointerIsFree(vegetation, mouseCaptured, lookHeld, widgetConsumedPointer);
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
            if (!vegetation.pickerOpen)
            {
                vegetation.pickerPointerLock = false;
            }
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
        if (!vegetation.pickerOpen)
        {
            vegetation.pickerPointerLock = false;
        }
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

inline std::string FormatTerrainVegetationHudName(
    const TerrainVegetationState& vegetation,
    const world::TerrainSpec& terrain)
{
    std::string name = FormatTerrainVegetationHudName(vegetation);
    if (TerrainVegetationHasSelectedEntry(vegetation, terrain))
    {
        name += "  ";
        name += TerrainVegetationModelDisplayName(
            terrain.vegetationEntries[static_cast<std::size_t>(vegetation.selectedEntry)].modelIdentity,
            {});
    }
    return name;
}

inline const char* TerrainVegetationViewportHintText(const TerrainVegetationState& vegetation)
{
    return vegetation.operation == world::TerrainVegetationBrushOperation::Erase
        ? "LMB erase selected | Esc exit"
        : "LMB paint selected | Esc exit";
}
}
