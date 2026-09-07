#pragma once

// Milestone 43: editor-only build-selector preference. Not Level Format,
// not authored data, and not reset by Reset Editor Layout.

#include "editor/EditorLayout.h"
#include "editor/EditorToolCommands.h"

#include <filesystem>
#include <string_view>

namespace editor
{
enum class EditorBuildTarget
{
    Debug,
    Development,
    Release,
    All,
};

inline constexpr EditorBuildTarget kDefaultEditorBuildTarget = EditorBuildTarget::Development;
inline constexpr std::string_view kEditorBuildSelectionFileName = "editor_build_selection.txt";

const char* EditorBuildTargetName(EditorBuildTarget target);
EditorBuildTarget ParseEditorBuildTarget(std::string_view text);
EditorToolKind EditorToolKindForBuildTarget(EditorBuildTarget target);

// Combo remains editable while a job runs. Changing it affects only the next
// Run; it never mutates the in-flight EditorToolRunner job.
inline bool EditorBuildSelectionEditableWhileRunning()
{
    return true;
}

std::filesystem::path MakeEditorBuildSelectionPath(
    const std::filesystem::path& userDataDirectory);
std::filesystem::path EditorBuildSelectionPath();

EditorBuildTarget LoadEditorBuildSelectionFromPath(const std::filesystem::path& path);
bool SaveEditorBuildSelectionToPath(
    const std::filesystem::path& path,
    EditorBuildTarget target);

EditorBuildTarget LoadEditorBuildSelection();
bool SaveEditorBuildSelection(EditorBuildTarget target);
}
