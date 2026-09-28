#pragma once

#include <algorithm>
#include <cmath>

namespace gameplay
{
// Matches the existing legacy Player maximum. Authored stats have no upper cap.
inline constexpr float kFallbackMaxHealth = 100.0f;
inline constexpr float kDamageFeedbackSeconds = 0.25f;
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
    void SetMaximum(float maximum)
    {
        maxHealth = ResolveRuntimeMaxHealth(maximum);
        currentHealth = std::min(currentHealth, maxHealth);
    }
    void Reset() { currentHealth = maxHealth; lifeState = RuntimeLifeState::Alive; damageFeedbackRemaining = 0.0f; }
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
        if (result.applied > 0.0f) damageFeedbackRemaining = kDamageFeedbackSeconds;
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
    float maxHealth;
    float currentHealth;
};
}
