#include "render/TerrainMesh.h"

#include "world/TerrainGeometry.h"

#include "raylib.h"

#include <cstring>

namespace render
{
namespace
{
void FreeTerrainMesh(Mesh& mesh)
{
    UnloadMesh(mesh);
    mesh = {};
}

bool FillTerrainMesh(const world::TerrainSpec& spec, Mesh& mesh)
{
    world::TerrainGeometry geometry{};
    if (!world::GenerateTerrainGeometry(spec, geometry) || geometry.positions.empty()
        || geometry.triangles.empty())
    {
        return false;
    }

    mesh = {};
    mesh.vertexCount = static_cast<int>(geometry.positions.size());
    mesh.triangleCount = static_cast<int>(geometry.triangles.size());
    mesh.vertices = static_cast<float*>(
        MemAlloc(static_cast<unsigned int>(mesh.vertexCount) * 3u * sizeof(float)));
    mesh.normals = static_cast<float*>(
        MemAlloc(static_cast<unsigned int>(mesh.vertexCount) * 3u * sizeof(float)));
    mesh.texcoords = static_cast<float*>(
        MemAlloc(static_cast<unsigned int>(mesh.vertexCount) * 2u * sizeof(float)));
    mesh.indices = static_cast<unsigned short*>(
        MemAlloc(static_cast<unsigned int>(mesh.triangleCount) * 3u * sizeof(unsigned short)));
    if (mesh.vertices == nullptr || mesh.normals == nullptr || mesh.texcoords == nullptr
        || mesh.indices == nullptr)
    {
        FreeTerrainMesh(mesh);
        return false;
    }

    const float invX =
        spec.resolutionX > 1 ? 1.0f / static_cast<float>(spec.resolutionX - 1) : 0.0f;
    const float invZ =
        spec.resolutionZ > 1 ? 1.0f / static_cast<float>(spec.resolutionZ - 1) : 0.0f;
    for (int index = 0; index < mesh.vertexCount; ++index)
    {
        const core::Vec3 position = geometry.positions[static_cast<std::size_t>(index)];
        const core::Vec3 normal = geometry.normals[static_cast<std::size_t>(index)];
        mesh.vertices[index * 3] = position.x;
        mesh.vertices[index * 3 + 1] = position.y;
        mesh.vertices[index * 3 + 2] = position.z;
        mesh.normals[index * 3] = normal.x;
        mesh.normals[index * 3 + 1] = normal.y;
        mesh.normals[index * 3 + 2] = normal.z;
        const int ix = index % spec.resolutionX;
        const int iz = index / spec.resolutionX;
        mesh.texcoords[index * 2] = static_cast<float>(ix) * invX;
        mesh.texcoords[index * 2 + 1] = static_cast<float>(iz) * invZ;
    }
    for (int triangle = 0; triangle < mesh.triangleCount; ++triangle)
    {
        const world::TerrainTriangle& indexed = geometry.triangles[static_cast<std::size_t>(triangle)];
        mesh.indices[triangle * 3] = static_cast<unsigned short>(indexed.i0);
        mesh.indices[triangle * 3 + 1] = static_cast<unsigned short>(indexed.i1);
        mesh.indices[triangle * 3 + 2] = static_cast<unsigned short>(indexed.i2);
    }

    UploadMesh(&mesh, false);
    return true;
}
}

TerrainGpuResources::~TerrainGpuResources()
{
    Unload();
}

void TerrainGpuResources::Unload()
{
    if (loaded)
    {
        FreeTerrainMesh(mesh);
        loaded = false;
        ++unloadCount;
    }
    hasSpec = false;
    lastSpec = {};
}

void TerrainGpuResources::Sync(const world::TerrainSpec* spec)
{
    if (spec == nullptr || !spec->enabled || !world::TerrainSpecIsValid(*spec))
    {
        Unload();
        return;
    }
    if (hasSpec && loaded && world::TerrainSpecEqual(lastSpec, *spec))
    {
        return;
    }

    Unload();
    if (!FillTerrainMesh(*spec, mesh))
    {
        mesh = {};
        loaded = false;
        hasSpec = false;
        lastSpec = {};
        return;
    }
    loaded = true;
    hasSpec = true;
    lastSpec = *spec;
    ++uploadCount;
}

bool TerrainGpuResources::HasMesh() const
{
    return loaded && mesh.vertexCount > 0;
}

const Mesh* TerrainGpuResources::GetMesh() const
{
    return HasMesh() ? &mesh : nullptr;
}

std::size_t TerrainGpuResources::UploadCount() const
{
    return uploadCount;
}

std::size_t TerrainGpuResources::UnloadCount() const
{
    return unloadCount;
}
}
