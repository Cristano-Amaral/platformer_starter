#include "render/TerrainMesh.h"

#include "assets/RuntimePng.h"
#include "assets/RuntimePngResolve.h"
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

void FillTerrainVertexColors(const world::TerrainSpec& spec, Mesh& mesh)
{
    if (mesh.colors == nullptr || mesh.vertexCount <= 0)
    {
        return;
    }
    for (int index = 0; index < mesh.vertexCount; ++index)
    {
        float weights[world::kMaxTerrainMaterialLayers];
        world::ReadTerrainSampleWeights(spec, index, weights);
        int quantized[world::kMaxTerrainMaterialLayers]{};
        world::QuantizeTerrainSampleWeights(weights, quantized);
        mesh.colors[index * 4] = static_cast<unsigned char>(quantized[0]);
        mesh.colors[index * 4 + 1] = static_cast<unsigned char>(quantized[1]);
        mesh.colors[index * 4 + 2] = static_cast<unsigned char>(quantized[2]);
        mesh.colors[index * 4 + 3] = static_cast<unsigned char>(quantized[3]);
    }
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
    mesh.colors = static_cast<unsigned char*>(
        MemAlloc(static_cast<unsigned int>(mesh.vertexCount) * 4u * sizeof(unsigned char)));
    mesh.indices = static_cast<unsigned short*>(
        MemAlloc(static_cast<unsigned int>(mesh.triangleCount) * 3u * sizeof(unsigned short)));
    if (mesh.vertices == nullptr || mesh.normals == nullptr || mesh.texcoords == nullptr
        || mesh.colors == nullptr || mesh.indices == nullptr)
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
    FillTerrainVertexColors(spec, mesh);
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

void TerrainGpuResources::SetAuthoringCookedRoot(const std::filesystem::path& cookedRoot)
{
    authoringCookedRoot = cookedRoot.empty() ? std::filesystem::path{} : cookedRoot.lexically_normal();
}

void TerrainGpuResources::SetAuthoringSourceRoot(const std::filesystem::path& sourceRoot)
{
    authoringSourceRoot = sourceRoot.empty() ? std::filesystem::path{} : sourceRoot.lexically_normal();
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
    layerCount = 1;
}

void TerrainGpuResources::UnloadLayerTexture(int layer)
{
    if (layer < 0 || layer >= world::kMaxTerrainMaterialLayers)
    {
        return;
    }
    if (textureLoaded[layer])
    {
        ::UnloadTexture(textures[layer]);
        textures[layer] = {};
        textureLoaded[layer] = false;
        ++textureUnloadCount;
    }
    lastTextureIdentity[layer].clear();
}

void TerrainGpuResources::UnloadTextures()
{
    for (int layer = 0; layer < world::kMaxTerrainMaterialLayers; ++layer)
    {
        UnloadLayerTexture(layer);
        loggedMissingIdentity[layer].clear();
    }
}

void TerrainGpuResources::Unload()
{
    UnloadMesh();
    UnloadTextures();
    if (missingLayerLoaded)
    {
        ::UnloadTexture(missingLayer);
        missingLayer = {};
        missingLayerLoaded = false;
    }
}

void TerrainGpuResources::EnsureMissingLayerTexture()
{
    if (missingLayerLoaded && missingLayer.id != 0)
    {
        return;
    }
    Image image = GenImageColor(1, 1, Color{255, 0, 255, 255});
    missingLayer = LoadTextureFromImage(image);
    UnloadImage(image);
    missingLayerLoaded = missingLayer.id != 0;
}

void TerrainGpuResources::SyncTextures(const world::TerrainSpec& spec)
{
    layerCount = world::TerrainMaterialLayerCount(spec);
    for (int layer = 0; layer < world::kMaxTerrainMaterialLayers; ++layer)
    {
        if (layer >= layerCount)
        {
            UnloadLayerTexture(layer);
            loggedMissingIdentity[layer].clear();
            continue;
        }

        const std::string& identity = world::TerrainLayerTextureIdentity(spec, layer);
        if (world::TerrainTextureIdentityIsNone(identity)
            || !assets::RuntimePngIdentityIsValid(identity))
        {
            UnloadLayerTexture(layer);
            continue;
        }
        if (textureLoaded[layer] && lastTextureIdentity[layer] == identity && textures[layer].id != 0)
        {
            continue;
        }

        UnloadLayerTexture(layer);
        const assets::RuntimePngLoadResolution resolved =
            assets::ResolveRuntimePngLoadFile(identity, authoringCookedRoot, {}, authoringSourceRoot);
        if (!assets::RuntimePngLoadFileIsAvailable(resolved))
        {
            if (loggedMissingIdentity[layer] != identity)
            {
                std::fprintf(
                    stderr,
                    "TerrainMaterial: missing runtime texture: %s\n",
                    identity.c_str());
                loggedMissingIdentity[layer] = identity;
            }
            continue;
        }

        const Texture2D loadedTexture = LoadTexture(resolved.path.string().c_str());
        if (loadedTexture.id == 0)
        {
            if (loggedMissingIdentity[layer] != identity)
            {
                std::fprintf(
                    stderr,
                    "TerrainMaterial: failed to load runtime texture: %s\n",
                    identity.c_str());
                loggedMissingIdentity[layer] = identity;
            }
            continue;
        }

        textures[layer] = loadedTexture;
        SetTextureWrap(textures[layer], TEXTURE_WRAP_REPEAT);
        textureLoaded[layer] = true;
        lastTextureIdentity[layer] = identity;
        loggedMissingIdentity[layer].clear();
        ++textureLoadCount;
    }
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
            UnloadTextures();
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
        lastSpec.extraLayers = spec->extraLayers;
    }

    SyncTextures(*spec);
    EnsureMissingLayerTexture();
}

bool TerrainGpuResources::HasMesh() const
{
    return loaded && mesh.vertexCount > 0;
}

bool TerrainGpuResources::HasTexture() const
{
    return textureLoaded[0] && textures[0].id != 0;
}

int TerrainGpuResources::LayerCount() const
{
    return layerCount;
}

const Mesh* TerrainGpuResources::GetMesh() const
{
    return HasMesh() ? &mesh : nullptr;
}

const Texture2D* TerrainGpuResources::GetTexture() const
{
    return GetLayerTexture(0);
}

const Texture2D* TerrainGpuResources::GetLayerTexture(int layer) const
{
    if (layer < 0 || layer >= world::kMaxTerrainMaterialLayers)
    {
        return nullptr;
    }
    if (!textureLoaded[layer] || textures[layer].id == 0)
    {
        return nullptr;
    }
    return &textures[layer];
}

const Texture2D* TerrainGpuResources::GetMissingLayerTexture() const
{
    return missingLayerLoaded && missingLayer.id != 0 ? &missingLayer : nullptr;
}

const world::TerrainSpec* TerrainGpuResources::LastSpec() const
{
    return hasSpec ? &lastSpec : nullptr;
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
