#pragma once

// Milestone 85 GPU lighting/shadow resources. Owned by Renderer.
// Deterministic load/unload. No per-frame create. No shader framework.

#include "core/Vec3.h"
#include "render/LightingEnvironment.h"
#include "render/LoadedModelMaterials.h"

#include "raylib.h"

#include <cstddef>
#include <memory>

namespace render
{
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

private:
    void DrawSolidBoxTransform(const Matrix& transform, Color color) const;

    struct GpuState;
    std::unique_ptr<GpuState> gpu;
};
}
