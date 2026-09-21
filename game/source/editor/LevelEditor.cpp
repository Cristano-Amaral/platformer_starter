#include "editor/LevelEditor.h"

#include "editor/AuthoredLifecycleCommands.h"
#include "editor/EditorHierarchy.h"
#include "editor/EditorSelection.h"
#include "editor/EditorSelectionSet.h"
#include "editor/EditorGroupRotate.h"
#include "editor/EditorGroupTranslate.h"

#if defined(PLATFORMER_ENABLE_DEBUG_UI) || defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
#include "editor/AuthoringPaths.h"
#include "world/LevelWriter.h"
#endif

#if defined(PLATFORMER_ENABLE_DEBUG_UI)
#include "editor/ContentBrowser.h"
#include "editor/ContentBrowserView.h"
#include "editor/DirectionalLightAuthoring.h"
#include "editor/LocalLightAuthoring.h"
#include "editor/EditorLayout.h"
#include "editor/EditorLayoutUi.h"
#include "editor/EditorPlacement.h"
#include "editor/StaticPropPlacement.h"
#include "editor/EditorToolCommands.h"
#include "editor/EditorToolRunner.h"
#include "editor/StaticPropTransform.h"
#include "gameplay/Inventory.h"
#include "platform/RuntimePaths.h"
#include "imgui.h"
#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
#include "editor/StaticModelThumbnailCache.h"
#include "editor/StaticModelFraming.h"
#include "render/StaticModelScene.h"
#include "render/StaticModelThumbnail.h"
#include "render/TextureThumbnail.h"
#include "render/StaticModelPreview.h"
#endif
#include "world/Door.h"
#include "world/ItemPickup.h"
#include "world/LevelIdentity.h"
#include "assets/RuntimePng.h"
#include "assets/RuntimePngResolve.h"
#include "assets/SourceTextureCatalog.h"
#include "editor/ContentBrowserOrganization.h"
#include "editor/TextureThumbnailLifecycle.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>
#endif

namespace editor
{
const char* LevelEditorApplyStatusName(LevelEditorApplyStatus status)
{
    switch (status)
    {
    case LevelEditorApplyStatus::NotAttempted:
        return "NotAttempted";
    case LevelEditorApplyStatus::Applied:
        return "Applied";
    case LevelEditorApplyStatus::Invalid:
        return "Invalid";
    case LevelEditorApplyStatus::Error:
        return "Error";
    }
    return "NotAttempted";
}

const char* LevelEditorSaveStatusName(LevelEditorSaveStatus status)
{
    switch (status)
    {
    case LevelEditorSaveStatus::NotAttempted:
        return "NotAttempted";
    case LevelEditorSaveStatus::Saved:
        return "Saved";
    case LevelEditorSaveStatus::Invalid:
        return "Invalid";
    case LevelEditorSaveStatus::Error:
        return "Error";
    }
    return "NotAttempted";
}

const char* LevelEditorReloadStatusName(LevelEditorReloadStatus status)
{
    switch (status)
    {
    case LevelEditorReloadStatus::NotAttempted:
        return "NotAttempted";
    case LevelEditorReloadStatus::Reloaded:
        return "Reloaded";
    case LevelEditorReloadStatus::Rejected:
        return "Rejected";
    case LevelEditorReloadStatus::Missing:
        return "Missing";
    case LevelEditorReloadStatus::Invalid:
        return "Invalid";
    case LevelEditorReloadStatus::Error:
        return "Error";
    }
    return "NotAttempted";
}

void ResetLevelActionStatuses(LevelEditorState& state)
{
    state.lastApplyStatus = LevelEditorApplyStatus::NotAttempted;
    state.lastSaveStatus = LevelEditorSaveStatus::NotAttempted;
    state.lastReloadStatus = LevelEditorReloadStatus::NotAttempted;
}

// Source authoring is Development only. Debug and Release compile the stub, so
// they contain no repository path literal and no write path at all.
#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)

LevelEditorSaveResult SaveLevelSource(const world::LevelDefinition& level)
{
    const std::filesystem::path path = AuthoringLevelSourcePath(level.id);
    if (path.empty())
    {
        return {LevelEditorSaveStatus::Error, "authoring root unavailable"};
    }

    const world::WriteLevelFileResult result = world::SaveLevelFile(path, level);
    switch (result.status)
    {
    case world::WriteLevelFileStatus::Saved:
        return {LevelEditorSaveStatus::Saved, path.string()};
    case world::WriteLevelFileStatus::Invalid:
        return {LevelEditorSaveStatus::Invalid, result.error};
    case world::WriteLevelFileStatus::Error:
        break;
    }
    return {LevelEditorSaveStatus::Error, result.error};
}

#else

LevelEditorSaveResult SaveLevelSource(const world::LevelDefinition&)
{
    return {
        LevelEditorSaveStatus::Error,
        "source authoring is unavailable in this configuration"};
}

#endif

void RefreshLevelEditorDerivedFlags(
    LevelEditorState& state,
    const world::LevelDefinition& activeLevel)
{
    // Derived from authored data rather than widget return values, so a field
    // that reports "edited" without changing the value stays unmodified.
    state.modified = !world::AuthoredLevelDataEqual(state.workingCopy, activeLevel);
    state.dirty = !world::AuthoredLevelDataEqual(activeLevel, state.savedSourceBaseline);
}

#if defined(PLATFORMER_ENABLE_DEBUG_UI)

namespace
{
// Enough precision that editing a field cannot silently round authored data
// such as 1.6732 down to 1.67. ImGui writes a field back only when edited.
constexpr const char* kFloatFormat = "%.6f";

void ApplyEditorWindowPlacement(
    const char* windowName,
    const LevelEditorViewContext& view)
{
    ApplyKnownEditorWindowPlacement(
        windowName, view.viewportWidth, view.viewportHeight, view.forceDefaultLayout);
}

void RecoverEditorWindowIfNeeded(const char* windowName, const LevelEditorViewContext& view)
{
    RecoverKnownEditorWindowIfOffscreen(
        windowName, view.viewportWidth, view.viewportHeight, view.recoverOffscreenLayout);
}

const char* BoolText(bool value)
{
    return value ? "true" : "false";
}

void EditVec3(const char* label, core::Vec3& value)
{
    ImGui::InputFloat3(label, &value.x, kFloatFormat);
}

void ClampColor01(core::Vec3& color)
{
    auto clampChannel = [](float channel) {
        if (!std::isfinite(channel) || channel < 0.0f)
        {
            return 0.0f;
        }
        if (channel > 1.0f)
        {
            return 1.0f;
        }
        return channel;
    };
    color.x = clampChannel(color.x);
    color.y = clampChannel(color.y);
    color.z = clampChannel(color.z);
}

void PersistEditorSnapPreferences(const EditorSnapPreferences& snap)
{
    SaveEditorSnapPreferencesToLayoutPath(EditorLayoutPath(), snap);
}

void PersistEditorViewportGridPreferences(const EditorViewportGridPreferences& grid)
{
    SaveEditorViewportGridPreferencesToLayoutPath(EditorLayoutPath(), grid);
}

void DrawEditorSnapControls(LevelEditorState& state, bool compact, const char* incrementId)
{
    if (ImGui::Checkbox(compact ? "Snap##Toolbar" : "Snap", &state.snap.enabled))
    {
        PersistEditorSnapPreferences(state.snap);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip(
            "Hold %s while dragging a gizmo to invert Snap. The stored toggle is unchanged.",
            kEditorSnapInvertModifierName);
    }

    ImGui::SameLine();
    ImGui::TextUnformatted(compact ? "Inc" : EditorSnapIncrementLabel(state.transformMode));
    ImGui::SameLine();
    float* increment = EditorSnapIncrementPointer(state.snap, state.transformMode);
    float value = *increment;
    const char* format =
        state.transformMode == EditorTransformMode::Rotate ? "%.1f" : "%.2f";
    ImGui::SetNextItemWidth(compact ? 56.0f : 72.0f);
    if (ImGui::InputFloat(incrementId, &value, 0.0f, 0.0f, format))
    {
        *increment = value;
    }
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        *increment = SanitizeSnapIncrement(value, EditorSnapDefaultIncrement(state.transformMode));
        PersistEditorSnapPreferences(state.snap);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip(
            "Active increment: %g",
            EditorSnapIncrementForMode(state.snap, state.transformMode));
    }

    ImGui::SameLine();
    if (ImGui::Checkbox(compact ? "Grid##Toolbar" : "Grid", &state.viewportGrid.visible))
    {
        PersistEditorViewportGridPreferences(state.viewportGrid);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip(
            "World XZ grid at Y=0. Visibility is independent of Snap. "
            "Minor spacing follows the Translate increment.");
    }
    ImGui::SameLine();
    const float gridSpacing =
        EffectiveEditorViewportGridMinorSpacing(state.snap.translateIncrement);
    if (compact)
    {
        ImGui::TextDisabled("%g", gridSpacing);
    }
    else
    {
        ImGui::Text("Spacing %g", gridSpacing);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip(
            "Effective minor Grid spacing (Translate increment). "
            "This is not a separate Grid setting.");
    }
}

void ReadOnlyVec3(const char* label, core::Vec3 value)
{
    ImGui::Text("%s: %.6f  %.6f  %.6f", label, value.x, value.y, value.z);
}

// Staged runtime assets are the only scene-render authority for Static Props
// (no canonical-source fallback). This only reports why the placeholder cube
// is drawn; it never cooks, stages, or loads from source.
bool StagedStaticModelExists(std::string_view identity)
{
    if (!world::StaticPropIdentityIsValid(identity))
    {
        return false;
    }
    std::error_code error;
    return std::filesystem::is_regular_file(platform::RuntimeAssetPath(identity), error);
}

void DrawStaticModelStagingHint(std::string_view identity)
{
    const char* message =
        StaticPropAssetStateMessage(
            ClassifyStaticPropAsset(identity, StagedStaticModelExists(identity)));
    if (message != nullptr)
    {
        ImGui::TextWrapped("%s", message);
    }
}

void DrawTextureStagingHint(std::string_view identity)
{
    if (!assets::RuntimePngIdentityIsValid(identity))
    {
        return;
    }
    const std::filesystem::path cookedRoot = CookedAssetsRoot(RepositoryRoot());
    const assets::RuntimePngLoadResolution resolved =
        assets::ResolveRuntimePngLoadFile(identity, cookedRoot, {}, AuthoringSourceRoot());
    if (!assets::RuntimePngLoadFileIsAvailable(resolved))
    {
        ImGui::TextWrapped("%s", assets::RuntimeTerrainTextureUnavailableTooltip());
    }
}

constexpr float kTerrainCategoryIconSize = 28.0f;
constexpr float kTerrainMaterialThumbSize = 36.0f;

std::string TerrainLayerDisplayName(std::string_view identity)
{
    if (identity.empty())
    {
        return "Solid Terrain fallback";
    }
    const std::string fileName = assets::RuntimePngDisplayName(identity);
    return fileName.empty() ? std::string(identity) : fileName;
}

std::string TerrainLayerTitle(int layer)
{
    if (layer == 0)
    {
        return "Layer 0 — Base";
    }
    return "Layer " + std::to_string(layer);
}

const char* TerrainInspectorCategoryTooltip(TerrainInspectorCategory category)
{
    switch (category)
    {
    case TerrainInspectorCategory::MaterialsPaint:
        return "Terrain Materials & Paint";
    case TerrainInspectorCategory::Sculpt:
        return "Terrain Sculpt";
    default:
        return "Terrain";
    }
}

void DrawTerrainCategoryGlyph(
    ImDrawList* drawList,
    TerrainInspectorCategory category,
    ImVec2 min,
    ImVec2 max,
    ImU32 color)
{
    if (drawList == nullptr)
    {
        return;
    }
    const ImVec2 center((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
    const float width = max.x - min.x;
    const float height = max.y - min.y;
    switch (category)
    {
    case TerrainInspectorCategory::MaterialsPaint:
        drawList->AddCircleFilled(center, std::min(width, height) * 0.28f, color, 16);
        drawList->AddCircle(center, std::min(width, height) * 0.28f, IM_COL32(20, 24, 32, 255), 16, 1.5f);
        break;
    case TerrainInspectorCategory::Sculpt:
        drawList->AddLine(
            ImVec2(center.x - width * 0.18f, center.y + height * 0.18f),
            ImVec2(center.x + width * 0.16f, center.y - height * 0.20f),
            color,
            2.5f);
        drawList->AddTriangleFilled(
            ImVec2(center.x + width * 0.16f, center.y - height * 0.20f),
            ImVec2(center.x + width * 0.28f, center.y - height * 0.04f),
            ImVec2(center.x + width * 0.04f, center.y - height * 0.08f),
            color);
        break;
    default:
        drawList->AddTriangleFilled(
            ImVec2(center.x - width * 0.28f, center.y + height * 0.16f),
            ImVec2(center.x - width * 0.08f, center.y - height * 0.18f),
            ImVec2(center.x + width * 0.06f, center.y + height * 0.16f),
            color);
        drawList->AddTriangleFilled(
            ImVec2(center.x - width * 0.02f, center.y + height * 0.16f),
            ImVec2(center.x + width * 0.14f, center.y - height * 0.10f),
            ImVec2(center.x + width * 0.30f, center.y + height * 0.16f),
            color);
        drawList->AddLine(
            ImVec2(center.x - width * 0.30f, center.y + height * 0.18f),
            ImVec2(center.x + width * 0.30f, center.y + height * 0.18f),
            color,
            1.5f);
        break;
    }
}

bool DrawTerrainCategoryIcon(
    TerrainInspectorCategory category,
    TerrainInspectorCategory current)
{
    const ImVec2 size(kTerrainCategoryIconSize, kTerrainCategoryIconSize);
    ImGui::PushID(static_cast<int>(category));
    const bool pressed = ImGui::InvisibleButton("##terrainCategory", size);
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const bool selected = category == current;
    const bool hovered = ImGui::IsItemHovered();
    const ImU32 background = selected ? IM_COL32(78, 96, 132, 255)
        : (hovered ? IM_COL32(58, 64, 78, 255) : IM_COL32(40, 44, 54, 255));
    const ImU32 border = selected ? IM_COL32(188, 206, 236, 255) : IM_COL32(96, 102, 118, 255);
    drawList->AddRectFilled(min, max, background, 3.0f);
    drawList->AddRect(min, max, border, 3.0f);
    DrawTerrainCategoryGlyph(
        drawList, category, min, max, selected ? IM_COL32(236, 240, 248, 255) : IM_COL32(176, 184, 198, 255));
    if (hovered)
    {
        ImGui::SetTooltip("%s", TerrainInspectorCategoryTooltip(category));
    }
    ImGui::PopID();
    return pressed;
}

void DrawTerrainRuntimeWarning(std::string_view identity)
{
    if (!assets::RuntimePngIdentityIsValid(identity))
    {
        return;
    }
    const std::filesystem::path cookedRoot = CookedAssetsRoot(RepositoryRoot());
    const assets::RuntimePngLoadResolution resolved =
        assets::ResolveRuntimePngLoadFile(identity, cookedRoot, {}, AuthoringSourceRoot());
    if (assets::RuntimePngLoadFileIsAvailable(resolved))
    {
        return;
    }
    ImGui::SameLine();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const float size = ImGui::GetTextLineHeight();
    ImGui::InvisibleButton("##runtimeMissing", ImVec2(size, size));
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    drawList->AddTriangleFilled(
        ImVec2((min.x + max.x) * 0.5f, min.y + 1.0f),
        ImVec2(min.x + 1.0f, max.y - 1.0f),
        ImVec2(max.x - 1.0f, max.y - 1.0f),
        IM_COL32(220, 170, 48, 255));
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", assets::RuntimeTerrainTextureUnavailableTooltip());
    }
}

void DrawTerrainLayerThumbnail(
    const LevelEditorViewContext& view,
    std::string_view identity,
    float thumbSize)
{
    unsigned int gpuId = 0;
    int imageWidth = 0;
    int imageHeight = 0;
    bool failed = false;
#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
    if (assets::RuntimePngIdentityIsValid(identity) && view.textureThumbnails != nullptr)
    {
        const std::filesystem::path sourceRoot = AuthoringSourceRoot();
        if (!sourceRoot.empty())
        {
            view.textureThumbnails->Ensure(identity, sourceRoot / std::string(identity));
        }
        gpuId = view.textureThumbnails->TextureGpuId(identity);
        imageWidth = view.textureThumbnails->TextureWidth(identity);
        imageHeight = view.textureThumbnails->TextureHeight(identity);
        failed = view.textureThumbnails->IsFailed(identity);
    }
#else
    (void)view;
    (void)identity;
#endif
    const ImVec2 dummyMin = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(thumbSize, thumbSize));
    const ImVec2 dummyMax = ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddRectFilled(
        dummyMin,
        dummyMax,
        failed ? IM_COL32(88, 64, 64, 255) : IM_COL32(48, 52, 62, 255));
    ImGui::GetWindowDrawList()->AddRect(dummyMin, dummyMax, IM_COL32(120, 126, 140, 255));
    if (gpuId != 0)
    {
        float drawWidth = thumbSize;
        float drawHeight = thumbSize;
        ComputeTextureThumbnailDrawSize(
            imageWidth, imageHeight, thumbSize - 4.0f, drawWidth, drawHeight);
        const float offsetX = dummyMin.x + (thumbSize - drawWidth) * 0.5f;
        const float offsetY = dummyMin.y + (thumbSize - drawHeight) * 0.5f;
        ImGui::SetCursorScreenPos(ImVec2(offsetX, offsetY));
        ImGui::Image(
            ImTextureRef(static_cast<ImTextureID>(static_cast<intptr_t>(gpuId))),
            ImVec2(drawWidth, drawHeight));
        ImGui::SetCursorScreenPos(ImVec2(dummyMin.x, dummyMax.y));
    }
}

void DrawContentBrowserAssetDetails(const LevelEditorState& state)
{
    const std::string& identity = state.contentBrowser.selectedIdentity;
    if (identity.empty())
    {
        ImGui::TextUnformatted("No asset selected.");
        return;
    }

    const ContentBrowserAssetKind kind = ClassifyContentBrowserIdentity(identity);
    const std::string folder =
        ContentBrowserAssignedFolder(state.contentBrowser.organization, identity);
    const bool favorite = ContentBrowserIsFavorite(state.contentBrowser.organization, identity);
    const assets::StaticModelCatalogEntry* model = state.contentBrowser.catalog.Find(identity);
    const assets::SourceTextureCatalogEntry* texture =
        state.contentBrowser.textureCatalog.Find(identity);
    const char* displayName = identity.c_str();
    const char* assetType = kind == ContentBrowserAssetKind::Texture ? "runtime_png" : "static_glb";
    if (texture != nullptr)
    {
        displayName = texture->displayName.c_str();
        assetType = texture->assetType.c_str();
    }
    else if (model != nullptr)
    {
        displayName = model->displayName.c_str();
        assetType = model->assetType.c_str();
    }

    ImGui::Text("Name: %s", displayName);
    ImGui::Text("Kind: %s", ContentBrowserAssetKindName(kind));
    ImGui::Text("Type: %s", assetType);
    ImGui::Text("Identity: %s", identity.c_str());
    ImGui::Text("Source: game/assets/source/%s", identity.c_str());
    ImGui::Text("Folder: %s", folder.empty() ? "(unfiled)" : folder.c_str());
    ImGui::Text("Favorite: %s", favorite ? "yes" : "no");

    if (kind == ContentBrowserAssetKind::Texture)
    {
        ImGui::Text("Recipe: %s", assets::kRuntimePngRecipe.data());
        const std::filesystem::path cookedRoot = CookedAssetsRoot(RepositoryRoot());
        std::error_code cookedError;
        const bool cooked = !cookedRoot.empty()
            && std::filesystem::is_regular_file(cookedRoot / identity, cookedError);
        ImGui::TextUnformatted(cooked ? "Cooked: present" : "Cooked: missing");
        DrawTextureStagingHint(identity);
        return;
    }

    DrawStaticModelStagingHint(identity);
}

const char* ContentBrowserCollectionLabel(ContentBrowserCollection collection)
{
    switch (collection)
    {
    case ContentBrowserCollection::AllAssets:
        return "All Assets";
    case ContentBrowserCollection::Favorites:
        return "Favorites";
    case ContentBrowserCollection::Models:
        return "Models";
    case ContentBrowserCollection::Textures:
        return "Textures";
    case ContentBrowserCollection::Folders:
        return "Folders";
    }
    return "All Assets";
}

void PersistContentBrowserLayout(LevelEditorState& state)
{
    PersistContentBrowserViewState(state.contentBrowser);
}

void ApplyContentBrowserCollection(
    LevelEditorState& state,
    ContentBrowserCollection collection,
    std::string_view folderPath = {})
{
    SetContentBrowserCollection(state.contentBrowser, collection, folderPath);
    PersistContentBrowserLayout(state);
}

void ReadOnlyFloat(const char* label, float value)
{
    ImGui::Text("%s: %.6f", label, value);
}

void DrawHierarchyObjectRow(LevelEditorState& state, EditorSelection selection)
{
    char label[64];
    FormatSelectionDisplayName(selection, label, sizeof(label));
    const bool isPrimary = state.selection == selection;
    const bool isSecondary = ContainsEditorSelection(state.additionalSelections, selection);
    if (isSecondary && !isPrimary)
    {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.85f, 0.52f, 0.18f, 0.70f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.90f, 0.58f, 0.22f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.92f, 0.62f, 0.24f, 0.90f));
    }
    if (ImGui::Selectable(label, isPrimary || isSecondary) && !state.gizmo.dragging)
    {
        ApplyEditorSelectionClick(
            state.selection,
            state.additionalSelections,
            selection,
            ImGui::GetIO().KeyCtrl);
    }
    if (isSecondary && !isPrimary)
    {
        ImGui::PopStyleColor(3);
    }
}

