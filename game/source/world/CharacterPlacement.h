#pragma once

#include "world/StaticProp.h"
#include "gameplay/GameplayIdentity.h"

namespace world
{
// Persistent request for a generic visual occurrence. No runtime identity/state.
struct CharacterPlacementSpec
{
    std::string definitionIdentity;
    core::Vec3 position{};
    core::Vec3 rotationDegrees{};
    core::Vec3 scale{1.0f, 1.0f, 1.0f};
    bool patrolEnabled = false;
    float patrolDistance = 2.0f;
    float patrolSpeed = 1.0f;
};

inline bool CharacterIdentityIsValid(std::string_view identity)
{
    gameplay::ParsedGameplayIdentity parsed;
    return gameplay::TryParseGameplayIdentity(identity, parsed)
        && parsed.category == gameplay::GameplayDefinitionCategory::Character;
}

inline constexpr float kMaxNpcPatrolDistance = 100.0f;
inline constexpr float kMaxNpcPatrolSpeed = 20.0f;
inline bool NpcPatrolSettingsAreValid(const CharacterPlacementSpec& placement)
{
    return std::isfinite(placement.patrolDistance) && placement.patrolDistance > 0.0f
        && placement.patrolDistance <= kMaxNpcPatrolDistance
        && std::isfinite(placement.patrolSpeed) && placement.patrolSpeed > 0.0f
        && placement.patrolSpeed <= kMaxNpcPatrolSpeed;
}

inline bool CharacterPlacementSpecIsValid(const CharacterPlacementSpec& placement)
{
    return CharacterIdentityIsValid(placement.definitionIdentity)
        && StaticPropPositionIsValid(placement.position)
        && StaticPropRotationIsValid(placement.rotationDegrees)
        && StaticPropScaleIsValid(placement.scale)
        && NpcPatrolSettingsAreValid(placement);
}

// Geometry-only adapter for existing editor transform/picking math.
inline StaticPropSpec CharacterPlacementVisualTransform(const CharacterPlacementSpec& placement)
{
    return {{}, placement.position, placement.rotationDegrees, placement.scale};
}
}
