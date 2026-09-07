#include "editor/EditorBuildPreference.h"

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

const char* EditorBuildTargetName(EditorBuildTarget target)
{
    switch (target)
    {
    case EditorBuildTarget::Debug:
        return "Debug";
    case EditorBuildTarget::Development:
        return "Development";
    case EditorBuildTarget::Release:
        return "Release";
    case EditorBuildTarget::All:
        return "All";
    }
    return "Development";
}

EditorBuildTarget ParseEditorBuildTarget(std::string_view text)
{
    const std::string trimmed = TrimCopy(text);
    if (NamesEqualIgnoreCase(trimmed, "Debug"))
    {
        return EditorBuildTarget::Debug;
    }
    if (NamesEqualIgnoreCase(trimmed, "Development"))
    {
        return EditorBuildTarget::Development;
    }
    if (NamesEqualIgnoreCase(trimmed, "Release"))
    {
        return EditorBuildTarget::Release;
    }
    if (NamesEqualIgnoreCase(trimmed, "All"))
    {
        return EditorBuildTarget::All;
    }
    return kDefaultEditorBuildTarget;
}

EditorToolKind EditorToolKindForBuildTarget(EditorBuildTarget target)
{
    switch (target)
    {
    case EditorBuildTarget::Debug:
        return EditorToolKind::BuildDebug;
    case EditorBuildTarget::Development:
        return EditorToolKind::BuildDevelopment;
    case EditorBuildTarget::Release:
        return EditorToolKind::BuildRelease;
    case EditorBuildTarget::All:
        return EditorToolKind::BuildAll;
    }
    return EditorToolKind::BuildDevelopment;
}

std::filesystem::path MakeEditorBuildSelectionPath(const std::filesystem::path& userDataDirectory)
{
    if (userDataDirectory.empty() || !userDataDirectory.is_absolute())
    {
        return {};
    }

    return (userDataDirectory / std::string(kEditorLayoutProjectDirectoryName)
            / std::string(kEditorBuildSelectionFileName))
        .lexically_normal();
}

std::filesystem::path EditorBuildSelectionPath()
{
    return MakeEditorBuildSelectionPath(platform::UserDataDirectory());
}

EditorBuildTarget LoadEditorBuildSelectionFromPath(const std::filesystem::path& path)
{
    if (path.empty())
    {
        return kDefaultEditorBuildTarget;
    }

    std::error_code error;
    if (!std::filesystem::exists(path, error) || error)
    {
        return kDefaultEditorBuildTarget;
    }

    std::ifstream in(path);
    if (!in)
    {
        return kDefaultEditorBuildTarget;
    }

    std::string line;
    if (!std::getline(in, line))
    {
        return kDefaultEditorBuildTarget;
    }
    return ParseEditorBuildTarget(line);
}

bool SaveEditorBuildSelectionToPath(
    const std::filesystem::path& path,
    EditorBuildTarget target)
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
    out << EditorBuildTargetName(target) << '\n';
    return static_cast<bool>(out);
}

EditorBuildTarget LoadEditorBuildSelection()
{
    return LoadEditorBuildSelectionFromPath(EditorBuildSelectionPath());
}

bool SaveEditorBuildSelection(EditorBuildTarget target)
{
    if (!EnsureEditorLayoutDirectory())
    {
        return false;
    }
    return SaveEditorBuildSelectionToPath(EditorBuildSelectionPath(), target);
}
}
