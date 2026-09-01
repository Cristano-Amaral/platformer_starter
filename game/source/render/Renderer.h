#pragma once

#include "core/Vec3.h"

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

    void BeginFrame();
    void DrawWorld(
        const gameplay::Player& player,
        const gameplay::PlatformerCamera& camera,
        core::Vec3 physicsTestBoxPosition,
        core::Vec3 physicsTestBoxSize,
        core::Vec3 movingPlatformPosition,
        core::Vec3 movingPlatformSize);
    void EndFrame();

private:
    void LoadTestCheckerTexture();
    void LoadTestStaticModel();

    struct GpuTexture;
    std::unique_ptr<GpuTexture> testTexture;
    bool testTextureLoaded = false;
    bool testTextureFallbackActive = false;

    struct GpuModel;
    std::unique_ptr<GpuModel> testModel;
    bool testModelLoaded = false;
    bool testModelFallbackActive = false;
};
}
