#include "render/StaticModelScene.h"

#include "assets/StaticGlb.h"
#include "gameplay/ItemPickupRuntime.h"
#include "platform/RuntimePaths.h"
#include "render/LoadedModelMaterials.h"
#include "world/TerrainVegetation.h"

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace render
{
namespace
{
struct SourceStamp
{
    std::uint64_t writeTimeTicks = 0;
    std::uintmax_t size = 0;
};

bool ReadStamp(const std::filesystem::path& path, SourceStamp& stamp)
{
    std::error_code error;
    const auto time = std::filesystem::last_write_time(path, error);
    if (error)
    {
        return false;
    }
    const auto size = std::filesystem::file_size(path, error);
    if (error)
    {
        return false;
    }
    stamp.writeTimeTicks = static_cast<std::uint64_t>(time.time_since_epoch().count());
    stamp.size = size;
    return true;
}

bool StampsEqual(const SourceStamp& a, const SourceStamp& b)
{
    return a.writeTimeTicks == b.writeTimeTicks && a.size == b.size;
}

bool ModelHasRenderableMesh(const Model& model)
{
    if (model.meshCount <= 0 || model.meshes == nullptr)
    {
        return false;
    }
    for (int i = 0; i < model.meshCount; ++i)
    {
        if (model.meshes[i].vertexCount > 0)
        {
            return true;
        }
    }
    return false;
}

void DrawPropTransform(const world::StaticPropSpec& spec)
{
    rlPushMatrix();
    rlTranslatef(spec.position.x, spec.position.y, spec.position.z);
    rlRotatef(spec.rotationDegrees.z, 0.0f, 0.0f, 1.0f);
    rlRotatef(spec.rotationDegrees.y, 0.0f, 1.0f, 0.0f);
    rlRotatef(spec.rotationDegrees.x, 1.0f, 0.0f, 0.0f);
    rlScalef(spec.scale.x, spec.scale.y, spec.scale.z);
}

// DrawModel/DrawMesh can leave default-shader colDiffuse/texture0 on the
// last material. Restore white * texture0 so later rlBegin greybox draws in
// this 3D pass use the expected default material. Persistent Gameplay blank
// frames were leftover Preview/thumbnail clip planes, not leftover tint.
void BeginIsolatedModelDraw()
{
    rlDrawRenderBatchActive();
}

constexpr Color kMissingPropColor{120, 72, 88, 255};
constexpr Color kMissingVegetationColor{120, 72, 88, 255};

Matrix VegetationInstanceMatrix(const world::TerrainVegetationInstance& instance)
{
    const float scale = instance.uniformScale;
    Matrix transform{};
    transform.m0 = instance.axisX.x * scale;
    transform.m1 = instance.axisX.y * scale;
    transform.m2 = instance.axisX.z * scale;
    transform.m3 = 0.0f;
    transform.m4 = instance.axisY.x * scale;
    transform.m5 = instance.axisY.y * scale;
    transform.m6 = instance.axisY.z * scale;
    transform.m7 = 0.0f;
    transform.m8 = instance.axisZ.x * scale;
    transform.m9 = instance.axisZ.y * scale;
    transform.m10 = instance.axisZ.z * scale;
    transform.m11 = 0.0f;
    transform.m12 = instance.position.x;
    transform.m13 = instance.position.y;
    transform.m14 = instance.position.z;
    transform.m15 = 1.0f;
    return transform;
}

void DrawVegetationInstanceMatrix(const Matrix& transform)
{
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(transform));
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

void SetVegetationInstanced(const Shader& shader, int location, int enabled)
{
    if (shader.id == 0 || location < 0)
    {
        return;
    }
    SetShaderValue(shader, location, &enabled, SHADER_UNIFORM_INT);
}
}

struct StaticModelSceneStore::GpuState
{
    struct Entry
    {
        Model model{};
        SourceStamp stamp{};
        core::Vec3 localMin{};
        core::Vec3 localMax{};
        bool hasModel = false;
        bool failed = false;
        bool hasBounds = false;
    };

    std::unordered_map<std::string, Entry> entries;
    mutable std::size_t drawSubmissions = 0;
    mutable std::size_t highlightSubmissions = 0;
    mutable std::size_t gameplayHighlightSubmissions = 0;
    mutable std::size_t vegetationInstanceCount = 0;
    mutable std::size_t vegetationRenderGroupCount = 0;
    mutable std::size_t vegetationInstancedSubmissions = 0;
    mutable std::size_t vegetationOrdinarySubmissions = 0;
    mutable std::vector<std::string> submittedIdentities;
    std::size_t loadCount = 0;
    mutable bool vegetationCacheValid = false;
    mutable std::uint64_t vegetationSignature = 0;
    mutable std::vector<world::TerrainVegetationInstance> vegetationInstances;
    mutable world::TerrainVegetationRenderPlan vegetationPlan;
    mutable std::vector<Matrix> vegetationMatrices;
};

StaticModelSceneStore::StaticModelSceneStore()
    : gpu(std::make_unique<GpuState>())
{
}

StaticModelSceneStore::~StaticModelSceneStore()
{
    Shutdown();
}

void StaticModelSceneStore::Shutdown()
{
    if (gpu == nullptr)
    {
        return;
    }
    for (auto& pair : gpu->entries)
    {
        if (pair.second.hasModel)
        {
            UnloadModel(pair.second.model);
            pair.second.model = {};
            pair.second.hasModel = false;
        }
    }
    gpu->entries.clear();
}

void StaticModelSceneStore::Sync(
    const world::LevelDefinition& level,
    std::string_view extraIdentity,
    const world::LevelDefinition* extraLevel,
    const gameplay::GameplayDefinitionRegistry* itemDefinitions)
{
    if (gpu == nullptr)
    {
        gpu = std::make_unique<GpuState>();
    }

    std::unordered_set<std::string> needed;
    needed.reserve(level.staticProps.size() + level.itemPickups.size() + 1);
    const auto collectIdentities = [&](const world::LevelDefinition& source) {
        for (const world::StaticPropSpec& prop : source.staticProps)
        {
            if (world::StaticPropIdentityIsValid(prop.modelIdentity))
            {
                needed.insert(prop.modelIdentity);
            }
        }
        for (const world::ItemPickupSpec& pickup : source.itemPickups)
        {
            if (world::StaticPropIdentityIsValid(pickup.modelIdentity))
            {
                needed.insert(pickup.modelIdentity);
            }
            if (itemDefinitions != nullptr)
            {
                const std::string_view resolved =
                    gameplay::ResolveItemPickupWorldModel(pickup, itemDefinitions);
                if (world::StaticPropIdentityIsValid(resolved))
                {
                    needed.insert(std::string(resolved));
                }
            }
        }
        if (source.hasTerrain)
        {
            for (const world::TerrainVegetationEntry& entry : source.terrain.vegetationEntries)
            {
                if (world::StaticPropIdentityIsValid(entry.modelIdentity))
                {
                    needed.insert(entry.modelIdentity);
                }
            }
        }
    };
    collectIdentities(level);
    if (itemDefinitions != nullptr)
    {
        for (const gameplay::GameplayDefinition& definition : itemDefinitions->Definitions())
            if (definition.category == gameplay::GameplayDefinitionCategory::Item
                && gameplay::IsValidItemWorldModelIdentity(definition.item.worldModelIdentity))
                needed.insert(definition.item.worldModelIdentity);
    }
    if (extraLevel != nullptr)
    {
        collectIdentities(*extraLevel);
    }
    if (world::StaticPropIdentityIsValid(extraIdentity))
    {
        needed.insert(std::string(extraIdentity));
    }

    for (auto it = gpu->entries.begin(); it != gpu->entries.end();)
    {
        if (needed.find(it->first) == needed.end())
        {
            if (it->second.hasModel)
            {
                UnloadModel(it->second.model);
            }
            it = gpu->entries.erase(it);
        }
        else
        {
            ++it;
        }
    }

    for (const std::string& identity : needed)
    {
        GpuState::Entry& entry = gpu->entries[identity];
        const std::filesystem::path path = platform::RuntimeAssetPath(identity);
        SourceStamp stamp{};
        const bool haveStamp = ReadStamp(path, stamp);
        if (entry.hasModel && haveStamp && StampsEqual(entry.stamp, stamp))
        {
            continue;
        }
        if (entry.failed && haveStamp && StampsEqual(entry.stamp, stamp))
        {
            continue;
        }
        if (entry.hasModel)
        {
            UnloadModel(entry.model);
            entry.model = {};
            entry.hasModel = false;
        }
        entry.failed = false;
        entry.hasBounds = false;
        entry.stamp = stamp;
        if (!haveStamp)
        {
            entry.failed = true;
            continue;
        }
        const assets::StaticGlbValidation validation = assets::ValidateStaticGlbFile(path);
        if (validation.status != assets::StaticGlbStatus::Ok)
        {
            entry.failed = true;
            continue;
        }
        Model model = LoadModel(path.string().c_str());
        ++gpu->loadCount;
        if (!ModelHasRenderableMesh(model))
        {
            UnloadModel(model);
            entry.failed = true;
            continue;
        }
        PrepareLoadedModelMaterials(model);
        const BoundingBox modelBounds = GetModelBoundingBox(model);
        entry.localMin = {modelBounds.min.x, modelBounds.min.y, modelBounds.min.z};
        entry.localMax = {modelBounds.max.x, modelBounds.max.y, modelBounds.max.z};
        entry.hasBounds = true;
        entry.model = model;
        entry.hasModel = true;
    }
}

void StaticModelSceneStore::DrawAttachment(std::string_view identity,
    const animation::Matrix4& transform, const ModelDrawOverride* override) const
{
    if (gpu == nullptr) return;
    const auto it = gpu->entries.find(std::string(identity));
    if (it == gpu->entries.end() || !it->second.hasModel) return;
    ++gpu->drawSubmissions;
    gpu->submittedIdentities.emplace_back(identity);
    BeginIsolatedModelDraw();
    rlPushMatrix();
    rlMultMatrixf(transform.values);
    DrawModelPreservingMaterials(it->second.model, Vector3{}, 1.0f, WHITE, override);
    rlPopMatrix();
    RestoreGreyboxImmediateState();
}

bool StaticModelSceneStore::HasModel(std::string_view identity) const
{
    if (gpu == nullptr)
    {
        return false;
    }
    const auto it = gpu->entries.find(std::string(identity));
    return it != gpu->entries.end() && it->second.hasModel;
}

bool StaticModelSceneStore::IsFailed(std::string_view identity) const
{
    if (gpu == nullptr)
    {
        return false;
    }
    const auto it = gpu->entries.find(std::string(identity));
    return it != gpu->entries.end() && it->second.failed;
}

bool StaticModelSceneStore::TryGetLoadedLocalBounds(
    std::string_view identity,
    core::Vec3& localMin,
    core::Vec3& localMax) const
{
    if (gpu == nullptr)
    {
        return false;
    }
    const auto it = gpu->entries.find(std::string(identity));
    if (it == gpu->entries.end() || !it->second.hasBounds)
    {
        return false;
    }
    localMin = it->second.localMin;
    localMax = it->second.localMax;
    return true;
}

std::size_t StaticModelSceneStore::UniqueLoadedCount() const
{
    if (gpu == nullptr)
    {
        return 0;
    }
    std::size_t count = 0;
    for (const auto& pair : gpu->entries)
    {
        if (pair.second.hasModel)
        {
            ++count;
        }
    }
    return count;
}

std::size_t StaticModelSceneStore::CachedIdentityCount() const
{
    if (gpu == nullptr)
    {
        return 0;
    }
    return gpu->entries.size();
}

std::size_t StaticModelSceneStore::LoadCount() const
{
    if (gpu == nullptr)
    {
        return 0;
    }
    return gpu->loadCount;
}

void StaticModelSceneStore::ResetDrawStats() const
{
    if (gpu == nullptr)
    {
        return;
    }
    gpu->drawSubmissions = 0;
    gpu->highlightSubmissions = 0;
    gpu->gameplayHighlightSubmissions = 0;
    gpu->vegetationInstanceCount = 0;
    gpu->vegetationRenderGroupCount = 0;
    gpu->vegetationInstancedSubmissions = 0;
    gpu->vegetationOrdinarySubmissions = 0;
    gpu->submittedIdentities.clear();
}

std::size_t StaticModelSceneStore::DrawSubmissionCount() const
{
    if (gpu == nullptr)
    {
        return 0;
    }
    return gpu->drawSubmissions;
}

std::size_t StaticModelSceneStore::VegetationInstanceCount() const
{
    return gpu == nullptr ? 0 : gpu->vegetationInstanceCount;
}

std::size_t StaticModelSceneStore::VegetationRenderGroupCount() const
{
    return gpu == nullptr ? 0 : gpu->vegetationRenderGroupCount;
}

std::size_t StaticModelSceneStore::VegetationInstancedSubmissionCount() const
{
    return gpu == nullptr ? 0 : gpu->vegetationInstancedSubmissions;
}

std::size_t StaticModelSceneStore::VegetationOrdinarySubmissionCount() const
{
    return gpu == nullptr ? 0 : gpu->vegetationOrdinarySubmissions;
}

std::size_t StaticModelSceneStore::HighlightSubmissionCount() const
{
    if (gpu == nullptr)
    {
        return 0;
    }
    return gpu->highlightSubmissions;
}

std::size_t StaticModelSceneStore::GameplayHighlightSubmissionCount() const
{
    if (gpu == nullptr)
    {
        return 0;
    }
    return gpu->gameplayHighlightSubmissions;
}

bool StaticModelSceneStore::SubmittedIdentity(std::string_view identity) const
{
    if (gpu == nullptr)
    {
        return false;
    }
    for (const std::string& submitted : gpu->submittedIdentities)
    {
        if (submitted == identity)
        {
            return true;
        }
    }
    return false;
}

std::vector<std::string> StaticModelSceneStore::SubmittedIdentities() const
{
    if (gpu == nullptr)
    {
        return {};
    }
    return gpu->submittedIdentities;
}

void RestoreGreyboxImmediateState()
{
    rlDrawRenderBatchActive();
    rlEnableShader(rlGetShaderIdDefault());
    const int* locs = rlGetShaderLocsDefault();
    if (locs != nullptr)
    {
        if (locs[SHADER_LOC_COLOR_DIFFUSE] >= 0)
        {
            const float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
            rlSetUniform(locs[SHADER_LOC_COLOR_DIFFUSE], white, SHADER_UNIFORM_VEC4, 1);
        }
        if (locs[SHADER_LOC_MAP_DIFFUSE] >= 0)
        {
            const int slot0 = 0;
            rlSetUniform(locs[SHADER_LOC_MAP_DIFFUSE], &slot0, SHADER_UNIFORM_INT, 1);
        }
    }
    rlActiveTextureSlot(0);
    rlEnableTexture(rlGetTextureIdDefault());
}

void StaticModelSceneStore::DrawPropTinted(
    const world::StaticPropSpec& spec,
    unsigned char red,
    unsigned char green,
    unsigned char blue,
    unsigned char alpha,
    const ModelDrawOverride* override) const
{
    if (!world::StaticPropTransformIsValid(spec))
    {
        return;
    }
    if (gpu != nullptr)
    {
        ++gpu->drawSubmissions;
        gpu->submittedIdentities.push_back(spec.modelIdentity);
    }
    BeginIsolatedModelDraw();
    DrawPropTransform(spec);
    const GpuState::Entry* entry = nullptr;
    if (gpu != nullptr)
    {
        const auto it = gpu->entries.find(spec.modelIdentity);
        if (it != gpu->entries.end())
        {
            entry = &it->second;
        }
    }
    const Color tint{red, green, blue, alpha};
    if (entry != nullptr && entry->hasModel)
    {
        DrawModelPreservingMaterials(
            entry->model, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, tint, override);
    }
    else
    {
        const bool useOverride = override != nullptr && override->shader.id != 0;
        if (useOverride)
        {
            BeginShaderMode(override->shader);
        }
        DrawCube(
            Vector3{0.0f, 0.0f, 0.0f},
            1.0f,
            1.0f,
            1.0f,
            alpha == 255 ? kMissingPropColor : tint);
        if (!useOverride)
        {
            DrawCubeWires(Vector3{0.0f, 0.0f, 0.0f}, 1.0f, 1.0f, 1.0f, WHITE);
        }
        if (useOverride)
        {
            EndShaderMode();
        }
    }
    rlPopMatrix();
    RestoreGreyboxImmediateState();
}

void StaticModelSceneStore::DrawTerrainVegetation(
    const world::TerrainSpec& terrain,
    const ModelDrawOverride* override) const
{
    if (gpu == nullptr || !terrain.enabled || !world::TerrainVegetationShouldWrite(terrain))
    {
        return;
    }
    const std::uint64_t signature = world::TerrainVegetationDeriveSignature(terrain);
    if (!gpu->vegetationCacheValid || signature != gpu->vegetationSignature)
    {
        world::BuildTerrainVegetationInstances(terrain, gpu->vegetationInstances);
        world::BuildTerrainVegetationRenderPlan(terrain, gpu->vegetationInstances, gpu->vegetationPlan);
        gpu->vegetationSignature = signature;
        gpu->vegetationCacheValid = true;
    }
    gpu->vegetationInstanceCount = gpu->vegetationInstances.size();
    gpu->vegetationRenderGroupCount = gpu->vegetationPlan.groups.size();
    gpu->vegetationInstancedSubmissions = 0;
    gpu->vegetationOrdinarySubmissions = 0;
    if (gpu->vegetationInstances.empty())
    {
        return;
    }

    const Shader overrideShader = override != nullptr ? override->shader : Shader{};
    const int instanceAttribute = overrideShader.id != 0 && overrideShader.locs != nullptr
        ? overrideShader.locs[SHADER_LOC_VERTEX_INSTANCETRANSFORM]
        : -1;
    const int instancedUniform = instanceAttribute >= 0
        ? GetShaderLocation(overrideShader, "vegetationInstanced")
        : -1;
    const bool canInstance = instanceAttribute >= 0 && instancedUniform >= 0;

    BeginIsolatedModelDraw();
    for (const world::TerrainVegetationRenderGroup& group : gpu->vegetationPlan.groups)
    {
        if (group.count == 0
            || group.entryIndex < 0
            || group.entryIndex >= static_cast<int>(terrain.vegetationEntries.size()))
        {
            continue;
        }
        const std::string& identity =
            terrain.vegetationEntries[static_cast<std::size_t>(group.entryIndex)].modelIdentity;
        gpu->submittedIdentities.push_back(identity);
        GpuState::Entry* entry = nullptr;
        const auto found = gpu->entries.find(identity);
        if (found != gpu->entries.end())
        {
            entry = &found->second;
        }
        const bool hasModel = entry != nullptr && entry->hasModel && entry->model.meshCount > 0
            && entry->model.meshes != nullptr;

        if (!hasModel || !canInstance)
        {
            SetVegetationInstanced(overrideShader, instancedUniform, 0);
            const bool bindOverride = hasModel && overrideShader.id != 0;
            Shader originalShaders[kMaxCapturedModelMaterials]{};
            Texture2D originalSlot1[kMaxCapturedModelMaterials]{};
            int restoreCount = 0;
            if (bindOverride)
            {
                restoreCount = entry->model.materials == nullptr || entry->model.materialCount <= 0
                    ? 0
                    : (entry->model.materialCount < kMaxCapturedModelMaterials
                           ? entry->model.materialCount
                           : kMaxCapturedModelMaterials);
                for (int materialIndex = 0; materialIndex < restoreCount; ++materialIndex)
                {
                    originalShaders[materialIndex] = entry->model.materials[materialIndex].shader;
                    entry->model.materials[materialIndex].shader = overrideShader;
                    if (entry->model.materials[materialIndex].maps != nullptr)
                    {
                        originalSlot1[materialIndex] =
                            entry->model.materials[materialIndex].maps[1].texture;
                        if (override != nullptr && override->slot1Texture.id != 0)
                        {
                            entry->model.materials[materialIndex].maps[1].texture = override->slot1Texture;
                        }
                    }
                }
            }
            for (std::size_t index = 0; index < group.count; ++index)
            {
                const int instanceIndex = gpu->vegetationPlan.instanceOrder[group.begin + index];
                if (instanceIndex < 0
                    || instanceIndex >= static_cast<int>(gpu->vegetationInstances.size()))
                {
                    continue;
                }
                const Matrix transform =
                    VegetationInstanceMatrix(gpu->vegetationInstances[static_cast<std::size_t>(instanceIndex)]);
                DrawVegetationInstanceMatrix(transform);
                if (hasModel)
                {
                    DrawModel(entry->model, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
                }
                else
                {
                    DrawCube(Vector3{0.0f, 0.0f, 0.0f}, 1.0f, 1.0f, 1.0f, kMissingVegetationColor);
                }
                rlPopMatrix();
                ++gpu->vegetationOrdinarySubmissions;
            }
            if (bindOverride)
            {
                for (int materialIndex = 0; materialIndex < restoreCount; ++materialIndex)
                {
                    entry->model.materials[materialIndex].shader = originalShaders[materialIndex];
                    if (entry->model.materials[materialIndex].maps != nullptr)
                    {
                        entry->model.materials[materialIndex].maps[1].texture = originalSlot1[materialIndex];
                    }
                }
            }
            continue;
        }

        gpu->vegetationMatrices.resize(group.count);
        for (std::size_t index = 0; index < group.count; ++index)
        {
            const int instanceIndex = gpu->vegetationPlan.instanceOrder[group.begin + index];
            const world::TerrainVegetationInstance& instance =
                gpu->vegetationInstances[static_cast<std::size_t>(instanceIndex)];
            gpu->vegetationMatrices[index] =
                MatrixMultiply(entry->model.transform, VegetationInstanceMatrix(instance));
        }

        SetVegetationInstanced(overrideShader, instancedUniform, 1);
        for (int meshIndex = 0; meshIndex < entry->model.meshCount; ++meshIndex)
        {
            const Mesh& mesh = entry->model.meshes[meshIndex];
            if (mesh.vertexCount <= 0)
            {
                continue;
            }
            int materialIndex = 0;
            if (entry->model.meshMaterial != nullptr)
            {
                materialIndex = entry->model.meshMaterial[meshIndex];
            }
            if (entry->model.materials == nullptr || materialIndex < 0
                || materialIndex >= entry->model.materialCount)
            {
                continue;
            }
            Texture2D savedSlot1{};
            const bool rebindShadow = override != nullptr && override->slot1Texture.id != 0
                && entry->model.materials[materialIndex].maps != nullptr;
            if (rebindShadow)
            {
                savedSlot1 = entry->model.materials[materialIndex].maps[1].texture;
                entry->model.materials[materialIndex].maps[1].texture = override->slot1Texture;
            }
            Material material = entry->model.materials[materialIndex];
            material.shader = overrideShader;
            DrawMeshInstanced(
                mesh,
                material,
                gpu->vegetationMatrices.data(),
                static_cast<int>(group.count));
            ClearMeshInstanceDivisors(mesh, instanceAttribute);
            if (rebindShadow)
            {
                entry->model.materials[materialIndex].maps[1].texture = savedSlot1;
            }
            ++gpu->vegetationInstancedSubmissions;
        }
        SetVegetationInstanced(overrideShader, instancedUniform, 0);
    }
    RestoreGreyboxImmediateState();
}

void StaticModelSceneStore::DrawProp(const world::StaticPropSpec& spec) const
{
    DrawPropTinted(spec, 255, 255, 255, 255);
}

void StaticModelSceneStore::DrawProp(
    const world::StaticPropSpec& spec,
    const ModelDrawOverride& override) const
{
    DrawPropTinted(spec, 255, 255, 255, 255, &override);
}

void StaticModelSceneStore::DrawPlacementPreview(const world::StaticPropSpec& spec) const
{
    // Editor-only ghost tint. Must not LoadModel, permanently mutate the
    // shared Model materials, or change rlgl clip planes (M49 Gameplay contract).
    DrawPropTinted(spec, 96, 220, 236, 160);
}

void RestoreEditorModelHighlightState()
{
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
    rlEnableDepthTest();
    RestoreGreyboxImmediateState();
}

void StaticModelSceneStore::DrawSelectionHighlight(const world::StaticPropSpec& spec) const
{
    if (!world::StaticPropTransformIsValid(spec))
    {
        return;
    }
    if (gpu != nullptr)
    {
        ++gpu->highlightSubmissions;
    }
    // Depth-respecting tint, then a quieter x-ray pass so occluded parts of
    // the selected model stay readable. Restore every changed rlgl state.
    rlDrawRenderBatchActive();
    DrawPropTinted(spec, 255, 220, 72, 150);
    rlDrawRenderBatchActive();
    rlDisableDepthTest();
    rlDisableDepthMask();
    rlDisableBackfaceCulling();
    DrawPropTinted(spec, 255, 236, 96, 56);
    RestoreEditorModelHighlightState();
}

void StaticModelSceneStore::DrawGameplayTargetHighlight(
    const world::StaticPropSpec& spec,
    unsigned char alpha) const
{
    DrawGameplayTargetHighlight(spec, 255, 220, 72, alpha);
}

void StaticModelSceneStore::DrawGameplayTargetHighlight(
    const world::StaticPropSpec& spec,
    unsigned char red,
    unsigned char green,
    unsigned char blue,
    unsigned char alpha) const
{
    if (alpha == 0 || !world::StaticPropTransformIsValid(spec))
    {
        return;
    }
    if (gpu != nullptr)
    {
        ++gpu->gameplayHighlightSubmissions;
    }
    // Intensity maps to pass alpha. Gold Amount maps to tint RGB
    // (white → existing gold). Not a second alpha multiplier. No x-ray.
    rlDrawRenderBatchActive();
    DrawPropTinted(spec, red, green, blue, alpha);
    RestoreGreyboxImmediateState();
}
}
