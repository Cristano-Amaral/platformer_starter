#include "render/TerrainMesh.h"

#include "assets/RuntimePng.h"
#include "assets/RuntimePngResolve.h"
#include "platform/RuntimePaths.h"
#include "world/TerrainGeometry.h"
#include "world/TerrainMaterialShading.h"

#include "external/glad.h"
#include "raylib.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <system_error>
#include <vector>

namespace render
{
namespace
{
void FreeTerrainMesh(Mesh& mesh)
{
    UnloadMesh(mesh);
    mesh = {};
}

void FillTerrainVertexColors(Mesh& mesh)
{
    if (mesh.colors == nullptr || mesh.vertexCount <= 0)
    {
        return;
    }
    for (int index = 0; index < mesh.vertexCount; ++index)
    {
        mesh.colors[index * 4] = 255;
        mesh.colors[index * 4 + 1] = 255;
        mesh.colors[index * 4 + 2] = 255;
        mesh.colors[index * 4 + 3] = 255;
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
    FillTerrainVertexColors(mesh);
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

unsigned int UploadTerrainChannelArray(
    int layers,
    Image* images,
    unsigned char* owns,
    Color layer0Fallback,
    Color missingFallback,
    bool repeatWrap,
    int& outWidth,
    int& outHeight)
{
    int width = 1;
    int height = 1;
    for (int layer = 0; layer < layers; ++layer)
    {
        if (owns[layer] == 0 || images[layer].data == nullptr)
        {
            continue;
        }
        if (images[layer].width > width)
        {
            width = images[layer].width;
        }
        if (images[layer].height > height)
        {
            height = images[layer].height;
        }
    }
    if (width > world::kTerrainArrayMaxDimension)
    {
        width = world::kTerrainArrayMaxDimension;
    }
    if (height > world::kTerrainArrayMaxDimension)
    {
        height = world::kTerrainArrayMaxDimension;
    }
    unsigned int id = 0;
    glGenTextures(1, &id);
    if (id == 0)
    {
        for (int layer = 0; layer < layers; ++layer)
        {
            if (owns[layer] != 0)
            {
                UnloadImage(images[layer]);
                images[layer] = {};
                owns[layer] = 0;
            }
        }
        outWidth = 0;
        outHeight = 0;
        return 0;
    }
    glBindTexture(GL_TEXTURE_2D_ARRAY, id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage3D(
        GL_TEXTURE_2D_ARRAY,
        0,
        GL_RGBA8,
        width,
        height,
        layers,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr);
    for (int layer = 0; layer < layers; ++layer)
    {
        Image slice{};
        if (owns[layer] != 0)
        {
            slice = images[layer];
            images[layer] = {};
            owns[layer] = 0;
            ImageFormat(&slice, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            if (slice.width != width || slice.height != height)
            {
                ImageResize(&slice, width, height);
            }
        }
        else
        {
            slice = GenImageColor(width, height, layer == 0 ? layer0Fallback : missingFallback);
        }
        if (slice.data != nullptr)
        {
            glTexSubImage3D(
                GL_TEXTURE_2D_ARRAY,
                0,
                0,
                0,
                layer,
                width,
                height,
                1,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                slice.data);
        }
        UnloadImage(slice);
    }
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    const int wrap = repeatWrap ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, wrap);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    outWidth = width;
    outHeight = height;
    return id;
}

void ApplyDirectXNormalGreenFlip(Image& image)
{
    if (image.data == nullptr || image.width <= 0 || image.height <= 0)
    {
        return;
    }
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    auto* pixels = static_cast<unsigned char*>(image.data);
    const int count = image.width * image.height;
    for (int index = 0; index < count; ++index)
    {
        pixels[index * 4 + 1] = world::FlipDirectXNormalGreenQuantum(pixels[index * 4 + 1]);
    }
}

void LoadTerrainChannelImage(
    const std::string& identity,
    const std::filesystem::path& cookedRoot,
    const std::filesystem::path& sourceRoot,
    Image& image,
    unsigned char& owns)
{
    image = {};
    owns = 0;
    if (world::TerrainTextureIdentityIsNone(identity) || !assets::RuntimePngIdentityIsValid(identity))
    {
        return;
    }
    const assets::RuntimePngLoadResolution resolved =
        assets::ResolveRuntimePngLoadFile(identity, cookedRoot, {}, sourceRoot);
    if (!assets::RuntimePngLoadFileIsAvailable(resolved))
    {
        return;
    }
    Image loaded = LoadImage(resolved.path.string().c_str());
    if (loaded.data == nullptr || loaded.width <= 0 || loaded.height <= 0)
    {
        UnloadImage(loaded);
        return;
    }
    image = loaded;
    owns = 1;
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
        loggedMissingNormal[layer].clear();
        loggedMissingRoughness[layer].clear();
    }
}

void TerrainGpuResources::UnloadArrays()
{
    if (albedoArrayId != 0)
    {
        glDeleteTextures(1, &albedoArrayId);
        albedoArrayId = 0;
    }
    if (normalArrayId != 0)
    {
        glDeleteTextures(1, &normalArrayId);
        normalArrayId = 0;
    }
    if (roughnessArrayId != 0)
    {
        glDeleteTextures(1, &roughnessArrayId);
        roughnessArrayId = 0;
    }
    if (weightArrayId != 0)
    {
        glDeleteTextures(1, &weightArrayId);
        weightArrayId = 0;
    }
    albedoArrayWidth = 0;
    albedoArrayHeight = 0;
    weightMapCount = 1;
    weightMapBytes.clear();
}

void TerrainGpuResources::Unload()
{
    UnloadMesh();
    UnloadTextures();
    UnloadArrays();
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
    const int previousLayerCount = layerCount;
    layerCount = world::TerrainMaterialLayerCount(spec);
    bool albedoDirty = previousLayerCount != layerCount || albedoArrayId == 0;
    for (int layer = 0; layer < world::kMaxTerrainMaterialLayers; ++layer)
    {
        if (layer >= layerCount)
        {
            if (textureLoaded[layer])
            {
                albedoDirty = true;
            }
            UnloadLayerTexture(layer);
            loggedMissingIdentity[layer].clear();
            continue;
        }

        const std::string& identity = world::TerrainLayerTextureIdentity(spec, layer);
        if (world::TerrainTextureIdentityIsNone(identity)
            || !assets::RuntimePngIdentityIsValid(identity))
        {
            if (textureLoaded[layer])
            {
                albedoDirty = true;
            }
            UnloadLayerTexture(layer);
            continue;
        }
        if (textureLoaded[layer] && lastTextureIdentity[layer] == identity && textures[layer].id != 0)
        {
            continue;
        }

        albedoDirty = true;
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

    bool normalDirty = previousLayerCount != layerCount || normalArrayId == 0;
    bool roughnessDirty = previousLayerCount != layerCount || roughnessArrayId == 0;
    for (int layer = 0; layer < world::kMaxTerrainMaterialLayers; ++layer)
    {
        if (layer >= layerCount)
        {
            if (!lastNormalIdentity[layer].empty())
            {
                normalDirty = true;
            }
            if (!lastRoughnessIdentity[layer].empty())
            {
                roughnessDirty = true;
            }
            lastNormalIdentity[layer].clear();
            lastRoughnessIdentity[layer].clear();
            loggedMissingNormal[layer].clear();
            loggedMissingRoughness[layer].clear();
            continue;
        }
        const std::string& normalIdentity = world::TerrainLayerNormalIdentity(spec, layer);
        if (lastNormalIdentity[layer] != normalIdentity)
        {
            normalDirty = true;
            lastNormalIdentity[layer] = normalIdentity;
        }
        if (!normalIdentity.empty()
            && !assets::RuntimePngLoadFileIsAvailable(assets::ResolveRuntimePngLoadFile(
                normalIdentity, authoringCookedRoot, {}, authoringSourceRoot)))
        {
            if (loggedMissingNormal[layer] != normalIdentity)
            {
                std::fprintf(
                    stderr,
                    "TerrainMaterial: missing normal map: %s\n",
                    normalIdentity.c_str());
                loggedMissingNormal[layer] = normalIdentity;
            }
        }
        else
        {
            loggedMissingNormal[layer].clear();
        }
        const std::string& roughnessIdentity = world::TerrainLayerRoughnessIdentity(spec, layer);
        if (lastRoughnessIdentity[layer] != roughnessIdentity)
        {
            roughnessDirty = true;
            lastRoughnessIdentity[layer] = roughnessIdentity;
        }
        if (!roughnessIdentity.empty()
            && !assets::RuntimePngLoadFileIsAvailable(assets::ResolveRuntimePngLoadFile(
                roughnessIdentity, authoringCookedRoot, {}, authoringSourceRoot)))
        {
            if (loggedMissingRoughness[layer] != roughnessIdentity)
            {
                std::fprintf(
                    stderr,
                    "TerrainMaterial: missing roughness map: %s\n",
                    roughnessIdentity.c_str());
                loggedMissingRoughness[layer] = roughnessIdentity;
            }
        }
        else
        {
            loggedMissingRoughness[layer].clear();
        }
    }
    if (albedoDirty)
    {
        RebuildAlbedoArray(spec);
    }
    if (normalDirty)
    {
        RebuildNormalArray(spec);
    }
    if (roughnessDirty)
    {
        RebuildRoughnessArray(spec);
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
        lastSpec.normalIdentity = spec->normalIdentity;
        lastSpec.roughnessIdentity = spec->roughnessIdentity;
        lastSpec.enabled = spec->enabled;
        lastSpec.extraLayers = spec->extraLayers;
        lastSpec.materialWeights = spec->materialWeights;
        lastSpec.weightResolutionX = spec->weightResolutionX;
        lastSpec.weightResolutionZ = spec->weightResolutionZ;
    }

    SyncTextures(*spec);
    SyncWeightArray(*spec);
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

std::size_t TerrainGpuResources::WeightUploadCount() const
{
    return weightUploadCount;
}

unsigned int TerrainGpuResources::AlbedoArrayId() const
{
    return albedoArrayId;
}

unsigned int TerrainGpuResources::NormalArrayId() const
{
    return normalArrayId;
}

unsigned int TerrainGpuResources::RoughnessArrayId() const
{
    return roughnessArrayId;
}

unsigned int TerrainGpuResources::WeightArrayId() const
{
    return weightArrayId;
}

int TerrainGpuResources::WeightMapCount() const
{
    return weightMapCount;
}

bool TerrainGpuResources::CopyWeightMapTexel(
    int map,
    int x,
    int z,
    unsigned char& red,
    unsigned char& green,
    unsigned char& blue,
    unsigned char& alpha) const
{
    if (map < 0 || map >= weightMapCount || weightMapBytes.empty() || !hasSpec)
    {
        return false;
    }
    const int width = lastSpec.weightResolutionX;
    const int height = lastSpec.weightResolutionZ;
    if (x < 0 || z < 0 || x >= width || z >= height)
    {
        return false;
    }
    const std::size_t offset = static_cast<std::size_t>(
        ((map * height + z) * width + x) * world::kTerrainWeightsPerMap);
    if (offset + 3 >= weightMapBytes.size())
    {
        return false;
    }
    red = weightMapBytes[offset];
    green = weightMapBytes[offset + 1];
    blue = weightMapBytes[offset + 2];
    alpha = weightMapBytes[offset + 3];
    return true;
}

void TerrainGpuResources::RebuildAlbedoArray(const world::TerrainSpec& spec)
{
    if (albedoArrayId != 0)
    {
        glDeleteTextures(1, &albedoArrayId);
        albedoArrayId = 0;
    }
    albedoArrayWidth = 0;
    albedoArrayHeight = 0;
    const int layers = layerCount > 0 ? layerCount : 1;
    std::vector<Image> images(static_cast<std::size_t>(layers));
    std::vector<unsigned char> owns(static_cast<std::size_t>(layers), 0);
    for (int layer = 0; layer < layers; ++layer)
    {
        LoadTerrainChannelImage(
            world::TerrainLayerTextureIdentity(spec, layer),
            authoringCookedRoot,
            authoringSourceRoot,
            images[static_cast<std::size_t>(layer)],
            owns[static_cast<std::size_t>(layer)]);
    }
    albedoArrayId = UploadTerrainChannelArray(
        layers,
        images.data(),
        owns.data(),
        Color{255, 255, 255, 255},
        Color{255, 0, 255, 255},
        true,
        albedoArrayWidth,
        albedoArrayHeight);
}

void TerrainGpuResources::RebuildNormalArray(const world::TerrainSpec& spec)
{
    if (normalArrayId != 0)
    {
        glDeleteTextures(1, &normalArrayId);
        normalArrayId = 0;
    }
    const int layers = layerCount > 0 ? layerCount : 1;
    std::vector<Image> images(static_cast<std::size_t>(layers));
    std::vector<unsigned char> owns(static_cast<std::size_t>(layers), 0);
    for (int layer = 0; layer < layers; ++layer)
    {
        LoadTerrainChannelImage(
            world::TerrainLayerNormalIdentity(spec, layer),
            authoringCookedRoot,
            authoringSourceRoot,
            images[static_cast<std::size_t>(layer)],
            owns[static_cast<std::size_t>(layer)]);
        if (owns[static_cast<std::size_t>(layer)] != 0
            && world::TerrainNormalMapIdentityIsDirectX(world::TerrainLayerNormalIdentity(spec, layer)))
        {
            ApplyDirectXNormalGreenFlip(images[static_cast<std::size_t>(layer)]);
        }
    }
    const Color flatNormal{128, 128, 255, 255};
    int width = 0;
    int height = 0;
    normalArrayId = UploadTerrainChannelArray(
        layers,
        images.data(),
        owns.data(),
        flatNormal,
        flatNormal,
        true,
        width,
        height);
}

void TerrainGpuResources::RebuildRoughnessArray(const world::TerrainSpec& spec)
{
    if (roughnessArrayId != 0)
    {
        glDeleteTextures(1, &roughnessArrayId);
        roughnessArrayId = 0;
    }
    const int layers = layerCount > 0 ? layerCount : 1;
    std::vector<Image> images(static_cast<std::size_t>(layers));
    std::vector<unsigned char> owns(static_cast<std::size_t>(layers), 0);
    for (int layer = 0; layer < layers; ++layer)
    {
        LoadTerrainChannelImage(
            world::TerrainLayerRoughnessIdentity(spec, layer),
            authoringCookedRoot,
            authoringSourceRoot,
            images[static_cast<std::size_t>(layer)],
            owns[static_cast<std::size_t>(layer)]);
    }
    const unsigned char quantum = world::TerrainDefaultRoughnessQuantum();
    const Color defaultRough{quantum, quantum, quantum, 255};
    int width = 0;
    int height = 0;
    roughnessArrayId = UploadTerrainChannelArray(
        layers,
        images.data(),
        owns.data(),
        defaultRough,
        defaultRough,
        true,
        width,
        height);
}

void TerrainGpuResources::SyncWeightArray(const world::TerrainSpec& spec)
{
    std::vector<unsigned char> bytes;
    world::BuildTerrainWeightMapRgba(spec, bytes);
    const int maps = world::TerrainPackedWeightMapCount(spec);
    if (weightArrayId != 0 && bytes == weightMapBytes && maps == weightMapCount)
    {
        return;
    }
    if (weightArrayId != 0)
    {
        glDeleteTextures(1, &weightArrayId);
        weightArrayId = 0;
    }
    const int width = spec.weightResolutionX;
    const int height = spec.weightResolutionZ;
    if (width <= 0 || height <= 0 || maps <= 0 || bytes.empty())
    {
        weightMapBytes.clear();
        weightMapCount = 1;
        return;
    }
    unsigned int id = 0;
    glGenTextures(1, &id);
    if (id == 0)
    {
        return;
    }
    glBindTexture(GL_TEXTURE_2D_ARRAY, id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage3D(
        GL_TEXTURE_2D_ARRAY,
        0,
        GL_RGBA8,
        width,
        height,
        maps,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        bytes.data());
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    weightArrayId = id;
    weightMapCount = maps;
    weightMapBytes = std::move(bytes);
    ++weightUploadCount;
}
}
