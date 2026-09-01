#pragma once

#include <filesystem>
#include <string_view>

namespace platform
{
inline constexpr std::string_view kTestCheckerLogicalId = "textures/test_checker.png";
inline constexpr std::string_view kTestStaticModelLogicalId = "models/test_static.glb";
inline constexpr std::string_view kRuntimeAssetDirectoryName = "assets";

std::filesystem::path ExecutableDirectory();
std::filesystem::path RuntimeAssetRoot();
std::filesystem::path RuntimeAssetPath(std::string_view logicalRelative);
}
