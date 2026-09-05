#pragma once

// Developer-local Dear ImGui layout. Not a game save, not Level Format, not
// BEST. Path lives under platform::UserDataDirectory() / Platformer3D.

#include <filesystem>
#include <string_view>

namespace editor
{
inline constexpr std::string_view kEditorLayoutFileName = "editor_layout.ini";
// Same per-user project folder as M29 BEST. Layout files are not BEST files.
inline constexpr std::string_view kEditorLayoutProjectDirectoryName = "Platformer3D";

inline constexpr const char* kMetricsWindowName = "Platformer3D Metrics";
inline constexpr const char* kHierarchyWindowName = "Hierarchy";
inline constexpr const char* kInspectorWindowName = "Inspector";
inline constexpr const char* kLevelEditorWindowName = "Level Editor";
inline constexpr const char* kToolOutputWindowName = "Tool Output";

struct EditorWindowPlacement
{
    const char* name = "";
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct EditorLayoutDefaults
{
    EditorWindowPlacement metrics{};
    EditorWindowPlacement hierarchy{};
    EditorWindowPlacement inspector{};
    EditorWindowPlacement levelEditor{};
    EditorWindowPlacement toolOutput{};
};

// Joins an already-resolved user-data root. Empty/relative roots yield empty.
std::filesystem::path MakeEditorLayoutPath(const std::filesystem::path& userDataDirectory);

// Windows: %LOCALAPPDATA%\Platformer3D\editor_layout.ini. Empty if the
// platform cannot resolve an absolute user-data directory.
std::filesystem::path EditorLayoutPath();

// Creates Platformer3D under user-data when the layout path is usable.
// Missing parent is first-run, not an error. Returns false only when the path
// cannot be formed.
bool EnsureEditorLayoutDirectory();

EditorLayoutDefaults ComputeDefaultEditorLayout(float viewportWidth, float viewportHeight);
EditorWindowPlacement ClampEditorWindowPlacement(
    EditorWindowPlacement placement,
    float viewportWidth,
    float viewportHeight);

bool EditorWindowNeedsClamp(
    EditorWindowPlacement placement,
    float viewportWidth,
    float viewportHeight);

const EditorWindowPlacement* FindDefaultPlacement(
    const EditorLayoutDefaults& defaults,
    const char* windowName);
}
