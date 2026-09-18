#pragma once

// Milestone 71: semantic gameplay SFX request recording and the lethal-hit
// cue precedence rule. Presentation only. Not a generic event bus, audio
// engine, mixer, or ResourceManager. Application plays through
// platform::GameplayAudio.

namespace gameplay
{
struct GameplaySfxEmit
{
    bool pickup = false;
    bool damage = false;
    bool death = false;
    bool respawn = false;
};

struct GameplaySfxRequestState
{
    int pickupCount = 0;
    int damageCount = 0;
    int deathCount = 0;
    int respawnCount = 0;
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
}

inline GameplaySfxEmit PickupCollectionSfx()
{
    GameplaySfxEmit emit{};
    emit.pickup = true;
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
