#pragma once

// Milestone 69: Application-owned Player Health and Hazard contact damage.
// Runtime-only. Not authored, not serialized, not a Stats/Attribute/Combat
// system, and not player death/respawn (M70).

#include "gameplay/GameFlowState.h"

#include <cstddef>
#include <cstdio>

namespace gameplay
{
inline constexpr int kMaxPlayerHealth = 100;
inline constexpr int kHazardDamageAmount = 25;
inline constexpr float kHazardDamageCadenceSeconds = 1.0f;

inline constexpr std::size_t kHealthHudTextCapacity = 32;
inline constexpr int kHealthHudMarginX = 20;
inline constexpr int kHealthHudFontSize = 18;
// OBJECTIVE ends at y=122. Keep Health in the same top-left band.
inline constexpr int kHealthHudY = 128;

struct PlayerHealthState
{
    int currentHealth = kMaxPlayerHealth;
    int maxHealth = kMaxPlayerHealth;
};

// Cooldown remaining until the next Hazard tick while overlapping.
// 0 means the next overlapping simulation step applies damage immediately.
struct HazardContactState
{
    float cooldownRemaining = 0.0f;
};

inline void ResetHazardContactState(HazardContactState& contact)
{
    contact.cooldownRemaining = 0.0f;
}

inline void InitializePlayerHealth(PlayerHealthState& health)
{
    health.maxHealth = kMaxPlayerHealth;
    health.currentHealth = kMaxPlayerHealth;
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
    return health.maxHealth > 0 && health.currentHealth >= 0
        && health.currentHealth <= health.maxHealth;
}

// Returns actual Health lost. Non-positive amounts are ignored. Clamps at 0.
inline int ApplyPlayerDamage(PlayerHealthState& health, int amount)
{
    if (health.maxHealth <= 0)
    {
        health.maxHealth = kMaxPlayerHealth;
    }
    if (health.currentHealth < 0)
    {
        health.currentHealth = 0;
    }
    else if (health.currentHealth > health.maxHealth)
    {
        health.currentHealth = health.maxHealth;
    }
    if (amount <= 0 || health.currentHealth <= 0)
    {
        return 0;
    }
    const int applied = amount > health.currentHealth ? health.currentHealth : amount;
    health.currentHealth -= applied;
    return applied;
}

inline bool HazardDamageIsAllowed(
    TopLevelFlow flow,
    bool editorActive,
    bool inventoryBlocksGameplay,
    bool runCompleteActive,
    bool levelCompleted,
    bool pauseBlocksGameplay)
{
    return flow == TopLevelFlow::Gameplay && !editorActive && !inventoryBlocksGameplay
        && !runCompleteActive && !levelCompleted && !pauseBlocksGameplay;
}

inline bool HealthHudIsVisible(
    TopLevelFlow flow,
    bool editorActive,
    bool inventoryOpen,
    bool runCompleteActive,
    bool levelCompleted,
    bool pauseActive = false)
{
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
        "HEALTH %d / %d",
        health.currentHealth,
        health.maxHealth);
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
    if (health.currentHealth <= 0)
    {
        health.currentHealth = 0;
        return false;
    }
    if (contact.cooldownRemaining <= 0.0f)
    {
        const int applied = ApplyPlayerDamage(health, kHazardDamageAmount);
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
