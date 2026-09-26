#pragma once

#include "animation/SkeletalAnimation.h"
#include "gameplay/GameplayIdentity.h"
#include "gameplay/ItemDefinition.h"

#include <string>
#include <string_view>
#include <algorithm>

namespace gameplay
{
inline constexpr std::size_t kMaxAnimationClipNameLength = 64;

struct AnimationDefinition
{
    std::string sourceAssetIdentity;
    std::string sourceClipName;
    animation::PlaybackMode playbackMode = animation::PlaybackMode::Loop;
};

inline bool IsValidAnimationIdentity(std::string_view identity)
{
    ParsedGameplayIdentity parsed;
    return TryParseGameplayIdentity(identity, parsed)
        && parsed.category == GameplayDefinitionCategory::Animation;
}

inline bool IsValidAnimationClipName(std::string_view name)
{
    return !name.empty() && name.size() <= kMaxAnimationClipNameLength
        && name.front() != ' ' && name.back() != ' '
        && std::all_of(name.begin(), name.end(), ItemAuthoredTextCharIsAllowed);
}

inline bool ValidateAnimationDefinition(const AnimationDefinition& definition)
{
    return IsValidItemWorldModelIdentity(definition.sourceAssetIdentity)
        && IsValidAnimationClipName(definition.sourceClipName);
}

inline bool AnimationDefinitionsEqual(const AnimationDefinition& a, const AnimationDefinition& b)
{
    return a.sourceAssetIdentity == b.sourceAssetIdentity
        && a.sourceClipName == b.sourceClipName
        && a.playbackMode == b.playbackMode;
}
}
