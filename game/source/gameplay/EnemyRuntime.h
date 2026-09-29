#pragma once

#include "gameplay/CharacterPatrolState.h"
#include "gameplay/RuntimeHealth.h"
#include <cstdint>
#include <cstddef>

namespace render { class CharacterInstance; }

namespace gameplay
{
using EnemyLocomotionState = CharacterPatrolLocomotionState;

// Session-only typed actor. The owner clears borrowed references before instances.
struct EnemyRuntimeActor : CharacterPatrolState
{
    void Advance(float deltaSeconds)
    {
        const bool reacting = health.HitReactionActive();
        health.AdvanceHitReaction(deltaSeconds);
        health.AdvanceDamageFeedback(deltaSeconds);
        if (health.Defeated()) locomotion = CharacterPatrolLocomotionState::Idle;
        else if (!reacting) CharacterPatrolState::Advance(deltaSeconds);
    }
    RuntimeHealth health{};
    std::uint64_t handle = 0;
    std::size_t sourcePlacementIndex = 0;
    render::CharacterInstance* instance = nullptr;

    EnemyRuntimeActor(std::size_t index, const world::CharacterPlacementSpec& placement,
        render::CharacterInstance* presentation, float maximum = kFallbackMaxHealth)
        : CharacterPatrolState(placement), health(maximum), sourcePlacementIndex(index), instance(presentation)
    {
        static std::uint64_t nextHandle = 1;
        handle = nextHandle++;
    }
};
}
