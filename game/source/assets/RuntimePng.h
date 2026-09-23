#pragma once

// Portable runtime PNG identity helpers. Same logical-id convention as
// models/<file>.glb: textures/<file>.png, posix separators, never an absolute
// path. Discovery lives in SourceTextureCatalog. Cooker/staging remain the
// runtime availability path. This is not a generalized AssetManager.

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace assets
{
inline constexpr std::string_view kRuntimeTexturesLogicalDirectory = "textures";
inline constexpr std::string_view kRuntimePngExtension = ".png";
inline constexpr std::string_view kRuntimePngNoneToken = "-";
inline constexpr std::string_view kRuntimePngAssetType = "runtime_png";
inline constexpr std::string_view kRuntimePngImportTempSuffix = ".importing.tmp";
inline constexpr std::string_view kRuntimePngRecipe = "runtime_png.max512.lanczos.v1";
inline constexpr int kRuntimePngMaxDimension = 512;

inline bool HasRuntimePngExtension(std::string_view fileName)
{
    return fileName.size() > kRuntimePngExtension.size()
        && fileName.ends_with(kRuntimePngExtension);
}

inline bool RuntimePngStemIsWindowsDeviceName(std::string_view stem)
{
    std::string upper;
    upper.reserve(stem.size());
    for (char ch : stem)
    {
        upper.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
    }
    const auto isDevice = [&](std::string_view name) {
        if (upper == name)
        {
            return true;
        }
        if (upper.size() == name.size() + 1 && upper.starts_with(name) && upper.back() >= '1'
            && upper.back() <= '9')
        {
            return name == "COM" || name == "LPT";
        }
        return false;
    };
    return isDevice("CON") || isDevice("PRN") || isDevice("AUX") || isDevice("NUL")
        || isDevice("COM") || isDevice("LPT");
}

// Terrain material identity is one Level-format token, so spaces are unsafe.
inline bool IsSafeRuntimePngFileName(std::string_view fileName, std::string* reason = nullptr)
{
    const auto fail = [&](const char* text) {
        if (reason != nullptr)
        {
            *reason = text;
        }
        return false;
    };
    if (fileName.empty())
    {
        return fail("file name is empty");
    }
    if (!HasRuntimePngExtension(fileName))
    {
        return fail("file name must end with .png");
    }
    const std::string_view stem = fileName.substr(0, fileName.size() - kRuntimePngExtension.size());
    if (stem.empty() || stem == "." || stem == "..")
    {
        return fail("file name stem is invalid");
    }
    if (stem.starts_with('.'))
    {
        return fail("hidden file names are not runtime textures");
    }
    for (char ch : fileName)
    {
        const unsigned char byte = static_cast<unsigned char>(ch);
        if (byte < 32 || ch == '/' || ch == '\\' || ch == ':' || ch == '*' || ch == '?'
            || ch == '"' || ch == '<' || ch == '>' || ch == '|' || ch == ' ')
        {
            return fail("file name contains an unsafe character");
        }
    }
    if (fileName.front() == ' ' || fileName.back() == ' ' || stem.back() == '.'
        || stem.back() == ' ')
    {
        return fail("file name has unsafe leading or trailing whitespace");
    }
    if (RuntimePngStemIsWindowsDeviceName(stem))
    {
        return fail("file name is a reserved Windows device name");
    }
    return true;
}

inline std::string CanonicalRuntimePngIdentity(std::string_view fileName)
{
    std::string identity;
    identity.reserve(kRuntimeTexturesLogicalDirectory.size() + 1 + fileName.size());
    identity.append(kRuntimeTexturesLogicalDirectory);
    identity.push_back('/');
    identity.append(fileName);
    return identity;
}

inline bool TryParseRuntimePngIdentity(
    std::string_view canonicalIdentity,
    std::string& fileName,
    std::string* reason = nullptr)
{
    fileName.clear();
    const auto fail = [&](const char* text) {
        if (reason != nullptr)
        {
            *reason = text;
        }
        return false;
    };
    if (canonicalIdentity.empty())
    {
        return fail("canonical identity is empty");
    }
    if (canonicalIdentity.find('\\') != std::string_view::npos)
    {
        return fail("canonical identity must use posix separators");
    }
    const std::string prefix = std::string(kRuntimeTexturesLogicalDirectory) + "/";
    if (!canonicalIdentity.starts_with(prefix))
    {
        return fail("canonical identity must be textures/<filename>.png");
    }
    const std::string_view remainder = canonicalIdentity.substr(prefix.size());
    std::string nameReason;
    if (!IsSafeRuntimePngFileName(remainder, &nameReason))
    {
        return fail(nameReason.empty() ? "canonical identity file name is unsafe" : nameReason.c_str());
    }
    if (CanonicalRuntimePngIdentity(remainder) != canonicalIdentity)
    {
        return fail("canonical identity is not normalized");
    }
    fileName = std::string(remainder);
    return true;
}

inline bool RuntimePngIdentityIsValid(std::string_view identity)
{
    std::string fileName;
    return TryParseRuntimePngIdentity(identity, fileName, nullptr);
}

inline std::string RuntimePngDisplayName(std::string_view canonicalIdentity)
{
    std::string fileName;
    if (!TryParseRuntimePngIdentity(canonicalIdentity, fileName, nullptr))
    {
        return {};
    }
    return fileName;
}

inline bool TryReadRuntimePngIhdr(
    const std::uint8_t* data,
    std::size_t size,
    int& width,
    int& height,
    int& bitDepth,
    int& colorType,
    std::string* reason = nullptr)
{
    width = 0;
    height = 0;
    bitDepth = 0;
    colorType = 0;
    const auto fail = [&](const char* text) {
        if (reason != nullptr)
        {
            *reason = text;
        }
        return false;
    };
    static constexpr std::uint8_t kSignature[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    if (data == nullptr || size < 8)
    {
        return fail("truncated PNG (too small to contain signature)");
    }
    for (int index = 0; index < 8; ++index)
    {
        if (data[index] != kSignature[index])
        {
            return fail("not a PNG (invalid signature)");
        }
    }
    if (size < 33)
    {
        return fail("truncated PNG (too small to contain IHDR)");
    }
    const std::uint32_t chunkLength = (static_cast<std::uint32_t>(data[8]) << 24)
        | (static_cast<std::uint32_t>(data[9]) << 16)
        | (static_cast<std::uint32_t>(data[10]) << 8)
        | static_cast<std::uint32_t>(data[11]);
    if (chunkLength != 13)
    {
        return fail("invalid PNG IHDR chunk length");
    }
    if (data[12] != 'I' || data[13] != 'H' || data[14] != 'D' || data[15] != 'R')
    {
        return fail("PNG missing IHDR as first chunk");
    }
    const std::uint32_t parsedWidth = (static_cast<std::uint32_t>(data[16]) << 24)
        | (static_cast<std::uint32_t>(data[17]) << 16)
        | (static_cast<std::uint32_t>(data[18]) << 8)
        | static_cast<std::uint32_t>(data[19]);
    const std::uint32_t parsedHeight = (static_cast<std::uint32_t>(data[20]) << 24)
        | (static_cast<std::uint32_t>(data[21]) << 16)
        | (static_cast<std::uint32_t>(data[22]) << 8)
        | static_cast<std::uint32_t>(data[23]);
    if (parsedWidth == 0 || parsedHeight == 0)
    {
        return fail("PNG has invalid dimensions");
    }
    if (parsedWidth > 65535 || parsedHeight > 65535)
    {
        return fail("PNG dimensions exceed the supported import range");
    }
    width = static_cast<int>(parsedWidth);
    height = static_cast<int>(parsedHeight);
    bitDepth = static_cast<int>(data[24]);
    colorType = static_cast<int>(data[25]);
    return true;
}

inline bool TryReadRuntimePngDimensions(
    const std::uint8_t* data,
    std::size_t size,
    int& width,
    int& height,
    std::string* reason = nullptr)
{
    int bitDepth = 0;
    int colorType = 0;
    return TryReadRuntimePngIhdr(data, size, width, height, bitDepth, colorType, reason);
}

inline bool RuntimePngDimensionsWithinRecipeLimit(int width, int height)
{
    return width > 0 && height > 0 && width <= kRuntimePngMaxDimension
        && height <= kRuntimePngMaxDimension;
}

// Development listing primitive used by SourceTextureCatalog. Not a persisted
// catalog and not used by Release. Non-recursive source/textures/*.png with
// valid identities. Filename rules only; byte validation belongs to import.
inline std::vector<std::string> CollectSourceRuntimePngIdentities(
    const std::filesystem::path& sourceRoot)
{
    std::vector<std::string> identities;
    if (sourceRoot.empty() || !sourceRoot.is_absolute())
    {
        return identities;
    }
    std::error_code error;
    const std::filesystem::path texturesRoot =
        (sourceRoot / std::string(kRuntimeTexturesLogicalDirectory)).lexically_normal();
    if (!std::filesystem::is_directory(texturesRoot, error) || error)
    {
        return identities;
    }

    const std::filesystem::directory_iterator end{};
    for (std::filesystem::directory_iterator it(texturesRoot, error);
         !error && it != end;
         it.increment(error))
    {
        if (error)
        {
            break;
        }
        const std::filesystem::path& path = it->path();
        if (!std::filesystem::is_regular_file(path, error) || error)
        {
            continue;
        }
        const std::string fileName = path.filename().string();
        if (!IsSafeRuntimePngFileName(fileName, nullptr))
        {
            continue;
        }
        identities.push_back(CanonicalRuntimePngIdentity(fileName));
    }

    std::sort(identities.begin(), identities.end());
    identities.erase(std::unique(identities.begin(), identities.end()), identities.end());
    return identities;
}
}
