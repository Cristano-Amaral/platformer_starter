#include "animation/CharacterAssetValidator.h"

#include "assets/StaticGlb.h"
#include "animation/HumanoidSkeletonMapping.h"

#include <raylib.h>
#include <raymath.h>

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

CharacterModelValidationResult DescribeSkeleton(const Model& model)
{
    CharacterModelValidationResult result;
    result.status = HasRenderableMesh(model) ? CharacterModelValidationStatus::Resolved
        : CharacterModelValidationStatus::LoadFailure;
    result.modelLoaded = HasRenderableMesh(model);
    result.jointCount = model.skeleton.boneCount;
    result.hasSkeleton = result.jointCount > 0 && model.skeleton.bones != nullptr;
    result.skinned = result.hasSkeleton;
    for (int index = 0; result.hasSkeleton && index < result.jointCount; ++index)
        result.allJoints.push_back({model.skeleton.bones[index].name, model.skeleton.bones[index].parent});
    return result;
}

RetargetValidationResult ValidateRetarget(const gameplay::AnimationDefinition& source,
    const gameplay::HumanoidSkeletonMapping& targetMapping,
    const CharacterModelValidationResult& target, const Model& targetModel,
    const Model& sourceModel,
    const ModelAnimation& clip)
{
    RetargetValidationResult result;
    result.sourceJoints.fill(-1);
    result.targetJoints.fill(-1);
    const auto sourceDescription = DescribeSkeleton(sourceModel);
    result.sourceJointCount = sourceDescription.jointCount;
    result.targetJointCount = target.jointCount;
    const auto sourceValidation = ValidateHumanoidMapping(source.sourceHumanoidMapping, sourceDescription);
    const auto targetValidation = ValidateHumanoidMapping(targetMapping, target);
    result.sourceMappingState = HumanoidMappingStateName(sourceValidation.state);
    result.targetMappingState = HumanoidMappingStateName(targetValidation.state);
    if (sourceValidation.state != HumanoidMappingState::Usable
        || targetValidation.state != HumanoidMappingState::Usable)
    {
        const bool sourceBad = sourceValidation.state != HumanoidMappingState::Usable;
        const auto& validation = sourceBad ? sourceValidation : targetValidation;
        result.status = validation.state == HumanoidMappingState::Invalid
            ? RetargetValidationStatus::InvalidMapping
            : sourceBad ? RetargetValidationStatus::SourceMappingMissing
                        : RetargetValidationStatus::TargetMappingMissing;
        result.detail = std::string(sourceBad ? "Source" : "Target") + " mapping: "
            + HumanoidMappingStateName(validation.state);
        for (const auto& item : validation.details)
            if (item.jointIndex < 0) { result.detail += "; " + item.detail; break; }
        return result;
    }
    if (sourceModel.skeleton.bindPose == nullptr || targetModel.skeleton.bindPose == nullptr
        || targetModel.currentPose == nullptr || targetModel.boneMatrices == nullptr
        || sourceModel.skeleton.boneCount > static_cast<int>(kMaxSkinJoints)
        || targetModel.skeleton.boneCount > static_cast<int>(kMaxSkinJoints)
        || clip.boneCount != sourceModel.skeleton.boneCount
        || clip.keyframeCount <= 0 || clip.keyframePoses == nullptr)
    {
        result.status = RetargetValidationStatus::UnsupportedSkeletonOrAnimation;
        result.detail = "Source clip and skeleton pose data are incomplete";
        return result;
    }
    for (std::size_t role = 0; role < gameplay::kHumanoidJointRoleCount; ++role)
    {
        const auto& sourceName = source.sourceHumanoidMapping.joints[role];
        const auto& targetName = targetMapping.joints[role];
        if (sourceName.empty() || targetName.empty()) continue;
        for (std::size_t joint = 0; joint < sourceDescription.allJoints.size(); ++joint)
            if (sourceDescription.allJoints[joint].name == sourceName) result.sourceJoints[role] = static_cast<int>(joint);
        for (std::size_t joint = 0; joint < target.allJoints.size(); ++joint)
            if (target.allJoints[joint].name == targetName) result.targetJoints[role] = static_cast<int>(joint);
    }
    result.status = RetargetValidationStatus::Retargetable;
    result.detail = "Mapped source (" + std::to_string(result.sourceJointCount)
        + " joints) and target (" + std::to_string(result.targetJointCount)
        + " joints) support rest-relative retargeting";
    return result;
}

