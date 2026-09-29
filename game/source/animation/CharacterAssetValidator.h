#pragma once

#include "gameplay/GameplayDefinition.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>
#include <array>

struct Model;
struct ModelAnimation;

namespace animation
{
inline constexpr std::size_t kCharacterAssetDiagnosticJointLimit = 12;

enum class CharacterModelValidationStatus { None, Resolved, Missing, LoadFailure };
enum class CharacterAnimationCompatibilityStatus
{
    NoneEmbedded, Compatible, MissingDefinition, MissingSourceAsset,
    MissingSourceClip, LoadFailure, SkeletonIncompatible,
};

const char* CharacterModelValidationStatusName(CharacterModelValidationStatus status);
const char* CharacterAnimationCompatibilityStatusName(
    CharacterAnimationCompatibilityStatus status);

struct CharacterJointDiagnostic { std::string name; int parentIndex = -1; };
enum class RetargetValidationStatus
{
    NotRequested, SourceMappingMissing, TargetMappingMissing, InvalidMapping,
    UnsupportedSkeletonOrAnimation, Retargetable,
};
const char* RetargetValidationStatusName(RetargetValidationStatus status);
struct RetargetValidationResult
{
    RetargetValidationStatus status = RetargetValidationStatus::NotRequested;
    std::string detail;
    std::string sourceMappingState;
    std::string targetMappingState;
    int sourceJointCount = 0;
    int targetJointCount = 0;
    std::array<int, gameplay::kHumanoidJointRoleCount> sourceJoints{};
    std::array<int, gameplay::kHumanoidJointRoleCount> targetJoints{};
};
struct CharacterModelValidationResult
{
    CharacterModelValidationStatus status = CharacterModelValidationStatus::None;
    bool modelLoaded = false;
    bool skinned = false;
    bool hasSkeleton = false;
    int jointCount = 0;
    std::vector<CharacterJointDiagnostic> joints;
    std::vector<CharacterJointDiagnostic> allJoints;
    bool jointListTruncated = false;
    std::string detail;
};
struct CharacterAnimationCompatibilityResult
{
    CharacterAnimationCompatibilityStatus status = CharacterAnimationCompatibilityStatus::NoneEmbedded;
    std::string animationIdentity;
    std::string sourceAssetIdentity;
    std::string sourceClipName;
    std::string detail;
    RetargetValidationResult retarget{};
};
struct CharacterAssetValidationResult
{
    CharacterModelValidationResult model;
    CharacterAnimationCompatibilityResult idle;
    CharacterAnimationCompatibilityResult move;
    CharacterAnimationCompatibilityResult jump;
    CharacterAnimationCompatibilityResult hitReaction;
};

bool RaylibSkeletonsExactlyCompatible(const Model& characterModel,
    const Model& sourceModel, const ModelAnimation& sourceAnimation);
float ResolveRaylibAnimationFrame(float timeSeconds, int keyframeCount, PlaybackMode mode);
bool ApplyRaylibAnimationPose(Model& model, const ModelAnimation& animation,
    float timeSeconds, PlaybackMode mode, float* sampledFrame = nullptr);
bool ApplyRaylibRetargetedPose(Model& target, const Model& source,
    const ModelAnimation& clip, const RetargetValidationResult& mapping,
    float timeSeconds, PlaybackMode mode, std::vector<unsigned char>& scratch,
    float* sampledFrame = nullptr);

CharacterAssetValidationResult ValidateCharacterAssets(
    const gameplay::GameplayDefinitionRegistry& registry,
    const gameplay::CharacterDefinition& character,
    const std::filesystem::path& assetRoot);
}
