#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace gameplay
{
// Matches the existing legacy Player maximum. Authored stats have no upper cap.
inline constexpr float kFallbackMaxHealth = 100.0f;
inline constexpr float kDamageFeedbackSeconds = 0.25f;
inline constexpr float kHitReactionMaxSeconds = 0.5f;
inline float ResolveRuntimeMaxHealth(float value)
{
    return std::isfinite(value) && value > 0.0f ? value : kFallbackMaxHealth;
}
struct DirectDamage { float amount = 0.0f; };
struct DirectHealing { float amount = 0.0f; };
struct HealthOperationResult
{
    bool accepted = false;
    float before = 0.0f;
    float after = 0.0f;
    float applied = 0.0f;
};

enum class RuntimeLifeState { Alive, Defeated };

class RuntimeHealth
{
public:
    explicit RuntimeHealth(float maximum = kFallbackMaxHealth)
        : maxHealth(ResolveRuntimeMaxHealth(maximum)), currentHealth(maxHealth) {}
    float Maximum() const { return maxHealth; }
    float Current() const { return currentHealth; }
    RuntimeLifeState LifeState() const { return lifeState; }
    bool Defeated() const { return lifeState == RuntimeLifeState::Defeated; }
    bool Depleted() const { return currentHealth <= 0.0f; }
    bool DamageFeedbackActive() const { return damageFeedbackRemaining > 0.0f; }
    float DamageFeedbackRemaining() const { return damageFeedbackRemaining; }
    void AdvanceDamageFeedback(float deltaSeconds)
    {
        if (std::isfinite(deltaSeconds) && deltaSeconds > 0.0f)
            damageFeedbackRemaining = std::max(0.0f, damageFeedbackRemaining - deltaSeconds);
    }
    // Optional animation availability is configured by the existing presentation owner.
    void SetHitReactionDuration(float seconds)
    {
        hitReactionDuration = std::isfinite(seconds) ? std::clamp(seconds, 0.0f, kHitReactionMaxSeconds) : 0.0f;
        hitReactionRemaining = std::min(hitReactionRemaining, hitReactionDuration);
    }
    bool HitReactionAvailable() const { return hitReactionDuration > 0.0f; }
    bool HitReactionActive() const { return hitReactionRemaining > 0.0f && !Defeated(); }
    float HitReactionTime() const { return hitReactionDuration - hitReactionRemaining; }
    float HitReactionRemaining() const { return hitReactionRemaining; }
    void AdvanceHitReaction(float seconds)
    {
        if (std::isfinite(seconds) && seconds > 0.0f)
            hitReactionRemaining = std::max(0.0f, hitReactionRemaining - seconds);
    }
    // The presentation owner supplies a resolved clip duration; no authored combat timer.
    void SetAttackDuration(float seconds)
    {
        attackDuration = std::isfinite(seconds) && seconds > 0.0f ? seconds : 0.0f;
        attackRemaining = std::min(attackRemaining, attackDuration);
        if (!AttackActive()) contactedTargets.clear();
    }
    void ClearAttack() { attackRemaining = 0.0f; contactedTargets.clear(); }
    bool RequestAttack()
    {
        if (Defeated() || HitReactionActive() || AttackActive() || attackDuration <= 0.0f) return false;
        attackRemaining = attackDuration;
        contactedTargets.clear();
        return true;
    }
    bool AttackActive() const { return attackRemaining > 0.0f && !Defeated(); }
    float AttackRemaining() const { return attackRemaining; }
    float AttackTime() const { return attackDuration - attackRemaining; }
    float AttackDuration() const { return attackDuration; }
    std::size_t AttackContactCount() const { return contactedTargets.size(); }
    std::uint64_t LastAttackTarget() const { return contactedTargets.empty() ? 0 : contactedTargets.back(); }
    bool RecordAttackContact(std::uint64_t target)
    {
        if (!AttackActive() || target == 0
            || std::find(contactedTargets.begin(), contactedTargets.end(), target) != contactedTargets.end()) return false;
        contactedTargets.push_back(target);
        return true;
    }
    void AdvanceAttack(float seconds)
    {
        if (std::isfinite(seconds) && seconds > 0.0f)
            attackRemaining = std::max(0.0f, attackRemaining - seconds);
        if (!AttackActive()) contactedTargets.clear();
    }
    void SetMaximum(float maximum)
    {
        maxHealth = ResolveRuntimeMaxHealth(maximum);
        currentHealth = std::min(currentHealth, maxHealth);
    }
    void Reset() { currentHealth = maxHealth; lifeState = RuntimeLifeState::Alive; damageFeedbackRemaining = 0.0f; hitReactionRemaining = 0.0f; ClearAttack(); }
    HealthOperationResult ApplyDamage(DirectDamage damage)
    {
        HealthOperationResult result{false, currentHealth, currentHealth, 0.0f};
        if (!std::isfinite(damage.amount) || damage.amount <= 0.0f) return result;
        result.accepted = true;
        result.applied = std::min(damage.amount, currentHealth);
        currentHealth -= result.applied;
        if (result.before > 0.0f && currentHealth == 0.0f)
            lifeState = RuntimeLifeState::Defeated;
        result.after = currentHealth;
        result.applied = result.before - result.after;
        if (result.applied > 0.0f)
        {
            damageFeedbackRemaining = kDamageFeedbackSeconds;
            hitReactionRemaining = Defeated() ? 0.0f : hitReactionDuration;
            if (Defeated() || HitReactionActive()) ClearAttack();
        }
        return result;
    }
    HealthOperationResult ApplyHealing(DirectHealing healing)
    {
        HealthOperationResult result{false, currentHealth, currentHealth, 0.0f};
        if (Defeated() || !std::isfinite(healing.amount) || healing.amount <= 0.0f) return result;
        result.accepted = true;
        // Widen the addition so even two largest finite float amounts cannot overflow.
        currentHealth = static_cast<float>(std::min(static_cast<double>(maxHealth),
            static_cast<double>(currentHealth) + static_cast<double>(healing.amount)));
        result.after = currentHealth;
        result.applied = result.after - result.before;
        return result;
    }
private:
    RuntimeLifeState lifeState = RuntimeLifeState::Alive;
    float damageFeedbackRemaining = 0.0f;
    float attackDuration = 0.0f;
    float attackRemaining = 0.0f;
    float hitReactionDuration = 0.0f;
    float hitReactionRemaining = 0.0f;
    float maxHealth;
    float currentHealth;
    std::vector<std::uint64_t> contactedTargets;
};
}
