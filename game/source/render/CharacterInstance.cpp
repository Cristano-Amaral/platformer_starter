#include "render/CharacterInstance.h"
#include "gameplay/RuntimeHealth.h"

#include "render/LoadedModelMaterials.h"

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#include <atomic>
#include <cmath>
#include <vector>

namespace render
{
namespace
{
std::atomic<std::uint64_t> nextHandle{1};

editor::CharacterPreviewSlot Slot(CharacterLocomotionState state)
{
    switch (state)
    {
    case CharacterLocomotionState::Move: return editor::CharacterPreviewSlot::Move;
    case CharacterLocomotionState::Jump: return editor::CharacterPreviewSlot::Jump;
    default: return editor::CharacterPreviewSlot::Idle;
    }
}

void ConfigureSkinningShader(Shader shader)
{
    const int one = 1;
    const int zero = 0;
    const float oneFloat = 1.0f;
    const float zeroFloat = 0.0f;
    const float white[3] = {1.0f, 1.0f, 1.0f};
    const auto setInt = [&](const char* name, int value) {
        const int location = GetShaderLocation(shader, name);
        if (location >= 0) SetShaderValue(shader, location, &value, SHADER_UNIFORM_INT);
    };
    const auto setFloat = [&](const char* name, float value) {
        const int location = GetShaderLocation(shader, name);
        if (location >= 0) SetShaderValue(shader, location, &value, SHADER_UNIFORM_FLOAT);
    };
    setInt("skinningEnabled", one);
    setInt("vegetationInstanced", zero);
    setInt("terrainLayerCount", zero);
    setInt("localLightCount", zero);
    setInt("groundCoverCutout", zero);
    setFloat("ambientIntensity", oneFloat);
    setFloat("lightEnabled", zeroFloat);
    setFloat("shadowsEnabled", zeroFloat);
    const int ambient = GetShaderLocation(shader, "ambientColor");
    if (ambient >= 0) SetShaderValue(shader, ambient, white, SHADER_UNIFORM_VEC3);
}
}

struct CharacterInstance::GpuState
{
    Model model{};
    bool hasModel = false;
    bool isStatic = false;
    Model sourceModel{};
    bool hasSourceModel = false;
    ModelAnimation* animations = nullptr;
    int animationCount = 0;
    int selectedAnimation = -1;
    std::vector<unsigned char> retargetScratch;
    Shader skinningShader{};
    bool hasSkinningShader = false;
};

CharacterInstance::CharacterInstance(std::string_view identity,
    const gameplay::GameplayDefinitionRegistry& definitions,
    const std::filesystem::path& root)
    : registry(definitions), assetRoot(root), gpu(std::make_unique<GpuState>()),
      handle(nextHandle.fetch_add(1, std::memory_order_relaxed)), definitionIdentity(identity)
{
    const auto* definition = registry.Find(definitionIdentity);
    if (definition == nullptr || definition->category != gameplay::GameplayDefinitionCategory::Character)
    { diagnostic = "CharacterDefinition is unavailable"; return; }
    const auto& character = definition->character;
    modelIdentity = character.worldModelIdentity;
    const auto validation = animation::ValidateCharacterAssets(registry, character, assetRoot);
    if (validation.model.status != animation::CharacterModelValidationStatus::Resolved)
    { diagnostic = validation.model.detail; return; }
    const auto modelPath = assetRoot / character.worldModelIdentity;
    gpu->model = LoadModel(modelPath.string().c_str());
    gpu->hasModel = gpu->model.meshCount > 0 && gpu->model.meshes != nullptr;
    if (!gpu->hasModel)
    { UnloadModel(gpu->model); gpu->model = {}; diagnostic = "World Model failed loading"; return; }
    PrepareLoadedModelMaterials(gpu->model);
    gpu->isStatic = !validation.model.skinned;
    if (!gpu->isStatic)
    {
        const auto vertex = assetRoot / "shaders/world_lit.vs";
        const auto fragment = assetRoot / "shaders/world_lit.fs";
        gpu->skinningShader = LoadShader(vertex.string().c_str(), fragment.string().c_str());
        gpu->hasSkinningShader = gpu->skinningShader.id != 0;
        if (gpu->hasSkinningShader) ConfigureSkinningShader(gpu->skinningShader);
    }
    ResolveSlot();
}

CharacterInstance::~CharacterInstance()
{
    ReleaseAnimation();
    if (gpu->hasModel) UnloadModel(gpu->model);
    if (gpu->hasSkinningShader) UnloadShader(gpu->skinningShader);
}

void CharacterInstance::ReleaseAnimation()
{
    if (gpu->animations != nullptr) UnloadModelAnimations(gpu->animations, gpu->animationCount);
    gpu->animations = nullptr;
    gpu->animationCount = 0;
    gpu->selectedAnimation = -1;
    if (gpu->hasSourceModel) UnloadModel(gpu->sourceModel);
    gpu->sourceModel = {};
    gpu->hasSourceModel = false;
    gpu->retargetScratch.clear();
    if (gpu->hasModel && gpu->model.boneMatrices != nullptr)
        for (int index = 0; index < gpu->model.skeleton.boneCount; ++index)
        {
            gpu->model.boneMatrices[index] = MatrixIdentity();
            if (gpu->model.currentPose != nullptr && gpu->model.skeleton.bindPose != nullptr)
                gpu->model.currentPose[index] = gpu->model.skeleton.bindPose[index];
        }
}

void CharacterInstance::ResolveSlot()
{
    ReleaseAnimation();
    mode = CharacterInstanceMode::Unavailable;
    const auto* definition = registry.Find(definitionIdentity);
    if (definition == nullptr || definition->category != gameplay::GameplayDefinitionCategory::Character)
    { diagnostic = "CharacterDefinition is unavailable"; return; }
    if (definition->character.worldModelIdentity != modelIdentity)
    { diagnostic = "World Model assignment changed; recreate this transient instance"; return; }
    const auto validation = animation::ValidateCharacterAssets(registry, definition->character, assetRoot);
    resolution = editor::ResolveCharacterPreviewAnimation(
        registry, definition->character, validation, Slot(locomotion));
    diagnostic = resolution.detail;
    if (!gpu->hasModel || gpu->isStatic || !resolution.CanSample()) return;
    const auto sourcePath = assetRoot / resolution.sourceAssetIdentity;
    gpu->sourceModel = LoadModel(sourcePath.string().c_str());
    gpu->hasSourceModel = true;
    if (gpu->sourceModel.skeleton.bindPose == nullptr)
    { diagnostic = "Animation source model failed loading"; ReleaseAnimation(); return; }
    gpu->animations = LoadModelAnimations(sourcePath.string().c_str(), &gpu->animationCount);
    for (int index = 0; index < gpu->animationCount; ++index)
        if (resolution.sourceClipName == gpu->animations[index].name)
        { gpu->selectedAnimation = index; break; }
    if (gpu->selectedAnimation < 0)
    { diagnostic = "Requested clip failed loading"; ReleaseAnimation(); return; }
    if (resolution.status != editor::CharacterPreviewAnimationStatus::Retargeted)
    {
        // Check the actually owned target, rather than assuming the validator's
        // separately loaded model is still identical after a staging change.
        if (!animation::RaylibSkeletonsExactlyCompatible(gpu->model, gpu->sourceModel,
            gpu->animations[gpu->selectedAnimation]))
        { diagnostic = "Loaded skeleton is no longer Exact compatible; recreate instance"; ReleaseAnimation(); return; }
        UnloadModel(gpu->sourceModel);
        gpu->sourceModel = {};
        gpu->hasSourceModel = false;
    }
    mode = resolution.status == editor::CharacterPreviewAnimationStatus::Retargeted
        ? CharacterInstanceMode::Retargeted : CharacterInstanceMode::Exact;
    Restart();
}

void CharacterInstance::SetLocomotion(CharacterLocomotionState value)
{
    if (locomotion == value) return;
    locomotion = value;
    timeSeconds = 0.0f;
    ResolveSlot();
}

void CharacterInstance::Restart()
{
    timeSeconds = 0.0f;
    Advance(0.0f);
}

void CharacterInstance::Advance(float deltaSeconds)
{
    if (gpu->selectedAnimation < 0) return;
    const ModelAnimation& clip = gpu->animations[gpu->selectedAnimation];
    if (playing && deltaSeconds > 0.0f && std::isfinite(deltaSeconds))
        timeSeconds = animation::ResolvePlaybackTime(timeSeconds + deltaSeconds,
            static_cast<float>(clip.keyframeCount > 0 ? clip.keyframeCount - 1 : 0) / 60.0f,
            resolution.playbackMode);
    const bool success = mode == CharacterInstanceMode::Retargeted
        ? animation::ApplyRaylibRetargetedPose(gpu->model, gpu->sourceModel, clip,
            resolution.retarget, timeSeconds, resolution.playbackMode, gpu->retargetScratch)
        : animation::ApplyRaylibAnimationPose(gpu->model, clip, timeSeconds, resolution.playbackMode);
    if (!success)
    {
        ReleaseAnimation();
        mode = CharacterInstanceMode::Unavailable;
        diagnostic = "Pose evaluation failed; using rest presentation";
    }
}

bool CharacterInstance::HasModel() const { return gpu->hasModel; }
bool CharacterInstance::IsStatic() const { return gpu->hasModel && gpu->isStatic; }
int CharacterInstance::BoneCount() const
{ return gpu->hasModel ? gpu->model.skeleton.boneCount : 0; }
core::Vec3 CharacterInstance::JointTranslation(int index) const
{
    if (!gpu->hasModel || gpu->model.currentPose == nullptr || index < 0
        || index >= gpu->model.skeleton.boneCount) return {};
    const Vector3 value = gpu->model.currentPose[index].translation;
    return {value.x, value.y, value.z};
}
double CharacterInstance::BoneMatrixChecksum() const
{
    if (!gpu->hasModel || gpu->model.boneMatrices == nullptr) return 0.0;
    double checksum = 0.0;
    for (int bone = 0; bone < gpu->model.skeleton.boneCount; ++bone)
    {
        const float16 values = MatrixToFloatV(gpu->model.boneMatrices[bone]);
        for (int component = 0; component < 16; ++component)
            checksum += values.v[component] * static_cast<double>(bone * 16 + component + 1);
    }
    return checksum;
}
const void* CharacterInstance::PoseAddress() const
{ return gpu->hasModel ? gpu->model.currentPose : nullptr; }
const void* CharacterInstance::BoneMatricesAddress() const
{ return gpu->hasModel ? gpu->model.boneMatrices : nullptr; }

bool CharacterInstance::DamageFeedbackActive() const
{
    return runtimeHealth != nullptr && runtimeHealth->DamageFeedbackActive();
}

void CharacterInstance::Draw(const ModelDrawOverride* override) const
{
    if (!gpu->hasModel) return;
    rlDrawRenderBatchActive();
    rlPushMatrix();
    rlTranslatef(transform.position.x, transform.position.y, transform.position.z);
    rlRotatef(transform.rotationDegrees.z, 0.0f, 0.0f, 1.0f);
    rlRotatef(transform.rotationDegrees.y, 0.0f, 1.0f, 0.0f);
    rlRotatef(transform.rotationDegrees.x, 1.0f, 0.0f, 0.0f);
    rlScalef(transform.scale.x, transform.scale.y, transform.scale.z);
    ModelDrawOverride shaderOverride = override != nullptr ? *override : ModelDrawOverride{};
    if (override == nullptr && !gpu->isStatic && gpu->hasSkinningShader)
        shaderOverride.shader = gpu->skinningShader;
    int skinningLocation = -1;
    if (override != nullptr && shaderOverride.shader.id != 0)
    {
        skinningLocation = GetShaderLocation(shaderOverride.shader, "skinningEnabled");
        if (skinningLocation >= 0)
        {
            const int enabled = gpu->isStatic ? 0 : 1;
            SetShaderValue(shaderOverride.shader, skinningLocation, &enabled, SHADER_UNIFORM_INT);
        }
    }
    DrawModelPreservingMaterials(gpu->model, {0.0f, 0.0f, 0.0f}, 1.0f, DamageFeedbackActive() ? Color{255, 48, 48, 255} : WHITE,
        shaderOverride.shader.id != 0 ? &shaderOverride : nullptr);
    rlPopMatrix();
    if (skinningLocation >= 0)
    {
        const int disabled = 0;
        SetShaderValue(shaderOverride.shader, skinningLocation, &disabled, SHADER_UNIFORM_INT);
    }
}
}
