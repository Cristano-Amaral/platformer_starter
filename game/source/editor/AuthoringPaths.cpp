#include "editor/AuthoringPaths.h"

#include "world/LevelIdentity.h"

#include <string>

namespace editor
{
bool IsLevelAuthoringAvailable()
{
    return !AuthoringSourceRoot().empty();
}

std::filesystem::path AuthoringSourceRoot()
{
#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING) && defined(PLATFORMER_AUTHORING_SOURCE_ROOT)
    // The literal exists only in configurations CMake injected it into.
    std::filesystem::path root =
        std::filesystem::path{PLATFORMER_AUTHORING_SOURCE_ROOT}.lexically_normal();
    if (root.empty() || !root.is_absolute())
    {
        return {};
    }
    return root;
#else
    return {};
#endif
}

std::filesystem::path AuthoringSourcePath(std::string_view logicalRelative)
{
    const std::filesystem::path root = AuthoringSourceRoot();
    if (root.empty())
    {
        return {};
    }

    std::filesystem::path relative =
        std::filesystem::path{std::string(logicalRelative)}.lexically_normal();
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

    std::filesystem::path resolved = (root / relative).lexically_normal();
    if (!resolved.is_absolute())
    {
        return {};
    }
    return resolved;
}

std::filesystem::path AuthoringLevelSourcePath(std::string_view levelId)
{
    const std::string logical = world::MakeRuntimeLevelLogicalId(levelId);
    if (logical.empty())
    {
        return {};
    }
    return AuthoringSourcePath(logical);
}

std::filesystem::path AuthoringLevel01SourcePath()
{
    return AuthoringLevelSourcePath(world::kLevel01Id);
}
}