CharacterAnimationCompatibilityResult ValidateAnimation(
    const gameplay::GameplayDefinitionRegistry& registry, std::string_view identity,
    const Model* characterModel, const CharacterModelValidationResult& targetDescription,
    const gameplay::HumanoidSkeletonMapping& targetMapping,
    const std::filesystem::path& assetRoot)
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
    if (!compatible && characterModel != nullptr)
        result.retarget = ValidateRetarget(definition->animation, targetMapping,
            targetDescription, *characterModel, sourceModel, animations[selected]);
    UnloadModel(sourceModel);
    UnloadModelAnimations(animations, animationCount);
    result.status = compatible ? CharacterAnimationCompatibilityStatus::Compatible
                               : CharacterAnimationCompatibilityStatus::SkeletonIncompatible;
    result.detail = compatible ? "Exact skeleton match"
                               : "Joint count, name, or parent hierarchy differs";
    return result;
}
}

const char* RetargetValidationStatusName(RetargetValidationStatus status)
{
    switch (status)
    {
    case RetargetValidationStatus::NotRequested: return "Not requested";
    case RetargetValidationStatus::SourceMappingMissing: return "Source mapping missing or incomplete";
    case RetargetValidationStatus::TargetMappingMissing: return "Target mapping missing or incomplete";
    case RetargetValidationStatus::InvalidMapping: return "Invalid mapping";
    case RetargetValidationStatus::UnsupportedSkeletonOrAnimation: return "Unsupported skeleton or animation";
    case RetargetValidationStatus::Retargetable: return "Retargetable";
    }
    return "Unsupported skeleton or animation";
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

bool ApplyRaylibRetargetedPose(Model& target, const Model& source,
    const ModelAnimation& clip, const RetargetValidationResult& mapping,
    float timeSeconds, PlaybackMode mode, std::vector<unsigned char>& scratch,
    float* sampledFrame)
{
    if (mapping.status != RetargetValidationStatus::Retargetable || target.currentPose == nullptr
        || target.boneMatrices == nullptr || target.skeleton.bindPose == nullptr
        || source.skeleton.bindPose == nullptr || source.skeleton.bones == nullptr
        || clip.keyframePoses == nullptr
        || clip.boneCount != source.skeleton.boneCount || clip.keyframeCount <= 0
        || target.skeleton.boneCount <= 0 || target.skeleton.bones == nullptr) return false;
    for (std::size_t role = 0; role < gameplay::kHumanoidJointRoleCount; ++role)
        if (mapping.sourceJoints[role] >= source.skeleton.boneCount
            || mapping.targetJoints[role] >= target.skeleton.boneCount
            || mapping.sourceJoints[role] < -1 || mapping.targetJoints[role] < -1) return false;
    const int count = target.skeleton.boneCount;
    scratch.assign(static_cast<std::size_t>(count), 0);
    const float frame = ResolveRaylibAnimationFrame(timeSeconds, clip.keyframeCount, mode);
    const int first = static_cast<int>(frame) % clip.keyframeCount;
    const int next = (first + 1) % clip.keyframeCount;
    if (clip.keyframePoses[first] == nullptr || clip.keyframePoses[next] == nullptr) return false;
    const float blend = frame - static_cast<float>(first);
    const auto sampledSource = [&](int joint) {
        const Transform& a = clip.keyframePoses[first][joint];
        const Transform& b = clip.keyframePoses[next][joint];
        return Transform{Vector3Lerp(a.translation, b.translation, blend),
            QuaternionSlerp(a.rotation, b.rotation, blend), Vector3Lerp(a.scale, b.scale, blend)};
    };
    const auto localRotation = [](::Quaternion global, ::Quaternion parent) {
        return QuaternionNormalize(QuaternionMultiply(QuaternionInvert(parent), global));
    };
    const auto localTranslation = [](Vector3 global, Vector3 parentPosition, ::Quaternion parentRotation,
        Vector3 parentScale) {
        const Vector3 offset = Vector3RotateByQuaternion(Vector3Subtract(global, parentPosition),
            QuaternionInvert(parentRotation));
        return Vector3Divide(offset, parentScale);
    };
    const auto globalMatrix = [](const Transform& pose) {
        return MatrixMultiply(MatrixMultiply(MatrixScale(pose.scale.x, pose.scale.y, pose.scale.z),
            QuaternionToMatrix(pose.rotation)),
            MatrixTranslate(pose.translation.x, pose.translation.y, pose.translation.z));
    };
    const auto resolve = [&](auto&& self, int joint) -> bool {
        if (scratch[static_cast<std::size_t>(joint)] == 2) return true;
        if (scratch[static_cast<std::size_t>(joint)] == 1) return false;
        scratch[static_cast<std::size_t>(joint)] = 1;
        const int parent = target.skeleton.bones[joint].parent;
        if (parent >= count || parent < -1 || (parent >= 0 && !self(self, parent))) return false;
        const Transform& targetRest = target.skeleton.bindPose[joint];
        const Transform parentRest = parent < 0 ? Transform{Vector3Zero(), QuaternionIdentity(), Vector3One()}
            : target.skeleton.bindPose[parent];
        const Transform parentCurrent = parent < 0 ? Transform{Vector3Zero(), QuaternionIdentity(), Vector3One()}
            : target.currentPose[parent];
        Transform local{};
        local.translation = localTranslation(targetRest.translation, parentRest.translation,
            parentRest.rotation, parentRest.scale);
        local.rotation = localRotation(targetRest.rotation, parentRest.rotation);
        local.scale = Vector3Divide(targetRest.scale, parentRest.scale);
        for (std::size_t role = 0; role < gameplay::kHumanoidJointRoleCount; ++role)
        {
            if (mapping.targetJoints[role] != joint || mapping.sourceJoints[role] < 0) continue;
            const int sourceJoint = mapping.sourceJoints[role];
            const int sourceParent = source.skeleton.bones[sourceJoint].parent;
            if (sourceParent >= source.skeleton.boneCount || sourceParent < -1) return false;
            const Transform sourceRest = source.skeleton.bindPose[sourceJoint];
            const Transform sourceRestParent = sourceParent < 0
                ? Transform{Vector3Zero(), QuaternionIdentity(), Vector3One()}
                : source.skeleton.bindPose[sourceParent];
            const Transform sourceCurrent = sampledSource(sourceJoint);
            const Transform sourceCurrentParent = sourceParent < 0
                ? Transform{Vector3Zero(), QuaternionIdentity(), Vector3One()}
                : sampledSource(sourceParent);
            const ::Quaternion restLocal = localRotation(sourceRest.rotation, sourceRestParent.rotation);
            const ::Quaternion animatedLocal = localRotation(sourceCurrent.rotation, sourceCurrentParent.rotation);
            const ::Quaternion delta = QuaternionMultiply(QuaternionInvert(restLocal), animatedLocal);
            local.rotation = QuaternionNormalize(QuaternionMultiply(local.rotation, delta));
            // Only root Hips translation is transferred, without proportion scaling.
            if (role == static_cast<std::size_t>(gameplay::HumanoidJointRole::Hips)
                && sourceParent < 0 && parent < 0)
                local.translation = Vector3Add(local.translation,
                    Vector3Subtract(sourceCurrent.translation, sourceRest.translation));
            break;
        }
        Transform& current = target.currentPose[joint];
        current.rotation = QuaternionNormalize(QuaternionMultiply(parentCurrent.rotation, local.rotation));
        current.scale = Vector3Multiply(parentCurrent.scale, local.scale);
        current.translation = Vector3Add(parentCurrent.translation,
            Vector3RotateByQuaternion(Vector3Multiply(parentCurrent.scale, local.translation), parentCurrent.rotation));
        target.boneMatrices[joint] = MatrixMultiply(MatrixInvert(globalMatrix(targetRest)), globalMatrix(current));
        scratch[static_cast<std::size_t>(joint)] = 2;
        return true;
    };
    for (int joint = 0; joint < count; ++joint)
        if (!resolve(resolve, joint)) return false;
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
    result.idle = ValidateAnimation(registry, character.animations.idleAsset, compatibleModel,
        result.model, character.humanoidMapping, assetRoot);
    result.move = ValidateAnimation(registry, character.animations.moveAsset, compatibleModel,
        result.model, character.humanoidMapping, assetRoot);
    result.jump = ValidateAnimation(registry, character.animations.jumpAsset, compatibleModel,
        result.model, character.humanoidMapping, assetRoot);
    if (ownsModel) UnloadModel(model);
    return result;
}
}
