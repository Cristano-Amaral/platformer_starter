#include "render/CharacterPreviewRenderer.h"

#include "assets/StaticGlb.h"
#include "render/LoadedModelMaterials.h"

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#include <cmath>

namespace render
{
namespace
{
constexpr Color kBackground{48, 52, 62, 255};

bool HasRenderableMesh(const Model& model)
{
    return model.meshCount > 0 && model.meshes != nullptr;
}

editor::ThumbnailModelBounds ToBounds(const BoundingBox& box)
{
    return editor::SanitizeStaticModelBounds({
        {box.min.x, box.min.y, box.min.z}, {box.max.x, box.max.y, box.max.z}});
}

Camera3D ToCamera(const editor::ThumbnailCameraFrame& frame)
{
    return {{frame.position.x, frame.position.y, frame.position.z},
        {frame.target.x, frame.target.y, frame.target.z},
        {frame.up.x, frame.up.y, frame.up.z}, frame.fieldOfViewY, CAMERA_PERSPECTIVE};
}
}

struct CharacterPreviewRenderer::GpuState
{
    Model model{};
    Model sourceModel{};
    bool hasSourceModel = false;
    ModelAnimation* animations = nullptr;
    int animationCount = 0;
    RenderTexture2D target{};
    bool hasTarget = false;
    Shader skinningShader{};
    bool hasSkinningShader = false;
};

CharacterPreviewRenderer::CharacterPreviewRenderer() : gpu(std::make_unique<GpuState>()) {}
CharacterPreviewRenderer::~CharacterPreviewRenderer() { Shutdown(); }

void CharacterPreviewRenderer::ReleaseAnimation()
{
    if (gpu && hasModel)
    {
        for (int bone = 0; gpu->model.boneMatrices != nullptr
            && bone < gpu->model.skeleton.boneCount; ++bone)
            gpu->model.boneMatrices[bone] = MatrixIdentity();
    }
    if (gpu && gpu->animations) UnloadModelAnimations(gpu->animations, gpu->animationCount);
    if (gpu && gpu->hasSourceModel) UnloadModel(gpu->sourceModel);
    if (gpu) { gpu->sourceModel = {}; gpu->hasSourceModel = false; }
    if (gpu) { gpu->animations = nullptr; gpu->animationCount = 0; }
    selectedAnimation = -1;
    lastSampledFrame = 0.0f;
    animationKey.clear();
    retarget = {};
    retargetScratch.clear();
}

void CharacterPreviewRenderer::ReleaseModel()
{
    ReleaseAnimation();
    if (gpu && hasModel) UnloadModel(gpu->model);
    if (gpu) gpu->model = {};
    hasModel = false;
    staticModel = false;
    loadedIdentity.clear();
    bounds = {};
}

void CharacterPreviewRenderer::ReleaseTarget()
{
    if (gpu && gpu->hasTarget) UnloadRenderTexture(gpu->target);
    if (gpu) { gpu->target = {}; gpu->hasTarget = false; }
    targetWidth = targetHeight = 0;
}

void CharacterPreviewRenderer::Shutdown()
{
    ReleaseModel();
    ReleaseTarget();
    if (gpu && gpu->hasSkinningShader)
    {
        UnloadShader(gpu->skinningShader);
        gpu->skinningShader = {};
        gpu->hasSkinningShader = false;
    }
    animationStatus.clear();
}

void CharacterPreviewRenderer::SyncModel(std::string_view identity,
    const std::filesystem::path& sourcePath,
    const animation::CharacterModelValidationResult& validation)
{
    if (loadedIdentity == identity && hasModel) return;
    ReleaseModel();
    animationStatus.clear();
    if (identity.empty() || validation.status != animation::CharacterModelValidationStatus::Resolved)
    { animationStatus = validation.detail; return; }
    Model model = LoadModel(sourcePath.string().c_str());
    if (!HasRenderableMesh(model))
    { UnloadModel(model); animationStatus = "World Model failed preview loading"; return; }
    PrepareLoadedModelMaterials(model);
    if (validation.skinned && !gpu->hasSkinningShader)
    {
        const std::filesystem::path assetRoot = sourcePath.parent_path().parent_path();
        const std::filesystem::path vertexShader = assetRoot / "shaders/world_lit.vs";
        const std::filesystem::path fragmentShader = assetRoot / "shaders/world_lit.fs";
        gpu->skinningShader = LoadShader(
            vertexShader.string().c_str(), fragmentShader.string().c_str());
        gpu->hasSkinningShader = gpu->skinningShader.id != 0;
        if (gpu->hasSkinningShader)
        {
            const int one = 1;
            const int zero = 0;
            const float oneFloat = 1.0f;
            const float zeroFloat = 0.0f;
            const float white[3] = {1.0f, 1.0f, 1.0f};
            const auto setInt = [&](const char* name, const int* value) {
                const int location = GetShaderLocation(gpu->skinningShader, name);
                if (location >= 0) SetShaderValue(gpu->skinningShader, location, value, SHADER_UNIFORM_INT);
            };
            const auto setFloat = [&](const char* name, const float* value) {
                const int location = GetShaderLocation(gpu->skinningShader, name);
                if (location >= 0) SetShaderValue(gpu->skinningShader, location, value, SHADER_UNIFORM_FLOAT);
            };
            setInt("skinningEnabled", &one);
            setInt("vegetationInstanced", &zero);
            setInt("terrainLayerCount", &zero);
            setInt("localLightCount", &zero);
            setInt("groundCoverCutout", &zero);
            setFloat("ambientIntensity", &oneFloat);
            setFloat("lightEnabled", &zeroFloat);
            setFloat("shadowsEnabled", &zeroFloat);
            const int ambientLocation = GetShaderLocation(gpu->skinningShader, "ambientColor");
            if (ambientLocation >= 0)
                SetShaderValue(gpu->skinningShader, ambientLocation, white, SHADER_UNIFORM_VEC3);
        }
    }
    gpu->model = model;
    hasModel = true;
    staticModel = !validation.skinned;
    loadedIdentity.assign(identity);
    bounds = ToBounds(GetModelBoundingBox(model));
}

void CharacterPreviewRenderer::SyncAnimation(
    const editor::CharacterPreviewAnimationResolution& resolution,
    const std::filesystem::path& assetRoot)
{
    std::string key = resolution.CanSample()
        ? resolution.sourceAssetIdentity + "\n" + resolution.sourceClipName
            + "\n" + std::to_string(static_cast<int>(resolution.status)) : std::string{};
    if (resolution.status == editor::CharacterPreviewAnimationStatus::Retargeted)
        for (std::size_t role = 0; role < gameplay::kHumanoidJointRoleCount; ++role)
            key += "\n" + std::to_string(resolution.retarget.sourceJoints[role])
                + ":" + std::to_string(resolution.retarget.targetJoints[role]);
    animationStatus = resolution.detail;
    animationPlaybackMode = resolution.playbackMode;
    if (!hasModel || staticModel || !resolution.CanSample())
    { ReleaseAnimation(); return; }
    if (key == animationKey && selectedAnimation >= 0) return;
    ReleaseAnimation();
    animationKey = key;
    retarget = resolution.retarget;
    const std::filesystem::path sourcePath = assetRoot / resolution.sourceAssetIdentity;
    if (resolution.status == editor::CharacterPreviewAnimationStatus::Retargeted)
    {
        gpu->sourceModel = LoadModel(sourcePath.string().c_str());
        gpu->hasSourceModel = gpu->sourceModel.skeleton.bindPose != nullptr;
        if (!gpu->hasSourceModel)
        { animationStatus = "Retarget source model failed loading"; ReleaseAnimation(); return; }
    }
    gpu->animations = LoadModelAnimations(sourcePath.string().c_str(), &gpu->animationCount);
    for (int index = 0; index < gpu->animationCount; ++index)
        if (resolution.sourceClipName == gpu->animations[index].name)
        { selectedAnimation = index; break; }
    if (selectedAnimation < 0)
    {
        animationStatus = "Requested preview clip could not be loaded";
        ReleaseAnimation();
    }
}

bool CharacterPreviewRenderer::Render(int width, int height,
    const editor::StaticModelPreviewOrbit& orbit,
    const editor::CharacterPreviewPlayback& playback)
{
    if (!hasModel || !gpu) return false;
    const auto size = editor::ResolvePreviewRenderSize(static_cast<float>(width), static_cast<float>(height));
    if (!size.valid) return false;
    if (!gpu->hasTarget || editor::PreviewRenderTargetNeedsResize(
        targetWidth, targetHeight, size.width, size.height))
    {
        ReleaseTarget();
        gpu->target = LoadRenderTexture(size.width, size.height);
        if (gpu->target.id == 0) return false;
        gpu->hasTarget = true;
        targetWidth = size.width; targetHeight = size.height;
    }
    if (selectedAnimation >= 0)
        SampleAnimation(playback.timeSeconds, animationPlaybackMode);
    const auto frame = editor::MakeStaticModelCameraFrameFromOrbit(orbit);
    BeginTextureMode(gpu->target);
    ClearBackground(kBackground);
    rlSetClipPlanes(static_cast<double>(frame.nearPlane), static_cast<double>(frame.farPlane));
    BeginMode3D(ToCamera(frame));
    ModelDrawOverride skinningOverride{};
    if (!staticModel && gpu->hasSkinningShader) skinningOverride.shader = gpu->skinningShader;
    DrawModelPreservingMaterials(gpu->model, {0.0f, 0.0f, 0.0f}, 1.0f, WHITE,
        skinningOverride.shader.id != 0 ? &skinningOverride : nullptr);
    EndMode3D();
    EndTextureMode();
    rlSetClipPlanes(RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
    return true;
}

unsigned int CharacterPreviewRenderer::TextureGpuId() const
{ return gpu && gpu->hasTarget ? gpu->target.texture.id : 0; }
bool CharacterPreviewRenderer::HasModel() const { return hasModel; }
bool CharacterPreviewRenderer::IsStaticModel() const { return hasModel && staticModel; }
bool CharacterPreviewRenderer::HasAnimation() const { return selectedAnimation >= 0; }
const std::string& CharacterPreviewRenderer::LoadedIdentity() const { return loadedIdentity; }
const std::string& CharacterPreviewRenderer::AnimationStatus() const { return animationStatus; }
float CharacterPreviewRenderer::AnimationDurationSeconds() const
{
    if (selectedAnimation < 0) return 0.0f;
    return static_cast<float>(gpu->animations[selectedAnimation].keyframeCount > 0
        ? gpu->animations[selectedAnimation].keyframeCount - 1 : 0) / 60.0f;
}
bool CharacterPreviewRenderer::SampleAnimation(float timeSeconds, animation::PlaybackMode mode)
{
    if (!gpu || !hasModel || selectedAnimation < 0) return false;
    const ModelAnimation& clip = gpu->animations[selectedAnimation];
    if (retarget.status == animation::RetargetValidationStatus::Retargetable && gpu->hasSourceModel)
        return animation::ApplyRaylibRetargetedPose(gpu->model, gpu->sourceModel, clip,
            retarget, timeSeconds, mode, retargetScratch, &lastSampledFrame);
    return animation::ApplyRaylibAnimationPose(
        gpu->model, clip, timeSeconds, mode, &lastSampledFrame);
}
float CharacterPreviewRenderer::LastSampledFrame() const { return lastSampledFrame; }
core::Vec3 CharacterPreviewRenderer::CurrentJointTranslation(int jointIndex) const
{
    if (!gpu || !hasModel || gpu->model.currentPose == nullptr || jointIndex < 0
        || jointIndex >= gpu->model.skeleton.boneCount) return {};
    const Vector3 value = gpu->model.currentPose[jointIndex].translation;
    return {value.x, value.y, value.z};
}
std::uint64_t CharacterPreviewRenderer::RenderedPixelChecksum() const
{
    if (!gpu || !gpu->hasTarget) return 0;
    Image image = LoadImageFromTexture(gpu->target.texture);
    Color* pixels = LoadImageColors(image);
    std::uint64_t hash = 14695981039346656037ull;
    const int count = image.width * image.height;
    for (int index = 0; pixels != nullptr && index < count; ++index)
    {
        for (unsigned char channel : {pixels[index].r, pixels[index].g,
                pixels[index].b, pixels[index].a})
        { hash ^= channel; hash *= 1099511628211ull; }
    }
    if (pixels != nullptr) UnloadImageColors(pixels);
    UnloadImage(image);
    return hash;
}
double CharacterPreviewRenderer::RenderBoneMatrixChecksum() const
{
    if (!gpu || !hasModel || gpu->model.boneMatrices == nullptr) return 0.0;
    double checksum = 0.0;
    for (int bone = 0; bone < gpu->model.skeleton.boneCount; ++bone)
    {
        const float* values = reinterpret_cast<const float*>(&gpu->model.boneMatrices[bone]);
        for (int index = 0; index < 16; ++index)
            checksum += static_cast<double>(values[index]) * static_cast<double>(bone * 16 + index + 1);
    }
    return checksum;
}
editor::ThumbnailModelBounds CharacterPreviewRenderer::Bounds() const { return bounds; }
}
