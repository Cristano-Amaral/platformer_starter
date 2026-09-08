#include "editor/EditorLayout.h"

#include "platform/RuntimePaths.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <system_error>

namespace editor
{
namespace
{
constexpr float kMargin = 8.0f;
constexpr float kMinVisible = 48.0f;
constexpr float kDefaultPanelWidth = 340.0f;
constexpr float kMetricsHeight = 200.0f;
constexpr float kHierarchyHeight = 400.0f;
constexpr float kInspectorHeight = 360.0f;
constexpr float kLevelEditorHeight = 260.0f;
constexpr float kToolOutputHeight = 160.0f;
constexpr float kObjectPaletteHeight = 200.0f;
constexpr float kContentBrowserHeight = 180.0f;

float Clamped(float value, float minimum, float maximum)
{
    if (value < minimum)
    {
        return minimum;
    }
    if (value > maximum)
    {
        return maximum;
    }
    return value;
}
}

std::filesystem::path MakeEditorLayoutPath(const std::filesystem::path& userDataDirectory)
{
    if (userDataDirectory.empty() || !userDataDirectory.is_absolute())
    {
        return {};
    }

    return (userDataDirectory / std::string(kEditorLayoutProjectDirectoryName)
            / std::string(kEditorLayoutFileName))
        .lexically_normal();
}

std::filesystem::path EditorLayoutPath()
{
    return MakeEditorLayoutPath(platform::UserDataDirectory());
}

bool EnsureEditorLayoutDirectory()
{
    const std::filesystem::path path = EditorLayoutPath();
    if (path.empty())
    {
        return false;
    }

    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    return !error;
}

EditorLayoutDefaults ComputeDefaultEditorLayout(float viewportWidth, float viewportHeight)
{
    const float width = viewportWidth > 1.0f ? viewportWidth : 1280.0f;
    const float height = viewportHeight > 1.0f ? viewportHeight : 720.0f;
    const float panelWidth = std::min(kDefaultPanelWidth, std::max(240.0f, width * 0.28f));

    EditorLayoutDefaults defaults{};
    defaults.metrics = {
        kMetricsWindowName,
        kMargin,
        kMargin,
        panelWidth,
        std::min(kMetricsHeight, height * 0.28f)};
    defaults.inspector = {
        kInspectorWindowName,
        width - panelWidth - kMargin,
        kMargin,
        panelWidth,
        std::min(kInspectorHeight, height * 0.50f)};
    defaults.levelEditor = {
        kLevelEditorWindowName,
        defaults.inspector.x,
        defaults.inspector.y + defaults.inspector.height + kMargin,
        panelWidth,
        std::min(kLevelEditorHeight, height - defaults.inspector.height - kMargin * 3.0f)};
    defaults.objectPalette = {
        kObjectPaletteWindowName,
        defaults.inspector.x,
        defaults.levelEditor.y + defaults.levelEditor.height + kMargin,
        panelWidth,
        std::min(kObjectPaletteHeight, height * 0.30f)};
    const float toolHeight = std::min(kToolOutputHeight, height * 0.22f);
    defaults.toolOutput = {
        kToolOutputWindowName,
        kMargin,
        height - toolHeight - kMargin,
        width - kMargin * 2.0f,
        toolHeight};
    const float contentHeight = std::min(kContentBrowserHeight, height * 0.26f);
    defaults.contentBrowser = {
        kContentBrowserWindowName,
        kMargin,
        defaults.toolOutput.y - contentHeight - kMargin,
        panelWidth,
        contentHeight};
    const float hierarchyTop = defaults.metrics.y + defaults.metrics.height + kMargin;
    defaults.hierarchy = {
        kHierarchyWindowName,
        kMargin,
        hierarchyTop,
        panelWidth,
        std::min(kHierarchyHeight, defaults.contentBrowser.y - hierarchyTop - kMargin)};

    defaults.metrics = ClampEditorWindowPlacement(defaults.metrics, width, height);
    defaults.hierarchy = ClampEditorWindowPlacement(defaults.hierarchy, width, height);
    defaults.inspector = ClampEditorWindowPlacement(defaults.inspector, width, height);
    defaults.levelEditor = ClampEditorWindowPlacement(defaults.levelEditor, width, height);
    defaults.objectPalette = ClampEditorWindowPlacement(defaults.objectPalette, width, height);
    defaults.contentBrowser = ClampEditorWindowPlacement(defaults.contentBrowser, width, height);
    defaults.toolOutput = ClampEditorWindowPlacement(defaults.toolOutput, width, height);
    return defaults;
}

EditorWindowPlacement ClampEditorWindowPlacement(
    EditorWindowPlacement placement,
    float viewportWidth,
    float viewportHeight)
{
    const float width = viewportWidth > 1.0f ? viewportWidth : 1280.0f;
    const float height = viewportHeight > 1.0f ? viewportHeight : 720.0f;
    placement.width = Clamped(placement.width, kMinVisible, width);
    placement.height = Clamped(placement.height, kMinVisible, height);
    placement.x = Clamped(placement.x, -placement.width + kMinVisible, width - kMinVisible);
    placement.y = Clamped(placement.y, -placement.height + kMinVisible, height - kMinVisible);
    return placement;
}

bool EditorWindowNeedsClamp(
    EditorWindowPlacement placement,
    float viewportWidth,
    float viewportHeight)
{
    const EditorWindowPlacement clamped =
        ClampEditorWindowPlacement(placement, viewportWidth, viewportHeight);
    return clamped.x != placement.x || clamped.y != placement.y
        || clamped.width != placement.width || clamped.height != placement.height;
}

const EditorWindowPlacement* FindDefaultPlacement(
    const EditorLayoutDefaults& defaults,
    const char* windowName)
{
    if (windowName == nullptr)
    {
        return nullptr;
    }
    if (std::strcmp(windowName, kMetricsWindowName) == 0)
    {
        return &defaults.metrics;
    }
    if (std::strcmp(windowName, kHierarchyWindowName) == 0)
    {
        return &defaults.hierarchy;
    }
    if (std::strcmp(windowName, kInspectorWindowName) == 0)
    {
        return &defaults.inspector;
    }
    if (std::strcmp(windowName, kLevelEditorWindowName) == 0)
    {
        return &defaults.levelEditor;
    }
    if (std::strcmp(windowName, kObjectPaletteWindowName) == 0)
    {
        return &defaults.objectPalette;
    }
    if (std::strcmp(windowName, kContentBrowserWindowName) == 0)
    {
        return &defaults.contentBrowser;
    }
    if (std::strcmp(windowName, kToolOutputWindowName) == 0)
    {
        return &defaults.toolOutput;
    }
    return nullptr;
}
}
