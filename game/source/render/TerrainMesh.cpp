#include "render/TerrainMesh.h"

#include "assets/RuntimePng.h"
#include "platform/RuntimePaths.h"
#include "world/TerrainGeometry.h"

#include "raylib.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <system_error>

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
        || geometry.triangles.empty() || geometry.texcoords.size() != geometry.positions.size())
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

    for (int index = 0; index < mesh.vertexCount; ++index)
    {
        const core::Vec3 position = geometry.positions[static_cast<std::size_t>(index)];
        const core::Vec3 normal = geometry.normals[static_cast<std::size_t>(index)];
        const world::TerrainTexCoord texcoord = geometry.texcoords[static_cast<std::size_t>(index)];
        mesh.vertices[index * 3] = position.x;
        mesh.vertices[index * 3 + 1] = position.y;
        mesh.vertices[index * 3 + 2] = position.z;
        mesh.normals[index * 3] = normal.x;
        mesh.normals[index * 3 + 1] = normal.y;
        mesh.normals[index * 3 + 2] = normal.z;
        mesh.texcoords[index * 2] = texcoord.u;
        mesh.texcoords[index * 2 + 1] = texcoord.v;
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

bool RegularFileExists(const std::filesystem::path& path)
{
    std::error_code error;
    return !path.empty() && path.is_absolute() && std::filesystem::is_regular_file(path, error)
        && !error;
}
}

TerrainGpuResources::~TerrainGpuResources()
{
    Unload();
}

void TerrainGpuResources::UnloadMesh()
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

void TerrainGpuResources::UnloadTexture()
{
    if (textureLoaded)
    {
        ::UnloadTexture(texture);
        texture = {};
        textureLoaded = false;
        ++textureUnloadCount;
    }
    lastTextureIdentity.clear();
}

void TerrainGpuResources::Unload()
{
    UnloadMesh();
    UnloadTexture();
    loggedMissingIdentity.clear();
}

void TerrainGpuResources::SyncTexture(const world::TerrainSpec& spec)
{
    if (world::TerrainTextureIdentityIsNone(spec.textureIdentity)
        || !assets::RuntimePngIdentityIsValid(spec.textureIdentity))
    {
        UnloadTexture();
        return;
    }
    if (textureLoaded && lastTextureIdentity == spec.textureIdentity)
    {
        return;
    }

    UnloadTexture();
    const std::filesystem::path path = platform::RuntimeAssetPath(spec.textureIdentity);
    if (!RegularFileExists(path))
    {
        if (loggedMissingIdentity != spec.textureIdentity)
        {
            std::fprintf(
                stderr,
                "TerrainMaterial: missing staged texture: %s\n",
                spec.textureIdentity.c_str());
            loggedMissingIdentity = spec.textureIdentity;
        }
        return;
    }

    const Texture2D loadedTexture = LoadTexture(path.string().c_str());
    if (loadedTexture.id == 0)
    {
        if (loggedMissingIdentity != spec.textureIdentity)
        {
            std::fprintf(
                stderr,
                "TerrainMaterial: failed to load staged texture: %s\n",
                spec.textureIdentity.c_str());
            loggedMissingIdentity = spec.textureIdentity;
        }
        return;
    }

    texture = loadedTexture;
    SetTextureWrap(texture, TEXTURE_WRAP_REPEAT);
    textureLoaded = true;
    lastTextureIdentity = spec.textureIdentity;
    loggedMissingIdentity.clear();
    ++textureLoadCount;
}

void TerrainGpuResources::Sync(const world::TerrainSpec* spec)
{
    if (spec == nullptr || !spec->enabled || !world::TerrainSpecIsValid(*spec))
    {
        Unload();
        return;
    }

    const bool meshUnchanged = hasSpec && loaded && world::TerrainMeshDataEqual(lastSpec, *spec);
    if (!meshUnchanged)
    {
        UnloadMesh();
        if (!FillTerrainMesh(*spec, mesh))
        {
            mesh = {};
            loaded = false;
            hasSpec = false;
            lastSpec = {};
            UnloadTexture();
            return;
        }
        loaded = true;
        hasSpec = true;
        lastSpec = *spec;
        ++uploadCount;
    }
    else
    {
        lastSpec.textureIdentity = spec->textureIdentity;
        lastSpec.enabled = spec->enabled;
    }

    SyncTexture(*spec);
}

bool TerrainGpuResources::HasMesh() const
{
    return loaded && mesh.vertexCount > 0;
}

bool TerrainGpuResources::HasTexture() const
{
    return textureLoaded && texture.id != 0;
}

const Mesh* TerrainGpuResources::GetMesh() const
{
    return HasMesh() ? &mesh : nullptr;
}

const Texture2D* TerrainGpuResources::GetTexture() const
{
    return HasTexture() ? &texture : nullptr;
}

std::size_t TerrainGpuResources::UploadCount() const
{
    return uploadCount;
}

std::size_t TerrainGpuResources::UnloadCount() const
{
    return unloadCount;
}

std::size_t TerrainGpuResources::TextureLoadCount() const
{
    return textureLoadCount;
}

std::size_t TerrainGpuResources::TextureUnloadCount() const
{
    return textureUnloadCount;
}
}