const char* HierarchyGroupActionDisableReason(
    bool authoringAvailable,
    bool gizmoDragging,
    const char* semanticReason,
    const char* fallback)
{
    if (!authoringAvailable)
    {
        return "Lifecycle editing is available in Development only.";
    }
    if (gizmoDragging)
    {
        return "Lifecycle blocked: finish the gizmo drag first.";
    }
    return semanticReason != nullptr ? semanticReason : fallback;
}

void CommitHierarchyGroupRename(LevelEditorState& state, std::size_t groupIndex)
{
    if (groupIndex >= state.workingCopy.authoringGroups.size())
    {
        return;
    }
    const AuthoringGroupEditResult renamedResult = RenameAuthoringGroup(
        state.workingCopy, groupIndex, state.authoringGroupRename.buffer);
    if (renamedResult.succeeded)
    {
        state.selection = renamedResult.selection;
        state.additionalSelections = renamedResult.additionalSelections;
        state.hierarchyExpansion.renameFromHierarchy = false;
        state.authoringGroupRename.editing = false;
        return;
    }
    SyncAuthoringGroupRenameField(
        state.authoringGroupRename,
        groupIndex,
        state.workingCopy.authoringGroups[groupIndex].name,
        false);
    state.hierarchyExpansion.renameFromHierarchy = false;
}

void CancelHierarchyGroupRename(LevelEditorState& state, std::size_t groupIndex)
{
    if (groupIndex < state.workingCopy.authoringGroups.size())
    {
        SyncAuthoringGroupRenameField(
            state.authoringGroupRename,
            groupIndex,
            state.workingCopy.authoringGroups[groupIndex].name,
            false);
    }
    state.hierarchyExpansion.renameFromHierarchy = false;
}

