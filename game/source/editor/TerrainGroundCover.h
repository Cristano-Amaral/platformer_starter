#pragma once

// Milestone 96: transient Development Editor Terrain Ground Cover brush.
// Palette/picker/preview state is editor-only. Not Level Format, not Dirty
// by itself, and never serialized.

#include "assets/RuntimePngCutout.h"
#include "editor/EditorPicking.h"
#include "editor/EditorSelection.h"
#include "editor/TerrainPickerCard.h"
#include "editor/TerrainSculpt.h"
#include "world/LevelDefinition.h"
#include "world/TerrainGroundCover.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace editor
{
struct TerrainGroundCoverPickerItem
{
    std::string canonicalIdentity;
    std::string displayName;
};

struct TerrainGroundCoverState
{
    bool mode = false;
    world::TerrainGroundCoverBrushOperation operation = world::TerrainGroundCoverBrushOperation::Paint;
    int selectedEntry = 0;
    float radius = world::kDefaultTerrainGroundCoverRadius;
    world::TerrainGroundCoverStroke stroke{};
    bool previewHit = false;
    core::Vec3 previewPoint{};
    bool pickerOpen = false;
    bool pickerPointerLock = false;
    std::string pickerFilter;
};

inline bool TerrainGroundCoverOperationIsValid(world::TerrainGroundCoverBrushOperation operation)
{
    return operation == world::TerrainGroundCoverBrushOperation::Paint
        || operation == world::TerrainGroundCoverBrushOperation::Erase;
}

inline bool TerrainGroundCoverPaletteIsEmpty(const world::TerrainSpec& terrain)
{
    return terrain.groundCoverEntries.empty();
}

inline bool TerrainGroundCoverPaletteIsFull(const world::TerrainSpec& terrain)
{
    return static_cast<int>(terrain.groundCoverEntries.size()) >= world::kMaxTerrainGroundCoverEntries;
}

inline bool TerrainGroundCoverHasSelectedEntry(
    const TerrainGroundCoverState& cover,
    const world::TerrainSpec& terrain)
{
    return cover.selectedEntry >= 0
        && cover.selectedEntry < static_cast<int>(terrain.groundCoverEntries.size());
}

inline char TerrainGroundCoverAsciiLower(char ch)
{
    return (ch >= 'A' && ch <= 'Z') ? static_cast<char>(ch - 'A' + 'a') : ch;
}

inline bool TerrainGroundCoverPickerQueryMatches(std::string_view haystack, std::string_view query)
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
            if (TerrainGroundCoverAsciiLower(haystack[start + index])
                != TerrainGroundCoverAsciiLower(query[index]))
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

inline std::string TerrainGroundCoverTextureFileName(std::string_view identity)
{
    constexpr std::string_view kPrefix = "textures/";
    if (identity.starts_with(kPrefix) && identity.size() > kPrefix.size())
    {
        return std::string(identity.substr(kPrefix.size()));
    }
    return std::string(identity);
}

inline std::string TerrainGroundCoverTextureDisplayName(
    std::string_view identity,
    const std::vector<TerrainGroundCoverPickerItem>& catalogItems)
{
    for (const TerrainGroundCoverPickerItem& item : catalogItems)
    {
        if (item.canonicalIdentity == identity)
        {
            return item.displayName.empty() ? TerrainGroundCoverTextureFileName(identity)
                                            : item.displayName;
        }
    }
    return TerrainGroundCoverTextureFileName(identity);
}

inline bool TerrainGroundCoverTextureIsCataloged(
    std::string_view identity,
    const std::vector<TerrainGroundCoverPickerItem>& catalogItems)
{
    for (const TerrainGroundCoverPickerItem& item : catalogItems)
    {
        if (item.canonicalIdentity == identity)
        {
            return true;
        }
    }
    return false;
}

inline std::vector<TerrainGroundCoverPickerItem> CollectCompatibleTerrainGroundCoverPickerItems(
    const std::vector<TerrainGroundCoverPickerItem>& catalogItems,
    const std::filesystem::path& sourceRoot)
{
    std::vector<TerrainGroundCoverPickerItem> compatible;
    if (sourceRoot.empty())
    {
        return compatible;
    }
    compatible.reserve(catalogItems.size());
    for (const TerrainGroundCoverPickerItem& item : catalogItems)
    {
        const std::filesystem::path path = (sourceRoot / item.canonicalIdentity).lexically_normal();
        if (assets::RuntimePngFileHasUsefulCutoutAlpha(path))
        {
            compatible.push_back(item);
        }
    }
    return compatible;
}

inline std::vector<TerrainGroundCoverPickerItem> FilterTerrainGroundCoverPickerItems(
    const std::vector<TerrainGroundCoverPickerItem>& items,
    std::string_view query)
{
    std::vector<TerrainGroundCoverPickerItem> filtered;
    filtered.reserve(items.size());
    for (const TerrainGroundCoverPickerItem& item : items)
    {
        if (TerrainGroundCoverPickerQueryMatches(item.canonicalIdentity, query)
            || TerrainGroundCoverPickerQueryMatches(item.displayName, query)
            || TerrainGroundCoverPickerQueryMatches(
                TerrainGroundCoverTextureFileName(item.canonicalIdentity), query))
        {
            filtered.push_back(item);
        }
    }
    return filtered;
}

inline const char* TerrainGroundCoverPaletteStatusText(const world::TerrainSpec& terrain)
{
    if (TerrainGroundCoverPaletteIsEmpty(terrain))
    {
        return "Palette is empty. Add a compatible cutout texture to paint.";
    }
    if (TerrainGroundCoverPaletteIsFull(terrain))
    {
        return "Palette limit reached (4 textures).";
    }
    return "";
}

inline const char* TerrainGroundCoverPickerStatusText(
    const std::vector<TerrainGroundCoverPickerItem>& catalogItems,
    const std::vector<TerrainGroundCoverPickerItem>& filteredItems,
    bool paletteFull)
{
    if (paletteFull)
    {
        return "Palette limit reached (4 textures).";
    }
    if (catalogItems.empty())
    {
        return "No compatible cutout PNG textures are available.";
    }
    if (filteredItems.empty())
    {
        return "No compatible cutout textures match this search.";
    }
    return "";
}

inline const char* TerrainGroundCoverMissingTextureStatusText()
{
    return "Referenced texture is missing from the catalog.";
}

inline void SanitizeTerrainGroundCoverState(
    TerrainGroundCoverState& cover,
    const world::TerrainSpec& terrain)
{
    cover.radius = world::SanitizeTerrainGroundCoverRadius(cover.radius);
    if (!TerrainGroundCoverOperationIsValid(cover.operation))
    {
        cover.operation = world::TerrainGroundCoverBrushOperation::Paint;
    }
    if (terrain.groundCoverEntries.empty())
    {
        cover.selectedEntry = 0;
        return;
    }
    if (cover.selectedEntry < 0
        || cover.selectedEntry >= static_cast<int>(terrain.groundCoverEntries.size()))
    {
        cover.selectedEntry = 0;
    }
}

inline bool TerrainGroundCoverHasAuthoringSurface(const world::LevelDefinition& workingCopy)
{
    return workingCopy.hasTerrain && workingCopy.terrain.enabled
        && world::TerrainSpecIsValid(workingCopy.terrain);
}

inline bool TerrainGroundCoverInteractionIsActive(
    const TerrainGroundCoverState& cover,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection)
{
    return cover.mode && selection.kind == EditorObjectKind::Terrain
        && TerrainGroundCoverHasAuthoringSurface(workingCopy);
}

inline bool TerrainGroundCoverUiBlocksPointer(const TerrainGroundCoverState& cover)
{
    return cover.pickerOpen || cover.pickerPointerLock;
}

inline bool TerrainGroundCoverPointerIsFree(
    const TerrainGroundCoverState& cover,
    bool mouseCaptured,
    bool lookHeld,
    bool widgetConsumedPointer)
{
    return !mouseCaptured && !lookHeld && !widgetConsumedPointer
        && !TerrainGroundCoverUiBlocksPointer(cover);
}

inline bool TerrainGroundCoverBrushPreviewShouldDraw(
    bool interactionActive,
    bool previewHit,
    const TerrainGroundCoverState& cover)
{
    return interactionActive && previewHit && !TerrainGroundCoverUiBlocksPointer(cover);
}

inline void ResetTerrainGroundCoverState(TerrainGroundCoverState& cover)
{
    cover = {};
}

inline void EndTerrainGroundCoverStroke(TerrainGroundCoverState& cover)
{
    world::EndTerrainGroundCoverStroke(cover.stroke);
}

inline void RemapTerrainGroundCoverSelectionAfterRemove(
    TerrainGroundCoverState& cover,
    int removedIndex)
{
    if (cover.selectedEntry == removedIndex)
    {
        cover.selectedEntry = 0;
    }
    else if (cover.selectedEntry > removedIndex)
    {
        --cover.selectedEntry;
    }
}

inline bool TrySelectTerrainGroundCoverEntry(
    TerrainGroundCoverState& cover,
    int entryIndex,
    const world::TerrainSpec& terrain)
{
    if (entryIndex < 0 || entryIndex >= static_cast<int>(terrain.groundCoverEntries.size()))
    {
        return false;
    }
    if (cover.selectedEntry != entryIndex)
    {
        cover.selectedEntry = entryIndex;
        EndTerrainGroundCoverStroke(cover);
    }
    return true;
}

inline bool TryAddTerrainGroundCoverPaletteEntry(
    world::TerrainSpec& terrain,
    TerrainGroundCoverState& cover,
    std::string_view textureIdentity)
{
    if (!world::TryAddTerrainGroundCoverEntry(terrain, textureIdentity))
    {
        return false;
    }
    cover.selectedEntry = static_cast<int>(terrain.groundCoverEntries.size()) - 1;
    cover.pickerOpen = false;
    EndTerrainGroundCoverStroke(cover);
    return true;
}

inline bool TryRemoveTerrainGroundCoverPaletteEntry(
    world::TerrainSpec& terrain,
    TerrainGroundCoverState& cover,
    int entryIndex)
{
    if (!world::TryRemoveTerrainGroundCoverEntry(terrain, entryIndex))
    {
        return false;
    }
    RemapTerrainGroundCoverSelectionAfterRemove(cover, entryIndex);
    EndTerrainGroundCoverStroke(cover);
    SanitizeTerrainGroundCoverState(cover, terrain);
    return true;
}

inline void NoteTerrainGroundCoverPickerOpen(TerrainGroundCoverState& cover, bool open)
{
    cover.pickerOpen = open;
    if (open)
    {
        cover.pickerPointerLock = true;
        EndTerrainGroundCoverStroke(cover);
    }
}

inline void ReconcileTerrainGroundCoverState(
    TerrainGroundCoverState& cover,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection)
{
    if (!workingCopy.hasTerrain)
    {
        cover.mode = false;
        EndTerrainGroundCoverStroke(cover);
        cover.previewHit = false;
        cover.previewPoint = {};
        cover.selectedEntry = 0;
        cover.pickerOpen = false;
        cover.pickerPointerLock = false;
        return;
    }
    SanitizeTerrainGroundCoverState(cover, workingCopy.terrain);
    if (selection.kind != EditorObjectKind::Terrain)
    {
        cover.mode = false;
        EndTerrainGroundCoverStroke(cover);
        cover.previewHit = false;
        cover.previewPoint = {};
        cover.pickerOpen = false;
        cover.pickerPointerLock = false;
        return;
    }
}

struct TerrainGroundCoverFrameResult
{
    bool mutatedWorkingCopy = false;
};

inline TerrainGroundCoverFrameResult TickTerrainGroundCover(
    TerrainGroundCoverState& cover,
    world::LevelDefinition& workingCopy,
    EditorSelection selection,
    const Ray3& ray,
    bool selectPressed,
    bool selectHeld,
    bool selectReleased,
    bool mouseCaptured,
    bool lookHeld,
    bool widgetConsumedPointer)
{
    TerrainGroundCoverFrameResult result{};
    ReconcileTerrainGroundCoverState(cover, workingCopy, selection);
    cover.previewHit = false;
    cover.previewPoint = {};
    if (cover.pickerOpen)
    {
        cover.pickerPointerLock = true;
    }
    if (!TerrainGroundCoverInteractionIsActive(cover, workingCopy, selection))
    {
        if (selectReleased || !selectHeld)
        {
            if (!cover.pickerOpen)
            {
                cover.pickerPointerLock = false;
            }
        }
        return result;
    }

    const bool pointerFree =
        TerrainGroundCoverPointerIsFree(cover, mouseCaptured, lookHeld, widgetConsumedPointer);
    core::Vec3 hit{};
    const bool hasHit = pointerFree && PickWorkingCopyTerrainSculptHit(workingCopy, ray, hit);
    if (hasHit)
    {
        cover.previewHit = true;
        cover.previewPoint = hit;
    }

    if (!pointerFree)
    {
        if (selectReleased || !selectHeld)
        {
            EndTerrainGroundCoverStroke(cover);
            if (!cover.pickerOpen)
            {
                cover.pickerPointerLock = false;
            }
        }
        return result;
    }

    world::TerrainGroundCoverStampRequest request{};
    request.operation = cover.operation;
    request.entryIndex = cover.selectedEntry;
    request.radius = cover.radius;

    if (selectPressed)
    {
        if (hasHit)
        {
            request.centerX = hit.x;
            request.centerZ = hit.z;
            result.mutatedWorkingCopy = world::BeginTerrainGroundCoverStroke(
                cover.stroke, workingCopy.terrain, request);
        }
    }
    else if (cover.stroke.active && selectHeld && hasHit)
    {
        request.centerX = hit.x;
        request.centerZ = hit.z;
        result.mutatedWorkingCopy = world::ContinueTerrainGroundCoverStroke(
            cover.stroke, workingCopy.terrain, request, hit.x, hit.z);
    }

    if (selectReleased || !selectHeld)
    {
        EndTerrainGroundCoverStroke(cover);
        if (!cover.pickerOpen)
        {
            cover.pickerPointerLock = false;
        }
    }
    return result;
}

inline bool ShouldCancelTerrainGroundCoverMode(
    const TerrainGroundCoverState& cover,
    bool escapePressed,
    bool imguiWantsKeyboard)
{
    return cover.mode && escapePressed && !imguiWantsKeyboard;
}

inline std::string FormatTerrainGroundCoverHudName(const TerrainGroundCoverState& cover)
{
    const char* operation =
        cover.operation == world::TerrainGroundCoverBrushOperation::Erase ? "Erase" : "Paint";
    return std::string(operation) + " " + std::to_string(cover.selectedEntry);
}

inline std::string FormatTerrainGroundCoverHudName(
    const TerrainGroundCoverState& cover,
    const world::TerrainSpec& terrain)
{
    std::string name = FormatTerrainGroundCoverHudName(cover);
    if (TerrainGroundCoverHasSelectedEntry(cover, terrain))
    {
        name += "  ";
        name += TerrainGroundCoverTextureDisplayName(
            terrain.groundCoverEntries[static_cast<std::size_t>(cover.selectedEntry)].textureIdentity,
            {});
    }
    return name;
}

inline const char* TerrainGroundCoverViewportHintText(const TerrainGroundCoverState& cover)
{
    return cover.operation == world::TerrainGroundCoverBrushOperation::Erase
        ? "LMB erase selected | Esc exit"
        : "LMB paint selected | Esc exit";
}
}
