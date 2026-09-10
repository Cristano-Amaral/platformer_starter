#include "editor/LevelEditor.h"

#include "editor/AuthoredLifecycleCommands.h"
#include "editor/EditorHierarchy.h"
#include "editor/EditorSelection.h"

#if defined(PLATFORMER_ENABLE_DEBUG_UI) || defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
#include "editor/AuthoringPaths.h"
#include "world/LevelWriter.h"
#endif

#if defined(PLATFORMER_ENABLE_DEBUG_UI)
#include "editor/ContentBrowser.h"
#include "editor/ContentBrowserView.h"
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
#include "render/StaticModelPreview.h"
#endif

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
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
    const std::filesystem::path path = AuthoringLevel01SourcePath();
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

void ReadOnlyFloat(const char* label, float value)
{
    ImGui::Text("%s: %.6f", label, value);
}

void DrawHierarchy(LevelEditorState& state, const LevelEditorViewContext& view)
{
    ApplyEditorWindowPlacement(kHierarchyWindowName, view);
    if (!ImGui::Begin(kHierarchyWindowName, &state.workspace.showHierarchy))
    {
        ImGui::End();
        return;
    }
    RecoverEditorWindowIfNeeded(kHierarchyWindowName, view);

    const std::vector<HierarchyEntry> entries = BuildHierarchyEntries(state.workingCopy);
    const char* openGroup = nullptr;
    bool groupVisible = true;
    for (const HierarchyEntry& entry : entries)
    {
        const bool grouped = entry.group[0] != '\0';
        if (grouped)
        {
            if (openGroup == nullptr || std::strcmp(openGroup, entry.group) != 0)
            {
                if (openGroup != nullptr && groupVisible)
                {
                    ImGui::TreePop();
                }
                openGroup = entry.group;
                groupVisible = ImGui::TreeNodeEx(entry.group, ImGuiTreeNodeFlags_DefaultOpen);
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

        char label[64];
        FormatSelectionDisplayName(entry.selection, label, sizeof(label));
        const bool selected = state.selection == entry.selection;
        if (ImGui::Selectable(label, selected) && !state.gizmo.dragging)
        {
            state.selection = entry.selection;
        }
    }
    if (openGroup != nullptr && groupVisible)
    {
        ImGui::TreePop();
    }

    ImGui::End();
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
    case EditorObjectKind::Ground:
        EditVec3("Center X Y Z", level.ground.center);
        EditVec3("Size X Y Z", level.ground.size);
        break;
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
        }
        break;
    case EditorObjectKind::Door:
        if (state.selection.index < level.doors.size())
        {
            world::DoorSpec& door = level.doors[state.selection.index];
            EditVec3("Position X Y Z", door.center);
            EditVec3("Size X Y Z", door.size);
            ImGui::InputFloat("Open Distance", &door.openDistance, 0.0f, 0.0f, kFloatFormat);
            ImGui::Checkbox("Requires Key", &door.requiresKey);
            ImGui::TextUnformatted("Opens +Y from the authored closed position.");
        }
        break;
    case EditorObjectKind::ItemPickup:
        if (state.selection.index < level.itemPickups.size())
        {
            world::ItemPickupSpec& pickup = level.itemPickups[state.selection.index];
            EditVec3("Position X Y Z", pickup.position);
            char itemId[gameplay::kMaxItemIdLength + 1]{};
            std::snprintf(
                itemId, sizeof(itemId), "%s", pickup.itemId.c_str());
            if (ImGui::InputText("Item ID", itemId, sizeof(itemId)))
            {
                if (gameplay::IsValidItemId(itemId))
                {
                    pickup.itemId = itemId;
                }
            }
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
            ImGui::TextUnformatted("Translate gizmo edits Position. Scale gizmo edits Scale. Rotation is Inspector-only.");
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
    case EditorObjectKind::Goal:
        ImGui::TextUnformatted("Read-only in M33.");
        ReadOnlyVec3("Center", level.goal.center);
        ReadOnlyVec3("Size", level.goal.size);
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

    ImGui::TextUnformatted("Registered static GLB assets. Selection is not a level object.");
    ImGui::TextWrapped(
        "Add Static Prop creates one authored instance immediately from the selected asset "
        "(working copy only). Place Static Prop enters viewport placement and creates nothing "
        "until a valid Ground/Platform/Slope click. Import copies canonical source only. Delete "
        "removes source plus matching cooked/staged copies.");

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
        if (view.modelPreview != nullptr)
        {
            view.modelPreview->AllowRetry();
        }
        state.contentBrowser.statusMessage = "Catalog refreshed from canonical source models.";
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!authoringAvailable || toolsBusy);
    if (ImGui::Button("Import Static GLB"))
    {
        request = ContentBrowserImportRequest();
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
    ImGui::TextUnformatted("View");
    ImGui::SameLine();
    if (ImGui::RadioButton(
            "Thumbnails", state.contentBrowser.viewMode == ContentBrowserViewMode::Thumbnails))
    {
        state.contentBrowser.viewMode = ContentBrowserViewMode::Thumbnails;
        SaveContentBrowserViewMode(state.contentBrowser.viewMode);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("List", state.contentBrowser.viewMode == ContentBrowserViewMode::List))
    {
        state.contentBrowser.viewMode = ContentBrowserViewMode::List;
        SaveContentBrowserViewMode(state.contentBrowser.viewMode);
    }

    const std::vector<assets::StaticModelCatalogEntry> visible =
        FilterContentBrowserEntries(state.contentBrowser.catalog, state.contentBrowser.filterQuery);
    const std::filesystem::path cacheRoot = ThumbnailCacheRoot();
    if (view.thumbnails != nullptr
        && state.contentBrowser.viewMode == ContentBrowserViewMode::Thumbnails)
    {
        for (const assets::StaticModelCatalogEntry& entry : visible)
        {
            view.thumbnails->Ensure(
                entry.canonicalIdentity, sourceRoot / entry.canonicalIdentity, cacheRoot);
        }
        std::string thumbnailFailure;
        if (view.thumbnails->ConsumeLastFailure(thumbnailFailure))
        {
            state.contentBrowser.statusMessage = thumbnailFailure;
        }
    }

    if (state.contentBrowser.catalog.Count() == 0)
    {
        ImGui::Spacing();
        ImGui::TextWrapped(
            "No static models are registered. Use Import Static GLB to copy a compatible .glb "
            "into game/assets/source/models/. Import does not place the asset in the level.");
    }
    else if (visible.empty())
    {
        ImGui::Spacing();
        ImGui::TextWrapped("No static models match the current search.");
    }
    else if (state.contentBrowser.viewMode == ContentBrowserViewMode::Thumbnails)
    {
        constexpr float kThumbSize = 96.0f;
        constexpr float kCellPad = 8.0f;
        const float avail = ImGui::GetContentRegionAvail().x;
        const int columns = std::max(1, static_cast<int>(avail / (kThumbSize + kCellPad * 2.0f)));
        int column = 0;
        if (ImGui::BeginChild("content-browser-thumbs", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None))
        {
            for (const assets::StaticModelCatalogEntry& entry : visible)
            {
                if (column > 0)
                {
                    ImGui::SameLine();
                }
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
                const unsigned int gpuId = view.thumbnails != nullptr
                    ? view.thumbnails->TextureGpuId(entry.canonicalIdentity)
                    : 0;
                if (gpuId != 0)
                {
                    ImGui::Image(
                        ImTextureRef(static_cast<ImTextureID>(static_cast<intptr_t>(gpuId))),
                        ImVec2(kThumbSize, kThumbSize));
                }
                else
                {
                    ImVec2 dummyMin = ImGui::GetCursorScreenPos();
                    ImGui::Dummy(ImVec2(kThumbSize, kThumbSize));
                    ImVec2 dummyMax = ImGui::GetItemRectMax();
                    const bool failed = view.thumbnails != nullptr
                        && view.thumbnails->IsFailed(entry.canonicalIdentity);
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        dummyMin,
                        dummyMax,
                        failed ? IM_COL32(88, 64, 64, 255) : IM_COL32(48, 52, 62, 255));
                    ImGui::GetWindowDrawList()->AddRect(
                        dummyMin, dummyMax, IM_COL32(120, 126, 140, 255));
                    (void)dummyMin;
                }
                ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + kThumbSize);
                ImGui::TextUnformatted(entry.displayName.c_str());
                ImGui::PopTextWrapPos();
                ImGui::EndGroup();
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip(
                        "%s\n%s\n%s",
                        entry.displayName.c_str(),
                        entry.assetType.c_str(),
                        entry.canonicalIdentity.c_str());
                }
                ImGui::PopID();
                column = (column + 1) % columns;
            }
        }
        ImGui::EndChild();
    }
    else if (ImGui::BeginTable(
                 "content-browser-assets",
                 3,
                 ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY
                     | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("Path");
        ImGui::TableHeadersRow();
        for (const assets::StaticModelCatalogEntry& entry : visible)
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
            ImGui::TextUnformatted(entry.assetType.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(entry.canonicalIdentity.c_str());
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    if (!state.contentBrowser.selectedIdentity.empty())
    {
        ImGui::Text("Selected asset: %s", state.contentBrowser.selectedIdentity.c_str());
        DrawStaticModelStagingHint(state.contentBrowser.selectedIdentity);
    }
    else
    {
        ImGui::TextUnformatted("Selected asset: none");
    }
    if (StaticPropPlacementIsActive(state.staticPropPlacement))
    {
        ImGui::Text(
            "Placing Static Prop: %s", state.staticPropPlacement.modelIdentity.c_str());
        ImGui::TextUnformatted(PlacementViewportActionHintText());
    }
    if (!state.contentBrowser.statusMessage.empty())
    {
        ImGui::TextWrapped("%s", state.contentBrowser.statusMessage.c_str());
    }

    if (state.contentBrowser.deleteConfirmOpen)
    {
        ImGui::OpenPopup("Delete Static Model");
    }
    if (ImGui::BeginPopupModal("Delete Static Model", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("Delete this registered static model?");
        ImGui::TextUnformatted(state.contentBrowser.selectedIdentity.c_str());
        ImGui::TextWrapped(
            "This removes the canonical source file and any matching cooked/staged copies. "
            "It does not edit the level. Cancel makes no filesystem changes.");
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

    const bool canFrame = preview != nullptr && preview->HasModel();
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
        ImGui::TextWrapped("No static model is selected. Choose an asset in the Content Browser.");
        ImGui::End();
        return;
    }

    const assets::StaticModelCatalogEntry* entry =
        state.contentBrowser.catalog.Find(identity);
    if (preview != nullptr && preview->IsFailed())
    {
        ImGui::TextWrapped("Preview failed to load this model. Refresh or reselect to retry.");
    }
    else
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
        ImGui::Text("Name: %s", entry != nullptr ? entry->displayName.c_str() : identity.c_str());
        ImGui::Text("Type: %s", entry != nullptr ? entry->assetType.c_str() : "static_glb");
        ImGui::TextUnformatted(identity.c_str());
        if (preview != nullptr && preview->HasModel())
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
            const std::string sourcePath = AuthoringLevel01SourcePath().string();
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
        ImGui::EndDisabled();
        if (state.gizmo.dragging)
        {
            ImGui::TextUnformatted("Mode locked while a gizmo drag is active.");
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
            "Scale: LMB on a cube handle changes Static Prop visual scale (not primitive Resize).");
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
    SaveContentBrowserViewMode(state.contentBrowser.viewMode);
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
        ImGui::EndDisabled();
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
                LevelEditorRequest::DuplicateSelected));
        if (ImGui::MenuItem("Duplicate Selected"))
        {
            request = LevelEditorRequest::DuplicateSelected;
        }
        ImGui::EndDisabled();
        ImGui::BeginDisabled(
            !CanIssueAuthoredLifecycleRequest(
                authoringAvailable,
                state.workingCopy,
                state.selection,
                gizmoDragging,
                LevelEditorRequest::DeleteSelected));
        if (ImGui::MenuItem("Delete Selected", "Delete"))
        {
            request = LevelEditorRequest::DeleteSelected;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            const char* reason = DeleteSelectedDisableReason(
                authoringAvailable, state.workingCopy, state.selection, gizmoDragging);
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
        if (ImGui::MenuItem("Import Static GLB"))
        {
            request = LevelEditorRequest::ImportStaticGlb;
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
        state, "Scale", "Scale visual model (Static Prop)", EditorTransformMode::Scale);

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

    if (state.workspace.showHierarchy)
    {
        DrawHierarchy(state, view);
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
#endif
    LevelEditorRequest request = LevelEditorRequest::None;
#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
    if (state.workspace.showContentBrowser)
    {
        request = DrawContentBrowser(state, view, toolRunner, cookStageReloadPending);
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
