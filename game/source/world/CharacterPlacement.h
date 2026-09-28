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
};

inline bool CharacterIdentityIsValid(std::string_view identity)
{
    gameplay::ParsedGameplayIdentity parsed;
    return gameplay::TryParseGameplayIdentity(identity, parsed)
        && parsed.category == gameplay::GameplayDefinitionCategory::Character;
}

inline bool CharacterPlacementSpecIsValid(const CharacterPlacementSpec& placement)
{
    return CharacterIdentityIsValid(placement.definitionIdentity)
        && StaticPropPositionIsValid(placement.position)
        && StaticPropRotationIsValid(placement.rotationDegrees)
        && StaticPropScaleIsValid(placement.scale);
}

// Geometry-only adapter for existing editor transform/picking math.
inline StaticPropSpec CharacterPlacementVisualTransform(const CharacterPlacementSpec& placement)
{
    return {{}, placement.position, placement.rotationDegrees, placement.scale};
}
}
