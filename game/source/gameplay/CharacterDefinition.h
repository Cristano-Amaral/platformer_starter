#pragma once

// Typed authored Character payload (Milestone 101). Definition metadata only;
// M102 projects Player base stats from it without creating a Character instance.

#include "gameplay/GameplayIdentity.h"
#include "gameplay/AnimationDefinition.h"
#include "gameplay/GameplayStat.h"
#include "gameplay/ItemDefinition.h"
#include "gameplay/HumanoidSkeletonMapping.h"
#include "core/Vec3.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cmath>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace gameplay
{
enum class CharacterType { Player = 0, Enemy, NPC, Animal };
inline constexpr std::size_t kCharacterTypeCount = 4;
inline constexpr std::size_t kMaxCharacterDisplayNameLength = 64;
inline constexpr std::size_t kMaxCharacterDescriptionLength = 200;
inline constexpr std::size_t kMaxCharacterAnimationClipNameLength = 64;
inline constexpr std::array<std::string_view, kCharacterTypeCount> kCharacterTypeNames = {
    "Player", "Enemy", "NPC", "Animal"};

inline std::string_view CharacterTypeName(CharacterType type)
{
    const auto index = static_cast<std::size_t>(type);
    return index < kCharacterTypeCount ? kCharacterTypeNames[index] : std::string_view{};
}

inline std::optional<CharacterType> CharacterTypeFromName(std::string_view name)
{
    for (std::size_t index = 0; index < kCharacterTypeCount; ++index)
    {
        if (name == kCharacterTypeNames[index]) return static_cast<CharacterType>(index);
    }
    return std::nullopt;
}

inline bool IsValidCharacterDisplayName(std::string_view text)
{
    return !text.empty() && text.size() <= kMaxCharacterDisplayNameLength
        && text.front() != ' ' && text.back() != ' '
        && std::all_of(text.begin(), text.end(), ItemAuthoredTextCharIsAllowed);
}

inline bool IsValidCharacterDescription(std::string_view text)
{
    return text.size() <= kMaxCharacterDescriptionLength
        && std::all_of(text.begin(), text.end(), ItemAuthoredTextCharIsAllowed);
}

inline bool IsValidCharacterAnimationClipName(std::string_view text)
{
    return !text.empty() && text.size() <= kMaxCharacterAnimationClipNameLength
        && text.front() != ' ' && text.back() != ' '
        && std::all_of(text.begin(), text.end(), ItemAuthoredTextCharIsAllowed);
}

struct CharacterLocomotionAnimations
{
    std::string idle;
    std::string move;
    std::string jump;
    std::string idleAsset;
    std::string moveAsset;
    std::string jumpAsset;
    std::string hitReaction;
    std::string hitReactionAsset;
    std::string attack;
    std::string attackAsset;
};

enum class CharacterAnimationBindingStatus { None, Resolved, Missing };

inline CharacterAnimationBindingStatus ResolveCharacterAnimationBinding(
    std::string_view binding, std::span<const std::string_view> availableClips)
{
    if (binding.empty()) return CharacterAnimationBindingStatus::None;
    return std::find(availableClips.begin(), availableClips.end(), binding) != availableClips.end()
        ? CharacterAnimationBindingStatus::Resolved : CharacterAnimationBindingStatus::Missing;
}

struct CharacterLocomotionBindingResolution
{
    CharacterAnimationBindingStatus idle = CharacterAnimationBindingStatus::None;
    CharacterAnimationBindingStatus move = CharacterAnimationBindingStatus::None;
    CharacterAnimationBindingStatus jump = CharacterAnimationBindingStatus::None;

    bool AllResolved() const
    {
        return idle == CharacterAnimationBindingStatus::Resolved
            && move == CharacterAnimationBindingStatus::Resolved
            && jump == CharacterAnimationBindingStatus::Resolved;
    }
};

inline CharacterLocomotionBindingResolution ResolveCharacterLocomotionBindings(
    const CharacterLocomotionAnimations& bindings,
    std::span<const std::string_view> availableClips)
{
    return {
        ResolveCharacterAnimationBinding(bindings.idle, availableClips),
        ResolveCharacterAnimationBinding(bindings.move, availableClips),
        ResolveCharacterAnimationBinding(bindings.jump, availableClips)};
}

inline std::string DefaultCharacterDisplayName(std::string_view identity)
{
    ParsedGameplayIdentity parsed;
    if (!TryParseGameplayIdentity(identity, parsed)
        || parsed.category != GameplayDefinitionCategory::Character) return {};
    return parsed.name;
}

struct CharacterDefinition
{
    struct MeleeHitDefinition
    {
        bool enabled = false;
        float windowStart = 0.0f;
        float windowEnd = 1.0f;
        core::Vec3 center{};
        core::Vec3 halfExtents{0.5f, 0.5f, 0.5f};
    } meleeHit{};
    std::string displayName;
    std::string description;
    CharacterType type = CharacterType::NPC;
    std::string worldModelIdentity;
    CharacterLocomotionAnimations animations{};
    HumanoidSkeletonMapping humanoidMapping{};
    std::array<bool, kGameplayStatCount> hasBaseStat{};
    std::array<float, kGameplayStatCount> baseStatValue{};
};

inline CharacterDefinition MakeDefaultCharacterDefinition(std::string_view identity)
{
    CharacterDefinition character;
    character.displayName = DefaultCharacterDisplayName(identity);
    return character;
}

enum class ValidateCharacterStatus
{
    Valid, InvalidDisplayName, InvalidDescription, InvalidType, InvalidWorldModel,
    InvalidAnimationBinding, InvalidBaseStat, InvalidMeleeHit,
};

inline const char* ValidateCharacterStatusName(ValidateCharacterStatus status)
{
    switch (status)
    {
    case ValidateCharacterStatus::Valid: return "Valid";
    case ValidateCharacterStatus::InvalidDisplayName: return "InvalidDisplayName";
    case ValidateCharacterStatus::InvalidDescription: return "InvalidDescription";
    case ValidateCharacterStatus::InvalidType: return "InvalidType";
    case ValidateCharacterStatus::InvalidWorldModel: return "InvalidWorldModel";
    case ValidateCharacterStatus::InvalidAnimationBinding: return "InvalidAnimationBinding";
    case ValidateCharacterStatus::InvalidBaseStat: return "InvalidBaseStat";
    case ValidateCharacterStatus::InvalidMeleeHit: return "InvalidMeleeHit";
    }
    return "InvalidBaseStat";
}

inline ValidateCharacterStatus ValidateCharacterDefinition(const CharacterDefinition& character)
{
    const auto& hit = character.meleeHit;
    const auto bounded = [](float value, float minimum, float maximum) {
        return std::isfinite(value) && value >= minimum && value <= maximum;
    };
    if (hit.enabled && (!bounded(hit.windowStart, 0.0f, 1.0f)
        || !bounded(hit.windowEnd, 0.0f, 1.0f) || hit.windowStart >= hit.windowEnd
        || !bounded(hit.center.x, -100.0f, 100.0f) || !bounded(hit.center.y, -100.0f, 100.0f)
        || !bounded(hit.center.z, -100.0f, 100.0f)
        || !bounded(hit.halfExtents.x, 0.001f, 100.0f)
        || !bounded(hit.halfExtents.y, 0.001f, 100.0f)
        || !bounded(hit.halfExtents.z, 0.001f, 100.0f))) return ValidateCharacterStatus::InvalidMeleeHit;
    if (!IsValidCharacterDisplayName(character.displayName)) return ValidateCharacterStatus::InvalidDisplayName;
    if (!IsValidCharacterDescription(character.description)) return ValidateCharacterStatus::InvalidDescription;
    if (CharacterTypeName(character.type).empty()) return ValidateCharacterStatus::InvalidType;
    if (!character.worldModelIdentity.empty()
        && !IsValidItemWorldModelIdentity(character.worldModelIdentity))
        return ValidateCharacterStatus::InvalidWorldModel;
    const auto validBinding = [](const std::string& binding) {
        return binding.empty() || IsValidCharacterAnimationClipName(binding);
    };
    if (!validBinding(character.animations.idle) || !validBinding(character.animations.move)
        || !validBinding(character.animations.jump) || !validBinding(character.animations.hitReaction) || !validBinding(character.animations.attack))
        return ValidateCharacterStatus::InvalidAnimationBinding;
    const auto validAsset = [](const std::string& identity) {
        return identity.empty() || IsValidAnimationIdentity(identity);
    };
    if (!validAsset(character.animations.idleAsset) || !validAsset(character.animations.moveAsset)
        || !validAsset(character.animations.jumpAsset) || !validAsset(character.animations.hitReactionAsset) || !validAsset(character.animations.attackAsset))
        return ValidateCharacterStatus::InvalidAnimationBinding;
    for (std::size_t index = 0; index < kGameplayStatCount; ++index)
    {
        if (character.hasBaseStat[index] && !IsValidGameplayStatValue(character.baseStatValue[index]))
            return ValidateCharacterStatus::InvalidBaseStat;
    }
    for (const auto& joint : character.humanoidMapping.joints)
        if (!joint.empty() && (!IsValidCharacterAnimationClipName(joint) || joint.size() > 63))
            return ValidateCharacterStatus::InvalidAnimationBinding;
    return ValidateCharacterStatus::Valid;
}

inline bool CharacterDefinitionsEqual(const CharacterDefinition& a, const CharacterDefinition& b)
{
    return a.displayName == b.displayName && a.description == b.description && a.type == b.type
        && a.worldModelIdentity == b.worldModelIdentity
        && a.animations.idle == b.animations.idle
        && a.animations.move == b.animations.move
        && a.animations.jump == b.animations.jump
        && a.animations.idleAsset == b.animations.idleAsset
        && a.animations.moveAsset == b.animations.moveAsset
        && a.animations.jumpAsset == b.animations.jumpAsset
        && a.animations.hitReaction == b.animations.hitReaction
        && a.animations.hitReactionAsset == b.animations.hitReactionAsset
        && a.animations.attack == b.animations.attack
        && a.animations.attackAsset == b.animations.attackAsset
        && a.meleeHit.enabled == b.meleeHit.enabled
        && a.meleeHit.windowStart == b.meleeHit.windowStart && a.meleeHit.windowEnd == b.meleeHit.windowEnd
        && a.meleeHit.center.x == b.meleeHit.center.x && a.meleeHit.center.y == b.meleeHit.center.y
        && a.meleeHit.center.z == b.meleeHit.center.z
        && a.meleeHit.halfExtents.x == b.meleeHit.halfExtents.x
        && a.meleeHit.halfExtents.y == b.meleeHit.halfExtents.y
        && a.meleeHit.halfExtents.z == b.meleeHit.halfExtents.z
        && a.humanoidMapping.joints == b.humanoidMapping.joints
        && a.hasBaseStat == b.hasBaseStat
        && a.baseStatValue == b.baseStatValue;
}
}
