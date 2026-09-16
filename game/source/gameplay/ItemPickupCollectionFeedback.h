#pragma once

// Milestone 61: Item Pickup collection feedback. Runtime-only, not authored,
// not serialized, not a generic FX/particle/event system. Spawn only after
// the existing TryCollectItemPickup path succeeds.

#include "core/Vec3.h"
#include "world/ItemPickup.h"

#include <cmath>
#include <cstddef>

namespace gameplay
{
inline constexpr float kItemPickupCollectionEffectDurationSeconds = 0.40f;
inline constexpr float kItemPickupCollectionEffectRadius = 0.32f;
inline constexpr int kItemPickupCollectionSparkCount = 8;
inline constexpr int kItemPickupCollectionEffectCapacity = 8;
inline constexpr float kItemPickupCollectionSparkSize = 0.055f;
inline constexpr float kItemPickupCollectionCoreSize = 0.090f;
inline constexpr unsigned char kItemPickupCollectionGoldRed = 255;
inline constexpr unsigned char kItemPickupCollectionGoldGreen = 220;
inline constexpr unsigned char kItemPickupCollectionGoldBlue = 72;

struct ItemPickupCollectionEffect
{
    core::Vec3 origin{};
    float ageSeconds = 0.0f;
    bool active = false;
};

struct ItemPickupCollectionFeedbackState
{
    ItemPickupCollectionEffect effects[kItemPickupCollectionEffectCapacity]{};
    int emittedCount = 0;
};

inline void ClearItemPickupCollectionFeedback(ItemPickupCollectionFeedbackState& state)
{
    state = ItemPickupCollectionFeedbackState{};
}

inline int ActiveItemPickupCollectionEffectCount(const ItemPickupCollectionFeedbackState& state)
{
    int count = 0;
    for (int index = 0; index < kItemPickupCollectionEffectCapacity; ++index)
    {
        if (state.effects[index].active)
        {
            ++count;
        }
    }
    return count;
}

inline void UpdateItemPickupCollectionFeedback(
    ItemPickupCollectionFeedbackState& state,
    float deltaSeconds)
{
    if (!(deltaSeconds > 0.0f))
    {
        return;
    }
    for (int index = 0; index < kItemPickupCollectionEffectCapacity; ++index)
    {
        ItemPickupCollectionEffect& effect = state.effects[index];
        if (!effect.active)
        {
            continue;
        }
        effect.ageSeconds += deltaSeconds;
        if (effect.ageSeconds >= kItemPickupCollectionEffectDurationSeconds)
        {
            effect.active = false;
            effect.ageSeconds = 0.0f;
            effect.origin = {};
        }
    }
}

inline bool SpawnItemPickupCollectionFeedbackAt(
    ItemPickupCollectionFeedbackState& state,
    core::Vec3 origin)
{
    int slot = 0;
    float oldestAge = -1.0f;
    for (int index = 0; index < kItemPickupCollectionEffectCapacity; ++index)
    {
        if (!state.effects[index].active)
        {
            slot = index;
            break;
        }
        if (state.effects[index].ageSeconds > oldestAge)
        {
            oldestAge = state.effects[index].ageSeconds;
            slot = index;
        }
    }
    ItemPickupCollectionEffect& effect = state.effects[slot];
    effect.origin = origin;
    effect.ageSeconds = 0.0f;
    effect.active = true;
    ++state.emittedCount;
    return true;
}

// Capture the current presented gameplay visual origin. Includes authored
// position, visualOffset, and M59 idle bob when enabled. Spin is ignored.
inline bool SpawnItemPickupCollectionFeedback(
    ItemPickupCollectionFeedbackState& state,
    const world::ItemPickupSpec& collectedPickup,
    double elapsedSeconds)
{
    return SpawnItemPickupCollectionFeedbackAt(
        state, world::ItemPickupPresentedVisualPosition(collectedPickup, elapsedSeconds));
}

inline float ItemPickupCollectionEffectNormalizedAge(const ItemPickupCollectionEffect& effect)
{
    if (!effect.active || kItemPickupCollectionEffectDurationSeconds <= 0.0f)
    {
        return 1.0f;
    }
    const float normalized = effect.ageSeconds / kItemPickupCollectionEffectDurationSeconds;
    if (normalized < 0.0f)
    {
        return 0.0f;
    }
    if (normalized > 1.0f)
    {
        return 1.0f;
    }
    return normalized;
}

inline core::Vec3 ItemPickupCollectionSparkDirection(int sparkIndex)
{
    const float angle = static_cast<float>(sparkIndex)
        * (6.28318530718f / static_cast<float>(kItemPickupCollectionSparkCount));
    const float x = std::cos(angle) * 0.75f;
    const float z = std::sin(angle) * 0.75f;
    const float y = 1.0f;
    const float length = std::sqrt(x * x + y * y + z * z);
    if (!(length > 0.0f))
    {
        return {0.0f, 1.0f, 0.0f};
    }
    return {x / length, y / length, z / length};
}

inline core::Vec3 ItemPickupCollectionSparkWorldPosition(
    const ItemPickupCollectionEffect& effect,
    int sparkIndex)
{
    const float t = ItemPickupCollectionEffectNormalizedAge(effect);
    const float travel = 1.0f - (1.0f - t) * (1.0f - t);
    const core::Vec3 direction = ItemPickupCollectionSparkDirection(sparkIndex);
    return {
        effect.origin.x + direction.x * kItemPickupCollectionEffectRadius * travel,
        effect.origin.y + direction.y * kItemPickupCollectionEffectRadius * travel,
        effect.origin.z + direction.z * kItemPickupCollectionEffectRadius * travel};
}

inline unsigned char ItemPickupCollectionEffectAlpha(const ItemPickupCollectionEffect& effect)
{
    const float fade = 1.0f - ItemPickupCollectionEffectNormalizedAge(effect);
    const long rounded = std::lround(static_cast<double>(fade) * 220.0);
    if (rounded <= 0)
    {
        return 0;
    }
    if (rounded >= 255)
    {
        return 255;
    }
    return static_cast<unsigned char>(rounded);
}

inline float ItemPickupCollectionSparkDrawRadius(const ItemPickupCollectionEffect& effect)
{
    const float t = ItemPickupCollectionEffectNormalizedAge(effect);
    return kItemPickupCollectionSparkSize * (1.0f - t * 0.45f);
}

inline float ItemPickupCollectionCoreDrawRadius(const ItemPickupCollectionEffect& effect)
{
    const float t = ItemPickupCollectionEffectNormalizedAge(effect);
    return kItemPickupCollectionCoreSize * (1.0f - t);
}
}
