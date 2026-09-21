#pragma once

// Milestone 86/88: GPU mesh and optional base-color texture for the singleton
// Terrain. Owned by Renderer. Mesh rebuilds when geometry/UVs change. Texture
// loads only when the authored identity changes. Not a generic mesh or resource
// manager.

#include "world/Terrain.h"

#include "raylib.h"

#include <cstddef>
#include <string>

namespace render
{
class TerrainGpuResources
{
public:
    TerrainGpuResources() = default;
    ~TerrainGpuResources();

    TerrainGpuResources(const TerrainGpuResources&) = delete;
    TerrainGpuResources& operator=(const TerrainGpuResources&) = delete;

    // spec == nullptr or disabled/invalid unloads mesh and texture. Unchanged
    // mesh data does not upload again. Unchanged texture identity does not
    // reload. Empty/invalid identity unloads the texture and uses fallback.
    void Sync(const world::TerrainSpec* spec);
    void Unload();
    bool HasMesh() const;
    bool HasTexture() const;
    const Mesh* GetMesh() const;
    const Texture2D* GetTexture() const;
    std::size_t UploadCount() const;
    std::size_t UnloadCount() const;
    std::size_t TextureLoadCount() const;
    std::size_t TextureUnloadCount() const;

private:
    void UnloadMesh();
    void UnloadTexture();
    void SyncTexture(const world::TerrainSpec& spec);

    Mesh mesh{};
    Texture2D texture{};
    bool loaded = false;
    bool textureLoaded = false;
    bool hasSpec = false;
    world::TerrainSpec lastSpec{};
    std::string lastTextureIdentity{};
    std::string loggedMissingIdentity{};
    std::size_t uploadCount = 0;
    std::size_t unloadCount = 0;
    std::size_t textureLoadCount = 0;
    std::size_t textureUnloadCount = 0;
};
}
