#include "render/LoadedModelMaterials.h"

#include "assets/ModelMaterialPresentation.h"

#include "rlgl.h"

#include <cmath>

namespace render
{
namespace
{
Color PresentationToColor(const assets::ModelMaterialPresentation& presentation)
{
    const auto channel = [](float value) {
        const float clamped = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
        return static_cast<unsigned char>(std::lround(clamped * 255.0f));
    };
    return Color{
        channel(presentation.baseColorR),
        channel(presentation.baseColorG),
        channel(presentation.baseColorB),
        channel(presentation.baseColorA)};
}

assets::ModelMaterialPresentation ColorToPresentation(const Material& material)
{
    assets::ModelMaterialPresentation presentation{};
    if (material.maps == nullptr)
    {
        return assets::ResolveMaterialFallback(presentation);
    }
    const Color color = material.maps[MATERIAL_MAP_DIFFUSE].color;
    presentation.baseColorR = static_cast<float>(color.r) / 255.0f;
    presentation.baseColorG = static_cast<float>(color.g) / 255.0f;
    presentation.baseColorB = static_cast<float>(color.b) / 255.0f;
    presentation.baseColorA = static_cast<float>(color.a) / 255.0f;
    const unsigned int textureId = material.maps[MATERIAL_MAP_DIFFUSE].texture.id;
    const unsigned int defaultTextureId = rlGetTextureIdDefault();
    const bool uniqueTexture = textureId != 0 && textureId != defaultTextureId;
    presentation.hasBaseColorTexture = uniqueTexture;
    presentation.textureDependency = assets::ResolveBaseColorTextureDependency(
        uniqueTexture, uniqueTexture, false);
    return assets::ResolveMaterialFallback(presentation);
}
}

void PrepareLoadedModelMaterials(Model& model)
{
    if (model.materials == nullptr || model.materialCount <= 0)
    {
        return;
    }
    for (int i = 0; i < model.materialCount; ++i)
    {
        Material& material = model.materials[i];
        if (material.maps == nullptr)
        {
            continue;
        }
        const assets::ModelMaterialPresentation resolved = ColorToPresentation(material);
        material.maps[MATERIAL_MAP_DIFFUSE].color = PresentationToColor(resolved);
    }
}

ModelMaterialGpuSnapshot CaptureModelMaterialGpuState(const Model& model)
{
    ModelMaterialGpuSnapshot snapshot{};
    if (model.materials == nullptr || model.materialCount <= 0)
    {
        return snapshot;
    }
    snapshot.materialCount =
        model.materialCount < kMaxCapturedModelMaterials ? model.materialCount
                                                         : kMaxCapturedModelMaterials;
    for (int i = 0; i < snapshot.materialCount; ++i)
    {
        const Material& material = model.materials[i];
        if (material.maps == nullptr)
        {
            continue;
        }
        snapshot.diffuseColor[i] = material.maps[MATERIAL_MAP_DIFFUSE].color;
        snapshot.diffuseTextureId[i] = material.maps[MATERIAL_MAP_DIFFUSE].texture.id;
    }
    return snapshot;
}

void RestoreModelMaterialGpuState(Model& model, const ModelMaterialGpuSnapshot& snapshot)
{
    if (model.materials == nullptr || snapshot.materialCount <= 0)
    {
        return;
    }
    const int count = snapshot.materialCount < model.materialCount ? snapshot.materialCount
                                                                   : model.materialCount;
    for (int i = 0; i < count; ++i)
    {
        Material& material = model.materials[i];
        if (material.maps == nullptr)
        {
            continue;
        }
        material.maps[MATERIAL_MAP_DIFFUSE].color = snapshot.diffuseColor[i];
    }
}

bool ModelMaterialGpuStateEqual(
    const ModelMaterialGpuSnapshot& left,
    const ModelMaterialGpuSnapshot& right)
{
    if (left.materialCount != right.materialCount)
    {
        return false;
    }
    for (int i = 0; i < left.materialCount; ++i)
    {
        const Color a = left.diffuseColor[i];
        const Color b = right.diffuseColor[i];
        if (a.r != b.r || a.g != b.g || a.b != b.b || a.a != b.a)
        {
            return false;
        }
        if (left.diffuseTextureId[i] != right.diffuseTextureId[i])
        {
            return false;
        }
    }
    return true;
}

void DrawModelPreservingMaterials(Model model, Vector3 position, float scale, Color tint)
{
    DrawModelPreservingMaterials(model, position, scale, tint, nullptr);
}

void DrawModelPreservingMaterials(
    Model model,
    Vector3 position,
    float scale,
    Color tint,
    const ModelDrawOverride* override)
{
    const ModelMaterialGpuSnapshot snapshot = CaptureModelMaterialGpuState(model);
    Shader originalShaders[kMaxCapturedModelMaterials]{};
    Texture2D originalSlot1[kMaxCapturedModelMaterials]{};
    const bool useOverride = override != nullptr && override->shader.id != 0;
    const int restoreCount =
        model.materials == nullptr || model.materialCount <= 0
        ? 0
        : (model.materialCount < kMaxCapturedModelMaterials ? model.materialCount
                                                            : kMaxCapturedModelMaterials);
    if (useOverride)
    {
        for (int i = 0; i < restoreCount; ++i)
        {
            originalShaders[i] = model.materials[i].shader;
            if (model.materials[i].maps != nullptr)
            {
                originalSlot1[i] = model.materials[i].maps[1].texture;
                if (override->slot1Texture.id != 0)
                {
                    model.materials[i].maps[1].texture = override->slot1Texture;
                }
            }
            model.materials[i].shader = override->shader;
        }
    }
    DrawModel(model, position, scale, tint);
    if (useOverride)
    {
        for (int i = 0; i < restoreCount; ++i)
        {
            model.materials[i].shader = originalShaders[i];
            if (model.materials[i].maps != nullptr)
            {
                model.materials[i].maps[1].texture = originalSlot1[i];
            }
        }
    }
    RestoreModelMaterialGpuState(model, snapshot);
}
}
