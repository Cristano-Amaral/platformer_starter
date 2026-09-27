#include "animation/CharacterAssetValidator.h"

#include "assets/StaticGlb.h"

#include <raylib.h>

#include <algorithm>
#include <system_error>

namespace animation
{
namespace
{
bool IsRegularFile(const std::filesystem::path& path)
{
    std::error_code error;
    return !path.empty() && std::filesystem::is_regular_file(path, error) && !error;
}

std::filesystem::path AssetPath(const std::filesystem::path& root, std::string_view identity)
{
    std::string fileName;
    if (root.empty() || !assets::TryParseStaticModelIdentity(identity, fileName)) return {};
    return root / std::filesystem::path(assets::kStaticModelsLogicalDirectory) / fileName;
}

bool HasRenderableMesh(const Model& model) { return model.meshCount > 0 && model.meshes != nullptr; }

CharacterAnimationCompatibilityResult ValidateAnimation(
    const gameplay::GameplayDefinitionRegistry& registry, std::string_view identity,
    const Model* characterModel, const std::filesystem::path& assetRoot)
{
    CharacterAnimationCompatibilityResult result;
    result.animationIdentity.assign(identity);
    if (identity.empty())
    {
        result.detail = "No reusable asset; authored embedded clip path applies";
        return result;
    }
    const gameplay::GameplayDefinition* definition = registry.Find(identity);
    if (definition == nullptr || definition->category != gameplay::GameplayDefinitionCategory::Animation)
    {
        result.status = CharacterAnimationCompatibilityStatus::MissingDefinition;
        result.detail = "Reusable Animation Asset identity does not resolve";
        return result;
    }
    if (!gameplay::ValidateAnimationDefinition(definition->animation))
    {
        result.status = CharacterAnimationCompatibilityStatus::LoadFailure;
        result.detail = "Animation definition is malformed";
        return result;
    }
    result.sourceAssetIdentity = definition->animation.sourceAssetIdentity;
    result.sourceClipName = definition->animation.sourceClipName;
    const std::filesystem::path sourcePath = AssetPath(assetRoot, result.sourceAssetIdentity);
    if (!IsRegularFile(sourcePath))
    {
        result.status = CharacterAnimationCompatibilityStatus::MissingSourceAsset;
        result.detail = "Animation source asset is missing";
        return result;
    }
    const assets::StaticGlbValidation validation = assets::ValidateStaticGlbFile(sourcePath);
    if (validation.status != assets::StaticGlbStatus::Ok)
    {
        result.status = CharacterAnimationCompatibilityStatus::LoadFailure;
        result.detail = validation.message;
        return result;
    }
    int animationCount = 0;
    ModelAnimation* animations = LoadModelAnimations(sourcePath.string().c_str(), &animationCount);
    int selected = -1;
    for (int index = 0; animations != nullptr && index < animationCount; ++index)
        if (result.sourceClipName == animations[index].name) { selected = index; break; }
    if (selected < 0)
    {
        if (animations != nullptr) UnloadModelAnimations(animations, animationCount);
        result.status = CharacterAnimationCompatibilityStatus::MissingSourceClip;
        result.detail = "Requested source clip was not found";
        return result;
    }
    Model sourceModel = LoadModel(sourcePath.string().c_str());
    if (!HasRenderableMesh(sourceModel))
    {
        UnloadModel(sourceModel);
        UnloadModelAnimations(animations, animationCount);
        result.status = CharacterAnimationCompatibilityStatus::LoadFailure;
        result.detail = "Animation source failed production model loading";
        return result;
    }
    const bool compatible = characterModel != nullptr
        && RaylibSkeletonsExactlyCompatible(*characterModel, sourceModel, animations[selected]);
    UnloadModel(sourceModel);
    UnloadModelAnimations(animations, animationCount);
    result.status = compatible ? CharacterAnimationCompatibilityStatus::Compatible
                               : CharacterAnimationCompatibilityStatus::SkeletonIncompatible;
    result.detail = compatible ? "Exact skeleton match"
                               : "Joint count, name, or parent hierarchy differs";
    return result;
}
}

const char* CharacterModelValidationStatusName(CharacterModelValidationStatus status)
{
    switch (status) { case CharacterModelValidationStatus::None: return "None";
    case CharacterModelValidationStatus::Resolved: return "Resolved";
    case CharacterModelValidationStatus::Missing: return "Missing";
    case CharacterModelValidationStatus::LoadFailure: return "Load failure"; }
    return "Load failure";
}

const char* CharacterAnimationCompatibilityStatusName(CharacterAnimationCompatibilityStatus status)
{
    switch (status) { case CharacterAnimationCompatibilityStatus::NoneEmbedded: return "None / embedded path";
    case CharacterAnimationCompatibilityStatus::Compatible: return "Compatible";
    case CharacterAnimationCompatibilityStatus::MissingDefinition: return "Missing definition";
    case CharacterAnimationCompatibilityStatus::MissingSourceAsset: return "Missing source asset";
    case CharacterAnimationCompatibilityStatus::MissingSourceClip: return "Missing source clip";
    case CharacterAnimationCompatibilityStatus::LoadFailure: return "Load failure";
    case CharacterAnimationCompatibilityStatus::SkeletonIncompatible: return "Skeleton incompatible"; }
    return "Load failure";
}

bool RaylibSkeletonsExactlyCompatible(const Model& characterModel,
    const Model& sourceModel, const ModelAnimation& sourceAnimation)
{
    const int jointCount = characterModel.skeleton.boneCount;
    if (jointCount <= 0 || characterModel.skeleton.bones == nullptr
        || sourceAnimation.boneCount != jointCount || sourceModel.skeleton.boneCount != jointCount
        || sourceModel.skeleton.bones == nullptr) return false;
    for (int joint = 0; joint < jointCount; ++joint)
        if (sourceModel.skeleton.bones[joint].parent != characterModel.skeleton.bones[joint].parent
            || std::string_view(sourceModel.skeleton.bones[joint].name)
                != characterModel.skeleton.bones[joint].name) return false;
    return true;
}

float ResolveRaylibAnimationFrame(float timeSeconds, int keyframeCount, PlaybackMode mode)
{
    const float lastFrame = static_cast<float>(keyframeCount > 0 ? keyframeCount - 1 : 0);
    const float durationSeconds = lastFrame / 60.0f;
    return ResolvePlaybackTime(timeSeconds, durationSeconds, mode) * 60.0f;
}

bool ApplyRaylibAnimationPose(Model& model, const ModelAnimation& animation,
    float timeSeconds, PlaybackMode mode, float* sampledFrame)
{
    if (model.boneMatrices == nullptr || model.currentPose == nullptr
        || animation.keyframeCount <= 0 || animation.keyframePoses == nullptr) return false;
    const float frame = ResolveRaylibAnimationFrame(timeSeconds, animation.keyframeCount, mode);
    UpdateModelAnimationEx(model, animation, frame, animation, frame, 0.0f);
    if (sampledFrame != nullptr) *sampledFrame = frame;
    return true;
}

CharacterAssetValidationResult ValidateCharacterAssets(
    const gameplay::GameplayDefinitionRegistry& registry,
    const gameplay::CharacterDefinition& character, const std::filesystem::path& assetRoot)
{
    CharacterAssetValidationResult result;
    Model model{};
    bool ownsModel = false;
    if (character.worldModelIdentity.empty()) result.model.detail = "No World Model assigned";
    else
    {
        const std::filesystem::path path = AssetPath(assetRoot, character.worldModelIdentity);
        if (!IsRegularFile(path))
        { result.model.status = CharacterModelValidationStatus::Missing; result.model.detail = "Authored World Model does not exist"; }
        else
        {
            const assets::StaticGlbValidation validation = assets::ValidateStaticGlbFile(path);
            if (validation.status != assets::StaticGlbStatus::Ok)
            { result.model.status = CharacterModelValidationStatus::LoadFailure; result.model.detail = validation.message; }
            else
            {
                model = LoadModel(path.string().c_str()); ownsModel = true;
                result.model.modelLoaded = HasRenderableMesh(model);
                if (!result.model.modelLoaded)
                { result.model.status = CharacterModelValidationStatus::LoadFailure; result.model.detail = "World Model failed production model loading"; }
                else
                {
                    result.model.status = CharacterModelValidationStatus::Resolved;
                    result.model.jointCount = model.skeleton.boneCount;
                    result.model.hasSkeleton = result.model.jointCount > 0 && model.skeleton.bones != nullptr;
                    result.model.skinned = result.model.hasSkeleton;
                    result.model.detail = result.model.skinned ? "Usable skeletal character model" : "Static / non-skinned model";
                    const int shown = std::min(result.model.jointCount, static_cast<int>(kCharacterAssetDiagnosticJointLimit));
                    result.model.joints.reserve(static_cast<std::size_t>(shown));
                    result.model.allJoints.reserve(static_cast<std::size_t>(result.model.jointCount));
                    for (int joint = 0; joint < result.model.jointCount; ++joint)
                    {
                        CharacterJointDiagnostic metadata{model.skeleton.bones[joint].name,
                            model.skeleton.bones[joint].parent};
                        result.model.allJoints.push_back(metadata);
                        if (joint < shown) result.model.joints.push_back(std::move(metadata));
                    }
                    result.model.jointListTruncated = shown < result.model.jointCount;
                }
            }
        }
    }
    const Model* compatibleModel = result.model.status == CharacterModelValidationStatus::Resolved
        && result.model.hasSkeleton ? &model : nullptr;
    result.idle = ValidateAnimation(registry, character.animations.idleAsset, compatibleModel, assetRoot);
    result.move = ValidateAnimation(registry, character.animations.moveAsset, compatibleModel, assetRoot);
    result.jump = ValidateAnimation(registry, character.animations.jumpAsset, compatibleModel, assetRoot);
    if (ownsModel) UnloadModel(model);
    return result;
}
}
