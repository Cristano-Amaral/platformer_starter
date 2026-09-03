#pragma once

#include "core/Vec3.h"
#include "world/CollectibleWorld.h"

#include <array>
#include <memory>

namespace gameplay
{
class Player;
class PlatformerCamera;
}

namespace render
{
class Renderer
{
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    void LoadRuntimeAssets();
    void UnloadRuntimeAssets();

    bool IsTestTextureLoaded() const;
    bool IsTestTextureFallbackActive() const;
    const char* TestTextureLogicalId() const;
    const char* TestTextureRuntimeRelativePath() const;

    bool IsTestModelLoaded() const;
    bool IsTestModelFallbackActive() const;
    const char* TestModelLogicalId() const;

    bool IsAuthoredModelLoaded() const;
    bool IsAuthoredModelFallbackActive() const;
    const char* AuthoredModelLogicalId() const;

    bool IsTexturedModelLoaded() const;
    bool IsTexturedModelFallbackActive() const;
    const char* TexturedModelLogicalId() const;
    int TexturedModelMaterialCount() const;
    bool TexturedModelHasAlbedoTexture() const;

    void BeginFrame();
    void DrawWorld(
        const gameplay::Player& player,
        const gameplay::PlatformerCamera& camera,
        core::Vec3 physicsTestBoxPosition,
        core::Vec3 physicsTestBoxSize,
        core::Vec3 movingPlatformPosition,
        core::Vec3 movingPlatformSize,
        std::array<world::CheckpointVisualState, world::kCheckpointCount> checkpointVisuals,
        bool levelCompleted,
        const std::array<bool, world::kCollectibleCount>& collectibleCollected,
        int collectedCount,
        double elapsedSeconds);
    void EndFrame();

private:
    void LoadTestCheckerTexture();
    void LoadTestStaticModel();
    void LoadTestAuthoredModel();
    void LoadTestTexturedModel();

    struct GpuTexture;
    std::unique_ptr<GpuTexture> testTexture;
    bool testTextureLoaded = false;
    bool testTextureFallbackActive = false;

    struct GpuModel;
    std::unique_ptr<GpuModel> testModel;
    bool testModelLoaded = false;
    bool testModelFallbackActive = false;

    std::unique_ptr<GpuModel> authoredModel;
    bool authoredModelLoaded = false;
    bool authoredModelFallbackActive = false;

    std::unique_ptr<GpuModel> texturedModel;
    bool texturedModelLoaded = false;
    bool texturedModelFallbackActive = false;
    int texturedModelMaterialCount = 0;
    bool texturedModelHasAlbedoTexture = false;
};
}
