#pragma once

#include <filesystem>
#include <string_view>

namespace platform
{
inline constexpr std::string_view kTestCheckerLogicalId = "textures/test_checker.png";
inline constexpr std::string_view kTestStaticModelLogicalId = "models/test_static.glb";
inline constexpr std::string_view kTestAuthoredModelLogicalId = "models/test_authored.glb";
inline constexpr std::string_view kTestTexturedModelLogicalId = "models/test_textured.glb";
inline constexpr std::string_view kRuntimeAssetDirectoryName = "assets";

std::filesystem::path ExecutableDirectory();
std::filesystem::path RuntimeAssetRoot();
std::filesystem::path RuntimeAssetPath(std::string_view logicalRelative);

// Writable per-user data root. Not the executable directory, not CWD, and
// not RuntimeAssetRoot(). Empty if the platform cannot resolve an absolute
// user-data path. Windows: FOLDERID_LocalAppData. Other platforms: not
// implemented in M29.
std::filesystem::path UserDataDirectory();
}
