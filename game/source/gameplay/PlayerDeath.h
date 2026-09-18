#pragma once

// Milestone 70: Application-owned player death phase and death-respawn
// Health restoration. Runtime-only. Not a GameState, scene stack,
// SceneManager, Game Over, lives, or Continue/retry menu.

#include "gameplay/PlayerHealth.h"
#include "gameplay/RespawnState.h"
#include "world/RespawnWorld.h"

namespace gameplay
{
inline constexpr float kPlayerDeathDelaySeconds = 1.25f;
inline constexpr float kPlayerDeathOverlayOpacity = 0.40f;
inline constexpr const char* kPlayerDeathTitle = "YOU DIED";

enum class PlayerDeathPhase
{
    None,
    Dying,
};

struct PlayerDeathState
{
    PlayerDeathPhase phase = PlayerDeathPhase::None;
    float remainingSeconds = 0.0f;
};

inline void ClearPlayerDeath(PlayerDeathState& death)
{
    death = PlayerDeathState{};
}

inline bool PlayerDeathIsActive(const PlayerDeathState& death)
{
    return death.phase == PlayerDeathPhase::Dying;
}

// True for the whole frame that death is or was active, so the respawn
// frame cannot leak movement / E / Pause / Inventory into gameplay.
inline bool PlayerDeathBlocksGameplay(bool wasActive, bool isActive)
{
    return wasActive || isActive;
}

inline bool TryBeginPlayerDeath(PlayerDeathState& death, int healthBefore, int healthAfter)
{
    if (PlayerDeathIsActive(death))
    {
        return false;
    }
    if (healthBefore <= 0 || healthAfter != 0)
    {
        return false;
    }
    death.phase = PlayerDeathPhase::Dying;
    death.remainingSeconds = kPlayerDeathDelaySeconds;
    return true;
}

inline void TickPlayerDeathDelay(PlayerDeathState& death, float deltaSeconds)
{
    if (!PlayerDeathIsActive(death))
    {
        return;
    }
    death.remainingSeconds -= deltaSeconds;
    if (death.remainingSeconds < 0.0f)
    {
        death.remainingSeconds = 0.0f;
    }
}

inline bool PlayerDeathDelayElapsed(const PlayerDeathState& death)
{
    return PlayerDeathIsActive(death) && death.remainingSeconds <= 0.0f;
}

inline void RestorePlayerHealthAfterDeathRespawn(PlayerHealthState& health)
{
    InitializePlayerHealth(health);
}

// Death uses the existing RespawnState destination: active Checkpoint
// respawnPosition if one is active, otherwise the current Level spawn
// already stored in respawnPosition.
inline core::Vec3 ResolveDeathRespawnPosition(const RespawnState& respawn)
{
    return respawn.respawnPosition;
}

inline bool DeathUsesActiveCheckpoint(const RespawnState& respawn)
{
    return respawn.activeCheckpointIndex != world::kNoActiveCheckpointIndex;
}
}
