#include "render/GroundCoverGpu.h"

#include "assets/RuntimePngResolve.h"

#include "raymath.h"
#include "rlgl.h"

#include <cstdio>
#include <cstring>

namespace render
{
namespace
{
constexpr Color kFallbackGroundCoverTint{86, 140, 62, 255};

Matrix GroundCoverInstanceMatrix(const world::TerrainGroundCoverInstance& instance)
{
    Matrix transform{};
    transform.m0 = instance.axisX.x * instance.width;
    transform.m1 = instance.axisX.y * instance.width;
    transform.m2 = instance.axisX.z * instance.width;
    transform.m3 = 0.0f;
    transform.m4 = instance.axisY.x * instance.height;
    transform.m5 = instance.axisY.y * instance.height;
    transform.m6 = instance.axisY.z * instance.height;
    transform.m7 = 0.0f;
    transform.m8 = instance.axisZ.x * instance.width;
    transform.m9 = instance.axisZ.y * instance.width;
    transform.m10 = instance.axisZ.z * instance.width;
    transform.m11 = 0.0f;
    transform.m12 = instance.position.x;
    transform.m13 = instance.position.y;
    transform.m14 = instance.position.z;
    transform.m15 = 1.0f;
    return transform;
}

void ClearMeshInstanceDivisors(const Mesh& mesh, int location)
{
    if (location < 0)
    {
        return;
    }
    const bool boundArray = mesh.vaoId != 0 && rlEnableVertexArray(mesh.vaoId);
    for (unsigned int column = 0; column < 4; ++column)
    {
        const unsigned int attribute = static_cast<unsigned int>(location) + column;
        rlSetVertexAttributeDivisor(attribute, 0);
        rlDisableVertexAttribute(attribute);
    }
    if (boundArray)
    {
        rlDisableVertexArray();
    }
}

void SetInstancedFlag(const Shader& shader, int location, int enabled)
{
    if (shader.id == 0 || location < 0)
    {
        return;
    }
    SetShaderValue(shader, location, &enabled, SHADER_UNIFORM_INT);
}

void SetCutoutFlag(const Shader& shader, int enabled)
{
    if (shader.id == 0)
    {
        return;
    }
    const int location = GetShaderLocation(shader, "groundCoverCutout");
    if (location >= 0)
    {
        SetShaderValue(shader, location, &enabled, SHADER_UNIFORM_INT);
    }
}

void AppendCardQuad(
    std::vector<float>& positions,
    std::vector<float>& texcoords,
    std::vector<float>& normals,
    std::vector<unsigned short>& indices,
    const float corners[12],
    const float normal[3])
{
    const unsigned short base = static_cast<unsigned short>(positions.size() / 3);
    for (int vertex = 0; vertex < 4; ++vertex)
    {
        positions.push_back(corners[vertex * 3 + 0]);
        positions.push_back(corners[vertex * 3 + 1]);
        positions.push_back(corners[vertex * 3 + 2]);
        normals.push_back(normal[0]);
        normals.push_back(normal[1]);
        normals.push_back(normal[2]);
    }
    const float uvs[8] = {0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    for (int vertex = 0; vertex < 4; ++vertex)
    {
        texcoords.push_back(uvs[vertex * 2 + 0]);
        texcoords.push_back(uvs[vertex * 2 + 1]);
    }
    const unsigned short front[6] = {0, 1, 2, 0, 2, 3};
    for (unsigned short index : front)
    {
        indices.push_back(static_cast<unsigned short>(base + index));
    }
}
}

GroundCoverGpuResources::~GroundCoverGpuResources()
{
    Unload();
}

void GroundCoverGpuResources::SetAuthoringCookedRoot(const std::filesystem::path& cookedRoot)
{
    authoringCookedRoot = cookedRoot;
}

void GroundCoverGpuResources::SetAuthoringSourceRoot(const std::filesystem::path& sourceRoot)
{
    authoringSourceRoot = sourceRoot;
}

void GroundCoverGpuResources::EnsureCardMesh()
{
    if (cardMeshLoaded)
    {
        return;
    }

    std::vector<float> positions;
    std::vector<float> texcoords;
    std::vector<float> normals;
    std::vector<unsigned short> indices;
    const float xyFront[12] = {
        -0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f, -0.5f, 1.0f, 0.0f};
    const float xyBack[12] = {
        0.5f, 0.0f, 0.0f, -0.5f, 0.0f, 0.0f, -0.5f, 1.0f, 0.0f, 0.5f, 1.0f, 0.0f};
    const float zyFront[12] = {
        0.0f, 0.0f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.5f, 0.0f, 1.0f, -0.5f};
    const float zyBack[12] = {
        0.0f, 0.0f, 0.5f, 0.0f, 0.0f, -0.5f, 0.0f, 1.0f, -0.5f, 0.0f, 1.0f, 0.5f};
    const float nPosZ[3] = {0.0f, 0.0f, 1.0f};
    const float nNegZ[3] = {0.0f, 0.0f, -1.0f};
    const float nPosX[3] = {1.0f, 0.0f, 0.0f};
    const float nNegX[3] = {-1.0f, 0.0f, 0.0f};
    AppendCardQuad(positions, texcoords, normals, indices, xyFront, nPosZ);
    AppendCardQuad(positions, texcoords, normals, indices, xyBack, nNegZ);
    AppendCardQuad(positions, texcoords, normals, indices, zyFront, nPosX);
    AppendCardQuad(positions, texcoords, normals, indices, zyBack, nNegX);

    cardMesh = {};
    cardMesh.vertexCount = static_cast<int>(positions.size() / 3);
    cardMesh.triangleCount = static_cast<int>(indices.size() / 3);
    cardMesh.vertices = static_cast<float*>(MemAlloc(static_cast<unsigned int>(positions.size() * sizeof(float))));
    cardMesh.texcoords = static_cast<float*>(MemAlloc(static_cast<unsigned int>(texcoords.size() * sizeof(float))));
    cardMesh.normals = static_cast<float*>(MemAlloc(static_cast<unsigned int>(normals.size() * sizeof(float))));
    cardMesh.indices = static_cast<unsigned short*>(
        MemAlloc(static_cast<unsigned int>(indices.size() * sizeof(unsigned short))));
    if (cardMesh.vertices == nullptr || cardMesh.texcoords == nullptr || cardMesh.normals == nullptr
        || cardMesh.indices == nullptr)
    {
        UnloadCardMesh();
        return;
    }
    std::memcpy(cardMesh.vertices, positions.data(), positions.size() * sizeof(float));
    std::memcpy(cardMesh.texcoords, texcoords.data(), texcoords.size() * sizeof(float));
    std::memcpy(cardMesh.normals, normals.data(), normals.size() * sizeof(float));
    std::memcpy(cardMesh.indices, indices.data(), indices.size() * sizeof(unsigned short));
    UploadMesh(&cardMesh, false);
    cardMeshLoaded = true;
}

void GroundCoverGpuResources::UnloadCardMesh()
{
    if (cardMeshLoaded)
    {
        UnloadMesh(cardMesh);
    }
    cardMesh = {};
    cardMeshLoaded = false;
}

void GroundCoverGpuResources::EnsureFallbackTexture()
{
    if (fallbackLoaded && fallbackTexture.id != 0)
    {
        return;
    }
    Image image = GenImageColor(4, 8, kFallbackGroundCoverTint);
    fallbackTexture = LoadTextureFromImage(image);
    UnloadImage(image);
    if (fallbackTexture.id != 0)
    {
        SetTextureFilter(fallbackTexture, TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(fallbackTexture, TEXTURE_WRAP_CLAMP);
        fallbackLoaded = true;
    }
}

void GroundCoverGpuResources::EnsureMaterial() const
{
    if (drawMaterialLoaded)
    {
        return;
    }
    drawMaterial = LoadMaterialDefault();
    drawMaterialLoaded = true;
}

void GroundCoverGpuResources::UnloadTextures()
{
    for (TextureSlot& slot : textures)
    {
        if (slot.loaded && slot.texture.id != 0)
        {
            UnloadTexture(slot.texture);
        }
    }
    textures.clear();
    if (fallbackLoaded && fallbackTexture.id != 0)
    {
        UnloadTexture(fallbackTexture);
    }
    fallbackTexture = {};
    fallbackLoaded = false;
}

const Texture2D* GroundCoverGpuResources::FindTexture(std::string_view identity) const
{
    for (const TextureSlot& slot : textures)
    {
        if (slot.identity == identity)
        {
            return slot.loaded && slot.texture.id != 0 ? &slot.texture : nullptr;
        }
    }
    return nullptr;
}

void GroundCoverGpuResources::SyncTextures(const world::TerrainSpec& spec)
{
    std::vector<std::string> needed;
    for (const world::TerrainGroundCoverEntry& entry : spec.groundCoverEntries)
    {
        bool seen = false;
        for (const std::string& existing : needed)
        {
            if (existing == entry.textureIdentity)
            {
                seen = true;
                break;
            }
        }
        if (!seen)
        {
            needed.push_back(entry.textureIdentity);
        }
    }

    std::vector<TextureSlot> next;
    next.reserve(needed.size());
    for (const std::string& identity : needed)
    {
        TextureSlot slot{};
        slot.identity = identity;
        for (TextureSlot& existing : textures)
        {
            if (existing.identity == identity && existing.loaded && existing.texture.id != 0)
            {
                slot = existing;
                existing.loaded = false;
                existing.texture = {};
                break;
            }
        }
        if (!slot.loaded)
        {
            const assets::RuntimePngLoadResolution resolved =
                assets::ResolveRuntimePngLoadFile(
                    identity, authoringCookedRoot, {}, authoringSourceRoot);
            if (assets::RuntimePngLoadFileIsAvailable(resolved))
            {
                const Texture2D loaded = LoadTexture(resolved.path.string().c_str());
                if (loaded.id != 0)
                {
                    slot.texture = loaded;
                    SetTextureFilter(slot.texture, TEXTURE_FILTER_BILINEAR);
                    SetTextureWrap(slot.texture, TEXTURE_WRAP_CLAMP);
                    slot.loaded = true;
                    ++textureLoadCount;
                }
            }
        }
        next.push_back(slot);
    }
    for (TextureSlot& leftover : textures)
    {
        if (leftover.loaded && leftover.texture.id != 0)
        {
            UnloadTexture(leftover.texture);
        }
    }
    textures = std::move(next);
}

void GroundCoverGpuResources::Unload()
{
    UnloadCardMesh();
    UnloadTextures();
    if (drawMaterialLoaded)
    {
        drawMaterial.shader = Shader{};
        UnloadMaterial(drawMaterial);
        drawMaterial = {};
        drawMaterialLoaded = false;
    }
    instances.clear();
    plan = {};
    matrices.clear();
    cacheValid = false;
    signature = 0;
    instanceCount = 0;
    renderGroupCount = 0;
    instancedSubmissions = 0;
    ordinarySubmissions = 0;
}

void GroundCoverGpuResources::Sync(const world::TerrainSpec* spec)
{
    if (spec == nullptr || !spec->enabled || !world::TerrainGroundCoverShouldWrite(*spec)
        || !world::TerrainGroundCoverDataIsValid(*spec))
    {
        instances.clear();
        plan = {};
        cacheValid = false;
        instanceCount = 0;
        renderGroupCount = 0;
        UnloadTextures();
        return;
    }
    EnsureCardMesh();
    EnsureFallbackTexture();
    SyncTextures(*spec);
    const std::uint64_t nextSignature = world::TerrainGroundCoverDeriveSignature(*spec);
    if (!cacheValid || nextSignature != signature)
    {
        world::BuildTerrainGroundCoverInstances(*spec, instances);
        world::BuildTerrainGroundCoverRenderPlan(*spec, instances, plan);
        signature = nextSignature;
        cacheValid = true;
    }
    instanceCount = instances.size();
    renderGroupCount = plan.groups.size();
}

void GroundCoverGpuResources::ResetDrawStats()
{
    instancedSubmissions = 0;
    ordinarySubmissions = 0;
}

std::size_t GroundCoverGpuResources::InstanceCount() const
{
    return instanceCount;
}

std::size_t GroundCoverGpuResources::RenderGroupCount() const
{
    return renderGroupCount;
}

std::size_t GroundCoverGpuResources::InstancedSubmissionCount() const
{
    return instancedSubmissions;
}

std::size_t GroundCoverGpuResources::OrdinarySubmissionCount() const
{
    return ordinarySubmissions;
}

std::size_t GroundCoverGpuResources::UniqueTextureCount() const
{
    std::size_t count = 0;
    for (const TextureSlot& slot : textures)
    {
        if (slot.loaded && slot.texture.id != 0)
        {
            ++count;
        }
    }
    return count;
}

std::size_t GroundCoverGpuResources::TextureLoadCount() const
{
    return textureLoadCount;
}

void GroundCoverGpuResources::Draw(
    const world::TerrainSpec& terrain,
    const ModelDrawOverride* override,
    bool allowShadowCast) const
{
    instancedSubmissions = 0;
    ordinarySubmissions = 0;
    if (!allowShadowCast)
    {
        return;
    }
    if (!cardMeshLoaded || instances.empty() || plan.groups.empty())
    {
        return;
    }
    EnsureMaterial();

    const Shader overrideShader = override != nullptr ? override->shader : Shader{};
    const int instanceAttribute = overrideShader.id != 0 && overrideShader.locs != nullptr
        ? overrideShader.locs[SHADER_LOC_VERTEX_INSTANCETRANSFORM]
        : -1;
    const int instancedUniform = instanceAttribute >= 0
        ? GetShaderLocation(overrideShader, "vegetationInstanced")
        : -1;
    const bool canInstance = instanceAttribute >= 0 && instancedUniform >= 0;
    SetCutoutFlag(overrideShader, 1);

    Material material = drawMaterial;
    if (overrideShader.id != 0)
    {
        material.shader = overrideShader;
    }
    if (material.maps != nullptr)
    {
        material.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
        if (override != nullptr && override->slot1Texture.id != 0)
        {
            material.maps[1].texture = override->slot1Texture;
        }
    }

    for (const world::TerrainGroundCoverRenderGroup& group : plan.groups)
    {
        if (group.count == 0
            || group.entryIndex < 0
            || group.entryIndex >= static_cast<int>(terrain.groundCoverEntries.size()))
        {
            continue;
        }
        const std::string& identity =
            terrain.groundCoverEntries[static_cast<std::size_t>(group.entryIndex)].textureIdentity;
        const Texture2D* albedo = FindTexture(identity);
        if (albedo == nullptr)
        {
            albedo = fallbackLoaded ? &fallbackTexture : nullptr;
        }
        if (material.maps != nullptr)
        {
            material.maps[MATERIAL_MAP_DIFFUSE].texture =
                albedo != nullptr ? *albedo : material.maps[MATERIAL_MAP_DIFFUSE].texture;
        }

        if (!canInstance)
        {
            SetInstancedFlag(overrideShader, instancedUniform, 0);
            for (std::size_t index = 0; index < group.count; ++index)
            {
                const int instanceIndex = plan.instanceOrder[group.begin + index];
                if (instanceIndex < 0 || instanceIndex >= static_cast<int>(instances.size()))
                {
                    continue;
                }
                DrawMesh(
                    cardMesh,
                    material,
                    GroundCoverInstanceMatrix(instances[static_cast<std::size_t>(instanceIndex)]));
                ++ordinarySubmissions;
            }
            continue;
        }

        matrices.resize(group.count);
        for (std::size_t index = 0; index < group.count; ++index)
        {
            const int instanceIndex = plan.instanceOrder[group.begin + index];
            matrices[index] =
                GroundCoverInstanceMatrix(instances[static_cast<std::size_t>(instanceIndex)]);
        }
        SetInstancedFlag(overrideShader, instancedUniform, 1);
        DrawMeshInstanced(
            cardMesh, material, matrices.data(), static_cast<int>(group.count));
        ClearMeshInstanceDivisors(cardMesh, instanceAttribute);
        ++instancedSubmissions;
        SetInstancedFlag(overrideShader, instancedUniform, 0);
    }

    SetCutoutFlag(overrideShader, 0);
}
}
