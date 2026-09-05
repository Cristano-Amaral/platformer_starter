#include "editor/LevelEditor.h"

#include "editor/EditorHierarchy.h"
#include "editor/EditorSelection.h"

#if defined(PLATFORMER_ENABLE_DEBUG_UI) || defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
#include "editor/AuthoringPaths.h"
#include "world/LevelWriter.h"
#endif

#if defined(PLATFORMER_ENABLE_DEBUG_UI)
#include "editor/EditorLayout.h"
#include "editor/EditorLayoutUi.h"
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

    const char* openGroup = nullptr;
    bool groupVisible = true;
    for (std::size_t index = 0; index < kHierarchyEntryCount; ++index)
    {
        const HierarchyEntry& entry = kHierarchyEntries[index];
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

        const bool selected = state.selection == entry.selection;
        if (ImGui::Selectable(entry.label, selected) && !state.gizmo.dragging)
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

    if (state.selection.kind == EditorObjectKind::None)
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
        ImGui::TextUnformatted("Read-only in M33.");
        if (state.selection.index < level.checkpoints.size())
        {
            const world::CheckpointSpec& checkpoint = level.checkpoints[state.selection.index];
            ReadOnlyVec3("Trigger center", checkpoint.center);
            ReadOnlyVec3("Trigger size", checkpoint.size);
            ReadOnlyVec3("Respawn position", checkpoint.respawnPosition);
        }
        break;
    case EditorObjectKind::Hazard:
        ImGui::TextUnformatted("Read-only in M33.");
        if (state.selection.index < level.hazards.size())
        {
            const world::HazardSpec& hazard = level.hazards[state.selection.index];
            ReadOnlyVec3("Center", hazard.center);
            ReadOnlyVec3("Size", hazard.size);
        }
        break;
    case EditorObjectKind::Collectible:
        ImGui::TextUnformatted("Read-only in M33.");
        if (state.selection.index < level.collectibles.size())
        {
            ReadOnlyVec3("Authored position", level.collectibles[state.selection.index].center);
        }
        break;
    case EditorObjectKind::Goal:
        ImGui::TextUnformatted("Read-only in M33.");
        ReadOnlyVec3("Center", level.goal.center);
        ReadOnlyVec3("Size", level.goal.size);
        break;
    case EditorObjectKind::DynamicBox:
        ImGui::TextUnformatted("Read-only in M33.");
        ReadOnlyVec3("Authored center", level.dynamicBox.center);
        ReadOnlyVec3("Authored size", level.dynamicBox.size);
        ReadOnlyFloat("Mass", level.dynamicBox.mass);
        ReadOnlyVec3("Runtime position", view.dynamicBoxRuntimeCenter);
        break;
    case EditorObjectKind::None:
        break;
    }

    ImGui::End();
}

LevelEditorRequest DrawLevelControls(
    LevelEditorState& state,
    const world::LevelDefinition& activeLevel,
    const LevelEditorViewContext& view)
{
    LevelEditorRequest request = LevelEditorRequest::None;
    ApplyEditorWindowPlacement(kLevelEditorWindowName, view);
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
        ImGui::Text("Last Apply: %s", LevelEditorApplyStatusName(state.lastApplyStatus));
        ImGui::Text("Last Save: %s", LevelEditorSaveStatusName(state.lastSaveStatus));
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

        if (ImGui::Button("Reset Editor Layout"))
        {
            ResetEditorWorkspaceLayout(state, view.viewportWidth, view.viewportHeight);
        }
        ImGui::TextWrapped(
            "Reset Editor Layout restores Metrics, Hierarchy, Inspector, and Level Editor "
            "positions and shows all four panels. It does not change the level, selection, or camera.");
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
        ImGui::TextWrapped(
            "Run python tools/cook_assets.py, then cmake --build, then relaunch to update the"
            " cooked and staged runtime files.");
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

LevelEditorRequest DrawEditorMenuBar(
    LevelEditorState& state,
    const world::LevelDefinition&,
    const LevelEditorViewContext& view)
{
    LevelEditorRequest request = LevelEditorRequest::None;
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

        ImGui::Separator();
        if (ImGui::MenuItem("Reset Editor Layout"))
        {
            ResetEditorWorkspaceLayout(state, view.viewportWidth, view.viewportHeight);
        }
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
    return request;
}

LevelEditorRequest DrawLevelEditor(
    LevelEditorState& state,
    const world::LevelDefinition& activeLevel,
    const LevelEditorViewContext& view)
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
    if (!state.workspace.showLevelEditor)
    {
        return LevelEditorRequest::None;
    }
    return DrawLevelControls(state, activeLevel, view);
}

#else

void ResetEditorWorkspaceLayout(LevelEditorState&, float, float) {}

LevelEditorRequest DrawEditorMenuBar(
    LevelEditorState&,
    const world::LevelDefinition&,
    const LevelEditorViewContext&)
{
    return LevelEditorRequest::None;
}

LevelEditorRequest DrawLevelEditor(
    LevelEditorState&,
    const world::LevelDefinition&,
    const LevelEditorViewContext&)
{
    return LevelEditorRequest::None;
}

#endif
}
