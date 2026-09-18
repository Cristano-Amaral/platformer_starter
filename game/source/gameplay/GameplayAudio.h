#pragma once

// Milestone 71/72/73: semantic gameplay SFX request recording, the lethal-hit
// cue precedence rule, player movement-audio presentation tracking, and
// world-interaction edge observation. Presentation only. Not a generic event
// bus, audio engine, mixer, ResourceManager, locomotion state machine,
// Trigger/Receiver framework, or gameplay authority.
// Application plays through platform::GameplayAudio.

#include <cstdint>
#include <span>
#include <vector>

namespace gameplay
{
inline constexpr float kFootstepStrideDistance = 2.0f;
inline constexpr float kFootstepMinHorizontalSpeed = 1.0f;
inline constexpr float kLandingMinAirborneSeconds = 0.12f;

struct GameplaySfxEmit
{
    bool pickup = false;
    bool damage = false;
    bool death = false;
    bool respawn = false;
    bool footstep = false;
    bool jump = false;
    bool landing = false;
    bool checkpointActivate = false;
    int plateActivateCount = 0;
    int plateDeactivateCount = 0;
    bool doorUnlock = false;
    bool levelGoalComplete = false;
};

struct GameplaySfxRequestState
{
    int pickupCount = 0;
    int damageCount = 0;
    int deathCount = 0;
    int respawnCount = 0;
    int footstepCount = 0;
    int jumpCount = 0;
    int landingCount = 0;
    int checkpointActivateCount = 0;
    int plateActivateCount = 0;
    int plateDeactivateCount = 0;
    int doorUnlockCount = 0;
    int levelGoalCompleteCount = 0;
};

// Runtime presentation only. Remembers cadence and grounded observation
// across frames; never mutates authored Level data.
struct PlayerMovementSfxState
{
    bool anchored = false;
    bool previousGrounded = false;
    float strideDistanceAccumulated = 0.0f;
};

struct PlayerMovementSfxInput
{
    bool allowed = false;
    bool grounded = false;
    bool becameGrounded = false;
    bool jumpAccepted = false;
    float airborneSecondsAtStart = 0.0f;
    float horizontalVelocity = 0.0f;
    float deltaSeconds = 0.0f;
};

// Runtime presentation only. Remembers last observed Pressure Plate active
// flags so edges can be distinguished from reconstruction. Never mutates
// authored Plate specs.
struct PressurePlateSfxState
{
    std::vector<std::uint8_t> previousActive;
};

enum class GameplayRespawnAudioKind
{
    Fall,
    Manual,
    HealthDeath,
};

inline void ClearGameplaySfxRequests(GameplaySfxRequestState& state)
{
    state = GameplaySfxRequestState{};
}

inline void RecordGameplaySfx(GameplaySfxRequestState& state, GameplaySfxEmit emit)
{
    if (emit.pickup)
    {
        ++state.pickupCount;
    }
    if (emit.damage)
    {
        ++state.damageCount;
    }
    if (emit.death)
    {
        ++state.deathCount;
    }
    if (emit.respawn)
    {
        ++state.respawnCount;
    }
    if (emit.footstep)
    {
        ++state.footstepCount;
    }
    if (emit.jump)
    {
        ++state.jumpCount;
    }
    if (emit.landing)
    {
        ++state.landingCount;
    }
    if (emit.checkpointActivate)
    {
        ++state.checkpointActivateCount;
    }
    state.plateActivateCount += emit.plateActivateCount;
    state.plateDeactivateCount += emit.plateDeactivateCount;
    if (emit.doorUnlock)
    {
        ++state.doorUnlockCount;
    }
    if (emit.levelGoalComplete)
    {
        ++state.levelGoalCompleteCount;
    }
}

inline void ReanchorPlayerMovementSfx(PlayerMovementSfxState& state, bool grounded)
{
    state.anchored = true;
    state.previousGrounded = grounded;
    state.strideDistanceAccumulated = 0.0f;
}

// Observes authoritative movement results. Caps at one footstep per tick so a
// hitch or leftover distance cannot burst. Blocked/unanchored ticks re-anchor
// without emitting and without accumulating cadence.
inline GameplaySfxEmit TickPlayerMovementSfx(
    PlayerMovementSfxState& state,
    const PlayerMovementSfxInput& input)
{
    GameplaySfxEmit emit{};
    if (!input.allowed || !state.anchored)
    {
        ReanchorPlayerMovementSfx(state, input.grounded);
        return emit;
    }

    if (input.jumpAccepted)
    {
        emit.jump = true;
    }

    if (input.becameGrounded
        && input.airborneSecondsAtStart >= kLandingMinAirborneSeconds)
    {
        emit.landing = true;
    }

    const float absSpeed =
        input.horizontalVelocity < 0.0f ? -input.horizontalVelocity : input.horizontalVelocity;
    const bool groundedMoving = input.grounded && !input.becameGrounded && !input.jumpAccepted
        && absSpeed >= kFootstepMinHorizontalSpeed && input.deltaSeconds > 0.0f;
    if (groundedMoving)
    {
        state.strideDistanceAccumulated += absSpeed * input.deltaSeconds;
        if (state.strideDistanceAccumulated >= kFootstepStrideDistance)
        {
            emit.footstep = true;
            state.strideDistanceAccumulated -= kFootstepStrideDistance;
        }
    }
    else
    {
        state.strideDistanceAccumulated = 0.0f;
    }

    state.previousGrounded = input.grounded;
    return emit;
}

inline GameplaySfxEmit PickupCollectionSfx()
{
    GameplaySfxEmit emit{};
    emit.pickup = true;
    return emit;
}

inline GameplaySfxEmit CheckpointActivatedSfx()
{
    GameplaySfxEmit emit{};
    emit.checkpointActivate = true;
    return emit;
}

inline GameplaySfxEmit DoorUnlockSfx()
{
    GameplaySfxEmit emit{};
    emit.doorUnlock = true;
    return emit;
}

inline GameplaySfxEmit LevelGoalCompleteSfx()
{
    GameplaySfxEmit emit{};
    emit.levelGoalComplete = true;
    return emit;
}

inline void SynchronizePressurePlateSfx(
    PressurePlateSfxState& state,
    std::span<const std::uint8_t> active)
{
    state.previousActive.assign(active.begin(), active.end());
}

// Observes existing runtime Active flags. Size mismatch is treated as
// reconstruction and synchronized silently. Held-active / held-inactive
// frames emit nothing.
inline GameplaySfxEmit TickPressurePlateSfx(
    PressurePlateSfxState& state,
    std::span<const std::uint8_t> active)
{
    GameplaySfxEmit emit{};
    if (state.previousActive.size() != active.size())
    {
        SynchronizePressurePlateSfx(state, active);
        return emit;
    }

    for (std::size_t index = 0; index < active.size(); ++index)
    {
        const bool wasActive = state.previousActive[index] != 0;
        const bool nowActive = active[index] != 0;
        if (!wasActive && nowActive)
        {
            ++emit.plateActivateCount;
        }
        else if (wasActive && !nowActive)
        {
            ++emit.plateDeactivateCount;
        }
    }
    SynchronizePressurePlateSfx(state, active);
    return emit;
}

// Lethal-hit rule B: the death cue takes precedence. A successful
// Health-zero transition suppresses the ordinary damage cue so the two
// one-shots do not stack on the same simulation step. Non-lethal
// successful damage still emits damage only.
inline GameplaySfxEmit ResolveHazardOutcomeSfx(bool appliedDamage, bool beganDeath)
{
    GameplaySfxEmit emit{};
    if (beganDeath)
    {
        emit.death = true;
        return emit;
    }
    emit.damage = appliedDamage;
    return emit;
}

// Health-zero death respawn is the only respawn kind that emits the
// respawn cue. Ordinary Fall and Manual R stay silent.
inline GameplaySfxEmit ResolveRespawnSfx(GameplayRespawnAudioKind kind)
{
    GameplaySfxEmit emit{};
    if (kind == GameplayRespawnAudioKind::HealthDeath)
    {
        emit.respawn = true;
    }
    return emit;
}
}
