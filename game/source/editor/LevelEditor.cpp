#include "editor/LevelEditor.h"

#include "editor/AuthoredLifecycleCommands.h"
#include "editor/EditorHierarchy.h"
#include "editor/EditorSelection.h"

#if defined(PLATFORMER_ENABLE_DEBUG_UI) || defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
#include "editor/AuthoringPaths.h"
#include "world/LevelWriter.h"
#endif

#if defined(PLATFORMER_ENABLE_DEBUG_UI)
#include "editor/EditorLayout.h"
#include "editor/EditorLayoutUi.h"
#include "editor/EditorPlacement.h"
#include "editor/EditorToolCommands.h"
#include "editor/EditorToolRunner.h"
#include "imgui.h"

#include <cstddef>
#include <cstring>
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

    ImGui::Separator();
    if (PlacementModeIsActive(state.placementMode))
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
            "Resize: LMB on a cube handle changes authored size; the cyan ghost is the true size.");
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
    if (!state.workspace.showLevelEditor)
    {
        return LevelEditorRequest::None;
    }
    return DrawLevelControls(
        state, activeLevel, view, toolRunner, cookStageReloadPending);
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
