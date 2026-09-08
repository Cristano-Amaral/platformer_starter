#pragma once

// Development static-model thumbnail GPU store. Uses raylib LoadModel +
// RenderTexture. Not a second renderer and not runtime asset authority.

#include "editor/StaticModelThumbnailCache.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

namespace render
{
class StaticModelThumbnailStore
{
public:
    StaticModelThumbnailStore() = default;
    ~StaticModelThumbnailStore();

    StaticModelThumbnailStore(const StaticModelThumbnailStore&) = delete;
    StaticModelThumbnailStore& operator=(const StaticModelThumbnailStore&) = delete;

    void BeginFrame();
    void Shutdown();

    // Cheap per-visible-item call. At most one GLB render per frame.
    void Ensure(
        std::string_view canonicalIdentity,
        const std::filesystem::path& sourcePath,
        const std::filesystem::path& cacheRoot);

    unsigned int TextureGpuId(std::string_view canonicalIdentity) const;
    bool HasReadyTexture(std::string_view canonicalIdentity) const;
    bool IsFailed(std::string_view canonicalIdentity) const;

    void Forget(std::string_view canonicalIdentity);
    void AllowRetryAll();
    bool ConsumeLastFailure(std::string& message);

private:
    struct Entry
    {
        unsigned int gpuId = 0;
        int width = 0;
        int height = 0;
        editor::ThumbnailSourceStamp stamp{};
        bool failed = false;
    };

    void UnloadEntry(Entry& entry);
    bool LoadFromCache(
        Entry& entry,
        std::string_view canonicalIdentity,
        const editor::ThumbnailSourceStamp& stamp,
        const std::filesystem::path& cacheRoot);
    bool Generate(
        Entry& entry,
        std::string_view canonicalIdentity,
        const std::filesystem::path& sourcePath,
        const editor::ThumbnailSourceStamp& stamp,
        const std::filesystem::path& cacheRoot);

    std::unordered_map<std::string, Entry> entries;
    int generationsThisFrame = 0;
    std::string lastFailureMessage;
};
}
