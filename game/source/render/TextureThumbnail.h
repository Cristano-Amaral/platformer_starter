#pragma once

// Development Texture thumbnail GPU store. Loads the source PNG itself via
// raylib LoadTexture. No derived thumbnail files. Not runtime asset authority.

#include "editor/StaticModelThumbnailCache.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace render
{
class TextureThumbnailStore
{
public:
    TextureThumbnailStore() = default;
    ~TextureThumbnailStore();

    TextureThumbnailStore(const TextureThumbnailStore&) = delete;
    TextureThumbnailStore& operator=(const TextureThumbnailStore&) = delete;

    void BeginFrame();
    void Shutdown();

    void Ensure(std::string_view canonicalIdentity, const std::filesystem::path& sourcePath);
    unsigned int TextureGpuId(std::string_view canonicalIdentity) const;
    int TextureWidth(std::string_view canonicalIdentity) const;
    int TextureHeight(std::string_view canonicalIdentity) const;
    bool HasReadyTexture(std::string_view canonicalIdentity) const;
    bool IsFailed(std::string_view canonicalIdentity) const;

    void Forget(std::string_view canonicalIdentity);
    void Reconcile(const std::vector<std::string>& catalogIdentities);
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
    bool LoadFromSource(
        Entry& entry,
        std::string_view canonicalIdentity,
        const std::filesystem::path& sourcePath,
        const editor::ThumbnailSourceStamp& stamp);

    std::unordered_map<std::string, Entry> entries;
    int loadsThisFrame = 0;
    std::string lastFailureMessage;
};
}
