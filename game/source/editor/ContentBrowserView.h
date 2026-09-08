#pragma once

// Content Browser Thumbnails/List presentation. Editor tooling state only.
// Not Level Format and not asset authority.

#include "editor/EditorLayout.h"

#include <filesystem>
#include <string_view>

namespace editor
{
enum class ContentBrowserViewMode
{
    Thumbnails,
    List,
};

inline constexpr ContentBrowserViewMode kDefaultContentBrowserViewMode =
    ContentBrowserViewMode::Thumbnails;
inline constexpr std::string_view kContentBrowserViewFileName =
    "editor_content_browser_view.txt";

const char* ContentBrowserViewModeName(ContentBrowserViewMode mode);
ContentBrowserViewMode ParseContentBrowserViewMode(std::string_view text);

std::filesystem::path MakeContentBrowserViewPath(
    const std::filesystem::path& userDataDirectory);
std::filesystem::path ContentBrowserViewPath();

ContentBrowserViewMode LoadContentBrowserViewModeFromPath(const std::filesystem::path& path);
bool SaveContentBrowserViewModeToPath(
    const std::filesystem::path& path,
    ContentBrowserViewMode mode);

ContentBrowserViewMode LoadContentBrowserViewMode();
bool SaveContentBrowserViewMode(ContentBrowserViewMode mode);
}
