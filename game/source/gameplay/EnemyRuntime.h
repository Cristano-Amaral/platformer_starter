#pragma once

#include "gameplay/CharacterPatrolState.h"
#include <cstdint>
#include <cstddef>

namespace render { class CharacterInstance; }

namespace gameplay
{
using EnemyLocomotionState = CharacterPatrolLocomotionState;

// Session-only typed actor. The owner clears borrowed references before instances.
struct EnemyRuntimeActor : CharacterPatrolState
{
    std::uint64_t handle = 0;
    std::size_t sourcePlacementIndex = 0;
    render::CharacterInstance* instance = nullptr;

    EnemyRuntimeActor(std::size_t index, const world::CharacterPlacementSpec& placement,
        render::CharacterInstance* presentation)
        : CharacterPatrolState(placement), sourcePlacementIndex(index), instance(presentation)
    {
        static std::uint64_t nextHandle = 1;
        handle = nextHandle++;
    }
};
}
