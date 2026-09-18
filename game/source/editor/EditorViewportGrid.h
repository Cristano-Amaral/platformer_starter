#pragma once

// Milestone 77: Development-editor world-space XZ viewport grid. Spacing and
// line generation are callable without ImGui. This is not a debug-draw
// framework, command protocol, Agent API, or authored Level object.

#include "core/Vec3.h"
#include "editor/EditorSnap.h"

#include <filesystem>
#include <string>
#include <string_view>

namespace editor
{
inline constexpr int kEditorViewportGridMajorCadence = 4;
inline constexpr int kEditorViewportGridMaxLinesPerAxis = 96;
inline constexpr int kEditorViewportGridMaxLines = 192;
inline constexpr float kEditorViewportGridMinHalfExtent = 6.0f;
inline constexpr float kEditorViewportGridMaxHalfExtent = 24.0f;
inline constexpr float kEditorViewportGridY = 0.0f;
// Render-only lift above canonical Ground top at Y = 0. Authored coordinates,
// physics, and picking stay at Y = 0.
inline constexpr float kEditorViewportGridRenderYOffset = 0.008f;

inline constexpr const char* kEditorViewportGridIniTypeName = "Platformer3D.ViewportGrid";
inline constexpr const char* kEditorViewportGridIniEntryName = "Settings";

enum class EditorViewportGridLineKind
{
    Minor = 0,
    Major = 1,
    AxisX = 2,
    AxisZ = 3,
};

struct EditorViewportGridPreferences
{
    bool visible = true;
};

inline EditorViewportGridPreferences MakeDefaultEditorViewportGridPreferences()
{
    return {};
}

// Authoring-grid draw policy. Gameplay/Release never render a world grid.
// Visibility only applies while the Development editor (F2) is active.
inline bool GameplayRendersWorldGrid()
{
    return false;
}

inline bool EditorViewportGridShouldDraw(bool editorActive, bool gridVisible)
{
    return editorActive && gridVisible;
}

struct EditorViewportGridLine
{
    core::Vec3 start{};
    core::Vec3 end{};
    EditorViewportGridLineKind kind = EditorViewportGridLineKind::Minor;
};

struct EditorViewportGridLines
{
    EditorViewportGridLine items[kEditorViewportGridMaxLines]{};
    int count = 0;
};

struct EditorViewportGridGenerateRequest
{
    float translateIncrement = kDefaultTranslateSnapIncrement;
    float focusX = 0.0f;
    float focusZ = 0.0f;
    float halfExtent = 12.0f;
};

float EffectiveEditorViewportGridMinorSpacing(float translateIncrement);
int EditorViewportGridVisualStride(float minorSpacing, float halfExtent);
EditorViewportGridLineKind ClassifyEditorViewportGridIndex(int minorIndex);

bool EditorViewportGridIsWorldMultiple(float value, float spacing);
bool EditorViewportGridIncludesOriginAxis(
    const EditorViewportGridLines& lines,
    EditorViewportGridLineKind kind);

EditorViewportGridGenerateRequest MakeEditorViewportGridGenerateRequest(
    float translateIncrement,
    core::Vec3 cameraPosition,
    core::Vec3 cameraTarget);

EditorViewportGridLines GenerateEditorViewportGridLines(
    const EditorViewportGridGenerateRequest& request);

bool ParseEditorViewportGridPreferenceLine(
    EditorViewportGridPreferences& preferences,
    std::string_view line);
EditorViewportGridPreferences ParseEditorViewportGridPreferencesFromLayoutText(
    std::string_view layoutText);
std::string SerializeEditorViewportGridPreferencesSection(
    const EditorViewportGridPreferences& preferences);
std::string MergeEditorViewportGridPreferencesIntoLayoutText(
    std::string_view layoutText,
    const EditorViewportGridPreferences& preferences);

EditorViewportGridPreferences LoadEditorViewportGridPreferencesFromLayoutPath(
    const std::filesystem::path& path);
bool SaveEditorViewportGridPreferencesToLayoutPath(
    const std::filesystem::path& path,
    const EditorViewportGridPreferences& preferences);

void BindEditorViewportGridPreferences(EditorViewportGridPreferences* preferences);
EditorViewportGridPreferences* BoundEditorViewportGridPreferences();
}
