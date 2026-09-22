#pragma once

// Milestone 86/88/90/92: GPU mesh, per-layer source textures, and the
// sampler2DArray albedo/weight representation. Mesh rebuilds when geometry
// or layer-0 UVs change. Weight maps upload independently of the mesh.
// Not a generic mesh or resource manager.

#include "world/Terrain.h"

#include "raylib.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace render
{
class TerrainGpuResources
{
public:
    TerrainGpuResources() = default;
    ~TerrainGpuResources();

    TerrainGpuResources(const TerrainGpuResources&) = delete;
    TerrainGpuResources& operator=(const TerrainGpuResources&) = delete;

    // Development authoring may resolve cooked PNGs when staged copies are
    // absent, then catalog source PNGs as a last-resort preview. Release
    // leaves both empty so only RuntimeAssetPath is used.
    void SetAuthoringCookedRoot(const std::filesystem::path& cookedRoot);
    void SetAuthoringSourceRoot(const std::filesystem::path& sourceRoot);

    // spec == nullptr or disabled/invalid unloads mesh and textures. Unchanged
    // mesh data does not upload again. Unchanged layer identities do not
    // reload. Empty/invalid identities unload that layer slot.
    void Sync(const world::TerrainSpec* spec);
    void Unload();
    bool HasMesh() const;
    bool HasTexture() const;
    int LayerCount() const;
    const Mesh* GetMesh() const;
    const Texture2D* GetTexture() const;
    const Texture2D* GetLayerTexture(int layer) const;
    const Texture2D* GetMissingLayerTexture() const;
    unsigned int AlbedoArrayId() const;
    unsigned int WeightArrayId() const;
    int WeightMapCount() const;
    bool CopyWeightMapTexel(
        int map,
        int x,
        int z,
        unsigned char& red,
        unsigned char& green,
        unsigned char& blue,
        unsigned char& alpha) const;
    const world::TerrainSpec* LastSpec() const;
    std::size_t UploadCount() const;
    std::size_t UnloadCount() const;
    std::size_t TextureLoadCount() const;
    std::size_t TextureUnloadCount() const;
    std::size_t WeightUploadCount() const;

private:
    void UnloadMesh();
    void UnloadLayerTexture(int layer);
    void UnloadTextures();
    void UnloadArrays();
    void EnsureMissingLayerTexture();
    void SyncTextures(const world::TerrainSpec& spec);
    void RebuildAlbedoArray(const world::TerrainSpec& spec);
    void SyncWeightArray(const world::TerrainSpec& spec);

    Mesh mesh{};
    std::filesystem::path authoringCookedRoot{};
    std::filesystem::path authoringSourceRoot{};
    Texture2D textures[world::kMaxTerrainMaterialLayers]{};
    bool textureLoaded[world::kMaxTerrainMaterialLayers]{};
    std::string lastTextureIdentity[world::kMaxTerrainMaterialLayers]{};
    Texture2D missingLayer{};
    bool missingLayerLoaded = false;
    bool loaded = false;
    int layerCount = 1;
    bool hasSpec = false;
    world::TerrainSpec lastSpec{};
    std::string loggedMissingIdentity[world::kMaxTerrainMaterialLayers]{};
    unsigned int albedoArrayId = 0;
    unsigned int weightArrayId = 0;
    int albedoArrayWidth = 0;
    int albedoArrayHeight = 0;
    int weightMapCount = 1;
    std::vector<unsigned char> weightMapBytes{};
    std::size_t uploadCount = 0;
    std::size_t unloadCount = 0;
    std::size_t textureLoadCount = 0;
    std::size_t textureUnloadCount = 0;
    std::size_t weightUploadCount = 0;
};
}
