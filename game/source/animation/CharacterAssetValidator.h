#pragma once

#include "gameplay/GameplayDefinition.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

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
struct CharacterModelValidationResult
{
    CharacterModelValidationStatus status = CharacterModelValidationStatus::None;
    bool modelLoaded = false;
    bool skinned = false;
    bool hasSkeleton = false;
    int jointCount = 0;
    std::vector<CharacterJointDiagnostic> joints;
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
};
struct CharacterAssetValidationResult
{
    CharacterModelValidationResult model;
    CharacterAnimationCompatibilityResult idle;
    CharacterAnimationCompatibilityResult move;
    CharacterAnimationCompatibilityResult jump;
};

bool RaylibSkeletonsExactlyCompatible(const Model& characterModel,
    const Model& sourceModel, const ModelAnimation& sourceAnimation);

CharacterAssetValidationResult ValidateCharacterAssets(
    const gameplay::GameplayDefinitionRegistry& registry,
    const gameplay::CharacterDefinition& character,
    const std::filesystem::path& assetRoot);
}
