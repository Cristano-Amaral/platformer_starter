#include "render/WorldLighting.h"

#include "platform/RuntimePaths.h"
#include "render/LocalLights.h"

#include "raymath.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <system_error>

namespace render
{
namespace
{
constexpr const char* kWorldLitVs = "shaders/world_lit.vs";
constexpr const char* kWorldLitFs = "shaders/world_lit.fs";
constexpr const char* kShadowDepthVs = "shaders/shadow_depth.vs";
constexpr const char* kShadowDepthFs = "shaders/shadow_depth.fs";

bool RegularFileExists(const std::filesystem::path& path)
{
    std::error_code error;
    return !path.empty() && path.is_absolute() && std::filesystem::is_regular_file(path, error)
        && !error;
}

bool ShaderFileUsable(const std::filesystem::path& path)
{
    if (!RegularFileExists(path))
    {
        return false;
    }
    std::error_code error;
    return std::filesystem::file_size(path, error) > 0 && !error;
}

Vector3 ToRaylib(core::Vec3 value)
{
    return Vector3{value.x, value.y, value.z};
}

Matrix MakeBoxTransform(core::Vec3 center, core::Vec3 size, Matrix rotation)
{
    const Matrix scale = MatrixScale(size.x, size.y, size.z);
    const Matrix translation = MatrixTranslate(center.x, center.y, center.z);
    return MatrixMultiply(MatrixMultiply(scale, rotation), translation);
}

RenderTexture2D LoadShadowMap(int resolution)
{
    RenderTexture2D target{};
    target.id = rlLoadFramebuffer();
    if (target.id == 0)
    {
        return target;
    }

    rlEnableFramebuffer(target.id);
    target.texture.id = rlLoadTexture(
        nullptr, resolution, resolution, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
    target.texture.width = resolution;
    target.texture.height = resolution;
    target.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    target.texture.mipmaps = 1;
    target.depth.id = rlLoadTextureDepth(resolution, resolution, false);
    target.depth.width = resolution;
    target.depth.height = resolution;
    target.depth.mipmaps = 1;
    target.depth.format = 19;
    if (target.texture.id == 0 || target.depth.id == 0)
    {
        rlDisableFramebuffer();
        rlUnloadFramebuffer(target.id);
        target = {};
        return target;
    }
    rlTextureParameters(target.depth.id, RL_TEXTURE_WRAP_S, RL_TEXTURE_WRAP_CLAMP);
    rlTextureParameters(target.depth.id, RL_TEXTURE_WRAP_T, RL_TEXTURE_WRAP_CLAMP);
    rlFramebufferAttach(
        target.id,
        target.texture.id,
        RL_ATTACHMENT_COLOR_CHANNEL0,
        RL_ATTACHMENT_TEXTURE2D,
        0);
    rlFramebufferAttach(
        target.id,
        target.depth.id,
        RL_ATTACHMENT_DEPTH,
        RL_ATTACHMENT_TEXTURE2D,
        0);
    if (!rlFramebufferComplete(target.id))
    {
        rlDisableFramebuffer();
        rlUnloadFramebuffer(target.id);
        target = {};
        return target;
    }
    rlDisableFramebuffer();
    target.texture.width = resolution;
    target.texture.height = resolution;
    return target;
}

void UnloadShadowMap(RenderTexture2D& target)
{
    if (target.id != 0)
    {
        rlUnloadFramebuffer(target.id);
    }
    target = {};
}

void LogLightingFailureOnce(bool& logged, const char* message)
{
    if (logged)
    {
        return;
    }
    logged = true;
    std::fprintf(stderr, "WorldLighting: %s\n", message);
}
}

struct WorldLightingResources::GpuState
{
    Shader lit{};
    Shader depth{};
    RenderTexture2D shadowMap{};
    Mesh unitCube{};
    Material solidMaterial{};
    int locAmbientColor = -1;
    int locAmbientIntensity = -1;
    int locLightRayDirection = -1;
    int locLightColor = -1;
    int locLightIntensity = -1;
    int locLightEnabled = -1;
    int locShadowsEnabled = -1;
    int locLightVP = -1;
    int locShadowMap = -1;
    int locShadowBias = -1;
    int locShadowMapResolution = -1;
    int locLocalLightCount = -1;
    int locLocalLightPosRange = -1;
    int locLocalLightColorIntensity = -1;
    int locLocalLightDirType = -1;
    int locLocalLightConeCos = -1;
    int shadowResolution = 0;
    std::size_t shaderLoadCount = 0;
    std::size_t shadowMapCreateCount = 0;
    bool hasLit = false;
    bool hasDepth = false;
    bool hasShadowMap = false;
    bool hasCube = false;
    bool hasMaterial = false;
    bool failureLogged = false;
    bool shadowPassActive = false;
    bool litPassActive = false;
};

WorldLightingResources::WorldLightingResources()
    : gpu(std::make_unique<GpuState>())
{
}

WorldLightingResources::~WorldLightingResources()
{
    Unload();
}

void WorldLightingResources::Unload()
{
    if (gpu == nullptr)
    {
        return;
    }
    if (gpu->shadowPassActive)
    {
        EndShadowPass();
    }
    if (gpu->litPassActive)
    {
        UnbindLitPass();
    }
    if (gpu->hasMaterial)
    {
        if (gpu->solidMaterial.maps != nullptr)
        {
            gpu->solidMaterial.maps[MATERIAL_MAP_DIFFUSE].texture.id = 0;
            gpu->solidMaterial.maps[1].texture.id = 0;
        }
        gpu->solidMaterial.shader = {};
        UnloadMaterial(gpu->solidMaterial);
        gpu->solidMaterial = {};
        gpu->hasMaterial = false;
    }
    if (gpu->hasCube)
    {
        UnloadMesh(gpu->unitCube);
        gpu->unitCube = {};
        gpu->hasCube = false;
    }
    if (gpu->hasShadowMap)
    {
        UnloadShadowMap(gpu->shadowMap);
        gpu->hasShadowMap = false;
    }
    if (gpu->hasLit)
    {
        UnloadShader(gpu->lit);
        gpu->lit = {};
        gpu->hasLit = false;
    }
    if (gpu->hasDepth)
    {
        UnloadShader(gpu->depth);
        gpu->depth = {};
        gpu->hasDepth = false;
    }
    gpu->locAmbientColor = -1;
    gpu->locAmbientIntensity = -1;
    gpu->locLightRayDirection = -1;
    gpu->locLightColor = -1;
    gpu->locLightIntensity = -1;
    gpu->locLightEnabled = -1;
    gpu->locShadowsEnabled = -1;
    gpu->locLightVP = -1;
    gpu->locShadowMap = -1;
    gpu->locShadowBias = -1;
    gpu->locShadowMapResolution = -1;
    gpu->shadowResolution = 0;
}

bool WorldLightingResources::IsReady() const
{
    return gpu != nullptr && gpu->hasLit && gpu->hasDepth && gpu->hasShadowMap && gpu->hasCube
        && gpu->hasMaterial;
}

std::size_t WorldLightingResources::ShaderLoadCount() const
{
    return gpu == nullptr ? 0 : gpu->shaderLoadCount;
}

std::size_t WorldLightingResources::ShadowMapCreateCount() const
{
    return gpu == nullptr ? 0 : gpu->shadowMapCreateCount;
}

unsigned int WorldLightingResources::LitShaderId() const
{
    return IsReady() ? gpu->lit.id : 0;
}

unsigned int WorldLightingResources::DepthShaderId() const
{
    return IsReady() ? gpu->depth.id : 0;
}

unsigned int WorldLightingResources::ShadowMapId() const
{
    return IsReady() ? gpu->shadowMap.depth.id : 0;
}

int WorldLightingResources::ShadowMapResolution() const
{
    return gpu == nullptr ? 0 : gpu->shadowResolution;
}

bool WorldLightingResources::FailureLogged() const
{
    return gpu != nullptr && gpu->failureLogged;
}

ModelDrawOverride WorldLightingResources::ShadowModelOverride() const
{
    ModelDrawOverride override{};
    if (IsReady())
    {
        override.shader = gpu->depth;
    }
    return override;
}

ModelDrawOverride WorldLightingResources::LitModelOverride() const
{
    ModelDrawOverride override{};
    if (IsReady())
    {
        override.shader = gpu->lit;
        override.slot1Texture = gpu->shadowMap.depth;
    }
    return override;
}

void WorldLightingResources::Load()
{
    if (gpu == nullptr)
    {
        gpu = std::make_unique<GpuState>();
    }
    if (IsReady())
    {
        return;
    }

    Unload();

    const LightingEnvironment environment = MakeDefaultLightingEnvironment();
    const std::filesystem::path litVs = platform::RuntimeAssetPath(kWorldLitVs);
    const std::filesystem::path litFs = platform::RuntimeAssetPath(kWorldLitFs);
    const std::filesystem::path depthVs = platform::RuntimeAssetPath(kShadowDepthVs);
    const std::filesystem::path depthFs = platform::RuntimeAssetPath(kShadowDepthFs);
    if (!ShaderFileUsable(litVs) || !ShaderFileUsable(litFs) || !ShaderFileUsable(depthVs)
        || !ShaderFileUsable(depthFs))
    {
        LogLightingFailureOnce(
            gpu->failureLogged,
            "missing staged lighting shaders; world lighting disabled");
        return;
    }

    gpu->lit = LoadShader(litVs.string().c_str(), litFs.string().c_str());
    gpu->depth = LoadShader(depthVs.string().c_str(), depthFs.string().c_str());
    gpu->shaderLoadCount += 1;
    if (gpu->lit.id == 0 || gpu->depth.id == 0)
    {
        LogLightingFailureOnce(gpu->failureLogged, "shader compile failed; world lighting disabled");
        Unload();
        return;
    }
    gpu->hasLit = true;
    gpu->hasDepth = true;

    gpu->locAmbientColor = GetShaderLocation(gpu->lit, "ambientColor");
    gpu->locAmbientIntensity = GetShaderLocation(gpu->lit, "ambientIntensity");
    gpu->locLightRayDirection = GetShaderLocation(gpu->lit, "lightRayDirection");
    gpu->locLightColor = GetShaderLocation(gpu->lit, "lightColor");
    gpu->locLightIntensity = GetShaderLocation(gpu->lit, "lightIntensity");
    gpu->locLightEnabled = GetShaderLocation(gpu->lit, "lightEnabled");
    gpu->locShadowsEnabled = GetShaderLocation(gpu->lit, "shadowsEnabled");
    gpu->locLightVP = GetShaderLocation(gpu->lit, "lightVP");
    gpu->locShadowMap = GetShaderLocation(gpu->lit, "shadowMap");
    gpu->locShadowBias = GetShaderLocation(gpu->lit, "shadowBias");
    gpu->locShadowMapResolution = GetShaderLocation(gpu->lit, "shadowMapResolution");
    gpu->locLocalLightCount = GetShaderLocation(gpu->lit, "localLightCount");
    gpu->locLocalLightPosRange = GetShaderLocation(gpu->lit, "localLightPosRange");
    gpu->locLocalLightColorIntensity = GetShaderLocation(gpu->lit, "localLightColorIntensity");
    gpu->locLocalLightDirType = GetShaderLocation(gpu->lit, "localLightDirType");
    gpu->locLocalLightConeCos = GetShaderLocation(gpu->lit, "localLightConeCos");

    gpu->shadowResolution = environment.shadows.mapResolution;
    gpu->shadowMap = LoadShadowMap(gpu->shadowResolution);
    gpu->shadowMapCreateCount += 1;
    if (gpu->shadowMap.id == 0 || gpu->shadowMap.depth.id == 0)
    {
        LogLightingFailureOnce(gpu->failureLogged, "shadow map create failed; world lighting disabled");
        Unload();
        return;
    }
    gpu->hasShadowMap = true;

    gpu->unitCube = GenMeshCube(1.0f, 1.0f, 1.0f);
    gpu->shaderLoadCount += 0;
    if (gpu->unitCube.vertexCount <= 0)
    {
        LogLightingFailureOnce(gpu->failureLogged, "unit cube mesh failed; world lighting disabled");
        Unload();
        return;
    }
    gpu->hasCube = true;

    gpu->solidMaterial = LoadMaterialDefault();
    if (gpu->solidMaterial.maps == nullptr)
    {
        LogLightingFailureOnce(gpu->failureLogged, "solid material failed; world lighting disabled");
        Unload();
        return;
    }
    gpu->hasMaterial = true;
}

void WorldLightingResources::BeginShadowPass(const LightingEnvironment& environment)
{
    if (!IsReady() || gpu->shadowPassActive || !DirectionalShadowsAreActive(environment))
    {
        return;
    }

    const LightingEnvironment validated = ValidateLightingEnvironment(environment);
    const DirectionalLightView lightView =
        BuildDirectionalLightView(validated.directional, validated.shadows);
    gpu->solidMaterial.shader = gpu->depth;
    gpu->solidMaterial.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    gpu->solidMaterial.maps[1].texture = {};

    rlSetClipPlanes(
        static_cast<double>(validated.shadows.nearPlane),
        static_cast<double>(validated.shadows.farPlane));

    BeginTextureMode(gpu->shadowMap);
    rlClearScreenBuffers();
    Camera3D lightCamera{};
    lightCamera.position = ToRaylib(lightView.eye);
    lightCamera.target = ToRaylib(lightView.target);
    lightCamera.up = ToRaylib(lightView.up);
    lightCamera.fovy = validated.shadows.coverage;
    lightCamera.projection = CAMERA_ORTHOGRAPHIC;
    BeginMode3D(lightCamera);
    gpu->shadowPassActive = true;
}

void WorldLightingResources::EndShadowPass()
{
    if (gpu == nullptr || !gpu->shadowPassActive)
    {
        return;
    }
    EndMode3D();
    EndTextureMode();
    rlSetClipPlanes(RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
    gpu->shadowPassActive = false;
}

void WorldLightingResources::BindLitPass(const LightingEnvironment& environment)
{
    if (!IsReady() || gpu->litPassActive)
    {
        return;
    }

    const LightingEnvironment validated = ValidateLightingEnvironment(environment);
    const DirectionalLightView lightView =
        BuildDirectionalLightView(validated.directional, validated.shadows);
    const ShadowProjection projection = BuildShadowProjection(validated.shadows);
    const Matrix view =
        MatrixLookAt(ToRaylib(lightView.eye), ToRaylib(lightView.target), ToRaylib(lightView.up));
    const Matrix proj = MatrixOrtho(
        projection.left,
        projection.right,
        projection.bottom,
        projection.top,
        projection.nearPlane,
        projection.farPlane);
    const Matrix lightVP = MatrixMultiply(view, proj);

    gpu->solidMaterial.shader = gpu->lit;
    gpu->solidMaterial.maps[1].texture = gpu->shadowMap.depth;

    const float ambientColor[3] = {
        validated.ambient.color.x,
        validated.ambient.color.y,
        validated.ambient.color.z};
    const float lightColor[3] = {
        validated.directional.color.x,
        validated.directional.color.y,
        validated.directional.color.z};
    const float ray[3] = {
        validated.directional.rayDirection.x,
        validated.directional.rayDirection.y,
        validated.directional.rayDirection.z};
    const int shadowSlot = 1;
    if (gpu->locAmbientColor >= 0)
    {
        SetShaderValue(gpu->lit, gpu->locAmbientColor, ambientColor, SHADER_UNIFORM_VEC3);
    }
    if (gpu->locAmbientIntensity >= 0)
    {
        SetShaderValue(
            gpu->lit, gpu->locAmbientIntensity, &validated.ambient.intensity, SHADER_UNIFORM_FLOAT);
    }
    if (gpu->locLightRayDirection >= 0)
    {
        SetShaderValue(gpu->lit, gpu->locLightRayDirection, ray, SHADER_UNIFORM_VEC3);
    }
    if (gpu->locLightColor >= 0)
    {
        SetShaderValue(gpu->lit, gpu->locLightColor, lightColor, SHADER_UNIFORM_VEC3);
    }
    if (gpu->locLightIntensity >= 0)
    {
        SetShaderValue(
            gpu->lit,
            gpu->locLightIntensity,
            &validated.directional.intensity,
            SHADER_UNIFORM_FLOAT);
    }
    const float lightEnabled = EffectiveDirectionalEnabled(validated) ? 1.0f : 0.0f;
    const float shadowsEnabled = DirectionalShadowsAreActive(validated) ? 1.0f : 0.0f;
    if (gpu->locLightEnabled >= 0)
    {
        SetShaderValue(gpu->lit, gpu->locLightEnabled, &lightEnabled, SHADER_UNIFORM_FLOAT);
    }
    if (gpu->locShadowsEnabled >= 0)
    {
        SetShaderValue(gpu->lit, gpu->locShadowsEnabled, &shadowsEnabled, SHADER_UNIFORM_FLOAT);
    }
    if (gpu->locLightVP >= 0)
    {
        SetShaderValueMatrix(gpu->lit, gpu->locLightVP, lightVP);
    }
    if (gpu->locShadowBias >= 0)
    {
        SetShaderValue(gpu->lit, gpu->locShadowBias, &validated.shadows.bias, SHADER_UNIFORM_FLOAT);
    }
    if (gpu->locShadowMapResolution >= 0)
    {
        SetShaderValue(
            gpu->lit, gpu->locShadowMapResolution, &gpu->shadowResolution, SHADER_UNIFORM_INT);
    }
    if (gpu->locShadowMap >= 0)
    {
        SetShaderValue(gpu->lit, gpu->locShadowMap, &shadowSlot, SHADER_UNIFORM_INT);
    }

    float posRange[kMaxActiveLocalLights * 4]{};
    float colorIntensity[kMaxActiveLocalLights * 4]{};
    float dirType[kMaxActiveLocalLights * 4]{};
    float coneCos[kMaxActiveLocalLights * 4]{};
    int localCount = validated.localLights.count;
    if (localCount < 0)
    {
        localCount = 0;
    }
    if (localCount > kMaxActiveLocalLights)
    {
        localCount = kMaxActiveLocalLights;
    }
    for (int index = 0; index < kMaxActiveLocalLights; ++index)
    {
        const PackedLocalLight& light = validated.localLights.lights[static_cast<std::size_t>(index)];
        const int base = index * 4;
        if (index < localCount && (light.type == kLocalLightTypePoint || light.type == kLocalLightTypeSpot))
        {
            posRange[base + 0] = light.position.x;
            posRange[base + 1] = light.position.y;
            posRange[base + 2] = light.position.z;
            posRange[base + 3] = light.range;
            colorIntensity[base + 0] = light.color.x;
            colorIntensity[base + 1] = light.color.y;
            colorIntensity[base + 2] = light.color.z;
            colorIntensity[base + 3] = light.intensity;
            dirType[base + 0] = light.direction.x;
            dirType[base + 1] = light.direction.y;
            dirType[base + 2] = light.direction.z;
            dirType[base + 3] = static_cast<float>(light.type);
            coneCos[base + 0] = light.innerCos;
            coneCos[base + 1] = light.outerCos;
        }
    }
    if (gpu->locLocalLightCount >= 0)
    {
        SetShaderValue(gpu->lit, gpu->locLocalLightCount, &localCount, SHADER_UNIFORM_INT);
    }
    if (gpu->locLocalLightPosRange >= 0)
    {
        SetShaderValueV(
            gpu->lit, gpu->locLocalLightPosRange, posRange, SHADER_UNIFORM_VEC4, kMaxActiveLocalLights);
    }
    if (gpu->locLocalLightColorIntensity >= 0)
    {
        SetShaderValueV(
            gpu->lit,
            gpu->locLocalLightColorIntensity,
            colorIntensity,
            SHADER_UNIFORM_VEC4,
            kMaxActiveLocalLights);
    }
    if (gpu->locLocalLightDirType >= 0)
    {
        SetShaderValueV(
            gpu->lit, gpu->locLocalLightDirType, dirType, SHADER_UNIFORM_VEC4, kMaxActiveLocalLights);
    }
    if (gpu->locLocalLightConeCos >= 0)
    {
        SetShaderValueV(
            gpu->lit, gpu->locLocalLightConeCos, coneCos, SHADER_UNIFORM_VEC4, kMaxActiveLocalLights);
    }

    rlActiveTextureSlot(1);
    rlEnableTexture(gpu->shadowMap.depth.id);
    gpu->litPassActive = true;
}

void WorldLightingResources::UnbindLitPass()
{
    if (gpu == nullptr || !gpu->litPassActive)
    {
        return;
    }
    if (gpu->hasMaterial && gpu->solidMaterial.maps != nullptr)
    {
        gpu->solidMaterial.maps[1].texture = {};
    }
    rlActiveTextureSlot(1);
    rlDisableTexture();
    rlActiveTextureSlot(0);
    gpu->litPassActive = false;
}

void WorldLightingResources::DrawSolidBoxTransform(const Matrix& transform, Color color) const
{
    if (!IsReady())
    {
        return;
    }
    if (gpu->litPassActive)
    {
        rlActiveTextureSlot(1);
        rlEnableTexture(gpu->shadowMap.depth.id);
    }
    gpu->solidMaterial.maps[MATERIAL_MAP_DIFFUSE].color = color;
    DrawMesh(gpu->unitCube, gpu->solidMaterial, transform);
}

void WorldLightingResources::DrawSolidBox(core::Vec3 center, core::Vec3 size, Color color) const
{
    DrawSolidBoxTransform(MakeBoxTransform(center, size, MatrixIdentity()), color);
}

void WorldLightingResources::DrawSolidBoxRotatedZ(
    core::Vec3 center,
    core::Vec3 size,
    float rotationZDegrees,
    Color color) const
{
    DrawSolidBoxTransform(
        MakeBoxTransform(center, size, MatrixRotateZ(rotationZDegrees * DEG2RAD)), color);
}

void WorldLightingResources::DrawSolidBoxEulerXYZ(
    core::Vec3 center,
    core::Vec3 size,
    core::Vec3 rotationDegrees,
    Color color) const
{
    const Matrix rotX = MatrixRotateX(rotationDegrees.x * DEG2RAD);
    const Matrix rotY = MatrixRotateY(rotationDegrees.y * DEG2RAD);
    const Matrix rotZ = MatrixRotateZ(rotationDegrees.z * DEG2RAD);
    const Matrix rotation = MatrixMultiply(MatrixMultiply(rotX, rotY), rotZ);
    DrawSolidBoxTransform(MakeBoxTransform(center, size, rotation), color);
}

void WorldLightingResources::DrawSolidBoxAxisAngle(
    core::Vec3 center,
    core::Vec3 size,
    float axisX,
    float axisY,
    float axisZ,
    float degrees,
    Color color) const
{
    Matrix rotation = MatrixIdentity();
    if (degrees != 0.0f)
    {
        rotation = MatrixRotate(Vector3{axisX, axisY, axisZ}, degrees * DEG2RAD);
    }
    DrawSolidBoxTransform(MakeBoxTransform(center, size, rotation), color);
}
}
