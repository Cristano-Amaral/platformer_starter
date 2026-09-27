#pragma once

#include "editor/CharacterPreview.h"
#include "editor/StaticModelFraming.h"

#include <filesystem>
#include <cstdint>
#include <memory>
#include <string>
#include "core/Vec3.h"

namespace render
{
class CharacterPreviewRenderer
{
public:
    CharacterPreviewRenderer();
    ~CharacterPreviewRenderer();
    CharacterPreviewRenderer(const CharacterPreviewRenderer&) = delete;
    CharacterPreviewRenderer& operator=(const CharacterPreviewRenderer&) = delete;

    void Shutdown();
    void SyncModel(std::string_view identity, const std::filesystem::path& sourcePath,
        const animation::CharacterModelValidationResult& validation);
    void SyncAnimation(const editor::CharacterPreviewAnimationResolution& resolution,
        const std::filesystem::path& assetRoot);
    bool Render(int width, int height, const editor::StaticModelPreviewOrbit& orbit,
        const editor::CharacterPreviewPlayback& playback);

    unsigned int TextureGpuId() const;
    bool HasModel() const;
    bool IsStaticModel() const;
    bool HasAnimation() const;
    const std::string& LoadedIdentity() const;
    const std::string& AnimationStatus() const;
    float AnimationDurationSeconds() const;
    bool SampleAnimation(float timeSeconds, animation::PlaybackMode mode);
    float LastSampledFrame() const;
    core::Vec3 CurrentJointTranslation(int jointIndex) const;
    std::uint64_t RenderedPixelChecksum() const;
    double RenderBoneMatrixChecksum() const;
    editor::ThumbnailModelBounds Bounds() const;

private:
    struct GpuState;
    void ReleaseModel();
    void ReleaseAnimation();
    void ReleaseTarget();
    std::unique_ptr<GpuState> gpu;
    std::string loadedIdentity;
    std::string animationKey;
    std::string animationStatus;
    editor::ThumbnailModelBounds bounds{};
    int targetWidth = 0;
    int targetHeight = 0;
    int selectedAnimation = -1;
    bool hasModel = false;
    bool staticModel = false;
    float lastSampledFrame = 0.0f;
    animation::PlaybackMode animationPlaybackMode = animation::PlaybackMode::Loop;
};
}
