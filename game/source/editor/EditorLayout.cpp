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
constexpr float kLevelsHeight = 160.0f;
constexpr float kModelPreviewHeight = 220.0f;
constexpr float kItemDatabaseWidth = 720.0f;
constexpr float kItemDatabaseHeight = 480.0f;

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
    const float hierarchyTop = defaults.metrics.y + defaults.metrics.height + kMargin;
    const float stackAvailable = std::max(0.0f, defaults.toolOutput.y - hierarchyTop - kMargin);
    float hierarchyHeight = std::min(kHierarchyHeight, height * 0.55f);
    float levelsHeight = std::min(kLevelsHeight, height * 0.24f);
    float contentHeight = std::min(kContentBrowserHeight, height * 0.26f);
    const float stackGaps = kMargin * 2.0f;
    const float stackNeeded = hierarchyHeight + levelsHeight + contentHeight + stackGaps;
    if (stackNeeded > stackAvailable && stackNeeded > 0.0f)
    {
        const float scale = stackAvailable / stackNeeded;
        hierarchyHeight *= scale;
        levelsHeight *= scale;
        contentHeight *= scale;
    }
    defaults.hierarchy = {
        kHierarchyWindowName,
        kMargin,
        hierarchyTop,
        panelWidth,
        hierarchyHeight};
    defaults.levels = {
        kLevelsWindowName,
        kMargin,
        defaults.hierarchy.y + hierarchyHeight + kMargin,
        panelWidth,
        levelsHeight};
    defaults.contentBrowser = {
        kContentBrowserWindowName,
        kMargin,
        defaults.levels.y + levelsHeight + kMargin,
        panelWidth,
        contentHeight};
    const float previewHeight = std::min(kModelPreviewHeight, height * 0.32f);
    const float previewWidth = std::min(
        panelWidth + 40.0f,
        std::max(240.0f, defaults.inspector.x - kMargin * 2.0f - panelWidth));
    float previewY = defaults.contentBrowser.y + contentHeight - previewHeight;
    if (previewY < kMargin)
    {
        previewY = defaults.contentBrowser.y;
    }
    defaults.modelPreview = {
        kModelPreviewWindowName,
        defaults.contentBrowser.x + defaults.contentBrowser.width + kMargin,
        previewY,
        previewWidth,
        previewHeight};
    const float itemDatabaseWidth = std::min(kItemDatabaseWidth, std::max(420.0f, width * 0.55f));
    const float itemDatabaseHeight = std::min(kItemDatabaseHeight, height * 0.70f);
    defaults.itemDatabase = {
        kItemDatabaseWindowName,
        std::max(kMargin, (width - itemDatabaseWidth) * 0.5f),
        std::max(kMargin, (height - itemDatabaseHeight) * 0.5f),
        itemDatabaseWidth,
        itemDatabaseHeight};

    defaults.metrics = ClampEditorWindowPlacement(defaults.metrics, width, height);
    defaults.hierarchy = ClampEditorWindowPlacement(defaults.hierarchy, width, height);
    defaults.inspector = ClampEditorWindowPlacement(defaults.inspector, width, height);
    defaults.levelEditor = ClampEditorWindowPlacement(defaults.levelEditor, width, height);
    defaults.objectPalette = ClampEditorWindowPlacement(defaults.objectPalette, width, height);
    defaults.contentBrowser = ClampEditorWindowPlacement(defaults.contentBrowser, width, height);
    defaults.levels = ClampEditorWindowPlacement(defaults.levels, width, height);
    defaults.modelPreview = ClampEditorWindowPlacement(defaults.modelPreview, width, height);
    defaults.itemDatabase = ClampEditorWindowPlacement(defaults.itemDatabase, width, height);
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
    if (std::strcmp(windowName, kLevelsWindowName) == 0)
    {
        return &defaults.levels;
    }
    if (std::strcmp(windowName, kModelPreviewWindowName) == 0)
    {
        return &defaults.modelPreview;
    }
    if (std::strcmp(windowName, kItemDatabaseWindowName) == 0)
    {
        return &defaults.itemDatabase;
    }
    if (std::strcmp(windowName, kToolOutputWindowName) == 0)
    {
        return &defaults.toolOutput;
    }
    return nullptr;
}
}
