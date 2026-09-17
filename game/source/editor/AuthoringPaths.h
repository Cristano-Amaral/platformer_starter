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
// Startup still loads staged Level 01. After M64 the current runtime identity
// may be another valid level such as level_02. No registry, no browser.
inline constexpr std::string_view kLevel01AuthoringLogicalId = "levels/level_01.level";

// True only when this configuration was built with an injected authoring root.
bool IsLevelAuthoringAvailable();

// Absolute canonical authored-source root (game/assets/source). Empty when
// authoring is unavailable for this configuration.
std::filesystem::path AuthoringSourceRoot();

// Absolute path under the authoring root for a logical relative identity, or
// empty when authoring is unavailable or the identity is unsafe.
std::filesystem::path AuthoringSourcePath(std::string_view logicalRelative);
// Absolute source path for a validated level identity such as level_01.
std::filesystem::path AuthoringLevelSourcePath(std::string_view levelId);
std::filesystem::path AuthoringLevel01SourcePath();
}
