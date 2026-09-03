#pragma once

// Development-only authoring path boundary (Milestone 32).
//
// The runtime loads the staged copy <exe>/assets/levels/level_01.level through
// platform::RuntimeAssetPath. The editor must instead write the canonical
// project source under game/assets/source/. Those are intentionally different
// files and the staged copy is never treated as canonical source.
//
// The authoring root is injected by CMake as PLATFORMER_AUTHORING_SOURCE_ROOT
// for the Development configuration only. It is never derived from the process
// current working directory and never located by searching parent directories.
// Release compiles none of it, so the shipped binary carries no repository path.

#include <filesystem>
#include <string_view>

namespace editor
{
// The single level M32 can author. No registry, no browser.
inline constexpr std::string_view kLevel01AuthoringLogicalId = "levels/level_01.level";

// True only when this configuration was built with an injected authoring root.
bool IsLevelAuthoringAvailable();

// Absolute canonical authored-source root (game/assets/source). Empty when
// authoring is unavailable for this configuration.
std::filesystem::path AuthoringSourceRoot();

// Absolute path of the canonical Level 01 authoring file, or empty.
std::filesystem::path AuthoringSourcePath(std::string_view logicalRelative);
std::filesystem::path AuthoringLevel01SourcePath();
}
