#pragma once

// Milestone 96: shared crossed-card mesh, per-identity textures, and
// instanced ground-cover draws. Derived transforms stay transient. Not a
// generalized foliage renderer.

#include "render/LoadedModelMaterials.h"
#include "world/Terrain.h"
#include "world/TerrainGroundCover.h"

#include "raylib.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace render
{
class GroundCoverGpuResources
{
public:
    GroundCoverGpuResources() = default;
    ~GroundCoverGpuResources();

    GroundCoverGpuResources(const GroundCoverGpuResources&) = delete;
    GroundCoverGpuResources& operator=(const GroundCoverGpuResources&) = delete;

    void SetAuthoringCookedRoot(const std::filesystem::path& cookedRoot);
    void SetAuthoringSourceRoot(const std::filesystem::path& sourceRoot);

    void Sync(const world::TerrainSpec* spec);
    void Unload();

    void ResetDrawStats();
    std::size_t InstanceCount() const;
    std::size_t RenderGroupCount() const;
    std::size_t InstancedSubmissionCount() const;
    std::size_t OrdinarySubmissionCount() const;
    std::size_t UniqueTextureCount() const;
    std::size_t TextureLoadCount() const;

    void Draw(
        const world::TerrainSpec& terrain,
        const ModelDrawOverride* override,
        bool allowShadowCast) const;

private:
    struct TextureSlot
    {
        std::string identity{};
        Texture2D texture{};
        bool loaded = false;
    };

    void EnsureCardMesh();
    void UnloadCardMesh();
    void SyncTextures(const world::TerrainSpec& spec);
    void UnloadTextures();
    const Texture2D* FindTexture(std::string_view identity) const;
    void EnsureFallbackTexture();
    void EnsureMaterial() const;

    Mesh cardMesh{};
    bool cardMeshLoaded = false;
    Texture2D fallbackTexture{};
    bool fallbackLoaded = false;
    mutable Material drawMaterial{};
    mutable bool drawMaterialLoaded = false;
    std::filesystem::path authoringCookedRoot{};
    std::filesystem::path authoringSourceRoot{};
    mutable std::vector<TextureSlot> textures{};
    mutable std::vector<world::TerrainGroundCoverInstance> instances{};
    mutable world::TerrainGroundCoverRenderPlan plan{};
    mutable std::vector<Matrix> matrices{};
    mutable std::uint64_t signature = 0;
    mutable bool cacheValid = false;
    mutable std::size_t instanceCount = 0;
    mutable std::size_t renderGroupCount = 0;
    mutable std::size_t instancedSubmissions = 0;
    mutable std::size_t ordinarySubmissions = 0;
    std::size_t textureLoadCount = 0;
};
}
