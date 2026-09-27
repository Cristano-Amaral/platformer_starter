#pragma once

#include "animation/SkeletalAnimation.h"
#include "gameplay/GameplayIdentity.h"
#include "gameplay/ItemDefinition.h"
#include "gameplay/HumanoidSkeletonMapping.h"

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
    HumanoidSkeletonMapping sourceHumanoidMapping{};
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
    if (!IsValidItemWorldModelIdentity(definition.sourceAssetIdentity)
        || !IsValidAnimationClipName(definition.sourceClipName)) return false;
    for (const auto& joint : definition.sourceHumanoidMapping.joints)
        if (!joint.empty() && !IsValidAnimationClipName(joint)) return false;
    return true;
}

inline bool AnimationDefinitionsEqual(const AnimationDefinition& a, const AnimationDefinition& b)
{
    return a.sourceAssetIdentity == b.sourceAssetIdentity
        && a.sourceClipName == b.sourceClipName
        && a.playbackMode == b.playbackMode
        && a.sourceHumanoidMapping.joints == b.sourceHumanoidMapping.joints;
}
}
