#pragma once

// Derived thumbnail cache identity and invalidation. Not asset authority,
// not cooked/staged runtime content, and not ImGui.

#include "core/Vec3.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace editor
{
inline constexpr int kStaticModelThumbnailSchemaVersion = 1;
inline constexpr int kStaticModelThumbnailWidth = 128;
inline constexpr int kStaticModelThumbnailHeight = 128;
inline constexpr float kStaticModelThumbnailFieldOfViewY = 40.0f;
inline constexpr float kStaticModelThumbnailBoundsPadding = 1.2f;
inline constexpr std::string_view kStaticModelThumbnailCacheDirectoryName = "thumbnails";

struct ThumbnailSourceStamp
{
    std::int64_t writeTimeTicks = 0;
    std::uintmax_t size = 0;
};

struct ThumbnailCacheMeta
{
    int schemaVersion = 0;
    std::string canonicalIdentity;
    ThumbnailSourceStamp stamp{};
};

struct ThumbnailCameraFrame
{
    core::Vec3 position{};
    core::Vec3 target{};
    core::Vec3 up{0.0f, 1.0f, 0.0f};
    float fieldOfViewY = kStaticModelThumbnailFieldOfViewY;
    float nearPlane = 0.05f;
    float farPlane = 100.0f;
};

struct ThumbnailModelBounds
{
    core::Vec3 min{};
    core::Vec3 max{};
};

std::uint64_t ThumbnailCacheKeyHash(std::string_view canonicalIdentity);
std::string ThumbnailCacheKeyHex(std::string_view canonicalIdentity);

std::filesystem::path MakeThumbnailCacheRoot(const std::filesystem::path& userDataDirectory);
std::filesystem::path ThumbnailCacheRoot();

bool TryResolveThumbnailCachePaths(
    std::string_view canonicalIdentity,
    const std::filesystem::path& cacheRoot,
    std::filesystem::path& imagePath,
    std::filesystem::path& metaPath,
    std::string& error);

bool ReadThumbnailSourceStamp(const std::filesystem::path& sourcePath, ThumbnailSourceStamp& stamp);
bool SourceStampsEqual(const ThumbnailSourceStamp& a, const ThumbnailSourceStamp& b);

bool WriteThumbnailCacheMeta(const std::filesystem::path& metaPath, const ThumbnailCacheMeta& meta);
bool ReadThumbnailCacheMeta(const std::filesystem::path& metaPath, ThumbnailCacheMeta& meta);

bool ThumbnailCacheIsValid(
    std::string_view canonicalIdentity,
    const ThumbnailSourceStamp& sourceStamp,
    const std::filesystem::path& cacheRoot);

bool RemoveThumbnailCacheEntry(
    std::string_view canonicalIdentity,
    const std::filesystem::path& cacheRoot);

enum class ThumbnailEnsureDecision
{
    ReuseReady,
    ReuseFailed,
    Refresh,
};

ThumbnailEnsureDecision ClassifyThumbnailEnsure(
    bool hasReadyTexture,
    bool failed,
    bool haveSourceStamp,
    const ThumbnailSourceStamp& entryStamp,
    const ThumbnailSourceStamp& sourceStamp);

ThumbnailCameraFrame MakeThumbnailCameraFrame(const ThumbnailModelBounds& bounds);
}
