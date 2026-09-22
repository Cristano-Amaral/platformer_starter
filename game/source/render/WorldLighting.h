#pragma once

// Milestone 85 GPU lighting/shadow resources. Owned by Renderer.
// Deterministic load/unload. No per-frame create. No shader framework.

#include "core/Vec3.h"
#include "render/LightingEnvironment.h"
#include "render/LoadedModelMaterials.h"

#include "world/Terrain.h"

#include "raylib.h"

#include <cstddef>
#include <memory>

namespace render
{
// raylib DrawMesh binds MATERIAL_MAP index i (0..11) to texture unit i.
// maps[1] is the directional shadow depth texture, so texture1 cannot be a
// Terrain sampler. Palette albedo and packed weight maps are sampler2DArray
// textures on units outside that material-map range.
inline constexpr int kWorldLitDiffuseTextureUnit = 0;
inline constexpr int kWorldLitShadowMapTextureUnit = 1;
inline constexpr int kWorldLitTerrainAlbedoArrayUnit = 12;
inline constexpr int kWorldLitTerrainWeightArrayUnit = 13;

struct TerrainLayerDrawRequest
{
    int layerCount = 0;
    unsigned int albedoArrayId = 0;
    unsigned int weightArrayId = 0;
    int weightMapCount = 1;
    int weightResolutionX = 2;
    int weightResolutionZ = 2;
    float tiling[world::kMaxTerrainMaterialLayers]{};
    float originX = 0.0f;
    float originZ = 0.0f;
    float sizeX = 1.0f;
    float sizeZ = 1.0f;
};

class WorldLightingResources
{
public:
    WorldLightingResources();
    ~WorldLightingResources();

    WorldLightingResources(const WorldLightingResources&) = delete;
    WorldLightingResources& operator=(const WorldLightingResources&) = delete;

    void Load();
    void Unload();
    bool IsReady() const;
    std::size_t ShaderLoadCount() const;
    std::size_t ShadowMapCreateCount() const;
    unsigned int LitShaderId() const;
    unsigned int DepthShaderId() const;
    unsigned int ShadowMapId() const;
    int ShadowMapResolution() const;
    bool FailureLogged() const;
    int TerrainAlbedoArraySamplerLocation() const;
    int TerrainWeightArraySamplerLocation() const;

    ModelDrawOverride ShadowModelOverride() const;
    ModelDrawOverride LitModelOverride() const;

    void BeginShadowPass(const LightingEnvironment& environment);
    void EndShadowPass();
    void BindLitPass(const LightingEnvironment& environment);
    void UnbindLitPass();

    void DrawSolidBox(core::Vec3 center, core::Vec3 size, Color color) const;
    void DrawSolidBoxRotatedZ(
        core::Vec3 center,
        core::Vec3 size,
        float rotationZDegrees,
        Color color) const;
    void DrawSolidBoxEulerXYZ(
        core::Vec3 center,
        core::Vec3 size,
        core::Vec3 rotationDegrees,
        Color color) const;
    void DrawSolidBoxAxisAngle(
        core::Vec3 center,
        core::Vec3 size,
        float axisX,
        float axisY,
        float axisZ,
        float degrees,
        Color color) const;
    void DrawWorldMesh(const Mesh& mesh, Color color) const;
    void DrawWorldMesh(const Mesh& mesh, Color color, const Texture2D* albedo) const;
    void DrawWorldTerrain(const Mesh& mesh, Color color, const TerrainLayerDrawRequest& request) const;

private:
    void DrawSolidBoxTransform(const Matrix& transform, Color color) const;

    struct GpuState;
    std::unique_ptr<GpuState> gpu;
};
}
