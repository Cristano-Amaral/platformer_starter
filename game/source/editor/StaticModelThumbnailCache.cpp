#include "editor/StaticModelThumbnailCache.h"

#include "assets/StaticGlb.h"
#include "editor/EditorLayout.h"
#include "editor/EditorMath.h"
#include "platform/RuntimePaths.h"

#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <system_error>

namespace editor
{
namespace
{
constexpr std::uint64_t kFnvOffset = 14695981039346656037ull;
constexpr std::uint64_t kFnvPrime = 1099511628211ull;
constexpr float kMinExtent = 1.0e-4f;
constexpr float kMinRadius = 0.05f;

bool IsHexCacheKey(std::string_view key)
{
    if (key.size() != 16)
    {
        return false;
    }
    for (char ch : key)
    {
        if (!std::isxdigit(static_cast<unsigned char>(ch)))
        {
            return false;
        }
    }
    return true;
}

std::string Hex16(std::uint64_t value)
{
    static constexpr char kDigits[] = "0123456789abcdef";
    std::string hex(16, '0');
    for (int i = 15; i >= 0; --i)
    {
        hex[static_cast<std::size_t>(i)] = kDigits[value & 0xFull];
        value >>= 4;
    }
    return hex;
}

bool RelativeStaysInsideRoot(
    const std::filesystem::path& root,
    const std::filesystem::path& relative)
{
    if (root.empty() || !root.is_absolute() || relative.empty() || relative.is_absolute()
        || relative.has_root_name())
    {
        return false;
    }
    for (const std::filesystem::path& part : relative)
    {
        if (part == "..")
        {
            return false;
        }
    }
    const std::filesystem::path resolved = (root / relative).lexically_normal();
    const std::filesystem::path relativeToRoot = resolved.lexically_relative(root);
    if (relativeToRoot.empty())
    {
        return false;
    }
    for (const std::filesystem::path& part : relativeToRoot)
    {
        if (part == "..")
        {
            return false;
        }
    }
    return true;
}

std::string ReadKeyValue(const std::string& line, std::string_view key)
{
    const std::string prefix = std::string(key) + "=";
    if (line.size() < prefix.size() || line.compare(0, prefix.size(), prefix) != 0)
    {
        return {};
    }
    return line.substr(prefix.size());
}
}

std::uint64_t ThumbnailCacheKeyHash(std::string_view canonicalIdentity)
{
    std::uint64_t hash = kFnvOffset;
    for (unsigned char byte : canonicalIdentity)
    {
        hash ^= byte;
        hash *= kFnvPrime;
    }
    return hash;
}

std::string ThumbnailCacheKeyHex(std::string_view canonicalIdentity)
{
    return Hex16(ThumbnailCacheKeyHash(canonicalIdentity));
}

std::filesystem::path MakeThumbnailCacheRoot(const std::filesystem::path& userDataDirectory)
{
    if (userDataDirectory.empty() || !userDataDirectory.is_absolute())
    {
        return {};
    }
    return (userDataDirectory / std::string(kEditorLayoutProjectDirectoryName)
            / std::string(kStaticModelThumbnailCacheDirectoryName))
        .lexically_normal();
}

std::filesystem::path ThumbnailCacheRoot()
{
    return MakeThumbnailCacheRoot(platform::UserDataDirectory());
}

bool TryResolveThumbnailCachePaths(
    std::string_view canonicalIdentity,
    const std::filesystem::path& cacheRoot,
    std::filesystem::path& imagePath,
    std::filesystem::path& metaPath,
    std::string& error)
{
    imagePath.clear();
    metaPath.clear();
    error.clear();
    std::string fileName;
    if (!assets::TryParseStaticModelIdentity(canonicalIdentity, fileName, &error))
    {
        return false;
    }
    (void)fileName;
    if (cacheRoot.empty() || !cacheRoot.is_absolute())
    {
        error = "thumbnail cache root is missing or not absolute";
        return false;
    }
    const std::string key = ThumbnailCacheKeyHex(canonicalIdentity);
    if (!IsHexCacheKey(key))
    {
        error = "thumbnail cache key is not a safe hex identity";
        return false;
    }
    const std::filesystem::path imageRelative{key + ".png"};
    const std::filesystem::path metaRelative{key + ".meta"};
    if (!RelativeStaysInsideRoot(cacheRoot, imageRelative)
        || !RelativeStaysInsideRoot(cacheRoot, metaRelative))
    {
        error = "thumbnail cache mapping would leave the authorized root";
        return false;
    }
    imagePath = (cacheRoot / imageRelative).lexically_normal();
    metaPath = (cacheRoot / metaRelative).lexically_normal();
    return true;
}

bool ReadThumbnailSourceStamp(const std::filesystem::path& sourcePath, ThumbnailSourceStamp& stamp)
{
    stamp = {};
    std::error_code error;
    if (!std::filesystem::is_regular_file(sourcePath, error) || error)
    {
        return false;
    }
    const auto writeTime = std::filesystem::last_write_time(sourcePath, error);
    if (error)
    {
        return false;
    }
    stamp.writeTimeTicks =
        static_cast<std::int64_t>(writeTime.time_since_epoch().count());
    stamp.size = std::filesystem::file_size(sourcePath, error);
    return !error;
}

bool SourceStampsEqual(const ThumbnailSourceStamp& a, const ThumbnailSourceStamp& b)
{
    return a.writeTimeTicks == b.writeTimeTicks && a.size == b.size;
}

bool WriteThumbnailCacheMeta(const std::filesystem::path& metaPath, const ThumbnailCacheMeta& meta)
{
    if (metaPath.empty())
    {
        return false;
    }
    std::error_code error;
    std::filesystem::create_directories(metaPath.parent_path(), error);
    if (error)
    {
        return false;
    }
    std::ofstream out(metaPath, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        return false;
    }
    out << "schema=" << meta.schemaVersion << '\n';
    out << "identity=" << meta.canonicalIdentity << '\n';
    out << "mtime=" << meta.stamp.writeTimeTicks << '\n';
    out << "size=" << meta.stamp.size << '\n';
    return static_cast<bool>(out);
}

bool ReadThumbnailCacheMeta(const std::filesystem::path& metaPath, ThumbnailCacheMeta& meta)
{
    meta = {};
    std::ifstream in(metaPath);
    if (!in)
    {
        return false;
    }
    std::string line;
    bool haveSchema = false;
    bool haveIdentity = false;
    bool haveMtime = false;
    bool haveSize = false;
    while (std::getline(in, line))
    {
        if (line.empty() || line.back() == '\r')
        {
            if (!line.empty())
            {
                line.pop_back();
            }
        }
        const std::string schemaValue = ReadKeyValue(line, "schema");
        if (!schemaValue.empty())
        {
            std::istringstream stream(schemaValue);
            stream >> meta.schemaVersion;
            haveSchema = !stream.fail();
            continue;
        }
        const std::string identityValue = ReadKeyValue(line, "identity");
        if (!identityValue.empty())
        {
            meta.canonicalIdentity = identityValue;
            haveIdentity = true;
            continue;
        }
        const std::string mtimeValue = ReadKeyValue(line, "mtime");
        if (!mtimeValue.empty())
        {
            std::istringstream stream(mtimeValue);
            stream >> meta.stamp.writeTimeTicks;
            haveMtime = !stream.fail();
            continue;
        }
        const std::string sizeValue = ReadKeyValue(line, "size");
        if (!sizeValue.empty())
        {
            std::istringstream stream(sizeValue);
            stream >> meta.stamp.size;
            haveSize = !stream.fail();
        }
    }
    return haveSchema && haveIdentity && haveMtime && haveSize;
}

bool ThumbnailCacheIsValid(
    std::string_view canonicalIdentity,
    const ThumbnailSourceStamp& sourceStamp,
    const std::filesystem::path& cacheRoot)
{
    std::filesystem::path imagePath;
    std::filesystem::path metaPath;
    std::string error;
    if (!TryResolveThumbnailCachePaths(canonicalIdentity, cacheRoot, imagePath, metaPath, error))
    {
        return false;
    }
    std::error_code existsError;
    if (!std::filesystem::is_regular_file(imagePath, existsError) || existsError)
    {
        return false;
    }
    ThumbnailCacheMeta meta{};
    if (!ReadThumbnailCacheMeta(metaPath, meta))
    {
        return false;
    }
    return meta.schemaVersion == kStaticModelThumbnailSchemaVersion
        && meta.canonicalIdentity == canonicalIdentity
        && SourceStampsEqual(meta.stamp, sourceStamp);
}

bool RemoveThumbnailCacheEntry(
    std::string_view canonicalIdentity,
    const std::filesystem::path& cacheRoot)
{
    std::filesystem::path imagePath;
    std::filesystem::path metaPath;
    std::string error;
    if (!TryResolveThumbnailCachePaths(canonicalIdentity, cacheRoot, imagePath, metaPath, error))
    {
        return false;
    }
    std::error_code removeError;
    std::filesystem::remove(imagePath, removeError);
    std::filesystem::remove(metaPath, removeError);
    return true;
}

ThumbnailEnsureDecision ClassifyThumbnailEnsure(
    bool hasReadyTexture,
    bool failed,
    bool haveSourceStamp,
    const ThumbnailSourceStamp& entryStamp,
    const ThumbnailSourceStamp& sourceStamp)
{
    if (haveSourceStamp && hasReadyTexture && SourceStampsEqual(entryStamp, sourceStamp))
    {
        return ThumbnailEnsureDecision::ReuseReady;
    }
    if (haveSourceStamp && failed && SourceStampsEqual(entryStamp, sourceStamp))
    {
        return ThumbnailEnsureDecision::ReuseFailed;
    }
    return ThumbnailEnsureDecision::Refresh;
}

ThumbnailCameraFrame MakeThumbnailCameraFrame(const ThumbnailModelBounds& bounds)
{
    core::Vec3 min = bounds.min;
    core::Vec3 max = bounds.max;
    if (!(max.x >= min.x) || !(max.y >= min.y) || !(max.z >= min.z)
        || !std::isfinite(min.x) || !std::isfinite(min.y) || !std::isfinite(min.z)
        || !std::isfinite(max.x) || !std::isfinite(max.y) || !std::isfinite(max.z))
    {
        min = {-0.5f, -0.5f, -0.5f};
        max = {0.5f, 0.5f, 0.5f};
    }

    core::Vec3 extent{max.x - min.x, max.y - min.y, max.z - min.z};
    if (extent.x < kMinExtent && extent.y < kMinExtent && extent.z < kMinExtent)
    {
        min = {-0.5f, -0.5f, -0.5f};
        max = {0.5f, 0.5f, 0.5f};
        extent = {1.0f, 1.0f, 1.0f};
    }
    else
    {
        if (extent.x < kMinExtent)
        {
            extent.x = kMinExtent;
        }
        if (extent.y < kMinExtent)
        {
            extent.y = kMinExtent;
        }
        if (extent.z < kMinExtent)
        {
            extent.z = kMinExtent;
        }
    }

    const core::Vec3 center{
        (min.x + max.x) * 0.5f,
        (min.y + max.y) * 0.5f,
        (min.z + max.z) * 0.5f};
    const float radius = Length(Scale(extent, 0.5f));
    const float paddedRadius =
        (radius > kMinRadius ? radius : kMinRadius) * kStaticModelThumbnailBoundsPadding;
    const float halfFov = kStaticModelThumbnailFieldOfViewY * 0.5f * kDegreesToRadians;
    const float tangent = std::tan(halfFov);
    float distance = paddedRadius / (tangent > 1.0e-4f ? tangent : 1.0e-4f);
    if (!std::isfinite(distance) || distance < paddedRadius * 1.5f)
    {
        distance = paddedRadius * 2.5f;
    }

    const core::Vec3 direction = NormalizeOr({1.0f, 0.85f, 1.0f}, {0.0f, 0.0f, 1.0f});
    ThumbnailCameraFrame frame{};
    frame.target = center;
    frame.position = center + Scale(direction, distance);
    frame.up = {0.0f, 1.0f, 0.0f};
    frame.fieldOfViewY = kStaticModelThumbnailFieldOfViewY;
    frame.nearPlane = distance - paddedRadius * 1.25f;
    if (!(frame.nearPlane > 0.01f) || !std::isfinite(frame.nearPlane))
    {
        frame.nearPlane = 0.05f;
    }
    frame.farPlane = distance + paddedRadius * 3.0f;
    if (!(frame.farPlane > frame.nearPlane + 0.1f) || !std::isfinite(frame.farPlane))
    {
        frame.farPlane = frame.nearPlane + paddedRadius * 8.0f + 10.0f;
    }
    return frame;
}
}
