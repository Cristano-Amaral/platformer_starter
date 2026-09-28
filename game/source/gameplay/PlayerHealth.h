#pragma once

// M114: Player uses RuntimeHealth, also consumed by legacy M69/70 Hazard damage.
// Legacy Hazard contact damage,
// and the Damage Vignette trigger. Runtime-only. Not authored, not
// serialized, and not a Stats/Attribute/Combat system. Player death phase
// lives in PlayerDeath.h.

#include "gameplay/GameFlowState.h"
#include "gameplay/RuntimeHealth.h"

#include <cstddef>
#include <cstdio>

namespace gameplay
{
// Legacy fixture/default constant; runtime Player maximum comes from effective stats.
inline constexpr int kMaxPlayerHealth = static_cast<int>(kFallbackMaxHealth);
inline constexpr int kHazardDamageAmount = 25;
inline constexpr float kHazardDamageCadenceSeconds = 1.0f;
inline constexpr float kDamageVignetteDurationSeconds = 0.35f;
inline constexpr float kDamageVignettePeakOpacity = 0.45f;
inline constexpr float kDamageVignetteEdgeFraction = 0.18f;
inline constexpr unsigned char kDamageVignetteRed = 196;
inline constexpr unsigned char kDamageVignetteGreen = 24;
inline constexpr unsigned char kDamageVignetteBlue = 32;

inline constexpr std::size_t kHealthHudTextCapacity = 32;
inline constexpr int kHealthHudMarginX = 20;
inline constexpr int kHealthHudFontSize = 18;
// OBJECTIVE ends at y=122. Keep Health in the same top-left band.
inline constexpr int kHealthHudY = 128;

using PlayerHealthState = RuntimeHealth;

// Cooldown remaining until the next Hazard tick while overlapping.
// 0 means the next overlapping simulation step applies damage immediately.
struct HazardContactState
{
    float cooldownRemaining = 0.0f;
};

struct DamageVignetteState
{
    float remainingSeconds = 0.0f;
};

inline void ResetHazardContactState(HazardContactState& contact)
{
    contact.cooldownRemaining = 0.0f;
}

// After Health-zero death respawn: clear stale cadence so the lethal
// contact cannot catch up. If the destination spawn/Checkpoint currently
// overlaps a Hazard, arm the existing cadence instead of applying the
// first-step immediate tick. Leaving still resets to immediate re-entry.
inline void ResetHazardContactAfterDeathRespawn(
    HazardContactState& contact,
    bool respawnOverlapsHazard)
{
    ResetHazardContactState(contact);
    if (respawnOverlapsHazard)
    {
        contact.cooldownRemaining = kHazardDamageCadenceSeconds;
    }
}

inline void ClearDamageVignette(DamageVignetteState& vignette)
{
    vignette.remainingSeconds = 0.0f;
}

inline void BeginDamageVignette(DamageVignetteState& vignette)
{
    vignette.remainingSeconds = kDamageVignetteDurationSeconds;
}

inline void TickDamageVignette(
    DamageVignetteState& vignette,
    float deltaSeconds,
    bool allowedToAge)
{
    if (!allowedToAge || vignette.remainingSeconds <= 0.0f)
    {
        return;
    }
    vignette.remainingSeconds -= deltaSeconds;
    if (vignette.remainingSeconds < 0.0f)
    {
        vignette.remainingSeconds = 0.0f;
    }
}

inline float DamageVignetteOpacity(const DamageVignetteState& vignette)
{
    if (vignette.remainingSeconds <= 0.0f || kDamageVignetteDurationSeconds <= 0.0f)
    {
        return 0.0f;
    }
    return (vignette.remainingSeconds / kDamageVignetteDurationSeconds)
        * kDamageVignettePeakOpacity;
}

inline void InitializePlayerHealth(PlayerHealthState& health)
{
    health.Reset();
}

inline void ResetPlayerHealthForNewRuntime(
    PlayerHealthState& health,
    HazardContactState& contact)
{
    InitializePlayerHealth(health);
    ResetHazardContactState(contact);
}

inline bool PlayerHealthInvariantsHold(const PlayerHealthState& health)
{
    return std::isfinite(health.Maximum()) && std::isfinite(health.Current())
        && health.Maximum() > 0 && health.Current() >= 0
        && health.Current() <= health.Maximum();
}

// Legacy Hazard adapter to the M114 direct contract.
inline float ApplyPlayerDamage(PlayerHealthState& health, float amount)
{
    return health.ApplyDamage({amount}).applied;
}

inline bool HazardDamageIsAllowed(
    TopLevelFlow flow,
    bool editorActive,
    bool inventoryBlocksGameplay,
    bool runCompleteActive,
    bool levelCompleted,
    bool pauseBlocksGameplay,
    bool deathBlocksGameplay = false)
{
    return flow == TopLevelFlow::Gameplay && !editorActive && !inventoryBlocksGameplay
        && !runCompleteActive && !levelCompleted && !pauseBlocksGameplay
        && !deathBlocksGameplay;
}

inline bool HealthHudIsVisible(
    TopLevelFlow flow,
    bool editorActive,
    bool inventoryOpen,
    bool runCompleteActive,
    bool levelCompleted,
    bool pauseActive = false,
    bool deathActive = false)
{
    return flow == TopLevelFlow::Gameplay && !editorActive && !inventoryOpen
        && !runCompleteActive && !levelCompleted && !pauseActive && !deathActive;
}

inline bool DamageVignetteIsVisible(
    TopLevelFlow flow,
    bool editorActive,
    bool inventoryOpen,
    bool runCompleteActive,
    bool levelCompleted,
    bool pauseActive,
    bool deathActive)
{
    (void)deathActive;
    return flow == TopLevelFlow::Gameplay && !editorActive && !inventoryOpen
        && !runCompleteActive && !levelCompleted && !pauseActive;
}

inline void FormatHealthHudText(
    char* buffer,
    std::size_t bufferSize,
    const PlayerHealthState& health)
{
    if (buffer == nullptr || bufferSize == 0)
    {
        return;
    }
    std::snprintf(
        buffer,
        bufferSize,
        "HEALTH %.6g / %.6g",
        health.Current(),
        health.Maximum());
}

// One simulation step of authored Hazard overlap damage.
//
// Rule:
// - Damage applies on the first overlapping simulation step of a contact.
// - While remaining overlapped, the next tick waits kHazardDamageCadenceSeconds
//   of simulation time after the previous application.
// - Leaving resets cooldown so re-entry damages immediately.
// - allowed == false freezes cooldown and applies no damage (no catch-up).
// - At zero Health, additional overlap does not underflow and does not tick
//   cadence bookkeeping.
//
// Returns true if this step applied damage.
inline bool TickHazardContactDamage(
    PlayerHealthState& health,
    HazardContactState& contact,
    bool overlapping,
    float deltaSeconds,
    bool allowed)
{
    if (!allowed)
    {
        return false;
    }
    if (!overlapping)
    {
        ResetHazardContactState(contact);
        return false;
    }
    if (health.Current() <= 0)
    {
        return false;
    }
    if (contact.cooldownRemaining <= 0.0f)
    {
        const float applied = ApplyPlayerDamage(health, kHazardDamageAmount);
        contact.cooldownRemaining = kHazardDamageCadenceSeconds;
        return applied > 0;
    }
    contact.cooldownRemaining -= deltaSeconds;
    if (contact.cooldownRemaining < 0.0f)
    {
        contact.cooldownRemaining = 0.0f;
    }
    return false;
}
}
