#include "editor/ContentBrowserView.h"

#include "platform/RuntimePaths.h"

#include <cctype>
#include <fstream>
#include <string>
#include <system_error>

namespace editor
{
namespace
{
std::string TrimCopy(std::string_view text)
{
    std::size_t begin = 0;
    while (begin < text.size()
        && (text[begin] == ' ' || text[begin] == '\t' || text[begin] == '\r'
            || text[begin] == '\n'))
    {
        ++begin;
    }
    std::size_t end = text.size();
    while (end > begin
        && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\r'
            || text[end - 1] == '\n'))
    {
        --end;
    }
    return std::string(text.substr(begin, end - begin));
}

bool NamesEqualIgnoreCase(std::string_view left, std::string_view right)
{
    if (left.size() != right.size())
    {
        return false;
    }
    for (std::size_t index = 0; index < left.size(); ++index)
    {
        const unsigned char a = static_cast<unsigned char>(left[index]);
        const unsigned char b = static_cast<unsigned char>(right[index]);
        if (std::tolower(a) != std::tolower(b))
        {
            return false;
        }
    }
    return true;
}
}

const char* ContentBrowserViewModeName(ContentBrowserViewMode mode)
{
    switch (mode)
    {
    case ContentBrowserViewMode::Thumbnails:
        return "Thumbnails";
    case ContentBrowserViewMode::List:
        return "List";
    }
    return "Thumbnails";
}

ContentBrowserViewMode ParseContentBrowserViewMode(std::string_view text)
{
    const std::string trimmed = TrimCopy(text);
    if (NamesEqualIgnoreCase(trimmed, "List"))
    {
        return ContentBrowserViewMode::List;
    }
    return kDefaultContentBrowserViewMode;
}

std::filesystem::path MakeContentBrowserViewPath(const std::filesystem::path& userDataDirectory)
{
    if (userDataDirectory.empty() || !userDataDirectory.is_absolute())
    {
        return {};
    }
    return (userDataDirectory / std::string(kEditorLayoutProjectDirectoryName)
            / std::string(kContentBrowserViewFileName))
        .lexically_normal();
}

std::filesystem::path ContentBrowserViewPath()
{
    return MakeContentBrowserViewPath(platform::UserDataDirectory());
}

ContentBrowserViewMode LoadContentBrowserViewModeFromPath(const std::filesystem::path& path)
{
    if (path.empty())
    {
        return kDefaultContentBrowserViewMode;
    }
    std::error_code error;
    if (!std::filesystem::exists(path, error) || error)
    {
        return kDefaultContentBrowserViewMode;
    }
    std::ifstream in(path);
    if (!in)
    {
        return kDefaultContentBrowserViewMode;
    }
    std::string line;
    if (!std::getline(in, line))
    {
        return kDefaultContentBrowserViewMode;
    }
    return ParseContentBrowserViewMode(line);
}

bool SaveContentBrowserViewModeToPath(
    const std::filesystem::path& path,
    ContentBrowserViewMode mode)
{
    if (path.empty())
    {
        return false;
    }
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error)
    {
        return false;
    }
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        return false;
    }
    out << ContentBrowserViewModeName(mode) << '\n';
    return static_cast<bool>(out);
}

ContentBrowserViewMode LoadContentBrowserViewMode()
{
    return LoadContentBrowserViewModeFromPath(ContentBrowserViewPath());
}

bool SaveContentBrowserViewMode(ContentBrowserViewMode mode)
{
    if (!EnsureEditorLayoutDirectory())
    {
        return false;
    }
    return SaveContentBrowserViewModeToPath(ContentBrowserViewPath(), mode);
}
}
