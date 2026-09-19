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

void PrepareLoadedModelMaterials(Model& model);
ModelMaterialGpuSnapshot CaptureModelMaterialGpuState(const Model& model);
void RestoreModelMaterialGpuState(Model& model, const ModelMaterialGpuSnapshot& snapshot);
bool ModelMaterialGpuStateEqual(
    const ModelMaterialGpuSnapshot& left,
    const ModelMaterialGpuSnapshot& right);
void DrawModelPreservingMaterials(Model model, Vector3 position, float scale, Color tint);
}