LevelEditorRequest DrawHierarchy(LevelEditorState& state, const LevelEditorViewContext& view)
{
    LevelEditorRequest request = LevelEditorRequest::None;
    ApplyEditorWindowPlacement(kHierarchyWindowName, view);
    if (!ImGui::Begin(kHierarchyWindowName, &state.workspace.showHierarchy))
    {
        ImGui::End();
        return request;
    }
    RecoverEditorWindowIfNeeded(kHierarchyWindowName, view);

    ReconcileHierarchyExpansion(state.hierarchyExpansion, state.workingCopy);
    SyncHierarchyRevealForSelection(
        state.hierarchyExpansion, state.workingCopy, state.selection);

    const bool authoringAvailable = IsLevelAuthoringAvailable();
    const bool gizmoDragging = state.gizmo.dragging;
    const std::size_t selectedGroupIndex = FindExactAuthoringGroup(
        state.workingCopy, state.selection, state.additionalSelections);
    if (selectedGroupIndex == kNoAuthoringGroupIndex)
    {
        state.hierarchyExpansion.renameFromHierarchy = false;
    }

    const bool canGroup = CanIssueAuthoredLifecycleRequest(
        authoringAvailable,
        state.workingCopy,
        state.selection,
        gizmoDragging,
        LevelEditorRequest::GroupSelected,
        {},
        state.additionalSelections);
    const bool canUngroup = CanIssueAuthoredLifecycleRequest(
        authoringAvailable,
        state.workingCopy,
        state.selection,
        gizmoDragging,
        LevelEditorRequest::UngroupSelected,
        {},
        state.additionalSelections);

    ImGui::BeginDisabled(!canGroup);
    if (ImGui::SmallButton("Group Selected"))
    {
        request = LevelEditorRequest::GroupSelected;
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        const char* reason = HierarchyGroupActionDisableReason(
            authoringAvailable,
            gizmoDragging,
            CreateAuthoringGroupDisableReason(
                state.workingCopy, state.selection, state.additionalSelections),
            "Group Selected requires two or more ungrouped authored objects.");
        if (reason != nullptr)
        {
            ImGui::SetTooltip("%s", reason);
        }
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(!canUngroup);
    if (ImGui::SmallButton("Rename"))
    {
        state.hierarchyExpansion.renameFromHierarchy = true;
        state.hierarchyExpansion.renameFocusPending = true;
        SyncAuthoringGroupRenameField(
            state.authoringGroupRename,
            selectedGroupIndex,
            state.workingCopy.authoringGroups[selectedGroupIndex].name,
            false);
        state.authoringGroupRename.editing = true;
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        const char* reason = HierarchyGroupActionDisableReason(
            authoringAvailable,
            gizmoDragging,
            UngroupAuthoringGroupDisableReason(
                state.workingCopy, state.selection, state.additionalSelections),
            "Rename requires a selected Authoring Group.");
        if (!canUngroup && reason != nullptr)
        {
            ImGui::SetTooltip("%s", reason);
        }
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(!canUngroup);
    if (ImGui::SmallButton("Ungroup"))
    {
        request = LevelEditorRequest::UngroupSelected;
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        const char* reason = HierarchyGroupActionDisableReason(
            authoringAvailable,
            gizmoDragging,
            UngroupAuthoringGroupDisableReason(
                state.workingCopy, state.selection, state.additionalSelections),
            "Ungroup requires a selected Authoring Group.");
        if (reason != nullptr)
        {
            ImGui::SetTooltip("%s", reason);
        }
    }

    ImGui::Separator();

    const std::vector<HierarchyRow> rows = BuildHierarchyRows(state.workingCopy);
    bool hasGroups = false;
    for (const HierarchyRow& row : rows)
    {
        if (row.kind == HierarchyRowKind::Group)
        {
            hasGroups = true;
            break;
        }
    }

    if (hasGroups)
    {
        const bool groupsVisible =
            ImGui::TreeNodeEx(kAuthoringGroupsSectionLabel, ImGuiTreeNodeFlags_DefaultOpen);
        if (groupsVisible)
        {
            for (std::size_t groupIndex = 0;
                 groupIndex < state.workingCopy.authoringGroups.size();
                 ++groupIndex)
            {
                const world::AuthoringGroup& group = state.workingCopy.authoringGroups[groupIndex];
                ImGui::PushID(static_cast<int>(groupIndex));
                const bool groupSelected = HierarchyGroupRowIsSelected(
                    state.workingCopy,
                    groupIndex,
                    state.selection,
                    state.additionalSelections);
                const bool renamingThis = state.hierarchyExpansion.renameFromHierarchy
                    && selectedGroupIndex == groupIndex;
                const bool expanded =
                    HierarchyGroupIsExpanded(state.hierarchyExpansion, group.name);
                ImGui::SetNextItemOpen(expanded, ImGuiCond_Always);
                char groupId[32];
                std::snprintf(groupId, sizeof(groupId), "##ag%zu", groupIndex);
                ImGuiTreeNodeFlags groupFlags = ImGuiTreeNodeFlags_OpenOnArrow;
                if (!renamingThis)
                {
                    groupFlags |= ImGuiTreeNodeFlags_SpanAvailWidth;
                }
                if (groupSelected)
                {
                    groupFlags |= ImGuiTreeNodeFlags_Selected;
                }
                const bool groupOpen = ImGui::TreeNodeEx(
                    groupId,
                    groupFlags,
                    "%s",
                    renamingThis ? "" : group.name.c_str());
                if (ImGui::IsItemToggledOpen())
                {
                    SetHierarchyGroupExpanded(state.hierarchyExpansion, group.name, groupOpen);
                }
                if (!renamingThis && ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()
                    && !gizmoDragging)
                {
                    HierarchyRow groupRow{};
                    groupRow.kind = HierarchyRowKind::Group;
                    groupRow.groupIndex = groupIndex;
                    ApplyHierarchyRowClick(
                        state.workingCopy,
                        groupRow,
                        state.selection,
                        state.additionalSelections,
                        ImGui::GetIO().KeyCtrl);
                }
                if (renamingThis)
                {
                    ImGui::SameLine();
                    if (state.hierarchyExpansion.renameFocusPending)
                    {
                        ImGui::SetKeyboardFocusHere();
                        state.hierarchyExpansion.renameFocusPending = false;
                    }
                    const bool confirmed = ImGui::InputText(
                        "##GroupRename",
                        state.authoringGroupRename.buffer,
                        kAuthoringGroupRenameBufferSize,
                        ImGuiInputTextFlags_EnterReturnsTrue
                            | ImGuiInputTextFlags_AutoSelectAll);
                    const bool renameActive = ImGui::IsItemActive();
                    state.authoringGroupRename.editing = renameActive;
                    if (confirmed || ImGui::IsItemDeactivatedAfterEdit())
                    {
                        CommitHierarchyGroupRename(state, groupIndex);
                    }
                    else if (renameActive && ImGui::IsKeyPressed(ImGuiKey_Escape))
                    {
                        CancelHierarchyGroupRename(state, groupIndex);
                    }
                }
                if (groupOpen)
                {
                    for (std::size_t memberIndex = 0; memberIndex < group.members.size();
                         ++memberIndex)
                    {
                        const EditorSelection memberSelection =
                            EditorSelectionFromAuthoringGroupMember(group.members[memberIndex]);
                        ImGui::PushID(static_cast<int>(memberIndex));
                        DrawHierarchyObjectRow(state, memberSelection);
                        ImGui::PopID();
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
        ImGui::Separator();
    }

    const char* openGroup = nullptr;
    bool groupVisible = true;
    for (const HierarchyRow& row : rows)
    {
        if (row.kind != HierarchyRowKind::UngroupedObject)
        {
            continue;
        }
        const bool grouped = row.category[0] != '\0';
        if (grouped)
        {
            if (openGroup == nullptr || std::strcmp(openGroup, row.category) != 0)
            {
                if (openGroup != nullptr && groupVisible)
                {
                    ImGui::TreePop();
                }
                openGroup = row.category;
                groupVisible = ImGui::TreeNodeEx(row.category, ImGuiTreeNodeFlags_DefaultOpen);
            }
            if (!groupVisible)
            {
                continue;
            }
        }
        else if (openGroup != nullptr)
        {
            if (groupVisible)
            {
                ImGui::TreePop();
            }
            openGroup = nullptr;
            groupVisible = true;
        }

        DrawHierarchyObjectRow(state, row.selection);
    }
    if (openGroup != nullptr && groupVisible)
    {
        ImGui::TreePop();
    }

    ImGui::End();
    return request;
}

void DrawInspector(LevelEditorState& state, const LevelEditorViewContext& view)
{
    ApplyEditorWindowPlacement(kInspectorWindowName, view);
    if (!ImGui::Begin(kInspectorWindowName, &state.workspace.showInspector))
    {
        ImGui::End();
        return;
    }
    RecoverEditorWindowIfNeeded(kInspectorWindowName, view);

    ImGui::Text("Selected: %s", SelectionDisplayName(state.selection));
    if (EditorSelectionSetIsMulti(state.selection, state.additionalSelections))
    {
        ImGui::Text("Multi-selection: %zu objects (primary listed)",
            EditorSelectionSetMembers(state.selection, state.additionalSelections).size());
        ImGui::TextUnformatted("Inspector edits the primary selection only.");
    }
    const std::size_t selectedGroupIndex = FindExactAuthoringGroup(
        state.workingCopy, state.selection, state.additionalSelections);
    if (selectedGroupIndex != kNoAuthoringGroupIndex)
    {
        world::AuthoringGroup& selectedGroup =
            state.workingCopy.authoringGroups[selectedGroupIndex];
        ImGui::Separator();
        ImGui::TextUnformatted("Authoring Group");
        AuthoringGroupRenameFieldState& renameField = state.authoringGroupRename;
        if (state.hierarchyExpansion.renameFromHierarchy)
        {
            ImGui::Text("Group Name: %s", selectedGroup.name.c_str());
        }
        else
        {
            const bool renameWasActive =
                renameField.editing && renameField.boundGroupIndex == selectedGroupIndex;
            SyncAuthoringGroupRenameField(
                renameField, selectedGroupIndex, selectedGroup.name, renameWasActive);
            const bool renamed = ImGui::InputText(
                "Group Name",
                renameField.buffer,
                kAuthoringGroupRenameBufferSize);
            const bool renameDeactivated = ImGui::IsItemDeactivatedAfterEdit();
            const bool renameActive = ImGui::IsItemActive();
            if (renamed)
            {
                const AuthoringGroupEditResult renamedResult = RenameAuthoringGroup(
                    state.workingCopy, selectedGroupIndex, renameField.buffer);
                if (!renamedResult.succeeded)
                {
                    SyncAuthoringGroupRenameField(
                        renameField, selectedGroupIndex, selectedGroup.name, false);
                }
            }
            if (renameDeactivated || (renameWasActive && !renameActive))
            {
                const AuthoringGroupEditResult renamedResult = RenameAuthoringGroup(
                    state.workingCopy, selectedGroupIndex, renameField.buffer);
                if (!renamedResult.succeeded)
                {
                    SyncAuthoringGroupRenameField(
                        renameField, selectedGroupIndex, selectedGroup.name, false);
                }
            }
            renameField.editing = renameActive;
        }
        if (ImGui::Button("Ungroup"))
        {
            const AuthoringGroupEditResult ungrouped =
                UngroupAuthoringGroup(state.workingCopy, selectedGroupIndex);
            if (ungrouped.succeeded)
            {
                state.selection = ungrouped.selection;
                state.additionalSelections = ungrouped.additionalSelections;
                state.hierarchyExpansion.renameFromHierarchy = false;
                ReconcileHierarchyExpansion(state.hierarchyExpansion, state.workingCopy);
                state.lastMessage = "Authoring Group removed.";
            }
        }
        ImGui::Separator();
    }
    else
    {
        SyncAuthoringGroupRenameField(
            state.authoringGroupRename, kNoAuthoringGroupIndex, {}, false);
    }
    ImGui::Text("Editor nav speed: %.1f (session only)", state.editorCamera.movementSpeed);
    ImGui::Separator();

    if (state.selection.kind == EditorObjectKind::None
        || !IsValidSelection(state.workingCopy, state.selection))
    {
        ImGui::TextUnformatted("No object selected.");
        ImGui::End();
        return;
    }

    // Inspector always reads/writes the working copy. Viewport pick/highlight
    // stay on the applied world until Apply Preview.
    world::LevelDefinition& level = state.workingCopy;
    switch (state.selection.kind)
    {
    case EditorObjectKind::Spawn:
        EditVec3("Spawn X Y Z", level.initialSpawnVisualCenter);
        break;
    case EditorObjectKind::Camera:
        ImGui::TextWrapped(
            "Gameplay camera authoring (LevelDefinition.camera). These values "
            "frame the follow camera after Apply Preview. They are not the "
            "editor navigation camera.");
        EditVec3("Offset X Y Z", level.camera.offset);
        ImGui::InputFloat("FOV Y", &level.camera.fieldOfViewY, 0.0f, 0.0f, kFloatFormat);
        break;
    case EditorObjectKind::Environment:
        ImGui::TextWrapped(
            "Level Environment (ambient). Changes preview live in the editor "
            "viewport. Apply promotes them; Save persists them.");
        ImGui::ColorEdit3("Ambient Color", &level.environment.ambientColor.x);
        ClampColor01(level.environment.ambientColor);
        ImGui::SliderFloat(
            "Ambient Intensity",
            &level.environment.ambientIntensity,
            0.0f,
            world::kMaxAuthoredAmbientIntensity,
            "%.3f");
        if (level.environment.ambientIntensity < 0.0f)
        {
            level.environment.ambientIntensity = 0.0f;
        }
        if (level.environment.ambientIntensity > world::kMaxAuthoredAmbientIntensity)
        {
            level.environment.ambientIntensity = world::kMaxAuthoredAmbientIntensity;
        }
        break;
    case EditorObjectKind::DirectionalLight:
    {
        ImGui::TextWrapped(
            "Primary Directional Light. Direction is the ray-travel vector "
            "(from the sun toward surfaces). The viewport sun is an authoring "
            "anchor only and does not position the light.");
        ImGui::Checkbox("Enabled", &level.environment.directionalEnabled);
        ImGui::Checkbox("Shadows Enabled", &level.environment.directionalShadowsEnabled);
        core::Vec3 direction = level.environment.directionalRayDirection;
        EditVec3("Direction X Y Z", direction);
        if (!TryCommitAuthoredDirectionalRay(
                direction, level.environment.directionalRayDirection))
        {
            // Keep the previous canonical direction. Zero/NaN edits are rejected.
        }
        ImGui::ColorEdit3("Color", &level.environment.directionalColor.x);
        ClampColor01(level.environment.directionalColor);
        ImGui::SliderFloat(
            "Intensity",
            &level.environment.directionalIntensity,
            0.0f,
            world::kMaxAuthoredDirectionalIntensity,
            "%.3f");
        if (level.environment.directionalIntensity < 0.0f)
        {
            level.environment.directionalIntensity = 0.0f;
        }
        if (level.environment.directionalIntensity > world::kMaxAuthoredDirectionalIntensity)
        {
            level.environment.directionalIntensity = world::kMaxAuthoredDirectionalIntensity;
        }
        break;
    }
    case EditorObjectKind::Ground:
        EditVec3("Center X Y Z", level.ground.center);
        EditVec3("Size X Y Z", level.ground.size);
        break;
    case EditorObjectKind::Terrain:
    {
        if (!level.hasTerrain)
        {
            ImGui::TextUnformatted("No Terrain in this Level.");
            break;
        }

        ImGui::BeginGroup();
        if (DrawTerrainCategoryIcon(
                TerrainInspectorCategory::Terrain, state.terrainInspectorCategory))
        {
            if (state.terrainInspectorCategory != TerrainInspectorCategory::Terrain)
            {
                editor::EndTerrainPaintStroke(state.terrainPaint);
                editor::EndTerrainSculptStroke(state.terrainSculpt);
                state.terrainInspectorCategory = TerrainInspectorCategory::Terrain;
            }
        }
        if (DrawTerrainCategoryIcon(
                TerrainInspectorCategory::MaterialsPaint, state.terrainInspectorCategory))
        {
            if (state.terrainInspectorCategory != TerrainInspectorCategory::MaterialsPaint)
            {
                editor::EndTerrainPaintStroke(state.terrainPaint);
                editor::EndTerrainSculptStroke(state.terrainSculpt);
                state.terrainInspectorCategory = TerrainInspectorCategory::MaterialsPaint;
            }
        }
        if (DrawTerrainCategoryIcon(
                TerrainInspectorCategory::Sculpt, state.terrainInspectorCategory))
        {
            if (state.terrainInspectorCategory != TerrainInspectorCategory::Sculpt)
            {
                editor::EndTerrainPaintStroke(state.terrainPaint);
                editor::EndTerrainSculptStroke(state.terrainSculpt);
                state.terrainInspectorCategory = TerrainInspectorCategory::Sculpt;
            }
        }
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();

        std::vector<std::string> textureChoices;
#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
        textureChoices = state.contentBrowser.textureCatalog.Identities();
#endif
        const int layerCount = world::TerrainMaterialLayerCount(level.terrain);
        editor::SanitizeTerrainPaintState(state.terrainPaint, level.terrain);

        if (state.terrainInspectorCategory == TerrainInspectorCategory::Terrain)
        {
            ImGui::TextUnformatted("Terrain");
            ImGui::TextWrapped(
                "Singleton Level Terrain. Origin is the min-X / min-Z sample. "
                "Heights are relative to Origin Y. Resolution is creation-time "
                "and is not resampled here.");
            ImGui::Checkbox("Enabled", &level.terrain.enabled);
            EditVec3("Origin X Y Z", level.terrain.origin);
            ImGui::InputFloat("Size X", &level.terrain.sizeX, 0.0f, 0.0f, kFloatFormat);
            ImGui::InputFloat("Size Z", &level.terrain.sizeZ, 0.0f, 0.0f, kFloatFormat);
            ImGui::BeginDisabled(true);
            ImGui::InputInt("Resolution X", &level.terrain.resolutionX);
            ImGui::InputInt("Resolution Z", &level.terrain.resolutionZ);
            ImGui::EndDisabled();
            ImGui::Text("Samples: %d", world::TerrainSampleCount(level.terrain));
        }
        else if (state.terrainInspectorCategory == TerrainInspectorCategory::MaterialsPaint)
        {
            ImGui::TextUnformatted("Terrain Materials");
            ImGui::TextWrapped(
                "Assigned materials. Layer 0 is the Base surface. Extra layers "
                "stay invisible until painted.");
            for (int layer = 0; layer < layerCount; ++layer)
            {
                ImGui::PushID(layer);
                const std::string& identity = world::TerrainLayerTextureIdentity(level.terrain, layer);
                const std::string display = TerrainLayerDisplayName(identity);
                const std::string title = TerrainLayerTitle(layer);
                ImGui::BeginGroup();
                ImGui::TextUnformatted(title.c_str());
                DrawTerrainLayerThumbnail(view, identity, kTerrainMaterialThumbSize);
                if (!identity.empty() && ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", identity.c_str());
                }
                ImGui::SameLine();
                ImGui::BeginGroup();
                ImGui::TextUnformatted(display.c_str());
                if (!identity.empty() && ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", identity.c_str());
                }
                DrawTerrainRuntimeWarning(identity);
                float tiling = world::TerrainLayerTextureTiling(level.terrain, layer);
                ImGui::SetNextItemWidth(96.0f);
                if (ImGui::InputFloat("Tiling", &tiling, 0.0f, 0.0f, kFloatFormat))
                {
                    world::TrySetTerrainLayerTextureTiling(level.terrain, layer, tiling);
                }
                if (layer == 0)
                {
                    const char* assignPreview = identity.empty() ? "Assign..." : "Change...";
                    if (ImGui::BeginCombo("##assignLayer0", assignPreview))
                    {
                        if (ImGui::Selectable("Solid Terrain fallback", identity.empty()))
                        {
                            world::TryClearTerrainTextureIdentity(level.terrain);
                        }
                        for (const std::string& choice : textureChoices)
                        {
                            const bool selected = choice == identity;
                            const std::string choiceName = TerrainLayerDisplayName(choice);
                            if (ImGui::Selectable(choiceName.c_str(), selected))
                            {
                                world::TryAssignTerrainTextureIdentity(level.terrain, choice);
                            }
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip("%s", choice.c_str());
                            }
                            if (selected)
                            {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Clear"))
                    {
                        world::TryClearTerrainTextureIdentity(level.terrain);
                    }
                }
                else if (ImGui::Button("Remove"))
                {
                    if (world::TryRemoveTerrainMaterialLayer(level.terrain, layer))
                    {
                        editor::RemapTerrainPaintLayerAfterRemove(state.terrainPaint, layer);
                        editor::SanitizeTerrainPaintState(state.terrainPaint, level.terrain);
                    }
                }
                ImGui::EndGroup();
                ImGui::EndGroup();
                ImGui::PopID();
                ImGui::Spacing();
            }
            ImGui::TextWrapped(
                "Tiling repeats per world unit. Range [%.2f, %.2f]. Default %.2f.",
                world::kMinTerrainTextureTiling,
                world::kMaxTerrainTextureTiling,
                world::kDefaultTerrainTextureTiling);
            if (layerCount < world::kMaxTerrainMaterialLayers)
            {
                if (ImGui::BeginCombo("Add Layer", "Select texture..."))
                {
                    for (const std::string& choice : textureChoices)
                    {
                        const bool alreadyAssigned =
                            world::TerrainReferencesTextureIdentity(level.terrain, choice);
                        if (alreadyAssigned)
                        {
                            ImGui::BeginDisabled();
                        }
                        const std::string choiceName = TerrainLayerDisplayName(choice);
                        if (ImGui::Selectable(choiceName.c_str(), false) && !alreadyAssigned)
                        {
                            world::TryAddTerrainMaterialLayer(level.terrain, choice);
                        }
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("%s", choice.c_str());
                        }
                        if (alreadyAssigned)
                        {
                            ImGui::EndDisabled();
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            else
            {
                ImGui::TextWrapped("Maximum of four Terrain material layers.");
            }

            ImGui::Separator();
            ImGui::TextUnformatted("Terrain Paint");
            ImGui::TextWrapped(
                "Paint Tool. Select the assigned material the brush applies. "
                "Paint edits working-copy weights only. Apply promotes render. "
                "Brush settings are not saved with the Level.");
            ImGui::TextUnformatted("Paint Layer");
            for (int layer = 0; layer < layerCount; ++layer)
            {
                ImGui::PushID(100 + layer);
                const std::string& identity = world::TerrainLayerTextureIdentity(level.terrain, layer);
                const bool selected = state.terrainPaint.selectedLayer == layer;
                const std::string display = TerrainLayerDisplayName(identity);
                const std::string rowLabel = TerrainLayerTitle(layer) + "  " + display;
                if (selected)
                {
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(70, 90, 130, 110));
                }
                ImGui::BeginChild(
                    "##paintLayerRow",
                    ImVec2(0.0f, kTerrainMaterialThumbSize + 10.0f),
                    true,
                    ImGuiWindowFlags_NoScrollbar);
                const bool radioClicked = ImGui::RadioButton("##paintLayer", selected);
                ImGui::SameLine();
                DrawTerrainLayerThumbnail(view, identity, kTerrainMaterialThumbSize);
                ImGui::SameLine();
                ImGui::BeginGroup();
                ImGui::TextUnformatted(rowLabel.c_str());
                DrawTerrainRuntimeWarning(identity);
                ImGui::EndGroup();
                if (!identity.empty() && ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", identity.c_str());
                }
                ImGui::EndChild();
                if (selected)
                {
                    ImGui::PopStyleColor();
                }
                if (radioClicked || (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)))
                {
                    editor::TrySelectTerrainPaintLayer(state.terrainPaint, layer, level.terrain);
                }
                ImGui::PopID();
            }

            const bool paintWasOn = state.terrainPaint.mode;
            if (ImGui::Checkbox("Paint Mode", &state.terrainPaint.mode))
            {
                editor::EndTerrainPaintStroke(state.terrainPaint);
                if (state.terrainPaint.mode && !paintWasOn)
                {
                    state.terrainSculpt.mode = false;
                    editor::EndTerrainSculptStroke(state.terrainSculpt);
                    CancelAllEditorPlacement(
                        state.placementMode,
                        state.placementPointerBlocked,
                        state.staticPropPlacement);
                }
            }
            ImGui::SliderFloat(
                "Paint Radius",
                &state.terrainPaint.radius,
                world::kMinTerrainPaintRadius,
                world::kMaxTerrainPaintRadius,
                "%.2f");
            ImGui::SliderFloat(
                "Paint Strength",
                &state.terrainPaint.strength,
                world::kMinTerrainPaintStrength,
                world::kMaxTerrainPaintStrength,
                "%.2f");
            editor::SanitizeTerrainPaintState(state.terrainPaint, level.terrain);
        }
        else
        {
            ImGui::TextUnformatted("Terrain Sculpt");
            ImGui::TextWrapped(
                "Sculpt edits working-copy heights only. Apply promotes render "
                "and collision. Brush settings are not saved with the Level.");
            const bool sculptWasOn = state.terrainSculpt.mode;
            if (ImGui::Checkbox("Sculpt Mode", &state.terrainSculpt.mode))
            {
                editor::EndTerrainSculptStroke(state.terrainSculpt);
                if (state.terrainSculpt.mode && !sculptWasOn)
                {
                    state.terrainPaint.mode = false;
                    editor::EndTerrainPaintStroke(state.terrainPaint);
                    CancelAllEditorPlacement(
                        state.placementMode,
                        state.placementPointerBlocked,
                        state.staticPropPlacement);
                }
            }
            int operation = static_cast<int>(state.terrainSculpt.operation);
            const char* operationNames[] = {"Raise", "Lower", "Smooth", "Flatten"};
            if (ImGui::Combo("Brush", &operation, operationNames, 4))
            {
                state.terrainSculpt.operation =
                    static_cast<world::TerrainSculptOperation>(operation);
                editor::EndTerrainSculptStroke(state.terrainSculpt);
            }
            ImGui::SliderFloat(
                "Radius",
                &state.terrainSculpt.radius,
                world::kMinTerrainSculptRadius,
                world::kMaxTerrainSculptRadius,
                "%.2f");
            ImGui::SliderFloat(
                "Strength",
                &state.terrainSculpt.strength,
                world::kMinTerrainSculptStrength,
                world::kMaxTerrainSculptStrength,
                "%.2f");
            editor::SanitizeTerrainSculptState(state.terrainSculpt);
        }
        ImGui::EndGroup();
        break;
    }
    case EditorObjectKind::ElevatedPlatform:
        if (state.selection.index < level.elevatedPlatforms.size())
        {
            world::Box& platform = level.elevatedPlatforms[state.selection.index];
            EditVec3("Center X Y Z", platform.center);
            EditVec3("Size X Y Z", platform.size);
        }
        break;
    case EditorObjectKind::Slope:
        ImGui::TextUnformatted("Read-only in M33.");
        if (state.selection.index < level.slopes.size())
        {
            const world::SlopeSpec& slope = level.slopes[state.selection.index];
            ReadOnlyVec3("Center", slope.center);
            ReadOnlyVec3("Size", slope.size);
            ReadOnlyFloat("Rotation Z degrees", slope.rotationZDegrees);
        }
        break;
    case EditorObjectKind::MovingPlatform:
        ImGui::TextUnformatted("Read-only in M33.");
        ReadOnlyVec3("Size", level.movingPlatform.size);
        ReadOnlyFloat("Path min X", level.movingPlatform.pathMinX);
        ReadOnlyFloat("Path max X", level.movingPlatform.pathMaxX);
        ReadOnlyFloat("Center Y", level.movingPlatform.centerY);
        ReadOnlyFloat("Center Z", level.movingPlatform.centerZ);
        ReadOnlyFloat("Speed", level.movingPlatform.speed);
        ReadOnlyFloat("Start X", level.movingPlatform.startX);
        ReadOnlyVec3("Runtime position", view.movingPlatformRuntimeCenter);
        break;
    case EditorObjectKind::Checkpoint:
        if (state.selection.index < level.checkpoints.size())
        {
            world::CheckpointSpec& checkpoint = level.checkpoints[state.selection.index];
            EditVec3("Trigger Center", checkpoint.center);
            EditVec3("Trigger Size", checkpoint.size);
            EditVec3("Respawn Position", checkpoint.respawnPosition);
        }
        break;
    case EditorObjectKind::Hazard:
        if (state.selection.index < level.hazards.size())
        {
            world::HazardSpec& hazard = level.hazards[state.selection.index];
            EditVec3("Center X Y Z", hazard.center);
            EditVec3("Size X Y Z", hazard.size);
        }
        break;
    case EditorObjectKind::Collectible:
        if (state.selection.index < level.collectibles.size())
        {
            world::CollectibleSpec& collectible = level.collectibles[state.selection.index];
            EditVec3("Center X Y Z", collectible.center);
            EditVec3("Size X Y Z", collectible.size);
        }
        break;
    case EditorObjectKind::Goal:
        if (state.selection.index < level.levelGoals.size())
        {
            world::LevelGoalSpec& goal = level.levelGoals[state.selection.index];
            EditVec3("Center X Y Z", goal.center);
            EditVec3("Size X Y Z", goal.size);
            const std::vector<NextLevelSelectorEntry> nextLevelChoices =
                MakeNextLevelSelectorEntries(state.authoredLevels, goal.nextLevelId);
            std::string preview = "None";
            bool currentMissing = false;
            for (const NextLevelSelectorEntry& entry : nextLevelChoices)
            {
                if (entry.id == goal.nextLevelId)
                {
                    currentMissing = entry.missing;
                    if (entry.id.empty())
                    {
                        preview = "None";
                    }
                    else if (entry.missing)
                    {
                        preview = entry.id + " (missing)";
                    }
                    else
                    {
                        preview = entry.id;
                    }
                    break;
                }
            }
            if (ImGui::BeginCombo("Next Level", preview.c_str()))
            {
                for (const NextLevelSelectorEntry& entry : nextLevelChoices)
                {
                    char label[96]{};
                    if (entry.id.empty())
                    {
                        std::snprintf(label, sizeof(label), "None");
                    }
                    else if (entry.missing)
                    {
                        std::snprintf(label, sizeof(label), "%s (missing)", entry.id.c_str());
                    }
                    else
                    {
                        std::snprintf(label, sizeof(label), "%s", entry.id.c_str());
                    }
                    const bool selected = entry.id == goal.nextLevelId;
                    if (ImGui::Selectable(label, selected))
                    {
                        ApplyNextLevelSelectorId(goal, entry.id);
                    }
                    if (selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            if (currentMissing)
            {
                ImGui::TextWrapped(
                    "Authored destination is missing from discovery. The value is kept until you "
                    "choose None or a discovered Level.");
            }
            else
            {
                ImGui::TextUnformatted("None is terminal. Selection writes the logical Level ID.");
            }
        }
        break;
    case EditorObjectKind::DynamicBox:
        if (state.selection.index < level.dynamicBoxes.size())
        {
            world::DynamicBoxSpec& box = level.dynamicBoxes[state.selection.index];
            EditVec3("Center X Y Z", box.center);
            EditVec3("Size X Y Z", box.size);
            ImGui::InputFloat("Mass (kg)", &box.massKg, 0.0f, 0.0f, kFloatFormat);
        }
        break;
    case EditorObjectKind::PressurePlate:
        if (state.selection.index < level.pressurePlates.size())
        {
            world::PressurePlateSpec& plate = level.pressurePlates[state.selection.index];
            EditVec3("Position X Y Z", plate.center);
            EditVec3("Size X Y Z", plate.size);
            {
                int current = plate.linkedDoorIndex + 1;
                if (current < 0 || current > static_cast<int>(level.doors.size()))
                {
                    current = 0;
                }
                if (ImGui::BeginCombo("Linked Door", current == 0 ? "None" : SelectionDisplayName(
                        {EditorObjectKind::Door, static_cast<std::size_t>(current - 1)})))
                {
                    if (ImGui::Selectable("None", current == 0))
                    {
                        plate.linkedDoorIndex = world::kNoLinkedDoor;
                    }
                    for (std::size_t doorIndex = 0; doorIndex < level.doors.size(); ++doorIndex)
                    {
                        char label[64]{};
                        FormatSelectionDisplayName(
                            {EditorObjectKind::Door, doorIndex}, label, sizeof(label));
                        if (ImGui::Selectable(
                                label, plate.linkedDoorIndex == static_cast<int>(doorIndex)))
                        {
                            plate.linkedDoorIndex = static_cast<int>(doorIndex);
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            ImGui::Checkbox("Activate By Dynamic Box", &plate.activateByDynamicBox);
            ImGui::Checkbox("Activate By Player", &plate.activateByPlayer);
            ImGui::Checkbox("Visible In Gameplay", &plate.visibleInGameplay);
            ImGui::Checkbox("Controls Directional Light", &plate.controlsDirectionalLight);
            ImGui::Separator();
            ImGui::TextUnformatted("Controlled Local Lights");
            if (plate.controlledLocalLights.empty())
            {
                ImGui::TextUnformatted("None");
            }
            for (std::size_t targetIndex = 0; targetIndex < plate.controlledLocalLights.size();
                 ++targetIndex)
            {
                ImGui::PushID(static_cast<int>(targetIndex));
                const world::LocalLightTarget target = plate.controlledLocalLights[targetIndex];
                const EditorObjectKind kind = target.kind == world::LocalLightKind::Point
                    ? EditorObjectKind::PointLight
                    : EditorObjectKind::SpotLight;
                char label[64]{};
                FormatSelectionDisplayName(
                    {kind, static_cast<std::size_t>(target.index)}, label, sizeof(label));
                ImGui::TextUnformatted(label);
                ImGui::SameLine();
                if (ImGui::SmallButton("Remove"))
                {
                    TryRemovePressurePlateLocalLightTargetAt(plate, targetIndex);
                    ImGui::PopID();
                    break;
                }
                ImGui::PopID();
            }
            {
                if (ImGui::BeginCombo("Add Point Light", "None"))
                {
                    ImGui::Selectable("None", true);
                    for (std::size_t lightIndex = 0; lightIndex < level.pointLights.size();
                         ++lightIndex)
                    {
                        const world::LocalLightTarget target{
                            world::LocalLightKind::Point, static_cast<int>(lightIndex)};
                        if (world::PressurePlateHasLocalLightTarget(plate, target))
                        {
                            continue;
                        }
                        char label[64]{};
                        FormatSelectionDisplayName(
                            {EditorObjectKind::PointLight, lightIndex}, label, sizeof(label));
                        if (ImGui::Selectable(label, false))
                        {
                            TryAddPressurePlateLocalLightTarget(
                                plate, target, level.pointLights.size(), level.spotLights.size());
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            {
                if (ImGui::BeginCombo("Add Spot Light", "None"))
                {
                    ImGui::Selectable("None", true);
                    for (std::size_t lightIndex = 0; lightIndex < level.spotLights.size();
                         ++lightIndex)
                    {
                        const world::LocalLightTarget target{
                            world::LocalLightKind::Spot, static_cast<int>(lightIndex)};
                        if (world::PressurePlateHasLocalLightTarget(plate, target))
                        {
                            continue;
                        }
                        char label[64]{};
                        FormatSelectionDisplayName(
                            {EditorObjectKind::SpotLight, lightIndex}, label, sizeof(label));
                        if (ImGui::Selectable(label, false))
                        {
                            TryAddPressurePlateLocalLightTarget(
                                plate, target, level.pointLights.size(), level.spotLights.size());
                        }
                    }
                    ImGui::EndCombo();
                }
            }
        }
        break;
    case EditorObjectKind::Door:
        if (state.selection.index < level.doors.size())
        {
            world::DoorSpec& door = level.doors[state.selection.index];
            EditVec3("Position X Y Z", door.center);
            EditVec3("Size X Y Z", door.size);
            ImGui::InputFloat("Open Distance", &door.openDistance, 0.0f, 0.0f, kFloatFormat);
            bool requiresItem = world::DoorRequiresInventoryItem(door);
            if (ImGui::Checkbox("Requires Item", &requiresItem))
            {
                if (!requiresItem)
                {
                    door.requiredItemId.clear();
                }
                else if (door.requiredItemId.empty())
                {
                    const std::vector<std::string> pickupIds =
                        world::UniqueAuthoredPickupItemIds(level.itemPickups);
                    if (!pickupIds.empty())
                    {
                        door.requiredItemId = pickupIds.front();
                    }
                }
            }
            if (world::DoorRequiresInventoryItem(door))
            {
                std::vector<std::string> itemIds =
                    world::UniqueAuthoredPickupItemIds(level.itemPickups);
                bool currentListed = false;
                for (const std::string& itemId : itemIds)
                {
                    if (itemId == door.requiredItemId)
                    {
                        currentListed = true;
                        break;
                    }
                }
                if (!currentListed)
                {
                    itemIds.insert(itemIds.begin(), door.requiredItemId);
                }
                if (ImGui::BeginCombo("Required Item", door.requiredItemId.c_str()))
                {
                    for (const std::string& itemId : itemIds)
                    {
                        const bool selected = door.requiredItemId == itemId;
                        if (ImGui::Selectable(itemId.c_str(), selected))
                        {
                            door.requiredItemId = itemId;
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            ImGui::TextUnformatted("Opens +Y from the authored closed position.");
        }
        break;
    case EditorObjectKind::ItemPickup:
        if (state.selection.index < level.itemPickups.size())
        {
            world::ItemPickupSpec& pickup = level.itemPickups[state.selection.index];
            EditVec3("Position X Y Z", pickup.position);
            ItemIdInspectorFieldState& itemIdField = state.itemIdInspector;
            const bool itemIdWidgetWasActive =
                itemIdField.editing && itemIdField.boundSelection == state.selection;
            SyncItemIdInspectorField(
                itemIdField, state.selection, pickup.itemId, itemIdWidgetWasActive);
            ImGui::PushID(static_cast<int>(state.selection.index));
            const bool itemIdEdited = ImGui::InputText(
                "Item ID", itemIdField.buffer, sizeof(itemIdField.buffer));
            const bool itemIdDeactivated = ImGui::IsItemDeactivatedAfterEdit();
            const bool itemIdActive = ImGui::IsItemActive();
            ImGui::PopID();
            if (itemIdEdited)
            {
                TryAcceptItemIdInspectorField(pickup.itemId, itemIdField);
            }
            if (itemIdDeactivated || (itemIdWidgetWasActive && !itemIdActive))
            {
                CommitItemIdInspectorFieldOnFocusLoss(pickup.itemId, itemIdField);
            }
            itemIdField.editing = itemIdActive;
            if (ImGui::InputInt("Quantity", &pickup.quantity))
            {
                if (pickup.quantity < 1)
                {
                    pickup.quantity = 1;
                }
                if (pickup.quantity > gameplay::kMaxItemQuantity)
                {
                    pickup.quantity = gameplay::kMaxItemQuantity;
                }
            }
            ImGui::TextUnformatted(
                pickup.modelIdentity.empty() ? "Model: (primitive fallback)" : "Model:");
            if (!pickup.modelIdentity.empty())
            {
                ImGui::TextWrapped("%s", pickup.modelIdentity.c_str());
                DrawStaticModelStagingHint(pickup.modelIdentity);
            }
            if (ImGui::Button("Assign Model from Content Browser"))
            {
                const std::string& identity = state.contentBrowser.selectedIdentity;
                if (world::StaticPropIdentityIsValid(identity))
                {
                    pickup.modelIdentity = identity;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear Model"))
            {
                pickup.modelIdentity.clear();
            }
            ImGui::TextUnformatted("Optional visual only. Empty uses the primitive fallback.");
            ImGui::Separator();
            ImGui::TextUnformatted("Visual");
            ImGui::TextUnformatted(
                "Fits the assigned model. Does not move the gameplay pickup position.");
            EditVec3("Visual Offset X Y Z", pickup.visualOffset);
            EditVec3("Visual Rotation X Y Z (deg)", pickup.visualRotationDegrees);
            EditVec3("Visual Scale X Y Z", pickup.visualScale);
            ImGui::Checkbox("Show Interaction Bounds", &pickup.showInteractionBounds);
            ImGui::TextUnformatted(
                "When targeted in Gameplay, draw the interaction-bounds wire. Does not change "
                "targeting, HUD, or collection.");
            ImGui::SliderFloat(
                "Target Highlight Intensity",
                &pickup.targetHighlightIntensity,
                world::kMinItemPickupTargetHighlightIntensity,
                world::kMaxItemPickupTargetHighlightIntensity,
                "%.2f");
            ImGui::TextUnformatted(
                "Gameplay target tint opacity only. 0 keeps HUD and targeting. Independent of "
                "interaction bounds and Gold Amount.");
            ImGui::SliderFloat(
                "Target Highlight Gold Amount",
                &pickup.targetHighlightGoldAmount,
                world::kMinItemPickupTargetHighlightGoldAmount,
                world::kMaxItemPickupTargetHighlightGoldAmount,
                "%.2f");
            ImGui::TextUnformatted(
                "How strongly the target pass pushes the model toward gold. Not another opacity "
                "control. Independent of Intensity and bounds.");
            ImGui::Separator();
            ImGui::TextUnformatted("Idle Presentation");
            ImGui::Checkbox("Idle Animation", &pickup.idleAnimationEnabled);
            ImGui::TextUnformatted(
                "Visual-only bob and Y spin. Off keeps the authored M58 transform. Does not "
                "change targeting.");
            ImGui::SliderFloat(
                "Bob Amplitude",
                &pickup.idleBobAmplitude,
                world::kMinItemPickupIdleBobAmplitude,
                world::kMaxItemPickupIdleBobAmplitude,
                "%.2f");
            ImGui::SliderFloat(
                "Bob Speed",
                &pickup.idleBobSpeed,
                world::kMinItemPickupIdleBobSpeed,
                world::kMaxItemPickupIdleBobSpeed,
                "%.2f");
            ImGui::SliderFloat(
                "Spin Speed (deg/s)",
                &pickup.idleSpinSpeedDegrees,
                world::kMinItemPickupIdleSpinSpeedDegrees,
                world::kMaxItemPickupIdleSpinSpeedDegrees,
                "%.1f");
            ImGui::TextUnformatted(
                "Idle values stay stored while animation is off. Runtime phase is never saved.");
            ImGui::TextUnformatted(
                "Translate edits Position. Rotate gizmo edits Visual Rotation. Scale gizmo edits "
                "Visual Scale. Offset remains Inspector-only.");
        }
        break;
    case EditorObjectKind::StaticProp:
        if (state.selection.index < level.staticProps.size())
        {
            world::StaticPropSpec& prop = level.staticProps[state.selection.index];
            ImGui::TextWrapped("%s", prop.modelIdentity.c_str());
            ImGui::TextUnformatted("Referenced Static Model Asset (not scene selection).");
            DrawStaticModelStagingHint(prop.modelIdentity);
            ImGui::TextUnformatted("Scale is visual model scale, not primitive Resize.");
            ImGui::TextUnformatted("Translate gizmo edits Position. Rotate gizmo edits Rotation. Scale gizmo edits Scale.");
            ImGui::TextUnformatted("Model Preview auto-frames and is not world size.");
#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
            if (view.staticPropModels != nullptr)
            {
                core::Vec3 localMin{};
                core::Vec3 localMax{};
                if (view.staticPropModels->TryGetLoadedLocalBounds(
                        prop.modelIdentity, localMin, localMax))
                {
                    const core::Vec3 loadedSize{
                        localMax.x - localMin.x,
                        localMax.y - localMin.y,
                        localMax.z - localMin.z};
                    ImGui::Text(
                        "Loaded staged size at Scale (1,1,1): %.2f x %.2f x %.2f",
                        loadedSize.x,
                        loadedSize.y,
                        loadedSize.z);
                    core::Vec3 worldCenter{};
                    core::Vec3 worldSize{};
                    StaticPropWorldAabb(prop, localMin, localMax, worldCenter, worldSize);
                    ImGui::Text(
                        "Current visual size: %.2f x %.2f x %.2f",
                        worldSize.x,
                        worldSize.y,
                        worldSize.z);
                    if (StaticPropContainsWorldPoint(
                            prop, localMin, localMax, view.gameplayCameraPosition)
                        || StaticPropContainsWorldPoint(
                            prop, localMin, localMax, view.gameplayCameraTarget))
                    {
                        ImGui::TextWrapped(
                            "Gameplay camera is inside this model's bounds, so Gameplay can look "
                            "like the sky. Move Position or reduce Scale.");
                    }
                }
                else
                {
                    ImGui::TextWrapped(
                        "Apply Preview to load the staged model and see its world size.");
                }
            }
#endif
            EditVec3("Position X Y Z", prop.position);
            EditVec3("Rotation X Y Z (deg)", prop.rotationDegrees);
            EditVec3("Scale X Y Z", prop.scale);
        }
        break;
    case EditorObjectKind::PointLight:
        if (state.selection.index < level.pointLights.size())
        {
            world::PointLightSpec& light = level.pointLights[state.selection.index];
            ImGui::TextWrapped(
                "Repeatable Point Light. Position is the real illumination origin.");
            ImGui::Checkbox("Enabled", &light.enabled);
            EditVec3("Position X Y Z", light.position);
            ImGui::ColorEdit3("Color", &light.color.x);
            ClampColor01(light.color);
            ImGui::SliderFloat(
                "Intensity",
                &light.intensity,
                0.0f,
                world::kMaxAuthoredLocalLightIntensity,
                "%.3f");
            light.intensity = world::ClampLocalLightIntensity(light.intensity);
            ImGui::SliderFloat(
                "Range",
                &light.range,
                world::kMinAuthoredLocalLightRange,
                world::kMaxAuthoredLocalLightRange,
                "%.3f");
            light.range = world::ClampLocalLightRange(light.range);
        }
        break;
    case EditorObjectKind::SpotLight:
        if (state.selection.index < level.spotLights.size())
        {
            world::SpotLightSpec& light = level.spotLights[state.selection.index];
            ImGui::TextWrapped(
                "Repeatable Spot Light. Position is the illumination origin. "
                "Direction is the cone axis (ray travel).");
            ImGui::Checkbox("Enabled", &light.enabled);
            EditVec3("Position X Y Z", light.position);
            core::Vec3 direction = light.direction;
            EditVec3("Direction X Y Z", direction);
            if (!TryCommitAuthoredSpotDirection(direction, light.direction))
            {
            }
            ImGui::ColorEdit3("Color", &light.color.x);
            ClampColor01(light.color);
            ImGui::SliderFloat(
                "Intensity",
                &light.intensity,
                0.0f,
                world::kMaxAuthoredLocalLightIntensity,
                "%.3f");
            light.intensity = world::ClampLocalLightIntensity(light.intensity);
            ImGui::SliderFloat(
                "Range",
                &light.range,
                world::kMinAuthoredLocalLightRange,
                world::kMaxAuthoredLocalLightRange,
                "%.3f");
            light.range = world::ClampLocalLightRange(light.range);
            ImGui::SliderFloat(
                "Inner Cone Angle",
                &light.innerConeDegrees,
                world::kMinSpotInnerConeDegrees,
                world::kMaxSpotOuterConeDegrees,
                "%.2f");
            ImGui::SliderFloat(
                "Outer Cone Angle",
                &light.outerConeDegrees,
                world::kMinSpotInnerConeDegrees,
                world::kMaxSpotOuterConeDegrees,
                "%.2f");
            world::ClampSpotConeAngles(light.innerConeDegrees, light.outerConeDegrees);
        }
        break;
    case EditorObjectKind::None:
    default:
        break;
    }

    ImGui::End();
}

#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
void DrawObjectPalette(LevelEditorState& state, const LevelEditorViewContext& view)
{
    ApplyEditorWindowPlacement(kObjectPaletteWindowName, view);
    if (!ImGui::Begin(kObjectPaletteWindowName, &state.workspace.showObjectPalette))
    {
        ImGui::End();
        return;
    }
    RecoverEditorWindowIfNeeded(kObjectPaletteWindowName, view);

    const bool authoringAvailable = IsLevelAuthoringAvailable();
    const bool gizmoDragging = state.gizmo.dragging;
    ImGui::TextUnformatted("Choose a category, then click in the viewport to place.");
    ImGui::TextWrapped("Click the same category again or press Esc to stop. Edit > Add still uses camera-region + spawn.z.");

    const auto paletteButton = [&](const char* label, PlacementMode mode, LevelEditorRequest addRequest) {
        const EditorObjectKind kind = KindFromPlacementMode(mode);
        const bool canAdd = CanIssueAuthoredLifecycleRequest(
            authoringAvailable, state.workingCopy, state.selection, gizmoDragging, addRequest);
        const bool selected = PaletteCategoryIsActive(state.placementMode, mode);
        ImGui::BeginDisabled(!canAdd);
        if (ImGui::Selectable(label, selected))
        {
            ApplyPaletteCategoryClick(state.placementMode, mode);
            CancelStaticPropPlacement(state.staticPropPlacement);
        }
        ImGui::EndDisabled();
        if (!canAdd)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("%s", CategoryCapacityReason(kind));
        }
    };

    paletteButton("Platform", PlacementMode::Platform, LevelEditorRequest::AddPlatform);
    paletteButton("Checkpoint", PlacementMode::Checkpoint, LevelEditorRequest::AddCheckpoint);
    paletteButton("Hazard", PlacementMode::Hazard, LevelEditorRequest::AddHazard);
    paletteButton("Collectible", PlacementMode::Collectible, LevelEditorRequest::AddCollectible);
    paletteButton("Dynamic Box", PlacementMode::DynamicBox, LevelEditorRequest::AddDynamicBox);
    paletteButton("Pressure Plate", PlacementMode::PressurePlate, LevelEditorRequest::AddPressurePlate);
    paletteButton("Door", PlacementMode::Door, LevelEditorRequest::AddDoor);
    paletteButton("Item Pickup", PlacementMode::ItemPickup, LevelEditorRequest::AddItemPickup);
    paletteButton("Level Goal", PlacementMode::Goal, LevelEditorRequest::AddGoal);
    paletteButton("Point Light", PlacementMode::PointLight, LevelEditorRequest::AddPointLight);
    paletteButton("Spot Light", PlacementMode::SpotLight, LevelEditorRequest::AddSpotLight);

    ImGui::Separator();
    if (StaticPropPlacementIsActive(state.staticPropPlacement))
    {
        ImGui::Text("Placement active: %s", StaticPropPlacementHudName());
        ImGui::TextUnformatted(PlacementStopHintText());
        ImGui::TextUnformatted(PlacementViewportActionHintText());
    }
    else if (PlacementModeIsActive(state.placementMode))
    {
        ImGui::Text("Placement active: %s", PlacementModeName(state.placementMode));
        ImGui::TextUnformatted(PlacementStopHintText());
        ImGui::TextUnformatted(PlacementViewportActionHintText());
    }
    else
    {
        ImGui::TextUnformatted("Placement inactive");
    }

    ImGui::End();
}

LevelEditorRequest DrawContentBrowser(
    LevelEditorState& state,
    const LevelEditorViewContext& view,
    EditorToolRunner& toolRunner,
    bool cookStageReloadPending)
{
    LevelEditorRequest request = LevelEditorRequest::None;
    ApplyEditorWindowPlacement(kContentBrowserWindowName, view);
    if (!ImGui::Begin(kContentBrowserWindowName, &state.workspace.showContentBrowser))
    {
        ImGui::End();
        return request;
    }
    RecoverEditorWindowIfNeeded(kContentBrowserWindowName, view);

    const bool authoringAvailable = IsLevelAuthoringAvailable();
    const bool toolsBusy = toolRunner.IsRunning() || cookStageReloadPending;
    const std::filesystem::path sourceRoot = AuthoringSourceRoot();

    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("(i)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    {
        ImGui::SetTooltip(
            "Development asset library.\n"
            "Content Browser selection is not a Level object.\n"
            "Logical folders and Favorites do not move source or cooked files "
            "and do not change runtime identity.\n"
            "Import Model copies a static GLB into models/<file>.glb.\n"
            "Import Texture copies a PNG into textures/<file>.png and reuses "
            "runtime_png.max512.lanczos.v1.");
    }
    ImGui::SameLine();
    char query[256];
    std::snprintf(query, sizeof(query), "%s", state.contentBrowser.filterQuery.c_str());
    if (ImGui::InputText("Search", query, sizeof(query)))
    {
        state.contentBrowser.filterQuery = query;
    }

    if (ImGui::Button("Refresh"))
    {
        RefreshContentBrowser(state.contentBrowser, sourceRoot);
        if (view.thumbnails != nullptr)
        {
            view.thumbnails->AllowRetryAll();
        }
        if (view.textureThumbnails != nullptr)
        {
            view.textureThumbnails->AllowRetryAll();
            view.textureThumbnails->Reconcile(state.contentBrowser.textureCatalog.Identities());
        }
        if (view.modelPreview != nullptr)
        {
            view.modelPreview->AllowRetry();
        }
        state.contentBrowser.statusMessage =
            "Catalogs refreshed from canonical source models and textures.";
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!authoringAvailable || toolsBusy);
    if (ImGui::Button("Import"))
    {
        ImGui::OpenPopup("content-browser-import-menu");
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    const bool canAddStaticProp = CanIssueAuthoredLifecycleRequest(
        authoringAvailable,
        state.workingCopy,
        state.selection,
        state.gizmo.dragging,
        ContentBrowserAddStaticPropRequest(),
        state.contentBrowser.selectedIdentity);
    ImGui::BeginDisabled(!canAddStaticProp);
    if (ImGui::Button(kContentBrowserAddStaticPropLabel))
    {
        request = ContentBrowserAddStaticPropRequest();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        const char* reason = AddStaticPropDisableReason(
            authoringAvailable,
            state.workingCopy,
            state.gizmo.dragging,
            state.contentBrowser.selectedIdentity);
        if (reason != nullptr)
        {
            ImGui::SetTooltip("%s", reason);
        }
        else
        {
            ImGui::SetTooltip(
                "Creates one Static Prop immediately at the camera-region default. "
                "Does not enter placement.");
        }
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!canAddStaticProp);
    if (ImGui::Button(kContentBrowserPlaceStaticPropLabel))
    {
        ApplyPlaceStaticPropClick(
            state.staticPropPlacement,
            state.placementMode,
            state.placementPointerBlocked,
            state.contentBrowser.selectedIdentity);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        const char* reason = AddStaticPropDisableReason(
            authoringAvailable,
            state.workingCopy,
            state.gizmo.dragging,
            state.contentBrowser.selectedIdentity);
        if (reason != nullptr)
        {
            ImGui::SetTooltip("%s", reason);
        }
        else if (StaticPropPlacementIsActive(state.staticPropPlacement)
            && state.staticPropPlacement.modelIdentity == state.contentBrowser.selectedIdentity)
        {
            ImGui::SetTooltip("Click again or press Esc to cancel Static Prop placement.");
        }
        else
        {
            ImGui::SetTooltip(
                "Enter viewport placement. Click a Ground, Platform, or Slope to create. "
                "Does not add until that click.");
        }
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    const bool hasSelection = !state.contentBrowser.selectedIdentity.empty();
    ImGui::BeginDisabled(!authoringAvailable || toolsBusy || !hasSelection);
    if (ImGui::Button("Delete Selected Asset"))
    {
        state.contentBrowser.deleteConfirmOpen = true;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!authoringAvailable || !hasSelection);
    const bool isFavorite = ContentBrowserIsFavorite(
        state.contentBrowser.organization, state.contentBrowser.selectedIdentity);
    if (ImGui::Button(isFavorite ? "Unfavorite" : "Favorite"))
    {
        std::string favoriteMessage;
        if (ContentBrowserOrganizationSucceeded(SetContentBrowserFavorite(
                state.contentBrowser.organization,
                state.contentBrowser.selectedIdentity,
                !isFavorite,
                favoriteMessage)))
        {
            PersistContentBrowserOrganization(state.contentBrowser, sourceRoot);
        }
        state.contentBrowser.statusMessage = favoriteMessage;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::TextUnformatted("View");
    ImGui::SameLine();
    if (ImGui::RadioButton(
            "Thumbnails", state.contentBrowser.viewMode == ContentBrowserViewMode::Thumbnails))
    {
        state.contentBrowser.viewMode = ContentBrowserViewMode::Thumbnails;
        PersistContentBrowserLayout(state);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("List", state.contentBrowser.viewMode == ContentBrowserViewMode::List))
    {
        state.contentBrowser.viewMode = ContentBrowserViewMode::List;
        PersistContentBrowserLayout(state);
    }

    if (hasSelection)
    {
        const std::string assigned = ContentBrowserAssignedFolder(
            state.contentBrowser.organization, state.contentBrowser.selectedIdentity);
        const char* movePreview = assigned.empty() ? "(unfiled)" : assigned.c_str();
        ImGui::SetNextItemWidth(220.0f);
        ImGui::BeginDisabled(!authoringAvailable);
        if (ImGui::BeginCombo("Move To", movePreview))
        {
            if (ImGui::Selectable("(unfiled)", assigned.empty()))
            {
                std::string moveMessage;
                if (ContentBrowserOrganizationSucceeded(MoveContentBrowserAsset(
                        state.contentBrowser.organization,
                        state.contentBrowser.selectedIdentity,
                        {},
                        moveMessage)))
                {
                    PersistContentBrowserOrganization(state.contentBrowser, sourceRoot);
                }
                state.contentBrowser.statusMessage = moveMessage;
            }
            for (const std::string& folder : state.contentBrowser.organization.folders)
            {
                if (ImGui::Selectable(folder.c_str(), assigned == folder))
                {
                    std::string moveMessage;
                    if (ContentBrowserOrganizationSucceeded(MoveContentBrowserAsset(
                            state.contentBrowser.organization,
                            state.contentBrowser.selectedIdentity,
                            folder,
                            moveMessage)))
                    {
                        PersistContentBrowserOrganization(state.contentBrowser, sourceRoot);
                    }
                    state.contentBrowser.statusMessage = moveMessage;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::EndDisabled();
    }

    const float statusBarHeight = ImGui::GetFrameHeightWithSpacing();
    const float paneHeight = std::max(120.0f, ImGui::GetContentRegionAvail().y - statusBarHeight);
    if (ImGui::BeginChild("content-browser-collections", ImVec2(220.0f, paneHeight), true))
    {
        const auto drawCollection = [&](ContentBrowserCollection collection, const char* label) {
            const bool selected = state.contentBrowser.collection == collection
                && (collection != ContentBrowserCollection::Folders
                    || state.contentBrowser.currentFolderPath.empty());
            if (ImGui::Selectable(label, selected))
            {
                ApplyContentBrowserCollection(state, collection);
            }
        };
        drawCollection(ContentBrowserCollection::AllAssets, "All Assets");
        drawCollection(ContentBrowserCollection::Favorites, "Favorites");
        drawCollection(ContentBrowserCollection::Models, "Models");
        drawCollection(ContentBrowserCollection::Textures, "Textures");
        ImGui::Spacing();
        ImGui::SeparatorText("Folders");
        ImGui::Indent(12.0f);
        if (ImGui::Selectable(
                "(unfiled)##folders-root",
                state.contentBrowser.collection == ContentBrowserCollection::Folders
                    && state.contentBrowser.currentFolderPath.empty()))
        {
            ApplyContentBrowserCollection(state, ContentBrowserCollection::Folders);
        }
        ImGui::Unindent(12.0f);
        for (const std::string& folder : state.contentBrowser.organization.folders)
        {
            int depth = 0;
            for (char ch : folder)
            {
                if (ch == '/')
                {
                    ++depth;
                }
            }
            const std::size_t slash = folder.rfind('/');
            const std::string label =
                slash == std::string::npos ? folder : folder.substr(slash + 1);
            ImGui::PushID(folder.c_str());
            ImGui::Indent(static_cast<float>(depth + 1) * 12.0f);
            const bool selected = state.contentBrowser.collection == ContentBrowserCollection::Folders
                && state.contentBrowser.currentFolderPath == folder;
            if (ImGui::Selectable(label.c_str(), selected))
            {
                ApplyContentBrowserCollection(state, ContentBrowserCollection::Folders, folder);
            }
            ImGui::Unindent(static_cast<float>(depth + 1) * 12.0f);
            ImGui::PopID();
        }
        ImGui::Spacing();
        ImGui::BeginDisabled(!authoringAvailable);
        if (ImGui::Button("Create Folder"))
        {
            state.contentBrowser.folderNameInput.clear();
            state.contentBrowser.folderNameDialogOpen = true;
            state.contentBrowser.folderRenameDialogOpen = false;
            state.contentBrowser.folderCreateSubfolder = false;
        }
        ImGui::BeginDisabled(state.contentBrowser.currentFolderPath.empty());
        if (ImGui::Button("Create Subfolder"))
        {
            state.contentBrowser.folderNameInput.clear();
            state.contentBrowser.folderNameDialogOpen = true;
            state.contentBrowser.folderRenameDialogOpen = false;
            state.contentBrowser.folderCreateSubfolder = true;
            state.contentBrowser.statusMessage.clear();
        }
        ImGui::EndDisabled();
        ImGui::BeginDisabled(state.contentBrowser.currentFolderPath.empty());
        if (ImGui::Button("Rename Folder"))
        {
            const std::string current = state.contentBrowser.currentFolderPath;
            const std::size_t slash = current.rfind('/');
            state.contentBrowser.folderNameInput =
                slash == std::string::npos ? current : current.substr(slash + 1);
            state.contentBrowser.folderRenameDialogOpen = true;
            state.contentBrowser.folderNameDialogOpen = false;
        }
        if (ImGui::Button("Delete Folder"))
        {
            state.contentBrowser.folderDeleteConfirmOpen = true;
        }
        ImGui::EndDisabled();
        ImGui::EndDisabled();
    }
    ImGui::EndChild();
    ImGui::SameLine();

    const std::vector<ContentBrowserAssetEntry> visible = QueryContentBrowserAssets(state.contentBrowser);
    const std::filesystem::path cacheRoot = ThumbnailCacheRoot();
    if (state.contentBrowser.viewMode == ContentBrowserViewMode::Thumbnails)
    {
        for (const ContentBrowserAssetEntry& entry : visible)
        {
            if (entry.kind == ContentBrowserAssetKind::Model && view.thumbnails != nullptr)
            {
                view.thumbnails->Ensure(
                    entry.canonicalIdentity, sourceRoot / entry.canonicalIdentity, cacheRoot);
            }
            else if (entry.kind == ContentBrowserAssetKind::Texture && view.textureThumbnails != nullptr)
            {
                view.textureThumbnails->Ensure(
                    entry.canonicalIdentity, sourceRoot / entry.canonicalIdentity);
            }
        }
        std::string thumbnailFailure;
        if (view.thumbnails != nullptr && view.thumbnails->ConsumeLastFailure(thumbnailFailure))
        {
            state.contentBrowser.statusMessage = thumbnailFailure;
        }
        if (view.textureThumbnails != nullptr
            && view.textureThumbnails->ConsumeLastFailure(thumbnailFailure))
        {
            state.contentBrowser.statusMessage = thumbnailFailure;
        }
    }

    const bool thumbnailView = state.contentBrowser.viewMode == ContentBrowserViewMode::Thumbnails;
    const ImGuiWindowFlags assetPaneFlags =
        thumbnailView ? ImGuiWindowFlags_AlwaysVerticalScrollbar : ImGuiWindowFlags_None;
    if (ImGui::BeginChild(
            "content-browser-assets-pane",
            ImVec2(0.0f, paneHeight),
            true,
            assetPaneFlags))
    {
        if (state.contentBrowser.catalog.Count() == 0 && state.contentBrowser.textureCatalog.Count() == 0)
        {
            ImGui::TextWrapped(
                "No Models or Textures are registered. Use Import > Import Model... or "
                "Import > Import Texture... . Import does not place the asset in the level.");
        }
        else if (visible.empty())
        {
            ImGui::TextWrapped("No assets match the current collection, folder, and search.");
        }
        else if (thumbnailView)
        {
            constexpr float kThumbSize = 96.0f;
            const ImGuiStyle& style = ImGui::GetStyle();
            const float columnSpacing = std::max(style.ItemSpacing.x, style.CellPadding.x * 2.0f);
            const int columns = ComputeContentBrowserThumbnailColumns(
                ImGui::GetContentRegionAvail().x,
                kThumbSize,
                columnSpacing);
            char tableId[48];
            std::snprintf(tableId, sizeof(tableId), "content-browser-thumbs/%d", columns);
            if (ImGui::BeginTable(
                    tableId,
                    columns,
                    ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings
                        | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoHostExtendX))
            {
                for (int columnIndex = 0; columnIndex < columns; ++columnIndex)
                {
                    char columnId[16];
                    std::snprintf(columnId, sizeof(columnId), "##c%d", columnIndex);
                    ImGui::TableSetupColumn(
                        columnId, ImGuiTableColumnFlags_WidthFixed, kThumbSize);
                }
                for (const ContentBrowserAssetEntry& entry : visible)
                {
                    ImGui::TableNextColumn();
                    ImGui::PushID(entry.canonicalIdentity.c_str());
                    const bool selected =
                        state.contentBrowser.selectedIdentity == entry.canonicalIdentity;
                    ImGui::BeginGroup();
                    if (ImGui::Selectable(
                            "##thumb",
                            selected,
                            ImGuiSelectableFlags_AllowOverlap,
                            ImVec2(kThumbSize, kThumbSize + ImGui::GetTextLineHeightWithSpacing())))
                    {
                        SelectContentBrowserIdentity(state.contentBrowser, entry.canonicalIdentity);
                        SyncStaticPropPlacementIdentityFromBrowser(
                            state.staticPropPlacement, state.contentBrowser.selectedIdentity);
                    }
                    const ImVec2 cellMin = ImGui::GetItemRectMin();
                    ImGui::SetCursorScreenPos(ImVec2(cellMin.x, cellMin.y));
                    unsigned int gpuId = 0;
                    int imageWidth = 0;
                    int imageHeight = 0;
                    bool failed = false;
                    if (entry.kind == ContentBrowserAssetKind::Texture
                        && view.textureThumbnails != nullptr)
                    {
                        gpuId = view.textureThumbnails->TextureGpuId(entry.canonicalIdentity);
                        imageWidth = view.textureThumbnails->TextureWidth(entry.canonicalIdentity);
                        imageHeight = view.textureThumbnails->TextureHeight(entry.canonicalIdentity);
                        failed = view.textureThumbnails->IsFailed(entry.canonicalIdentity);
                    }
                    else if (view.thumbnails != nullptr)
                    {
                        gpuId = view.thumbnails->TextureGpuId(entry.canonicalIdentity);
                        failed = view.thumbnails->IsFailed(entry.canonicalIdentity);
                        imageWidth = static_cast<int>(kThumbSize);
                        imageHeight = static_cast<int>(kThumbSize);
                    }
                    ImVec2 dummyMin = ImGui::GetCursorScreenPos();
                    ImGui::Dummy(ImVec2(kThumbSize, kThumbSize));
                    ImVec2 dummyMax = ImGui::GetItemRectMax();
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        dummyMin,
                        dummyMax,
                        failed ? IM_COL32(88, 64, 64, 255) : IM_COL32(48, 52, 62, 255));
                    ImGui::GetWindowDrawList()->AddRect(
                        dummyMin, dummyMax, IM_COL32(120, 126, 140, 255));
                    if (gpuId != 0)
                    {
                        float drawWidth = kThumbSize;
                        float drawHeight = kThumbSize;
                        ComputeTextureThumbnailDrawSize(
                            imageWidth, imageHeight, kThumbSize - 4.0f, drawWidth, drawHeight);
                        const float offsetX = dummyMin.x + (kThumbSize - drawWidth) * 0.5f;
                        const float offsetY = dummyMin.y + (kThumbSize - drawHeight) * 0.5f;
                        ImGui::SetCursorScreenPos(ImVec2(offsetX, offsetY));
                        ImGui::Image(
                            ImTextureRef(static_cast<ImTextureID>(static_cast<intptr_t>(gpuId))),
                            ImVec2(drawWidth, drawHeight));
                    }
                    ImGui::SetCursorScreenPos(ImVec2(dummyMin.x, dummyMax.y));
                    ImGui::PushTextWrapPos(dummyMin.x + kThumbSize);
                    ImGui::TextUnformatted(entry.displayName.c_str());
                    ImGui::PopTextWrapPos();
                    ImGui::EndGroup();
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip(
                            "%s\n%s\n%s\n%s%s",
                            entry.displayName.c_str(),
                            ContentBrowserAssetKindName(entry.kind),
                            entry.canonicalIdentity.c_str(),
                            entry.logicalFolder.empty() ? "(unfiled)" : entry.logicalFolder.c_str(),
                            entry.favorite ? "\nFavorite" : "");
                    }
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
        }
        else if (ImGui::BeginTable(
                     "content-browser-assets",
                     4,
                     ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg
                         | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY
                         | ImGuiTableFlags_NoSavedSettings,
                     ImVec2(0.0f, ImGui::GetContentRegionAvail().y)))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Kind");
            ImGui::TableSetupColumn("Identity");
            ImGui::TableSetupColumn("Folder");
            ImGui::TableHeadersRow();
            for (const ContentBrowserAssetEntry& entry : visible)
            {
                ImGui::PushID(entry.canonicalIdentity.c_str());
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                const bool selected = state.contentBrowser.selectedIdentity == entry.canonicalIdentity;
                if (ImGui::Selectable(
                        entry.displayName.c_str(),
                        selected,
                        ImGuiSelectableFlags_SpanAllColumns))
                {
                    SelectContentBrowserIdentity(state.contentBrowser, entry.canonicalIdentity);
                    SyncStaticPropPlacementIdentityFromBrowser(
                        state.staticPropPlacement, state.contentBrowser.selectedIdentity);
                }
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(ContentBrowserAssetKindName(entry.kind));
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(entry.canonicalIdentity.c_str());
                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(
                    entry.logicalFolder.empty() ? "(unfiled)" : entry.logicalFolder.c_str());
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }
    ImGui::EndChild();

    {
        std::string displayName;
        std::string kindName;
        if (hasSelection)
        {
            kindName = ContentBrowserAssetKindName(
                ClassifyContentBrowserIdentity(state.contentBrowser.selectedIdentity));
            if (const assets::StaticModelCatalogEntry* model =
                    state.contentBrowser.catalog.Find(state.contentBrowser.selectedIdentity))
            {
                displayName = model->displayName;
            }
            else if (
                const assets::SourceTextureCatalogEntry* texture =
                    state.contentBrowser.textureCatalog.Find(state.contentBrowser.selectedIdentity))
            {
                displayName = texture->displayName;
            }
            else
            {
                displayName = state.contentBrowser.selectedIdentity;
            }
        }
        std::string statusLine;
        if (StaticPropPlacementIsActive(state.staticPropPlacement)
            && state.contentBrowser.statusMessage.empty())
        {
            statusLine = "Placing Static Prop | ";
            statusLine += state.staticPropPlacement.modelIdentity;
        }
        else
        {
            statusLine = MakeContentBrowserStatusBarText(
                state.contentBrowser.statusMessage, kindName, displayName, hasSelection);
        }
        ImGui::Separator();
        ImGui::TextUnformatted(statusLine.c_str());
    }

    if (ImGui::BeginPopup("content-browser-import-menu"))
    {
        if (ImGui::MenuItem("Import Model..."))
        {
            request = ContentBrowserImportRequest();
        }
        if (ImGui::MenuItem("Import Texture..."))
        {
            request = ContentBrowserImportTextureRequest();
        }
        ImGui::EndPopup();
    }

    if (state.contentBrowser.deleteConfirmOpen)
    {
        ImGui::OpenPopup("Delete Content Browser Asset");
    }
    if (ImGui::BeginPopupModal(
            "Delete Content Browser Asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        const bool deletingTexture =
            ContentBrowserIdentityIsTexture(state.contentBrowser.selectedIdentity);
        ImGui::TextUnformatted(
            deletingTexture ? "Delete this registered texture?"
                            : "Delete this registered static model?");
        ImGui::TextUnformatted(state.contentBrowser.selectedIdentity.c_str());
        ImGui::TextWrapped(
            "This removes the canonical source file and any matching cooked/staged copies. "
            "It does not edit the level. Logical folder membership is editor organization only. "
            "Cancel makes no filesystem changes.");
        if (ImGui::Button("Delete"))
        {
            if (ContentBrowserDeleteConfirmed(true))
            {
                request = LevelEditorRequest::DeleteContentBrowserAsset;
            }
            state.contentBrowser.deleteConfirmOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            (void)ContentBrowserDeleteConfirmed(false);
            state.contentBrowser.deleteConfirmOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (state.contentBrowser.folderNameDialogOpen)
    {
        ImGui::OpenPopup("Content Browser Folder Name");
    }
    if (ImGui::BeginPopupModal(
            "Content Browser Folder Name", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        const bool creatingSubfolder = state.contentBrowser.folderCreateSubfolder;
        ImGui::TextUnformatted(creatingSubfolder ? "Subfolder name" : "Folder name");
        char nameBuffer[80];
        std::snprintf(
            nameBuffer, sizeof(nameBuffer), "%s", state.contentBrowser.folderNameInput.c_str());
        if (ImGui::InputText("##folder-name", nameBuffer, sizeof(nameBuffer)))
        {
            state.contentBrowser.folderNameInput = nameBuffer;
        }
        if (ImGui::Button("Create"))
        {
            std::string createdPath;
            std::string createMessage;
            const std::string parent = creatingSubfolder ? state.contentBrowser.currentFolderPath
                                                         : std::string();
            if (ContentBrowserOrganizationSucceeded(CreateContentBrowserFolder(
                    state.contentBrowser.organization,
                    parent,
                    state.contentBrowser.folderNameInput,
                    createdPath,
                    createMessage)))
            {
                PersistContentBrowserOrganization(state.contentBrowser, sourceRoot);
                ApplyContentBrowserCollection(
                    state, ContentBrowserCollection::Folders, createdPath);
            }
            state.contentBrowser.statusMessage = createMessage;
            state.contentBrowser.folderNameDialogOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            state.contentBrowser.folderNameDialogOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (state.contentBrowser.folderRenameDialogOpen)
    {
        ImGui::OpenPopup("Rename Content Browser Folder");
    }
    if (ImGui::BeginPopupModal(
            "Rename Content Browser Folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        char nameBuffer[80];
        std::snprintf(
            nameBuffer, sizeof(nameBuffer), "%s", state.contentBrowser.folderNameInput.c_str());
        if (ImGui::InputText("##rename-folder", nameBuffer, sizeof(nameBuffer)))
        {
            state.contentBrowser.folderNameInput = nameBuffer;
        }
        if (ImGui::Button("Rename"))
        {
            std::string renamedPath;
            std::string renameMessage;
            if (ContentBrowserOrganizationSucceeded(RenameContentBrowserFolder(
                    state.contentBrowser.organization,
                    state.contentBrowser.currentFolderPath,
                    state.contentBrowser.folderNameInput,
                    renamedPath,
                    renameMessage)))
            {
                PersistContentBrowserOrganization(state.contentBrowser, sourceRoot);
                ApplyContentBrowserCollection(
                    state, ContentBrowserCollection::Folders, renamedPath);
            }
            state.contentBrowser.statusMessage = renameMessage;
            state.contentBrowser.folderRenameDialogOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            state.contentBrowser.folderRenameDialogOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (state.contentBrowser.folderDeleteConfirmOpen)
    {
        ImGui::OpenPopup("Delete Logical Folder");
    }
    if (ImGui::BeginPopupModal("Delete Logical Folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("Delete this logical folder?");
        ImGui::TextUnformatted(state.contentBrowser.currentFolderPath.c_str());
        ImGui::TextWrapped(
            "Physical Models and Textures are never deleted. Contained assets and subfolders "
            "are reparented to the parent folder or (unfiled).");
        if (ImGui::Button("Delete Folder"))
        {
            std::string deleteMessage;
            const std::string parent =
                ContentBrowserFolderParentPath(state.contentBrowser.currentFolderPath);
            if (ContentBrowserOrganizationSucceeded(DeleteContentBrowserFolder(
                    state.contentBrowser.organization,
                    state.contentBrowser.currentFolderPath,
                    deleteMessage)))
            {
                PersistContentBrowserOrganization(state.contentBrowser, sourceRoot);
                ApplyContentBrowserCollection(state, ContentBrowserCollection::Folders, parent);
            }
            state.contentBrowser.statusMessage = deleteMessage;
            state.contentBrowser.folderDeleteConfirmOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            state.contentBrowser.folderDeleteConfirmOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
    return request;
}

LevelEditorRequest DrawLevelsBrowser(
    LevelEditorState& state,
    const world::LevelDefinition& activeLevel,
    const LevelEditorViewContext& view)
{
    LevelEditorRequest request = LevelEditorRequest::None;
    ApplyEditorWindowPlacement(kLevelsWindowName, view);
    if (!ImGui::Begin(kLevelsWindowName, &state.workspace.showLevels))
    {
        ImGui::End();
        return request;
    }
    RecoverEditorWindowIfNeeded(kLevelsWindowName, view);

    const bool authoringAvailable = IsLevelAuthoringAvailable();
    const std::filesystem::path sourceRoot = AuthoringSourceRoot();
    ImGui::TextUnformatted("Authored source Levels. Discovery is Development-only.");
    ImGui::Text("Current: %s", activeLevel.id.empty() ? "(none)" : activeLevel.id.c_str());
    ImGui::TextWrapped(
        "Open loads the authored source file into the editor runtime. Gameplay and Release still "
        "load staged files only. Refresh does not scan every frame.");

    ImGui::BeginDisabled(!authoringAvailable);
    if (ImGui::Button("Refresh"))
    {
        state.authoredLevels.Refresh(sourceRoot);
        state.authoredLevelsStatus = "Authored Levels refreshed from the source directory.";
    }
    ImGui::SameLine();
    if (ImGui::Button("New Level"))
    {
        if (state.newLevelIdInput.empty())
        {
            state.newLevelIdInput = "level_";
        }
        ImGui::OpenPopup("New Level");
    }
    ImGui::EndDisabled();

    if (ImGui::BeginPopupModal("New Level", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("Logical Level ID. Example: level_03");
        char idBuffer[64]{};
        std::snprintf(idBuffer, sizeof(idBuffer), "%s", state.newLevelIdInput.c_str());
        if (ImGui::InputText("Level ID", idBuffer, sizeof(idBuffer)))
        {
            state.newLevelIdInput = idBuffer;
        }
        if (ImGui::Button("Create"))
        {
            const std::string requestedId = state.newLevelIdInput;
            if (!world::IsValidLevelIdToken(requestedId))
            {
                state.authoredLevelsStatus =
                    "Rejected: use a safe logical ID such as level_03. No file was written.";
            }
            else
            {
                state.pendingAuthoredLevelAction = AuthoredLevelPendingAction::Create;
                state.pendingAuthoredLevelId = requestedId;
                if (AuthoredLevelSwitchNeedsDiscard(state.modified, state.dirty))
                {
                    state.authoredLevelDiscardGuardOpen = true;
                }
                else
                {
                    request = LevelEditorRequest::CreateAuthoredLevel;
                }
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::BeginChild("AuthoredLevelList", ImVec2(0.0f, 88.0f), true);
    if (!authoringAvailable)
    {
        ImGui::TextUnformatted("Authoring is unavailable in this configuration.");
    }
    else if (state.authoredLevels.Count() == 0)
    {
        ImGui::TextUnformatted("No valid authored Levels discovered.");
    }
    else
    {
        for (const AuthoredLevelEntry& entry : state.authoredLevels.Entries())
        {
            const bool isCurrent = entry.id == activeLevel.id;
            char label[96]{};
            if (isCurrent)
            {
                std::snprintf(label, sizeof(label), "%s  (current)", entry.id.c_str());
            }
            else
            {
                std::snprintf(label, sizeof(label), "%s", entry.id.c_str());
            }
            if (ImGui::Selectable(label, isCurrent))
            {
                state.pendingAuthoredLevelAction = AuthoredLevelPendingAction::Open;
                state.pendingAuthoredLevelId = entry.id;
                if (AuthoredLevelSwitchNeedsDiscard(state.modified, state.dirty))
                {
                    state.authoredLevelDiscardGuardOpen = true;
                }
                else
                {
                    request = LevelEditorRequest::OpenAuthoredLevel;
                }
            }
        }
    }
    ImGui::EndChild();

    ImGui::BeginDisabled(
        !authoringAvailable || state.pendingAuthoredLevelId.empty()
            || state.pendingAuthoredLevelAction != AuthoredLevelPendingAction::Open);
    if (ImGui::Button("Open"))
    {
        if (AuthoredLevelSwitchNeedsDiscard(state.modified, state.dirty))
        {
            state.authoredLevelDiscardGuardOpen = true;
        }
        else
        {
            request = LevelEditorRequest::OpenAuthoredLevel;
        }
    }
    ImGui::EndDisabled();

    if (!state.authoredLevelsStatus.empty())
    {
        ImGui::TextWrapped("%s", state.authoredLevelsStatus.c_str());
    }

    if (state.authoredLevelDiscardGuardOpen)
    {
        ImGui::OpenPopup("Discard Pending Level Edits?");
    }
    if (ImGui::BeginPopupModal(
            "Discard Pending Level Edits?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted(
            "Pending working-copy or unsaved applied edits will be discarded.");
        ImGui::TextWrapped(
            "Cancel leaves the current Level, working copy, pending edits, and Dirty state "
            "unchanged.");
        const char* discardLabel =
            state.pendingAuthoredLevelAction == AuthoredLevelPendingAction::Create
            ? "Discard and Create"
            : "Discard and Open";
        if (ImGui::Button(discardLabel))
        {
            request = state.pendingAuthoredLevelAction == AuthoredLevelPendingAction::Create
                ? LevelEditorRequest::CreateAuthoredLevel
                : LevelEditorRequest::OpenAuthoredLevel;
            state.authoredLevelDiscardGuardOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            state.pendingAuthoredLevelAction = AuthoredLevelPendingAction::None;
            state.pendingAuthoredLevelId.clear();
            state.authoredLevelDiscardGuardOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
    return request;
}

void DrawModelPreview(LevelEditorState& state, const LevelEditorViewContext& view)
{
    ApplyEditorWindowPlacement(kModelPreviewWindowName, view);
    if (!ImGui::Begin(kModelPreviewWindowName, &state.workspace.showModelPreview))
    {
        ImGui::End();
        return;
    }
    RecoverEditorWindowIfNeeded(kModelPreviewWindowName, view);

    ImGui::TextUnformatted("Interactive view of the Content Browser selection. Not a level object.");
    render::StaticModelPreviewRenderer* preview = view.modelPreview;
    const std::string& identity = state.contentBrowser.selectedIdentity;
    const bool selectedModel = !identity.empty() && ContentBrowserIdentityIsModel(identity);
    const bool selectedTexture = !identity.empty() && ContentBrowserIdentityIsTexture(identity);
    if (preview != nullptr && preview->HasModel()
        && state.modelPreviewFramedIdentity != preview->LoadedIdentity())
    {
        ResetStaticModelPreviewOrbit(state.modelPreviewOrbit, preview->Bounds());
        state.modelPreviewFramedIdentity = preview->LoadedIdentity();
    }
    else if (preview != nullptr && preview->IsFailed())
    {
        state.modelPreviewFramedIdentity = preview->LoadedIdentity();
    }
    else if (preview == nullptr || (!preview->HasModel() && !preview->IsFailed()))
    {
        state.modelPreviewFramedIdentity.clear();
    }

    const bool canFrame = selectedModel && preview != nullptr && preview->HasModel();
    ImGui::BeginDisabled(!canFrame);
    if (ImGui::Button("Reset View"))
    {
        if (canFrame)
        {
            ResetStaticModelPreviewOrbit(state.modelPreviewOrbit, preview->Bounds());
        }
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::TextDisabled("LMB orbit  |  Wheel zoom");

    if (identity.empty())
    {
        ImGui::Spacing();
        ImGui::TextWrapped("No asset is selected. Choose a Model or Texture in the Content Browser.");
    }
    else if (selectedTexture)
    {
        ImGui::Spacing();
        ImGui::TextWrapped(
            "Texture selected. The Content Browser thumbnail is the preview. "
            "This is not a Static Model.");
    }
    else if (preview != nullptr && preview->IsFailed())
    {
        ImGui::TextWrapped("Preview failed to load this model. Refresh or reselect to retry.");
    }
    else if (selectedModel)
    {
        // Reserve one collapsed header row so details stay below the canvas
        // without permanently shrinking the Preview for the expanded body.
        const float detailsReserve = ImGui::GetFrameHeightWithSpacing();
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const float previewHeight = avail.y - detailsReserve;
        const editor::PreviewRenderSize size = ResolvePreviewRenderSize(avail.x, previewHeight);
        if (size.valid)
        {
            const ImVec2 canvas(static_cast<float>(size.width), static_cast<float>(size.height));
            const ImVec2 screen = ImGui::GetCursorScreenPos();
            unsigned int gpuId = 0;
            if (preview != nullptr
                && preview->Render(size.width, size.height, state.modelPreviewOrbit))
            {
                gpuId = preview->TextureGpuId();
            }
            if (gpuId != 0)
            {
                ImGui::Image(
                    ImTextureRef(static_cast<ImTextureID>(static_cast<intptr_t>(gpuId))),
                    canvas,
                    ImVec2(0.0f, 1.0f),
                    ImVec2(1.0f, 0.0f));
            }
            else
            {
                ImGui::Dummy(canvas);
                ImGui::GetWindowDrawList()->AddRectFilled(
                    screen,
                    ImVec2(screen.x + canvas.x, screen.y + canvas.y),
                    IM_COL32(48, 52, 62, 255));
            }
            ImGui::SetCursorScreenPos(screen);
            ImGui::InvisibleButton("model-preview-orbit", canvas);
            const bool hovered = ImGui::IsItemHovered();
            const bool held = ImGui::IsItemActive();
            const ImGuiIO& io = ImGui::GetIO();
            if (held && preview != nullptr && preview->HasModel())
            {
                ApplyStaticModelPreviewOrbit(
                    state.modelPreviewOrbit, io.MouseDelta.x, io.MouseDelta.y);
            }
            if (hovered && preview != nullptr && preview->HasModel())
            {
                ApplyStaticModelPreviewDolly(state.modelPreviewOrbit, io.MouseWheel);
            }
        }
    }

    // Collapsed by default (no DefaultOpen). Open state may persist with the
    // existing ImGui ini; no dedicated preferences file.
    if (ImGui::CollapsingHeader("Asset Details"))
    {
        DrawContentBrowserAssetDetails(state);
        if (selectedModel && preview != nullptr && preview->HasModel())
        {
            const editor::ThumbnailModelBounds bounds = preview->Bounds();
            ImGui::Text(
                "Bounds: %.3f %.3f %.3f  ..  %.3f %.3f %.3f",
                bounds.min.x,
                bounds.min.y,
                bounds.min.z,
                bounds.max.x,
                bounds.max.y,
                bounds.max.z);
        }
    }

    ImGui::End();
}
#endif

LevelEditorRequest DrawLevelControls(
    LevelEditorState& state,
    const world::LevelDefinition& activeLevel,
    const LevelEditorViewContext& view,
    EditorToolRunner& toolRunner,
    bool cookStageReloadPending)
{
    LevelEditorRequest request = LevelEditorRequest::None;
    ApplyEditorWindowPlacement(kLevelEditorWindowName, view);
#if !defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
    (void)toolRunner;
    (void)cookStageReloadPending;
#endif
    if (!ImGui::Begin(kLevelEditorWindowName, &state.workspace.showLevelEditor))
    {
        ImGui::End();
        return request;
    }
    RecoverEditorWindowIfNeeded(kLevelEditorWindowName, view);

    const bool authoringAvailable = IsLevelAuthoringAvailable();
    const bool workingCopyValid = world::IsWritableLevelDefinition(state.workingCopy);

    if (ImGui::CollapsingHeader("Level", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("ID: %s", activeLevel.id.c_str());
        ImGui::Text("Editor: %s", state.active ? "Active (simulation paused)" : "Inactive");
        ImGui::Text("Modified (unapplied edits): %s", BoolText(state.modified));
        ImGui::Text("Dirty (applied but unsaved): %s", BoolText(state.dirty));
        ImGui::Text("Authoring: %s", authoringAvailable ? "Available" : "Unavailable");
        if (state.lastApplyStatus != LevelEditorApplyStatus::NotAttempted)
        {
            ImGui::Text("Last Apply: %s", LevelEditorApplyStatusName(state.lastApplyStatus));
        }
        if (state.lastSaveStatus != LevelEditorSaveStatus::NotAttempted)
        {
            ImGui::Text("Last Save: %s", LevelEditorSaveStatusName(state.lastSaveStatus));
        }
        if (state.lastReloadStatus != LevelEditorReloadStatus::NotAttempted)
        {
            ImGui::Text("Last Reload: %s", LevelEditorReloadStatusName(state.lastReloadStatus));
        }
        if (!state.lastMessage.empty())
        {
            ImGui::TextWrapped("Detail: %s", state.lastMessage.c_str());
        }

        if (authoringAvailable)
        {
            const std::string sourcePath = AuthoringLevelSourcePath(activeLevel.id).string();
            ImGui::TextWrapped("Source (read-only): %s", sourcePath.c_str());
        }
        else
        {
            ImGui::TextUnformatted(
                "Source (read-only): (authoring disabled in this configuration)");
        }
        ImGui::TextWrapped("Runtime staged (read-only): %s", view.runtimeLevelPath);
    }

    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::BeginDisabled(state.gizmo.dragging);
        if (ImGui::RadioButton(
                "Translate", state.transformMode == EditorTransformMode::Translate))
        {
            editor::TrySetEditorTransformMode(
                state.transformMode, state.gizmo.dragging, EditorTransformMode::Translate);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Resize", state.transformMode == EditorTransformMode::Resize))
        {
            editor::TrySetEditorTransformMode(
                state.transformMode, state.gizmo.dragging, EditorTransformMode::Resize);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", state.transformMode == EditorTransformMode::Scale))
        {
            editor::TrySetEditorTransformMode(
                state.transformMode, state.gizmo.dragging, EditorTransformMode::Scale);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", state.transformMode == EditorTransformMode::Rotate))
        {
            editor::TrySetEditorTransformMode(
                state.transformMode, state.gizmo.dragging, EditorTransformMode::Rotate);
        }
        ImGui::EndDisabled();
        if (state.gizmo.dragging)
        {
            ImGui::TextUnformatted("Mode locked while a gizmo drag is active.");
        }
        DrawEditorSnapControls(state, false, "##SnapIncrementPanel");
        ImGui::TextWrapped(
            "Hold Ctrl while dragging a gizmo to invert Snap. The Snap toggle is not changed.");
        ImGui::TextWrapped(
            "Ctrl+click adds or removes authored objects. Ordinary click replaces selection.");
        ImGui::TextWrapped(
            "Grid visibility is independent of Snap. Minor spacing follows Translate increment.");
        if (EditorSelectionSetIsMulti(state.selection, state.additionalSelections)
            && state.transformMode == EditorTransformMode::Translate)
        {
            const char* reason = GroupTranslateDisableReason(
                state.workingCopy, state.selection, state.additionalSelections);
            if (reason != nullptr)
            {
                ImGui::TextUnformatted(reason);
            }
        }
        if (EditorSelectionSetIsMulti(state.selection, state.additionalSelections)
            && state.transformMode == EditorTransformMode::Rotate)
        {
            const char* reason = GroupRotateDisableReason(
                state.workingCopy, state.selection, state.additionalSelections);
            if (reason != nullptr)
            {
                ImGui::TextUnformatted(reason);
            }
        }
        if (EditorSelectionSetIsMulti(state.selection, state.additionalSelections))
        {
            const char* groupReason = MultiSelectionTransformDisableReason(state.transformMode);
            if (groupReason != nullptr)
            {
                ImGui::TextUnformatted(groupReason);
            }
        }
        if (state.transformMode == EditorTransformMode::Resize
            && state.selection.kind != EditorObjectKind::None
            && !IsResizeSelection(state.selection))
        {
            ImGui::TextUnformatted("Selected object is not resizable");
        }
        if (state.transformMode == EditorTransformMode::Scale
            && state.selection.kind != EditorObjectKind::None
            && !IsScaleSelection(state.selection))
        {
            ImGui::TextUnformatted("Selected object is not scalable");
        }
        if (state.transformMode == EditorTransformMode::Rotate
            && !EditorSelectionSetIsMulti(state.selection, state.additionalSelections)
            && state.selection.kind != EditorObjectKind::None
            && !IsRotateSelection(state.selection))
        {
            ImGui::TextUnformatted("Selected object is not rotatable");
        }
        ImGui::TextWrapped(
            "Nudge (Translate mode): Ctrl+Arrows/PageUp/PageDown. Precision: Ctrl+Shift.");
        ImGui::TextWrapped("Dolly: Alt+mouse wheel. Ordinary wheel still changes nav speed.");
    }

    if (ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::BeginDisabled(!editor::CanApplyPreview(state.modified, workingCopyValid));
        if (ImGui::Button("Apply Preview"))
        {
            request = LevelEditorRequest::ApplyPreview;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::BeginDisabled(!editor::CanRevertWorkingCopy(state.modified));
        if (ImGui::Button("Revert Working Copy"))
        {
            request = LevelEditorRequest::RevertWorkingCopy;
        }
        ImGui::EndDisabled();

        if (state.modified && !workingCopyValid)
        {
            ImGui::TextUnformatted(
                "Apply disabled: working copy fails validation (size <= 0, FOV out of range,"
                " or non-finite value).");
        }

        // Save always writes the active/applied definition, so unapplied edits
        // cannot reach the source file.
        ImGui::BeginDisabled(!editor::CanSaveLevelSource(authoringAvailable, state.modified));
        if (ImGui::Button("Save Level Source"))
        {
            request = LevelEditorRequest::SaveLevelSource;
        }
        ImGui::EndDisabled();
        if (!authoringAvailable)
        {
            ImGui::TextUnformatted("Save unavailable: this configuration cannot author source.");
        }
        else if (state.modified)
        {
            ImGui::TextUnformatted("Save disabled: Apply Preview first.");
        }

#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
        const bool toolRunning = toolRunner.IsRunning();
        ImGui::BeginDisabled(
            !editor::CanReloadRuntimeLevel(
                authoringAvailable, state.modified, toolRunning, cookStageReloadPending));
        if (ImGui::Button("Reload Runtime Level"))
        {
            request = LevelEditorRequest::ReloadRuntimeLevel;
        }
        ImGui::EndDisabled();
        if (state.modified)
        {
            ImGui::TextUnformatted(
                "Reload disabled: Apply Preview or Revert Working Copy first.");
        }
        else if (toolRunning)
        {
            ImGui::TextUnformatted(
                "Reload disabled: wait for the running Build tool to finish.");
        }
        else if (cookStageReloadPending)
        {
            ImGui::TextUnformatted(
                "Reload disabled: Cook, Stage & Reload is still finishing.");
        }
#endif

        if (ImGui::Button("Reset Editor Layout"))
        {
            ResetEditorWorkspaceLayout(state, view.viewportWidth, view.viewportHeight);
        }
        ImGui::TextWrapped(
            "Reset Editor Layout restores Metrics, Hierarchy, Inspector, Level Editor, "
            "Object Palette, Tool Output, and Quick Toolbar visibility and window positions. "
            "It does not change the level, selection, camera, or last build configuration.");
    }

    if (ImGui::CollapsingHeader("Information", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextWrapped(
            "RMB look, WASD move, Q/E down/up, Shift faster, wheel speed, Alt+wheel dolly. "
            "Translate: LMB on an X/Y/Z handle moves the working copy. "
            "Resize: LMB on a cube handle changes authored primitive size; the cyan ghost is the true size. "
            "Scale: LMB on a cube handle changes Static Prop or Item Pickup visual scale (not primitive Resize).");
        ImGui::TextWrapped(
            "The gizmo and pending ghost follow unapplied working-copy edits. "
            "Active render, physics, picking and highlight stay put until Apply Preview.");
        ImGui::TextWrapped("Save updates the project source only. It does not recook or rebuild.");
#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
        ImGui::TextWrapped(
            "Reload Runtime Level re-reads the staged runtime file in-process. "
            "It does not Save, Cook, Stage, Build, or restart.");
        ImGui::TextWrapped(
            "After Save, use Build > Cook, Stage & Reload (or Cook & Stage then "
            "Level > Reload Runtime Level). Staging does not reload in-memory assets by itself.");
#endif
        ImGui::TextWrapped(
            "Applied but unsaved edits live in memory only and are lost when the process exits.");
        ImGui::TextWrapped(
            "Closing and reopening the editor discards unapplied working-copy edits.");
        ImGui::TextWrapped(
            "Editable: spawn, gameplay camera offset/FOV, ground and elevated platform"
            " center/size. Other hierarchy objects are read-only in M33.");
    }

    ImGui::End();
    return request;
}
}

void ResetEditorWorkspaceLayout(
    LevelEditorState& state,
    float viewportWidth,
    float viewportHeight)
{
    ResetEditorWorkspaceVisibility(state.workspace);
    state.contentBrowser.viewMode = kDefaultContentBrowserViewMode;
    state.contentBrowser.collection = kDefaultContentBrowserCollection;
    state.contentBrowser.currentFolderPath.clear();
    PersistContentBrowserViewState(state.contentBrowser);
    SnapKnownEditorWindowsToDefaults(viewportWidth, viewportHeight);
    state.forceDefaultLayoutFrames = 2;
}

void RequestEditorToolStart(
    EditorToolRunner& toolRunner,
    EditorWorkspaceState& workspace,
    EditorToolKind kind)
{
    if (toolRunner.TryStart(kind, RepositoryRoot(), IsEditorToolExecutionAvailable()))
    {
        workspace.showToolOutput = true;
    }
}

LevelEditorRequest DrawEditorMenuBar(
    LevelEditorState& state,
    const world::LevelDefinition&,
    const LevelEditorViewContext& view,
    EditorToolRunner& toolRunner,
    bool cookStageReloadPending)
{
    LevelEditorRequest request = LevelEditorRequest::None;
#if !defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
    (void)cookStageReloadPending;
#endif
    if (!ImGui::BeginMainMenuBar())
    {
        return request;
    }

    state.menuBarHeight = ImGui::GetFrameHeight();

    if (ImGui::BeginMenu("View"))
    {
        ImGui::MenuItem("Metrics", "F1", &state.workspace.showMetrics);
        ImGui::MenuItem("Hierarchy", nullptr, &state.workspace.showHierarchy);
        ImGui::MenuItem("Inspector", nullptr, &state.workspace.showInspector);
        ImGui::MenuItem("Level Editor", nullptr, &state.workspace.showLevelEditor);
#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
        ImGui::MenuItem("Object Palette", nullptr, &state.workspace.showObjectPalette);
        ImGui::MenuItem("Levels", nullptr, &state.workspace.showLevels);
        ImGui::MenuItem("Content Browser", nullptr, &state.workspace.showContentBrowser);
        ImGui::MenuItem("Model Preview", nullptr, &state.workspace.showModelPreview);
        ImGui::MenuItem("Quick Toolbar", nullptr, &state.workspace.showQuickToolbar);
#endif
#if defined(PLATFORMER_ENABLE_EDITOR_TOOLS)
        ImGui::MenuItem("Tool Output", nullptr, &state.workspace.showToolOutput);
#endif
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Transform"))
    {
        ImGui::BeginDisabled(state.gizmo.dragging);
        if (ImGui::MenuItem(
                "Translate",
                nullptr,
                state.transformMode == EditorTransformMode::Translate))
        {
            TrySetEditorTransformMode(
                state.transformMode, state.gizmo.dragging, EditorTransformMode::Translate);
        }
        if (ImGui::MenuItem(
                "Resize",
                nullptr,
                state.transformMode == EditorTransformMode::Resize))
        {
            TrySetEditorTransformMode(
                state.transformMode, state.gizmo.dragging, EditorTransformMode::Resize);
        }
        if (ImGui::MenuItem(
                "Scale",
                nullptr,
                state.transformMode == EditorTransformMode::Scale))
        {
            TrySetEditorTransformMode(
                state.transformMode, state.gizmo.dragging, EditorTransformMode::Scale);
        }
        if (ImGui::MenuItem(
                "Rotate",
                nullptr,
                state.transformMode == EditorTransformMode::Rotate))
        {
            TrySetEditorTransformMode(
                state.transformMode, state.gizmo.dragging, EditorTransformMode::Rotate);
        }
        ImGui::EndDisabled();
        if (ImGui::MenuItem("Snap", "Ctrl invert", &state.snap.enabled))
        {
            PersistEditorSnapPreferences(state.snap);
        }
        if (ImGui::MenuItem("Grid", nullptr, &state.viewportGrid.visible))
        {
            PersistEditorViewportGridPreferences(state.viewportGrid);
        }
        ImGui::EndMenu();
    }

#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
    if (ImGui::BeginMenu("Edit"))
    {
        const bool authoringAvailable = IsLevelAuthoringAvailable();
        const bool gizmoDragging = state.gizmo.dragging;
        if (ImGui::BeginMenu("Add"))
        {
            ImGui::BeginDisabled(
                !CanIssueAuthoredLifecycleRequest(
                    authoringAvailable,
                    state.workingCopy,
                    state.selection,
                    gizmoDragging,
                    EditAddMenuRequest(EditorObjectKind::Terrain)));
            if (ImGui::MenuItem("Terrain"))
            {
                request = EditAddMenuRequest(EditorObjectKind::Terrain);
            }
            ImGui::EndDisabled();
            ImGui::BeginDisabled(
                !CanIssueAuthoredLifecycleRequest(
                    authoringAvailable,
                    state.workingCopy,
                    state.selection,
                    gizmoDragging,
                    EditAddMenuRequest(EditorObjectKind::ElevatedPlatform)));
            if (ImGui::MenuItem("Platform"))
            {
                request = EditAddMenuRequest(EditorObjectKind::ElevatedPlatform);
            }
            ImGui::EndDisabled();
            ImGui::BeginDisabled(
                !CanIssueAuthoredLifecycleRequest(
                    authoringAvailable,
                    state.workingCopy,
                    state.selection,
                    gizmoDragging,
                    EditAddMenuRequest(EditorObjectKind::Checkpoint)));
            if (ImGui::MenuItem("Checkpoint"))
            {
                request = EditAddMenuRequest(EditorObjectKind::Checkpoint);
            }
            ImGui::EndDisabled();
            ImGui::BeginDisabled(
                !CanIssueAuthoredLifecycleRequest(
                    authoringAvailable,
                    state.workingCopy,
                    state.selection,
                    gizmoDragging,
                    EditAddMenuRequest(EditorObjectKind::Hazard)));
            if (ImGui::MenuItem("Hazard"))
            {
                request = EditAddMenuRequest(EditorObjectKind::Hazard);
            }
            ImGui::EndDisabled();
            ImGui::BeginDisabled(
                !CanIssueAuthoredLifecycleRequest(
                    authoringAvailable,
                    state.workingCopy,
                    state.selection,
                    gizmoDragging,
                    EditAddMenuRequest(EditorObjectKind::Collectible)));
            if (ImGui::MenuItem("Collectible"))
            {
                request = EditAddMenuRequest(EditorObjectKind::Collectible);
            }
            ImGui::EndDisabled();
            ImGui::BeginDisabled(
                !CanIssueAuthoredLifecycleRequest(
                    authoringAvailable,
                    state.workingCopy,
                    state.selection,
                    gizmoDragging,
                    EditAddMenuRequest(EditorObjectKind::DynamicBox)));
            if (ImGui::MenuItem("Dynamic Box"))
            {
                request = EditAddMenuRequest(EditorObjectKind::DynamicBox);
            }
            ImGui::EndDisabled();
            ImGui::BeginDisabled(
                !CanIssueAuthoredLifecycleRequest(
                    authoringAvailable,
                    state.workingCopy,
                    state.selection,
                    gizmoDragging,
                    EditAddMenuRequest(EditorObjectKind::PressurePlate)));
            if (ImGui::MenuItem("Pressure Plate"))
            {
                request = EditAddMenuRequest(EditorObjectKind::PressurePlate);
            }
            ImGui::EndDisabled();
            ImGui::BeginDisabled(
                !CanIssueAuthoredLifecycleRequest(
                    authoringAvailable,
                    state.workingCopy,
                    state.selection,
                    gizmoDragging,
                    EditAddMenuRequest(EditorObjectKind::Door)));
            if (ImGui::MenuItem("Door"))
            {
                request = EditAddMenuRequest(EditorObjectKind::Door);
            }
            ImGui::EndDisabled();
            ImGui::BeginDisabled(
                !CanIssueAuthoredLifecycleRequest(
                    authoringAvailable,
                    state.workingCopy,
                    state.selection,
                    gizmoDragging,
                    EditAddMenuRequest(EditorObjectKind::ItemPickup)));
            if (ImGui::MenuItem("Item Pickup"))
            {
                request = EditAddMenuRequest(EditorObjectKind::ItemPickup);
            }
            ImGui::EndDisabled();
            ImGui::BeginDisabled(
                !CanIssueAuthoredLifecycleRequest(
                    authoringAvailable,
                    state.workingCopy,
                    state.selection,
                    gizmoDragging,
                    EditAddMenuRequest(EditorObjectKind::Goal)));
            if (ImGui::MenuItem("Level Goal"))
            {
                request = EditAddMenuRequest(EditorObjectKind::Goal);
            }
            ImGui::EndDisabled();
            ImGui::BeginDisabled(
                !CanIssueAuthoredLifecycleRequest(
                    authoringAvailable,
                    state.workingCopy,
                    state.selection,
                    gizmoDragging,
                    EditAddMenuRequest(EditorObjectKind::PointLight)));
            if (ImGui::MenuItem("Point Light"))
            {
                request = EditAddMenuRequest(EditorObjectKind::PointLight);
            }
            ImGui::EndDisabled();
            ImGui::BeginDisabled(
                !CanIssueAuthoredLifecycleRequest(
                    authoringAvailable,
                    state.workingCopy,
                    state.selection,
                    gizmoDragging,
                    EditAddMenuRequest(EditorObjectKind::SpotLight)));
            if (ImGui::MenuItem("Spot Light"))
            {
                request = EditAddMenuRequest(EditorObjectKind::SpotLight);
            }
            ImGui::EndDisabled();
            // Static Prop is the only Add entry whose enablement depends on
            // another window, so the row states the asset it would use. The
            // hidden dependency is what made the command look unavailable.
            const std::string& propIdentity = state.contentBrowser.selectedIdentity;
            const std::string propHint = AddStaticPropMenuHint(propIdentity);
            ImGui::BeginDisabled(
                !CanIssueAuthoredLifecycleRequest(
                    authoringAvailable,
                    state.workingCopy,
                    state.selection,
                    gizmoDragging,
                    EditAddMenuRequest(EditorObjectKind::StaticProp),
                    propIdentity));
            if (ImGui::MenuItem("Static Prop", propHint.c_str()))
            {
                request = EditAddMenuRequest(EditorObjectKind::StaticProp);
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                const char* reason = AddStaticPropDisableReason(
                    authoringAvailable,
                    state.workingCopy,
                    gizmoDragging,
                    propIdentity);
                if (reason != nullptr)
                {
                    ImGui::SetTooltip("%s", reason);
                }
            }
            ImGui::EndDisabled();
            ImGui::Separator();
            ImGui::TextDisabled("Static Prop uses the Content Browser selection.");
            ImGui::EndMenu();
        }

        ImGui::BeginDisabled(
            !CanIssueAuthoredLifecycleRequest(
                authoringAvailable,
                state.workingCopy,
                state.selection,
                gizmoDragging,
                LevelEditorRequest::DuplicateSelected,
                {},
                state.additionalSelections));
        if (ImGui::MenuItem("Duplicate Selected"))
        {
            request = LevelEditorRequest::DuplicateSelected;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            const char* reason = DuplicateSelectedDisableReason(
                authoringAvailable,
                state.workingCopy,
                state.selection,
                gizmoDragging,
                state.additionalSelections);
            if (reason != nullptr)
            {
                ImGui::SetTooltip("%s", reason);
            }
        }
        ImGui::EndDisabled();
        ImGui::BeginDisabled(
            !CanIssueAuthoredLifecycleRequest(
                authoringAvailable,
                state.workingCopy,
                state.selection,
                gizmoDragging,
                LevelEditorRequest::DeleteSelected,
                {},
                state.additionalSelections));
        if (ImGui::MenuItem("Delete Selected", "Delete"))
        {
            request = LevelEditorRequest::DeleteSelected;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            const char* reason = DeleteSelectedDisableReason(
                authoringAvailable,
                state.workingCopy,
                state.selection,
                gizmoDragging,
                state.additionalSelections);
            if (reason != nullptr)
            {
                ImGui::SetTooltip("%s", reason);
            }
        }
        ImGui::EndDisabled();
        ImGui::BeginDisabled(
            !CanIssueAuthoredLifecycleRequest(
                authoringAvailable,
                state.workingCopy,
                state.selection,
                gizmoDragging,
                LevelEditorRequest::GroupSelected,
                {},
                state.additionalSelections));
        if (ImGui::MenuItem("Group Selected"))
        {
            request = LevelEditorRequest::GroupSelected;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            const char* reason = CreateAuthoringGroupDisableReason(
                state.workingCopy, state.selection, state.additionalSelections);
            if (!authoringAvailable)
            {
                reason = "Lifecycle editing is available in Development only.";
            }
            else if (gizmoDragging)
            {
                reason = "Lifecycle blocked: finish the gizmo drag first.";
            }
            if (reason != nullptr)
            {
                ImGui::SetTooltip("%s", reason);
            }
        }
        ImGui::EndDisabled();
        ImGui::BeginDisabled(
            !CanIssueAuthoredLifecycleRequest(
                authoringAvailable,
                state.workingCopy,
                state.selection,
                gizmoDragging,
                LevelEditorRequest::UngroupSelected,
                {},
                state.additionalSelections));
        if (ImGui::MenuItem("Ungroup"))
        {
            request = LevelEditorRequest::UngroupSelected;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            const char* reason = UngroupAuthoringGroupDisableReason(
                state.workingCopy, state.selection, state.additionalSelections);
            if (!authoringAvailable)
            {
                reason = "Lifecycle editing is available in Development only.";
            }
            else if (gizmoDragging)
            {
                reason = "Lifecycle blocked: finish the gizmo drag first.";
            }
            if (reason != nullptr)
            {
                ImGui::SetTooltip("%s", reason);
            }
        }
        ImGui::EndDisabled();
        ImGui::EndMenu();
    }
#endif

#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
    if (ImGui::BeginMenu("Assets"))
    {
        const bool toolsBusy = toolRunner.IsRunning() || cookStageReloadPending;
        ImGui::BeginDisabled(!IsLevelAuthoringAvailable() || toolsBusy);
        if (ImGui::BeginMenu("Import"))
        {
            if (ImGui::MenuItem("Import Model..."))
            {
                request = LevelEditorRequest::ImportStaticGlb;
            }
            if (ImGui::MenuItem("Import Texture..."))
            {
                request = LevelEditorRequest::ImportTexture;
            }
            ImGui::EndMenu();
        }
        ImGui::EndDisabled();
        ImGui::EndMenu();
    }
#endif

    if (ImGui::BeginMenu("Level"))
    {
        const bool authoringAvailable = IsLevelAuthoringAvailable();
        const bool workingCopyValid = world::IsWritableLevelDefinition(state.workingCopy);

        ImGui::BeginDisabled(!CanApplyPreview(state.modified, workingCopyValid));
        if (ImGui::MenuItem("Apply Preview"))
        {
            request = LevelEditorRequest::ApplyPreview;
        }
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!CanRevertWorkingCopy(state.modified));
        if (ImGui::MenuItem("Revert Working Copy"))
        {
            request = LevelEditorRequest::RevertWorkingCopy;
        }
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!CanSaveLevelSource(authoringAvailable, state.modified));
        if (ImGui::MenuItem("Save Level Source"))
        {
            request = LevelEditorRequest::SaveLevelSource;
        }
        ImGui::EndDisabled();

#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
        ImGui::Separator();
        ImGui::BeginDisabled(
            !CanReloadRuntimeLevel(
                authoringAvailable,
                state.modified,
                toolRunner.IsRunning(),
                cookStageReloadPending));
        if (ImGui::MenuItem("Reload Runtime Level"))
        {
            request = LevelEditorRequest::ReloadRuntimeLevel;
        }
        ImGui::EndDisabled();
#endif

        ImGui::Separator();
        if (ImGui::MenuItem("Reset Editor Layout"))
        {
            ResetEditorWorkspaceLayout(state, view.viewportWidth, view.viewportHeight);
        }
        ImGui::EndMenu();
    }

#if defined(PLATFORMER_ENABLE_EDITOR_TOOLS)
    if (ImGui::BeginMenu("Build"))
    {
        const bool toolRunning = toolRunner.IsRunning();
        const bool toolsBusy = toolRunning || cookStageReloadPending;
        ImGui::BeginDisabled(toolsBusy);
        if (ImGui::MenuItem("Cook Assets"))
        {
            RequestEditorToolStart(toolRunner, state.workspace, EditorToolKind::CookAssets);
        }
        // Concise label; the job always stages Development runtime assets.
        if (ImGui::MenuItem("Stage Runtime Assets"))
        {
            RequestEditorToolStart(toolRunner, state.workspace, EditorToolKind::StageRuntimeAssets);
        }
        if (ImGui::MenuItem("Cook & Stage"))
        {
            RequestEditorToolStart(toolRunner, state.workspace, EditorToolKind::CookAndStage);
        }
        ImGui::EndDisabled();

        ImGui::BeginDisabled(
            !CanStartCookStageReload(
                IsLevelAuthoringAvailable(),
                state.modified,
                toolRunning,
                cookStageReloadPending));
        if (ImGui::MenuItem("Cook, Stage & Reload"))
        {
            request = LevelEditorRequest::CookStageAndReload;
        }
        ImGui::EndDisabled();

        ImGui::Separator();
        ImGui::BeginDisabled(toolsBusy);
        if (ImGui::MenuItem("Build Debug"))
        {
            RequestEditorToolStart(toolRunner, state.workspace, EditorToolKind::BuildDebug);
        }
        if (ImGui::MenuItem("Build Development"))
        {
            RequestEditorToolStart(toolRunner, state.workspace, EditorToolKind::BuildDevelopment);
        }
        if (ImGui::MenuItem("Build Release"))
        {
            RequestEditorToolStart(toolRunner, state.workspace, EditorToolKind::BuildRelease);
        }
        if (ImGui::MenuItem("Build All"))
        {
            RequestEditorToolStart(toolRunner, state.workspace, EditorToolKind::BuildAll);
        }
        ImGui::EndDisabled();
        ImGui::Separator();
        if (ImGui::MenuItem("Tool Output"))
        {
            state.workspace.showToolOutput = true;
        }
        ImGui::EndMenu();
    }
#else
    (void)toolRunner;
#endif

    ImGui::EndMainMenuBar();
    return request;
}

namespace
{
void DrawQuickToolbarTransformButton(
    LevelEditorState& state,
    const char* label,
    const char* tooltip,
    EditorTransformMode mode)
{
    const bool active = state.transformMode == mode;
    if (active)
    {
        const ImVec4 pressed = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
        ImGui::PushStyleColor(ImGuiCol_Button, pressed);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, pressed);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, pressed);
    }
    ImGui::BeginDisabled(state.gizmo.dragging);
    if (ImGui::Button(label))
    {
        TrySetEditorTransformMode(state.transformMode, state.gizmo.dragging, mode);
    }
    ImGui::EndDisabled();
    if (active)
    {
        ImGui::PopStyleColor(3);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("%s", tooltip);
    }
}

void DrawQuickToolbarCommandButton(
    const char* label,
    const char* tooltip,
    bool enabled,
    LevelEditorRequest command,
    LevelEditorRequest& request)
{
    ImGui::BeginDisabled(!enabled);
    if (ImGui::Button(label))
    {
        request = command;
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("%s", tooltip);
    }
}
}

LevelEditorRequest DrawEditorQuickToolbar(
    LevelEditorState& state,
    const world::LevelDefinition&,
    EditorToolRunner& toolRunner,
    bool cookStageReloadPending)
{
    LevelEditorRequest request = LevelEditorRequest::None;
#if !defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
    (void)toolRunner;
    (void)cookStageReloadPending;
    state.toolbarHeight = 0.0f;
    return request;
#else
    if (!state.workspace.showQuickToolbar)
    {
        state.toolbarHeight = 0.0f;
        return request;
    }

    const float menuHeight = ResolveEditorMenuBarHeight(state.menuBarHeight);
    const ImGuiIO& io = ImGui::GetIO();
    const float toolbarH =
        ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y * 2.0f;
    ImGui::SetNextWindowPos(ImVec2(0.0f, menuHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, toolbarH), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoNavFocus;
    if (!ImGui::Begin("##EditorQuickToolbar", nullptr, flags))
    {
        ImGui::End();
        ImGui::PopStyleVar(2);
        state.toolbarHeight = 0.0f;
        return request;
    }

    DrawQuickToolbarTransformButton(
        state, "Translate", "Translate", EditorTransformMode::Translate);
    ImGui::SameLine();
    DrawQuickToolbarTransformButton(state, "Resize", "Resize", EditorTransformMode::Resize);
    ImGui::SameLine();
    DrawQuickToolbarTransformButton(
        state, "Scale", "Scale visual model (Static Prop / Item Pickup)", EditorTransformMode::Scale);
    ImGui::SameLine();
    DrawQuickToolbarTransformButton(
        state,
        "Rotate",
        "Rotate (Static Prop rotation / Item Pickup visual rotation)",
        EditorTransformMode::Rotate);

    ImGui::SameLine();
    DrawEditorSnapControls(state, true, "##SnapIncrementToolbar");

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    const bool workingCopyValid = world::IsWritableLevelDefinition(state.workingCopy);
    DrawQuickToolbarCommandButton(
        "Apply",
        "Apply Preview",
        CanApplyPreview(state.modified, workingCopyValid),
        QuickToolbarApplyPreviewRequest(),
        request);
    ImGui::SameLine();
    DrawQuickToolbarCommandButton(
        "Revert",
        "Revert Working Copy",
        CanRevertWorkingCopy(state.modified),
        QuickToolbarRevertWorkingCopyRequest(),
        request);
    ImGui::SameLine();
    DrawQuickToolbarCommandButton(
        "Save",
        "Save Level Source",
        CanSaveLevelSource(IsLevelAuthoringAvailable(), state.modified),
        QuickToolbarSaveLevelSourceRequest(),
        request);

#if defined(PLATFORMER_ENABLE_EDITOR_TOOLS)
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::TextUnformatted("Build:");
    ImGui::SameLine();
    const char* buildItems[] = {"Debug", "Development", "Release", "All"};
    int buildIndex = static_cast<int>(state.selectedBuildTarget);
    if (buildIndex < 0 || buildIndex > 3)
    {
        buildIndex = static_cast<int>(kDefaultEditorBuildTarget);
        state.selectedBuildTarget = kDefaultEditorBuildTarget;
    }
    ImGui::SetNextItemWidth(132.0f);
    if (ImGui::Combo("##QuickToolbarBuild", &buildIndex, buildItems, 4))
    {
        state.selectedBuildTarget = static_cast<EditorBuildTarget>(buildIndex);
        SaveEditorBuildSelection(state.selectedBuildTarget);
    }

    ImGui::SameLine();
    const bool toolsBusy = toolRunner.IsRunning() || cookStageReloadPending;
    ImGui::BeginDisabled(toolsBusy || !CanStartEditorToolJob(
                                          IsEditorToolExecutionAvailable(), toolRunner.IsRunning()));
    if (ImGui::Button("Run"))
    {
        RequestEditorToolStart(
            toolRunner,
            state.workspace,
            EditorToolKindForBuildTarget(state.selectedBuildTarget));
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Run selected build");
    }
#else
    (void)toolRunner;
    (void)cookStageReloadPending;
#endif

    state.toolbarHeight = ImGui::GetWindowSize().y;
    ImGui::End();
    ImGui::PopStyleVar(2);
    return request;
#endif
}

void DrawEditorToolOutput(
    LevelEditorState& state,
    const LevelEditorViewContext& view,
    EditorToolRunner& toolRunner)
{
#if !defined(PLATFORMER_ENABLE_EDITOR_TOOLS)
    (void)state;
    (void)view;
    (void)toolRunner;
#else
    if (!state.workspace.showToolOutput)
    {
        return;
    }

    ApplyKnownEditorWindowPlacement(
        kToolOutputWindowName, view.viewportWidth, view.viewportHeight, view.forceDefaultLayout);
    if (!ImGui::Begin(kToolOutputWindowName, &state.workspace.showToolOutput))
    {
        ImGui::End();
        return;
    }
    RecoverKnownEditorWindowIfOffscreen(
        kToolOutputWindowName,
        view.viewportWidth,
        view.viewportHeight,
        view.recoverOffscreenLayout);

    const EditorToolJobSnapshot snapshot = toolRunner.Snapshot();
    const char* jobLabel = snapshot.displayLabel.empty() ? "(none)" : snapshot.displayLabel.c_str();
    if (snapshot.state == EditorToolJobState::Idle && snapshot.log.empty())
    {
        jobLabel = "(none)";
    }
    ImGui::Text("Job: %s", jobLabel);
    ImGui::Text("State: %s", EditorToolJobStateName(snapshot.state));
    ImGui::Text("Elapsed: %.1f s", snapshot.elapsedSeconds);
    if (snapshot.hasExitCode)
    {
        ImGui::Text("Exit code: %d", snapshot.exitCode);
    }
    else
    {
        ImGui::TextUnformatted("Exit code: -");
    }
    if (snapshot.sequenceStepCount > 0
        && (snapshot.state == EditorToolJobState::Running
            || snapshot.state == EditorToolJobState::Failed
            || snapshot.state == EditorToolJobState::Succeeded))
    {
        ImGui::Text(
            "%s: %d/%d %s",
            snapshot.displayLabel.c_str(),
            snapshot.sequenceStepIndex,
            snapshot.sequenceStepCount,
            snapshot.sequenceStepLabel.c_str());
    }

    ImGui::BeginDisabled(toolRunner.IsRunning());
    if (ImGui::Button("Clear"))
    {
        toolRunner.ClearLog();
    }
    ImGui::EndDisabled();

    ImGui::BeginChild("ToolOutputLog", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
    const bool stickToBottom =
        ImGui::GetScrollMaxY() <= 0.0f || ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 16.0f;
    ImGui::TextUnformatted(snapshot.log.c_str());
    if (stickToBottom)
    {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
    ImGui::End();
#endif
}

LevelEditorRequest DrawLevelEditor(
    LevelEditorState& state,
    const world::LevelDefinition& activeLevel,
    const LevelEditorViewContext& view,
    EditorToolRunner& toolRunner,
    bool cookStageReloadPending)
{
    RefreshLevelEditorDerivedFlags(state, activeLevel);

    LevelEditorRequest request = LevelEditorRequest::None;
    if (state.workspace.showHierarchy)
    {
        request = DrawHierarchy(state, view);
    }
    if (state.workspace.showInspector)
    {
        DrawInspector(state, view);
    }
#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
    if (state.workspace.showObjectPalette)
    {
        DrawObjectPalette(state, view);
    }
    if (state.workspace.showLevels)
    {
        const LevelEditorRequest levelsRequest = DrawLevelsBrowser(state, activeLevel, view);
        if (request == LevelEditorRequest::None)
        {
            request = levelsRequest;
        }
    }
    if (state.workspace.showContentBrowser)
    {
        const LevelEditorRequest browserRequest =
            DrawContentBrowser(state, view, toolRunner, cookStageReloadPending);
        if (request == LevelEditorRequest::None)
        {
            request = browserRequest;
        }
    }
    if (state.workspace.showModelPreview)
    {
        DrawModelPreview(state, view);
    }
#endif
    if (!state.workspace.showLevelEditor)
    {
        return request;
    }
    const LevelEditorRequest controls = DrawLevelControls(
        state, activeLevel, view, toolRunner, cookStageReloadPending);
    return request != LevelEditorRequest::None ? request : controls;
}

#else

void ResetEditorWorkspaceLayout(LevelEditorState&, float, float) {}

LevelEditorRequest DrawEditorMenuBar(
    LevelEditorState&,
    const world::LevelDefinition&,
    const LevelEditorViewContext&,
    EditorToolRunner&,
    bool)
{
    return LevelEditorRequest::None;
}

LevelEditorRequest DrawEditorQuickToolbar(
    LevelEditorState& state,
    const world::LevelDefinition&,
    EditorToolRunner&,
    bool)
{
    state.toolbarHeight = 0.0f;
    return LevelEditorRequest::None;
}

void DrawEditorToolOutput(LevelEditorState&, const LevelEditorViewContext&, EditorToolRunner&) {}

LevelEditorRequest DrawLevelEditor(
    LevelEditorState&,
    const world::LevelDefinition&,
    const LevelEditorViewContext&,
    EditorToolRunner&,
    bool)
{
    return LevelEditorRequest::None;
}

#endif
}
