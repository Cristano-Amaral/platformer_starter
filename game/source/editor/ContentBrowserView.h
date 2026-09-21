#pragma once

// Content Browser Thumbnails/List presentation. Editor tooling state only.
// Not Level Format and not asset authority.

#include "editor/EditorLayout.h"

#include <filesystem>
#include <string>
#include <string_view>

namespace editor
{
enum class ContentBrowserViewMode
{
    Thumbnails,
    List,
};

enum class ContentBrowserCollection
{
    AllAssets,
    Favorites,
    Models,
    Textures,
    Folders,
};

inline constexpr ContentBrowserViewMode kDefaultContentBrowserViewMode =
    ContentBrowserViewMode::Thumbnails;
inline constexpr ContentBrowserCollection kDefaultContentBrowserCollection =
    ContentBrowserCollection::AllAssets;
inline constexpr std::string_view kContentBrowserViewFileName =
    "editor_content_browser_view.txt";

struct ContentBrowserViewState
{
    ContentBrowserViewMode viewMode = kDefaultContentBrowserViewMode;
    ContentBrowserCollection collection = kDefaultContentBrowserCollection;
    std::string folderPath;
};

const char* ContentBrowserViewModeName(ContentBrowserViewMode mode);
ContentBrowserViewMode ParseContentBrowserViewMode(std::string_view text);
const char* ContentBrowserCollectionName(ContentBrowserCollection collection);
ContentBrowserCollection ParseContentBrowserCollection(std::string_view text);

std::filesystem::path MakeContentBrowserViewPath(
    const std::filesystem::path& userDataDirectory);
std::filesystem::path ContentBrowserViewPath();

ContentBrowserViewMode LoadContentBrowserViewModeFromPath(const std::filesystem::path& path);
bool SaveContentBrowserViewModeToPath(
    const std::filesystem::path& path,
    ContentBrowserViewMode mode);

ContentBrowserViewState LoadContentBrowserViewStateFromPath(const std::filesystem::path& path);
bool SaveContentBrowserViewStateToPath(
    const std::filesystem::path& path,
    const ContentBrowserViewState& state);

ContentBrowserViewMode LoadContentBrowserViewMode();
bool SaveContentBrowserViewMode(ContentBrowserViewMode mode);
ContentBrowserViewState LoadContentBrowserViewState();
bool SaveContentBrowserViewState(const ContentBrowserViewState& state);
}
