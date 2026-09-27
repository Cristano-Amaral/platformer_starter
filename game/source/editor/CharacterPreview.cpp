#include "editor/CharacterPreview.h"

#include "animation/SkeletalAnimation.h"

#include <algorithm>
#include <cmath>

namespace editor
{
namespace
{
std::string_view AssetIdentity(const gameplay::CharacterDefinition& character, CharacterPreviewSlot slot)
{
    if (slot == CharacterPreviewSlot::Move) return character.animations.moveAsset;
    if (slot == CharacterPreviewSlot::Jump) return character.animations.jumpAsset;
    return character.animations.idleAsset;
}

std::string_view EmbeddedClip(const gameplay::CharacterDefinition& character, CharacterPreviewSlot slot)
{
    if (slot == CharacterPreviewSlot::Move) return character.animations.move;
    if (slot == CharacterPreviewSlot::Jump) return character.animations.jump;
    return character.animations.idle;
}

const animation::CharacterAnimationCompatibilityResult& Compatibility(
    const animation::CharacterAssetValidationResult& validation, CharacterPreviewSlot slot)
{
    if (slot == CharacterPreviewSlot::Move) return validation.move;
    if (slot == CharacterPreviewSlot::Jump) return validation.jump;
    return validation.idle;
}
}

const char* CharacterPreviewSlotName(CharacterPreviewSlot slot)
{
    if (slot == CharacterPreviewSlot::Move) return "Move";
    if (slot == CharacterPreviewSlot::Jump) return "Jump";
    return "Idle";
}

const char* CharacterPreviewAnimationStatusName(CharacterPreviewAnimationStatus status)
{
    switch (status)
    {
    case CharacterPreviewAnimationStatus::Unavailable: return "Unavailable";
    case CharacterPreviewAnimationStatus::Embedded: return "Embedded clip";
    case CharacterPreviewAnimationStatus::Reusable: return "Reusable asset";
    case CharacterPreviewAnimationStatus::InvalidExplicit: return "Invalid explicit assignment";
    }
    return "Unavailable";
}

CharacterPreviewAnimationResolution ResolveCharacterPreviewAnimation(
    const gameplay::GameplayDefinitionRegistry& registry,
    const gameplay::CharacterDefinition& character,
    const animation::CharacterAssetValidationResult& validation,
    CharacterPreviewSlot slot)
{
    CharacterPreviewAnimationResolution result;
    result.playbackMode = slot == CharacterPreviewSlot::Jump
        ? animation::PlaybackMode::Clamp : animation::PlaybackMode::Loop;
    if (validation.model.status != animation::CharacterModelValidationStatus::Resolved
        || !validation.model.skinned || !validation.model.hasSkeleton)
    {
        result.detail = validation.model.status == animation::CharacterModelValidationStatus::Resolved
            ? "Skeletal animation unavailable for a static / non-skinned model"
            : validation.model.detail;
        return result;
    }

    const std::string_view assetIdentity = AssetIdentity(character, slot);
    if (assetIdentity.empty())
    {
        result.sourceAssetIdentity = character.worldModelIdentity;
        result.sourceClipName = EmbeddedClip(character, slot);
        if (result.sourceClipName.empty())
        {
            result.detail = "No embedded clip is authored";
            return result;
        }
        result.status = CharacterPreviewAnimationStatus::Embedded;
        result.detail = "Using the CharacterDefinition embedded clip";
        return result;
    }

    result.animationIdentity.assign(assetIdentity);
    const auto& compatibility = Compatibility(validation, slot);
    if (compatibility.status != animation::CharacterAnimationCompatibilityStatus::Compatible)
    {
        result.status = CharacterPreviewAnimationStatus::InvalidExplicit;
        result.detail = compatibility.detail;
        return result;
    }
    const auto* definition = registry.Find(assetIdentity);
    if (definition == nullptr || definition->category != gameplay::GameplayDefinitionCategory::Animation
        || !gameplay::ValidateAnimationDefinition(definition->animation))
    {
        result.status = CharacterPreviewAnimationStatus::InvalidExplicit;
        result.detail = "Reusable Animation Asset definition is unavailable";
        return result;
    }
    result.status = CharacterPreviewAnimationStatus::Reusable;
    result.sourceAssetIdentity = definition->animation.sourceAssetIdentity;
    result.sourceClipName = definition->animation.sourceClipName;
    result.playbackMode = definition->animation.playbackMode;
    result.detail = compatibility.detail;
    return result;
}

void AdvanceCharacterPreviewPlayback(CharacterPreviewPlayback& playback, float deltaSeconds,
    float durationSeconds, animation::PlaybackMode mode)
{
    if (!playback.playing || !(deltaSeconds > 0.0f) || !std::isfinite(deltaSeconds)) return;
    playback.timeSeconds = animation::ResolvePlaybackTime(
        playback.timeSeconds + deltaSeconds, durationSeconds, mode);
}
}
