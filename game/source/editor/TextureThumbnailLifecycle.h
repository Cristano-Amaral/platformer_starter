#pragma once

// CPU-side Texture thumbnail ownership/invalidation. No GPU, no disk cache,
// and not a versioned asset. GPU upload lives in render::TextureThumbnailStore.

#include "editor/StaticModelThumbnailCache.h"

#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace editor
{
enum class TextureThumbnailEnsureDecision
{
    ReuseReady,
    ReuseFailed,
    Reload,
    Missing,
};

struct TextureThumbnailRecord
{
    std::string canonicalIdentity;
    ThumbnailSourceStamp stamp{};
    bool failed = false;
};

inline TextureThumbnailEnsureDecision ClassifyTextureThumbnailEnsure(
    bool hasEntry,
    bool failed,
    const ThumbnailSourceStamp& loadedStamp,
    bool sourceExists,
    const ThumbnailSourceStamp& sourceStamp)
{
    if (!sourceExists)
    {
        return TextureThumbnailEnsureDecision::Missing;
    }
    if (!hasEntry)
    {
        return TextureThumbnailEnsureDecision::Reload;
    }
    if (!SourceStampsEqual(loadedStamp, sourceStamp))
    {
        return TextureThumbnailEnsureDecision::Reload;
    }
    return failed ? TextureThumbnailEnsureDecision::ReuseFailed
                  : TextureThumbnailEnsureDecision::ReuseReady;
}

inline void CollectStaleTextureThumbnailIdentities(
    const std::vector<std::string>& loadedIdentities,
    const std::vector<std::string>& catalogIdentities,
    std::vector<std::string>& staleIdentities)
{
    staleIdentities.clear();
    std::unordered_set<std::string> known(catalogIdentities.begin(), catalogIdentities.end());
    for (const std::string& identity : loadedIdentities)
    {
        if (known.find(identity) == known.end())
        {
            staleIdentities.push_back(identity);
        }
    }
}

inline void ComputeTextureThumbnailDrawSize(
    int imageWidth,
    int imageHeight,
    float cellSize,
    float& drawWidth,
    float& drawHeight)
{
    if (imageWidth <= 0 || imageHeight <= 0 || cellSize <= 0.0f)
    {
        drawWidth = cellSize;
        drawHeight = cellSize;
        return;
    }
    const float width = static_cast<float>(imageWidth);
    const float height = static_cast<float>(imageHeight);
    if (width >= height)
    {
        drawWidth = cellSize;
        drawHeight = cellSize * (height / width);
    }
    else
    {
        drawHeight = cellSize;
        drawWidth = cellSize * (width / height);
    }
}
}
