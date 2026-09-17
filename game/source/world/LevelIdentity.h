#pragma once

// Level identity grammar and staged runtime logical paths.
// Destination tokens reuse the `id` record rule. Unsafe path/traversal
// identities are rejected here before RuntimeAssetPath is consulted.

#include <filesystem>
#include <string>
#include <string_view>

namespace world
{
inline constexpr std::string_view kLevel01Id = "level_01";
inline constexpr std::string_view kLevel02Id = "level_02";
inline constexpr std::string_view kLevel01RuntimeLogicalId = "levels/level_01.level";
inline constexpr std::string_view kLevel02RuntimeLogicalId = "levels/level_02.level";
inline constexpr std::string_view kLevelRuntimeLogicalPrefix = "levels/";
inline constexpr std::string_view kLevelRuntimeExtension = ".level";

// Grammar rule for the `id` record and destination-bearing Level Goal token:
// [A-Za-z_][A-Za-z0-9_]*. Shared so the writer cannot emit an identifier the
// parser would reject, and so destination IDs cannot encode paths.
inline bool IsValidLevelIdToken(std::string_view token)
{
    if (token.empty())
    {
        return false;
    }
    const char first = token.front();
    if (!((first >= 'A' && first <= 'Z') || (first >= 'a' && first <= 'z') || first == '_'))
    {
        return false;
    }
    for (const char character : token)
    {
        const bool ok = (character >= 'A' && character <= 'Z')
            || (character >= 'a' && character <= 'z')
            || (character >= '0' && character <= '9')
            || character == '_';
        if (!ok)
        {
            return false;
        }
    }
    return true;
}

// Empty means terminal (M63). Non-empty must be a valid identity token.
inline bool IsValidNextLevelId(std::string_view token)
{
    return token.empty() || IsValidLevelIdToken(token);
}

// "level_02" -> "levels/level_02.level". Empty when the identity is unsafe.
inline std::string MakeRuntimeLevelLogicalId(std::string_view levelId)
{
    if (!IsValidLevelIdToken(levelId))
    {
        return {};
    }
    std::string logical;
    logical.reserve(
        kLevelRuntimeLogicalPrefix.size() + levelId.size() + kLevelRuntimeExtension.size());
    logical.append(kLevelRuntimeLogicalPrefix);
    logical.append(levelId);
    logical.append(kLevelRuntimeExtension);
    return logical;
}

inline bool RuntimeLevelPathStemMatchesId(
    const std::filesystem::path& path,
    std::string_view levelId)
{
    return !path.empty() && path.stem() == levelId;
}
}
