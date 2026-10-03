#pragma once

#include "gameplay/CharacterDefinition.h"
#include "gameplay/RuntimeHealth.h"
#include "core/Vec3.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace gameplay
{
// Session identity 1 is the Player; placed CharacterInstance handles are offset by one.
inline constexpr std::uint64_t kPlayerContactIdentity = 1;
inline std::uint64_t PlacedContactIdentity(std::uint64_t handle) { return handle + 1; }

struct MeleeContact
{
    std::uint64_t attacker = 0;
    std::uint64_t target = 0;
};

struct MeleeWorldTransform
{
    core::Vec3 position{};
    core::Vec3 rotationDegrees{};
    core::Vec3 scale{1.0f, 1.0f, 1.0f};
};

inline core::Vec3 RotateMelee(core::Vec3 point, core::Vec3 degrees)
{
    constexpr float kRadians = 0.01745329251994329577f;
    const float x = degrees.x * kRadians, y = degrees.y * kRadians, z = degrees.z * kRadians;
    const float cx = std::cos(x), sx = std::sin(x);
    const float cy = std::cos(y), sy = std::sin(y);
    const float cz = std::cos(z), sz = std::sin(z);
    const core::Vec3 rx{point.x, cx * point.y - sx * point.z, sx * point.y + cx * point.z};
    const core::Vec3 ry{cy * rx.x + sy * rx.z, rx.y, -sy * rx.x + cy * rx.z};
    return {cz * ry.x - sz * ry.y, sz * ry.x + cz * ry.y, ry.z};
}

inline bool MeleeWindowActive(const CharacterDefinition::MeleeHitDefinition& hit,
    const RuntimeHealth& health)
{
    if (!hit.enabled || !health.AttackActive() || health.HitReactionActive() || health.Defeated()
        || health.AttackDuration() <= 0.0f) return false;
    const float progress = health.AttackTime() / health.AttackDuration();
    return progress >= hit.windowStart && progress < hit.windowEnd;
}

inline bool MeleeWindowIntersectsAdvance(const CharacterDefinition::MeleeHitDefinition& hit,
    const RuntimeHealth& health, float seconds)
{
    if (!hit.enabled || !health.AttackActive() || health.HitReactionActive() || health.Defeated()
        || health.AttackDuration() <= 0.0f || !std::isfinite(seconds) || seconds <= 0.0f) return false;
    const float start = health.AttackTime() / health.AttackDuration();
    const float end = (health.AttackTime() + seconds) / health.AttackDuration();
    return start < hit.windowEnd && end >= hit.windowStart;
}

// Query a world-space target sphere against the attacker's oriented local box.
inline bool MeleeSphereOverlaps(const CharacterDefinition::MeleeHitDefinition& hit,
    const MeleeWorldTransform& attacker, core::Vec3 targetCenter, float targetRadius)
{
    if (!hit.enabled || !std::isfinite(targetRadius) || targetRadius < 0.0f) return false;
    const auto xAxis = RotateMelee({1, 0, 0}, attacker.rotationDegrees);
    const auto yAxis = RotateMelee({0, 1, 0}, attacker.rotationDegrees);
    const auto zAxis = RotateMelee({0, 0, 1}, attacker.rotationDegrees);
    const core::Vec3 offset{hit.center.x * attacker.scale.x, hit.center.y * attacker.scale.y,
        hit.center.z * attacker.scale.z};
    const core::Vec3 center = attacker.position + RotateMelee(offset, attacker.rotationDegrees);
    const core::Vec3 delta{targetCenter.x - center.x, targetCenter.y - center.y, targetCenter.z - center.z};
    const core::Vec3 half{hit.halfExtents.x * std::abs(attacker.scale.x),
        hit.halfExtents.y * std::abs(attacker.scale.y), hit.halfExtents.z * std::abs(attacker.scale.z)};
    const auto excess = [](float projection, float extent) { return std::max(0.0f, std::abs(projection) - extent); };
    const float dx = excess(delta.x * xAxis.x + delta.y * xAxis.y + delta.z * xAxis.z, half.x);
    const float dy = excess(delta.x * yAxis.x + delta.y * yAxis.y + delta.z * yAxis.z, half.y);
    const float dz = excess(delta.x * zAxis.x + delta.y * zAxis.y + delta.z * zAxis.z, half.z);
    return dx * dx + dy * dy + dz * dz <= targetRadius * targetRadius;
}

inline bool TryMeleeContact(const CharacterDefinition::MeleeHitDefinition& hit,
    RuntimeHealth& health, std::uint64_t attackerIdentity, const MeleeWorldTransform& transform,
    std::uint64_t targetIdentity, core::Vec3 targetCenter, float targetRadius,
    bool windowIntersectsAdvance = false)
{
    return attackerIdentity != targetIdentity
        && (MeleeWindowActive(hit, health) || windowIntersectsAdvance)
        && MeleeSphereOverlaps(hit, transform, targetCenter, targetRadius)
        && health.RecordAttackContact(targetIdentity);
}
}
