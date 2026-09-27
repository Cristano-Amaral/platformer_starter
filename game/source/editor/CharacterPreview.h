#pragma once

#include "animation/CharacterAssetValidator.h"
#include "gameplay/CharacterDefinition.h"
#include "gameplay/GameplayDefinition.h"

#include <string>
#include <string_view>

namespace editor
{
enum class CharacterPreviewSlot { Idle, Move, Jump };
enum class CharacterPreviewAnimationStatus
{
    Unavailable,
    Embedded,
    Reusable,
    Retargeted,
    InvalidExplicit,
};

struct CharacterPreviewAnimationResolution
{
    CharacterPreviewAnimationStatus status = CharacterPreviewAnimationStatus::Unavailable;
    std::string animationIdentity;
    std::string sourceAssetIdentity;
    std::string sourceClipName;
    animation::PlaybackMode playbackMode = animation::PlaybackMode::Loop;
    std::string detail;
    animation::RetargetValidationResult retarget{};

    bool CanSample() const
    {
        return status == CharacterPreviewAnimationStatus::Embedded
            || status == CharacterPreviewAnimationStatus::Reusable
            || status == CharacterPreviewAnimationStatus::Retargeted;
    }
};

const char* CharacterPreviewSlotName(CharacterPreviewSlot slot);
const char* CharacterPreviewAnimationStatusName(CharacterPreviewAnimationStatus status);
CharacterPreviewAnimationResolution ResolveCharacterPreviewAnimation(
    const gameplay::GameplayDefinitionRegistry& registry,
    const gameplay::CharacterDefinition& character,
    const animation::CharacterAssetValidationResult& validation,
    CharacterPreviewSlot slot);

struct CharacterPreviewPlayback
{
    bool playing = true;
    float timeSeconds = 0.0f;
};

inline void RestartCharacterPreviewPlayback(CharacterPreviewPlayback& playback)
{
    playback.timeSeconds = 0.0f;
}
void AdvanceCharacterPreviewPlayback(
    CharacterPreviewPlayback& playback,
    float deltaSeconds,
    float durationSeconds,
    animation::PlaybackMode mode);
}
