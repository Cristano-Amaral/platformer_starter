#pragma once

// Resolves a runtime PNG identity to a loadable file. Staged
// (RuntimeAssetPath) is always preferred. An optional cooked root is a
// Development authoring preview fallback. An optional source root is a
// further Development last resort so an assigned catalog identity can
// preview before cook/stage; Release leaves both empty. Source PNGs are
// never shipping runtime authority. Identities stay portable.

#include "assets/RuntimePng.h"
#include "platform/RuntimePaths.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

namespace assets
{
enum class RuntimePngLoadSource
{
    Missing,
    Staged,
    Cooked,
    Source,
};

struct RuntimePngLoadResolution
{
    RuntimePngLoadSource source = RuntimePngLoadSource::Missing;
    std::filesystem::path path{};
};

inline bool RuntimePngFileExists(const std::filesystem::path& path)
{
    std::error_code error;
    return !path.empty() && path.is_absolute() && std::filesystem::is_regular_file(path, error)
        && !error;
}

inline std::filesystem::path RuntimePngPathUnderRoot(
    const std::filesystem::path& root,
    std::string_view identity)
{
    if (root.empty() || !root.is_absolute() || !RuntimePngIdentityIsValid(identity))
    {
        return {};
    }

    const std::filesystem::path normalizedRoot = root.lexically_normal();
    const std::filesystem::path resolved = (normalizedRoot / std::string(identity)).lexically_normal();
    if (!resolved.is_absolute())
    {
        return {};
    }
    const std::filesystem::path relative = resolved.lexically_relative(normalizedRoot);
    if (relative.empty() || relative == std::filesystem::path("."))
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
    return resolved;
}

inline std::filesystem::path CookedRuntimePngPath(
    const std::filesystem::path& cookedRoot,
    std::string_view identity)
{
    return RuntimePngPathUnderRoot(cookedRoot, identity);
}

// stagedRootOverride is test-only. Production callers leave it empty so
// platform::RuntimeAssetPath remains the staged authority. sourceRoot is a
// Development authoring last resort and must stay empty in Release.
inline RuntimePngLoadResolution ResolveRuntimePngLoadFile(
    std::string_view identity,
    const std::filesystem::path& cookedRoot,
    const std::filesystem::path& stagedRootOverride = {},
    const std::filesystem::path& sourceRoot = {})
{
    RuntimePngLoadResolution resolution{};
    if (!RuntimePngIdentityIsValid(identity))
    {
        return resolution;
    }

    std::filesystem::path staged;
    if (!stagedRootOverride.empty())
    {
        staged = (stagedRootOverride / std::string(identity)).lexically_normal();
        if (!staged.is_absolute())
        {
            staged.clear();
        }
    }
    else
    {
        staged = platform::RuntimeAssetPath(identity);
    }
    if (RuntimePngFileExists(staged))
    {
        resolution.source = RuntimePngLoadSource::Staged;
        resolution.path = staged;
        return resolution;
    }

    const std::filesystem::path cooked = CookedRuntimePngPath(cookedRoot, identity);
    if (RuntimePngFileExists(cooked))
    {
        resolution.source = RuntimePngLoadSource::Cooked;
        resolution.path = cooked;
        return resolution;
    }

    const std::filesystem::path source = RuntimePngPathUnderRoot(sourceRoot, identity);
    if (RuntimePngFileExists(source))
    {
        resolution.source = RuntimePngLoadSource::Source;
        resolution.path = source;
        return resolution;
    }

    return resolution;
}

inline bool RuntimePngLoadFileIsAvailable(const RuntimePngLoadResolution& resolution)
{
    return resolution.source != RuntimePngLoadSource::Missing && RuntimePngFileExists(resolution.path);
}

inline const char* RuntimeTerrainTextureUnavailableTooltip()
{
    return "Runtime Terrain texture unavailable; using fallback.";
}
}
