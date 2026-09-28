#pragma once

#include <algorithm>
#include <cmath>

namespace gameplay
{
// Matches the existing legacy Player maximum. Authored stats have no upper cap.
inline constexpr float kFallbackMaxHealth = 100.0f;
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

class RuntimeHealth
{
public:
    explicit RuntimeHealth(float maximum = kFallbackMaxHealth)
        : maxHealth(ResolveRuntimeMaxHealth(maximum)), currentHealth(maxHealth) {}
    float Maximum() const { return maxHealth; }
    float Current() const { return currentHealth; }
    bool Depleted() const { return currentHealth <= 0.0f; }
    void SetMaximum(float maximum)
    {
        maxHealth = ResolveRuntimeMaxHealth(maximum);
        currentHealth = std::min(currentHealth, maxHealth);
    }
    void Reset() { currentHealth = maxHealth; }
    HealthOperationResult ApplyDamage(DirectDamage damage)
    {
        HealthOperationResult result{false, currentHealth, currentHealth, 0.0f};
        if (!std::isfinite(damage.amount) || damage.amount <= 0.0f) return result;
        result.accepted = true;
        result.applied = std::min(damage.amount, currentHealth);
        currentHealth -= result.applied;
        result.after = currentHealth;
        result.applied = result.before - result.after;
        return result;
    }
    HealthOperationResult ApplyHealing(DirectHealing healing)
    {
        HealthOperationResult result{false, currentHealth, currentHealth, 0.0f};
        if (!std::isfinite(healing.amount) || healing.amount <= 0.0f) return result;
        result.accepted = true;
        // Widen the addition so even two largest finite float amounts cannot overflow.
        currentHealth = static_cast<float>(std::min(static_cast<double>(maxHealth),
            static_cast<double>(currentHealth) + static_cast<double>(healing.amount)));
        result.after = currentHealth;
        result.applied = result.after - result.before;
        return result;
    }
private:
    float maxHealth;
    float currentHealth;
};
}
