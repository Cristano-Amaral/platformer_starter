#pragma once

// Milestone 84 GPU side of imported material presentation. raylib Model owns
// meshes, materials, and textures loaded from a supported GLB. This prepares
// Base Color / Base Color Texture after LoadModel and restores material color
// after a tinted DrawModel so highlights cannot permanently mutate imports.

#include "raylib.h"

namespace render
{
inline constexpr int kMaxCapturedModelMaterials = 64;

struct ModelMaterialGpuSnapshot
{
    int materialCount = 0;
    Color diffuseColor[kMaxCapturedModelMaterials]{};
    unsigned int diffuseTextureId[kMaxCapturedModelMaterials]{};
};

// Transient GPU bind for a single DrawModel. Restored before return.
// shader.id 0 keeps the imported material shader. slot1Texture is a non-owning
// alias (shadow map) written to maps[1] for the pass only.
struct ModelDrawOverride
{
    Shader shader{};
    Texture2D slot1Texture{};
};

void PrepareLoadedModelMaterials(Model& model);
ModelMaterialGpuSnapshot CaptureModelMaterialGpuState(const Model& model);
void RestoreModelMaterialGpuState(Model& model, const ModelMaterialGpuSnapshot& snapshot);
bool ModelMaterialGpuStateEqual(
    const ModelMaterialGpuSnapshot& left,
    const ModelMaterialGpuSnapshot& right);
void DrawModelPreservingMaterials(Model model, Vector3 position, float scale, Color tint);
void DrawModelPreservingMaterials(
    Model model,
    Vector3 position,
    float scale,
    Color tint,
    const ModelDrawOverride* override);
}
