#pragma once

// Milestone 90: transient Development Editor Terrain Paint tool state.
// Not Level Format, not Dirty by itself, and never serialized.

#include "assets/RuntimePng.h"
#include "editor/EditorPicking.h"
#include "editor/EditorSelection.h"
#include "editor/TerrainPickerCard.h"
#include "editor/TerrainSculpt.h"
#include "world/LevelDefinition.h"
#include "world/TerrainPaint.h"

#include <string>
#include <string_view>
#include <vector>

namespace editor
{
enum class TerrainMaterialChannelKind
{
    Albedo,
    Normal,
    Roughness
};

struct TerrainPaintState
{
    bool mode = false;
    int selectedLayer = 0;
    float radius = world::kDefaultTerrainPaintRadius;
    float strength = world::kDefaultTerrainPaintStrength;
    world::TerrainPaintStroke stroke{};
    bool previewHit = false;
    core::Vec3 previewPoint{};
    bool pickerOpen = false;
    bool pickerPointerLock = false;
    bool pickerOpenRequested = false;
    std::string pickerFilter;
    TerrainMaterialChannelKind pickerChannel = TerrainMaterialChannelKind::Albedo;
    int pickerLayer = 0;
};

inline constexpr const char* kTerrainMaterialChannelPopupId = "Terrain Material Channel";

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
    if (paint.pickerLayer == removedIndex)
    {
        paint.pickerLayer = 0;
    }
    else if (paint.pickerLayer > removedIndex)
    {
        paint.pickerLayer -= 1;
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

inline bool TerrainPaintUiBlocksPointer(const TerrainPaintState& paint)
{
    return paint.pickerOpen || paint.pickerPointerLock;
}

inline void NoteTerrainPaintPickerOpen(TerrainPaintState& paint, bool open)
{
    paint.pickerOpen = open;
    if (open)
    {
        paint.pickerPointerLock = true;
        world::EndTerrainPaintStroke(paint.stroke);
    }
}

// Button clicks record an open request. OpenPopup/BeginPopup must run at the
// same ImGui ID stack as each other, not inside the channel-row PushID that
// owns Assign/Replace. Opening does not depend on catalog size.
inline bool RequestTerrainMaterialChannelPicker(
    TerrainPaintState& paint,
    int layer,
    TerrainMaterialChannelKind channel)
{
    paint.pickerChannel = channel;
    paint.pickerLayer = layer;
    paint.pickerFilter.clear();
    paint.pickerOpenRequested = true;
    return true;
}

inline bool ConsumeTerrainMaterialChannelPickerOpenRequest(TerrainPaintState& paint)
{
    if (!paint.pickerOpenRequested)
    {
        return false;
    }
    paint.pickerOpenRequested = false;
    return true;
}

inline bool TerrainMaterialChannelIdentityIsCompatible(
    TerrainMaterialChannelKind channel,
    std::string_view identity)
{
    if (channel == TerrainMaterialChannelKind::Normal)
    {
        return world::TerrainNormalMapIdentityIsCompatible(identity);
    }
    if (channel == TerrainMaterialChannelKind::Roughness)
    {
        return world::TerrainRoughnessMapIdentityIsCompatible(identity);
    }
    return assets::RuntimePngIdentityIsValid(identity);
}

inline char TerrainPaintAsciiLower(char ch)
{
    return (ch >= 'A' && ch <= 'Z') ? static_cast<char>(ch - 'A' + 'a') : ch;
}

inline bool TerrainPaintPickerQueryMatches(std::string_view haystack, std::string_view query)
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
            if (TerrainPaintAsciiLower(haystack[start + index])
                != TerrainPaintAsciiLower(query[index]))
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

inline std::vector<std::string> FilterTerrainMaterialChannelIdentities(
    const std::vector<std::string>& identities,
    TerrainMaterialChannelKind channel,
    std::string_view query)
{
    std::vector<std::string> filtered;
    for (const std::string& identity : identities)
    {
        if (!TerrainMaterialChannelIdentityIsCompatible(channel, identity))
        {
            continue;
        }
        if (!TerrainPaintPickerQueryMatches(identity, query)
            && !TerrainPaintPickerQueryMatches(assets::RuntimePngDisplayName(identity), query))
        {
            continue;
        }
        filtered.push_back(identity);
    }
    return filtered;
}

inline const char* TerrainMaterialChannelPickerStatusText(
    const std::vector<std::string>& catalog,
    const std::vector<std::string>& filtered,
    TerrainMaterialChannelKind channel)
{
    int compatible = 0;
    for (const std::string& identity : catalog)
    {
        if (TerrainMaterialChannelIdentityIsCompatible(channel, identity))
        {
            ++compatible;
        }
    }
    if (compatible == 0)
    {
        if (channel == TerrainMaterialChannelKind::Normal)
        {
            return "No compatible Normal textures found. Use filenames ending in "
                   "_NormalGL, _NormalDX, _Normal, _normal, or _nrm.";
        }
        if (channel == TerrainMaterialChannelKind::Roughness)
        {
            return "No compatible Roughness textures found. Use filenames ending in "
                   "_Roughness, _roughness, or _rough.";
        }
        if (catalog.empty())
        {
            return "No PNG textures are available.";
        }
        return "No compatible PNG textures are available.";
    }
    if (filtered.empty())
    {
        return "No compatible textures match this search.";
    }
    return "";
}

struct TerrainMaterialChannelPickerView
{
    TerrainMaterialChannelKind channel = TerrainMaterialChannelKind::Albedo;
    int layer = 0;
    std::vector<std::string> identities{};
    const char* statusText = "";
};

inline TerrainMaterialChannelPickerView BuildTerrainMaterialChannelPickerView(
    const TerrainPaintState& paint,
    const std::vector<std::string>& catalog)
{
    TerrainMaterialChannelPickerView view{};
    view.channel = paint.pickerChannel;
    view.layer = paint.pickerLayer;
    view.identities =
        FilterTerrainMaterialChannelIdentities(catalog, paint.pickerChannel, paint.pickerFilter);
    view.statusText =
        TerrainMaterialChannelPickerStatusText(catalog, view.identities, paint.pickerChannel);
    return view;
}

inline bool TryAssignTerrainMaterialChannel(
    world::TerrainSpec& terrain,
    int layer,
    TerrainMaterialChannelKind channel,
    std::string_view identity)
{
    if (channel == TerrainMaterialChannelKind::Normal)
    {
        return world::TryAssignTerrainLayerNormal(terrain, layer, identity);
    }
    if (channel == TerrainMaterialChannelKind::Roughness)
    {
        return world::TryAssignTerrainLayerRoughness(terrain, layer, identity);
    }
    if (layer <= 0)
    {
        return world::TryAssignTerrainTextureIdentity(terrain, identity);
    }
    if (world::TerrainLayerTextureIdentity(terrain, layer) == identity)
    {
        return false;
    }
    if (!assets::RuntimePngIdentityIsValid(identity)
        || world::TerrainReferencesAlbedoIdentity(terrain, identity))
    {
        return false;
    }
    const int extra = layer - 1;
    if (extra < 0 || extra >= static_cast<int>(terrain.extraLayers.size()))
    {
        return false;
    }
    terrain.extraLayers[static_cast<std::size_t>(extra)].textureIdentity = std::string(identity);
    return true;
}

inline bool TryClearTerrainMaterialChannel(
    world::TerrainSpec& terrain,
    int layer,
    TerrainMaterialChannelKind channel)
{
    if (channel == TerrainMaterialChannelKind::Normal)
    {
        return world::TryClearTerrainLayerNormal(terrain, layer);
    }
    if (channel == TerrainMaterialChannelKind::Roughness)
    {
        return world::TryClearTerrainLayerRoughness(terrain, layer);
    }
    if (layer <= 0)
    {
        return world::TryClearTerrainTextureIdentity(terrain);
    }
    return false;
}

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
    if (paint.pickerOpen)
    {
        paint.pickerPointerLock = true;
    }
    if (!TerrainPaintInteractionIsActive(paint, workingCopy, selection))
    {
        if ((selectReleased || !selectHeld) && !paint.pickerOpen)
        {
            paint.pickerPointerLock = false;
        }
        return result;
    }

    const bool pointerFree = !mouseCaptured && !lookHeld && !widgetConsumedPointer
        && !TerrainPaintUiBlocksPointer(paint);
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
            if (!paint.pickerOpen)
            {
                paint.pickerPointerLock = false;
            }
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
