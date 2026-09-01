#include "platform/RuntimePaths.h"

#include <string>

namespace platform
{
namespace detail
{
std::filesystem::path QueryExecutableDirectory();
}

std::filesystem::path ExecutableDirectory()
{
    std::filesystem::path directory = detail::QueryExecutableDirectory().lexically_normal();
    if (directory.empty() || !directory.is_absolute())
    {
        // Never fall back to the process CWD. A relative result would make
        // LoadTexture resolve against whatever directory launched the exe.
        return {};
    }

    return directory;
}

std::filesystem::path RuntimeAssetRoot()
{
    const std::filesystem::path executableDirectory = ExecutableDirectory();
    if (executableDirectory.empty())
    {
        return {};
    }

    return executableDirectory / std::string(kRuntimeAssetDirectoryName);
}

std::filesystem::path RuntimeAssetPath(std::string_view logicalRelative)
{
    const std::filesystem::path assetRoot = RuntimeAssetRoot();
    if (assetRoot.empty())
    {
        return {};
    }

    std::filesystem::path relative{std::string(logicalRelative)};
    relative = relative.lexically_normal();
    if (relative.empty() || relative.is_absolute() || relative.has_root_name())
    {
        return {};
    }

    for (const std::filesystem::path& part : relative)
    {
        if (part == "..")
        {
            return {};
        }
    }

    std::filesystem::path resolved = (assetRoot / relative).lexically_normal();
    if (!resolved.is_absolute())
    {
        return {};
    }

    return resolved;
}
}
