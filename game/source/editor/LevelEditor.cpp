#include "editor/LevelEditor.h"

#if defined(PLATFORMER_ENABLE_DEBUG_UI) || defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
#include "editor/AuthoringPaths.h"
#include "world/LevelWriter.h"
#endif

#if defined(PLATFORMER_ENABLE_DEBUG_UI)
#include "imgui.h"

#include <cstddef>
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

#if defined(PLATFORMER_ENABLE_DEBUG_UI)

namespace
{
// Enough precision that editing a field cannot silently round authored data
// such as 1.6732 down to 1.67. ImGui writes a field back only when edited.
constexpr const char* kFloatFormat = "%.6f";

const char* BoolText(bool value)
{
    return value ? "true" : "false";
}

void EditVec3(const char* label, core::Vec3& value)
{
    ImGui::InputFloat3(label, &value.x, kFloatFormat);
}

const char* SelectionLabel(int index)
{
    switch (index)
    {
    case 0:
        return "Platform 0";
    case 1:
        return "Platform 1";
    case 2:
        return "Platform 2";
    case 3:
        return "Platform 3";
    case 4:
        return "Platform 4";
    case 5:
        return "Platform 5";
    default:
        break;
    }
    return "Ground";
}

world::Box& SelectedBox(world::LevelDefinition& level, int selectionIndex)
{
    if (selectionIndex < 0 || selectionIndex >= world::kLevel01ElevatedPlatformCount)
    {
        return level.ground;
    }
    return level.elevatedPlatforms[static_cast<std::size_t>(selectionIndex)];
}
}

LevelEditorRequest DrawLevelEditor(
    LevelEditorState& state,
    const world::LevelDefinition& activeLevel,
    const char* runtimeLevelPath)
{
    // Derived from authored data rather than widget return values, so a field
    // that reports "edited" without changing the value stays unmodified.
    state.modified = !world::AuthoredLevelDataEqual(state.workingCopy, activeLevel);
    state.dirty = !world::AuthoredLevelDataEqual(activeLevel, state.savedSourceBaseline);

    LevelEditorRequest request = LevelEditorRequest::None;
    if (!ImGui::Begin("Level Editor"))
    {
        ImGui::End();
        return request;
    }

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
        ImGui::TextWrapped("Runtime staged (read-only): %s", runtimeLevelPath);
    }

    if (ImGui::CollapsingHeader("Spawn", ImGuiTreeNodeFlags_DefaultOpen))
    {
        EditVec3("Spawn X Y Z", state.workingCopy.initialSpawnVisualCenter);
    }

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
    {
        EditVec3("Offset X Y Z", state.workingCopy.camera.offset);
        ImGui::InputFloat(
            "FOV Y", &state.workingCopy.camera.fieldOfViewY, 0.0f, 0.0f, kFloatFormat);
    }

    if (ImGui::CollapsingHeader("Static Geometry", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::BeginCombo("Selection", SelectionLabel(state.selectedPlatformIndex)))
        {
            for (int index = kGroundSelectionIndex;
                 index < world::kLevel01ElevatedPlatformCount;
                 ++index)
            {
                if (ImGui::Selectable(
                        SelectionLabel(index),
                        state.selectedPlatformIndex == index))
                {
                    state.selectedPlatformIndex = index;
                }
            }
            ImGui::EndCombo();
        }

        world::Box& box = SelectedBox(state.workingCopy, state.selectedPlatformIndex);
        EditVec3("Center X Y Z", box.center);
        EditVec3("Size X Y Z", box.size);
    }

    if (ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::BeginDisabled(!state.modified || !workingCopyValid);
        if (ImGui::Button("Apply Preview"))
        {
            request = LevelEditorRequest::ApplyPreview;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::BeginDisabled(!state.modified);
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
        ImGui::BeginDisabled(!authoringAvailable || state.modified);
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
    }

    if (ImGui::CollapsingHeader("Information", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextWrapped("Save updates the project source only. It does not recook or rebuild.");
        ImGui::TextWrapped(
            "Run python tools/cook_assets.py, then cmake --build, then relaunch to update the"
            " cooked and staged runtime files.");
        ImGui::TextWrapped(
            "Applied but unsaved edits live in memory only and are lost when the process exits.");
        ImGui::TextWrapped(
            "Closing and reopening the editor discards unapplied working-copy edits.");
        ImGui::TextWrapped(
            "Editable in M32: spawn, camera offset and FOV, ground and elevated platform"
            " center/size. Everything else is read-only.");
    }

    ImGui::End();
    return request;
}

#else

LevelEditorRequest DrawLevelEditor(
    LevelEditorState&,
    const world::LevelDefinition&,
    const char*)
{
    return LevelEditorRequest::None;
}

#endif
}
