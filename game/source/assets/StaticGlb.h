#pragma once

// Static GLB container and compatibility checks shared by M47 import and
// catalog discovery. This is not a general glTF toolkit and not a runtime
// loader. Cooker copy remains byte-identical after a file passes these gates.

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>

namespace assets
{
inline constexpr std::string_view kStaticModelsLogicalDirectory = "models";
inline constexpr std::string_view kStaticGlbExtension = ".glb";
inline constexpr std::string_view kStaticGlbAssetType = "static_glb";
inline constexpr std::string_view kStaticGlbImportTempSuffix = ".importing.tmp";

enum class StaticGlbStatus
{
    Ok,
    Missing,
    NotAFile,
    UnsupportedExtension,
    UnsafeName,
    InvalidContainer,
    Incompatible,
};

const char* StaticGlbStatusName(StaticGlbStatus status);

struct StaticGlbValidation
{
    StaticGlbStatus status = StaticGlbStatus::InvalidContainer;
    std::string message;
};

bool HasStaticGlbExtension(std::string_view fileName);
bool IsSafeStaticGlbFileName(std::string_view fileName, std::string* reason = nullptr);
std::string CanonicalStaticModelIdentity(std::string_view fileName);
bool TryParseStaticModelIdentity(
    std::string_view canonicalIdentity,
    std::string& fileName,
    std::string* reason = nullptr);
std::string StaticModelDisplayName(std::string_view canonicalIdentity);

StaticGlbValidation ValidateStaticGlbBytes(std::span<const std::uint8_t> data);
StaticGlbValidation ValidateStaticGlbFile(const std::filesystem::path& path);
}
