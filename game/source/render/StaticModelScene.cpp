#include "render/StaticModelScene.h"

#include "assets/StaticGlb.h"
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
    mutable std::vector<std::string> submittedIdentities;
    std::size_t loadCount = 0;
    mutable std::uint64_t vegetationSignature = 0;
    mutable std::vector<world::TerrainVegetationInstance> vegetationInstances;
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
    const world::LevelDefinition* extraLevel)
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
    if (signature != gpu->vegetationSignature)
    {
        world::BuildTerrainVegetationInstances(terrain, gpu->vegetationInstances);
        gpu->vegetationSignature = signature;
    }
    if (gpu->vegetationInstances.empty())
    {
        return;
    }

    std::vector<world::TerrainVegetationDrawBatch> batches;
    world::BuildTerrainVegetationDrawBatches(gpu->vegetationInstances, batches);
    BeginIsolatedModelDraw();
    for (const world::TerrainVegetationDrawBatch& batch : batches)
    {
        if (batch.count == 0
            || batch.entryIndex < 0
            || batch.entryIndex >= static_cast<int>(terrain.vegetationEntries.size()))
        {
            continue;
        }
        const std::string& identity = terrain.vegetationEntries[static_cast<std::size_t>(batch.entryIndex)]
                                          .modelIdentity;
        if (gpu != nullptr)
        {
            gpu->drawSubmissions += batch.count;
            gpu->submittedIdentities.push_back(identity);
        }
        GpuState::Entry* entry = nullptr;
        const auto found = gpu->entries.find(identity);
        if (found != gpu->entries.end())
        {
            entry = &found->second;
        }

        ModelMaterialGpuSnapshot snapshot{};
        Shader originalShaders[kMaxCapturedModelMaterials]{};
        Texture2D originalSlot1[kMaxCapturedModelMaterials]{};
        int restoreCount = 0;
        const bool useOverride = override != nullptr && override->shader.id != 0 && entry != nullptr
            && entry->hasModel;
        if (useOverride)
        {
            snapshot = CaptureModelMaterialGpuState(entry->model);
            restoreCount = entry->model.materials == nullptr || entry->model.materialCount <= 0
                ? 0
                : (entry->model.materialCount < kMaxCapturedModelMaterials
                       ? entry->model.materialCount
                       : kMaxCapturedModelMaterials);
            for (int materialIndex = 0; materialIndex < restoreCount; ++materialIndex)
            {
                originalShaders[materialIndex] = entry->model.materials[materialIndex].shader;
                if (entry->model.materials[materialIndex].maps != nullptr)
                {
                    originalSlot1[materialIndex] = entry->model.materials[materialIndex].maps[1].texture;
                    if (override->slot1Texture.id != 0)
                    {
                        entry->model.materials[materialIndex].maps[1].texture = override->slot1Texture;
                    }
                }
                entry->model.materials[materialIndex].shader = override->shader;
            }
        }

        for (std::size_t index = 0; index < batch.count; ++index)
        {
            const world::TerrainVegetationInstance& instance =
                gpu->vegetationInstances[batch.begin + index];
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
            rlPushMatrix();
            rlMultMatrixf(MatrixToFloat(transform));
            if (entry != nullptr && entry->hasModel)
            {
                DrawModel(entry->model, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
            }
            else
            {
                DrawCube(Vector3{0.0f, 0.0f, 0.0f}, 1.0f, 1.0f, 1.0f, Color{120, 72, 88, 255});
            }
            rlPopMatrix();
        }

        if (useOverride && entry != nullptr)
        {
            for (int materialIndex = 0; materialIndex < restoreCount; ++materialIndex)
            {
                entry->model.materials[materialIndex].shader = originalShaders[materialIndex];
                if (entry->model.materials[materialIndex].maps != nullptr)
                {
                    entry->model.materials[materialIndex].maps[1].texture = originalSlot1[materialIndex];
                }
            }
            RestoreModelMaterialGpuState(entry->model, snapshot);
        }
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
