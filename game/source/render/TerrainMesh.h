#pragma once

// Milestone 86: GPU mesh for the singleton Terrain. Owned by Renderer.
// Rebuilds only when authored Terrain changes. Not a generic mesh framework.

#include "world/Terrain.h"

#include "raylib.h"

#include <cstddef>

namespace render
{
class TerrainGpuResources
{
public:
    TerrainGpuResources() = default;
    ~TerrainGpuResources();

    TerrainGpuResources(const TerrainGpuResources&) = delete;
    TerrainGpuResources& operator=(const TerrainGpuResources&) = delete;

    // spec == nullptr or disabled/invalid unloads. Unchanged authored data
    // does not upload again.
    void Sync(const world::TerrainSpec* spec);
    void Unload();
    bool HasMesh() const;
    const Mesh* GetMesh() const;
    std::size_t UploadCount() const;
    std::size_t UnloadCount() const;

private:
    Mesh mesh{};
    bool loaded = false;
    bool hasSpec = false;
    world::TerrainSpec lastSpec{};
    std::size_t uploadCount = 0;
    std::size_t unloadCount = 0;
};
}
