#pragma once

#include "gameplay/CharacterPatrolState.h"
#include "gameplay/RuntimeHealth.h"
#include <cstdint>
#include <cstddef>

namespace render { class CharacterInstance; }

namespace gameplay
{
using NpcLocomotionState = CharacterPatrolLocomotionState;

// Session-only typed actor. The owner clears borrowed references before instances.
struct NpcRuntimeActor : CharacterPatrolState
{
    void Advance(float deltaSeconds)
    {
        // Hold the entire step, including expiry, so patrol never catches up.
        const bool actionActive = health.HitReactionActive() || health.AttackActive();
        health.AdvanceHitReaction(deltaSeconds);
        health.AdvanceAttack(deltaSeconds);
        health.AdvanceDamageFeedback(deltaSeconds);
        if (health.Defeated()) locomotion = CharacterPatrolLocomotionState::Idle;
        else if (!actionActive) CharacterPatrolState::Advance(deltaSeconds);
    }
    RuntimeHealth health{};
    std::uint64_t handle = 0;
    std::size_t sourcePlacementIndex = 0;
    render::CharacterInstance* instance = nullptr;

    NpcRuntimeActor(std::size_t index, const world::CharacterPlacementSpec& placement,
        render::CharacterInstance* presentation, float maximum = kFallbackMaxHealth)
        : CharacterPatrolState(placement), health(maximum), sourcePlacementIndex(index), instance(presentation)
    {
        static std::uint64_t nextHandle = 1;
        handle = nextHandle++;
    }
};
}
