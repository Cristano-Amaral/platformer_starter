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
    const ModelMaterialGpuSnapshot snapshot = CaptureModelMaterialGpuState(model);
    DrawModel(model, position, scale, tint);
    RestoreModelMaterialGpuState(model, snapshot);
}
}
