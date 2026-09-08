#pragma once

// Development interactive static-model preview. Renders the real selected
// GLB through raylib LoadModel + RenderTexture. Independent of thumbnail
// PNG cache authority.

#include "editor/StaticModelFraming.h"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace render
{
class StaticModelPreviewRenderer
{
public:
    StaticModelPreviewRenderer();
    ~StaticModelPreviewRenderer();

    StaticModelPreviewRenderer(const StaticModelPreviewRenderer&) = delete;
    StaticModelPreviewRenderer& operator=(const StaticModelPreviewRenderer&) = delete;

    void Shutdown();
    void Clear();
    void AllowRetry();

    void Sync(
        std::string_view canonicalIdentity,
        const std::filesystem::path& sourcePath);

    bool Render(int width, int height, const editor::StaticModelPreviewOrbit& orbit);

    unsigned int TextureGpuId() const;
    bool HasModel() const;
    bool IsFailed() const;
    const std::string& LoadedIdentity() const;
    editor::ThumbnailModelBounds Bounds() const;
    editor::ThumbnailSourceStamp SourceStamp() const;

private:
    struct GpuState;

    void ReleaseLoadedModel();
    void ReleaseTarget();
    bool LoadModelFromSource(const std::filesystem::path& sourcePath);

    std::unique_ptr<GpuState> gpu;
    std::string loadedIdentity;
    editor::ThumbnailSourceStamp stamp{};
    editor::ThumbnailModelBounds bounds{};
    int targetWidth = 0;
    int targetHeight = 0;
    bool hasModel = false;
    bool failed = false;
};
}
