#pragma once

// Checkpoint types, visual states, and AABB helpers. Authored checkpoint
// instances live in the external level file / LevelDefinition. Player visual
// size is character configuration, not level authoring.

#include "core/Vec3.h"

#include <array>

namespace world
{
inline constexpr core::Vec3 kPlayerVisualSize{0.8f, 1.6f, 0.8f};

inline constexpr int kCheckpointCount = 2;
inline constexpr int kNoActiveCheckpointIndex = -1;

struct CheckpointSpec
{
    core::Vec3 center;
    core::Vec3 size;
    core::Vec3 respawnPosition;
};

enum class CheckpointVisualState
{
    Future,
    Current,
    PreviouslyActivated,
};

constexpr bool PointInsideAabb(core::Vec3 center, core::Vec3 size, core::Vec3 point)
{
    const float halfX = size.x * 0.5f;
    const float halfY = size.y * 0.5f;
    const float halfZ = size.z * 0.5f;
    return point.x >= center.x - halfX && point.x <= center.x + halfX
        && point.y >= center.y - halfY && point.y <= center.y + halfY
        && point.z >= center.z - halfZ && point.z <= center.z + halfZ;
}

constexpr bool AabbOverlaps(
    core::Vec3 aCenter,
    core::Vec3 aSize,
    core::Vec3 bCenter,
    core::Vec3 bSize)
{
    const auto absf = [](float value) { return value < 0.0f ? -value : value; };
    return absf(aCenter.x - bCenter.x) * 2.0f < (aSize.x + bSize.x)
        && absf(aCenter.y - bCenter.y) * 2.0f < (aSize.y + bSize.y)
        && absf(aCenter.z - bCenter.z) * 2.0f < (aSize.z + bSize.z);
}

constexpr bool PointInsideCheckpoint(const CheckpointSpec& spec, core::Vec3 visualCenter)
{
    return PointInsideAabb(spec.center, spec.size, visualCenter);
}

constexpr bool IsValidCheckpointIndex(int index)
{
    return index >= 0 && index < kCheckpointCount;
}

constexpr int NextExpectedCheckpointIndex(int activeCheckpointIndex)
{
    return activeCheckpointIndex + 1;
}

constexpr CheckpointVisualState CheckpointVisualStateForIndex(
    int checkpointIndex,
    int activeCheckpointIndex)
{
    if (!IsValidCheckpointIndex(checkpointIndex) || activeCheckpointIndex < 0)
    {
        return CheckpointVisualState::Future;
    }
    if (checkpointIndex < activeCheckpointIndex)
    {
        return CheckpointVisualState::PreviouslyActivated;
    }
    if (checkpointIndex == activeCheckpointIndex)
    {
        return CheckpointVisualState::Current;
    }
    return CheckpointVisualState::Future;
}

static_assert(kNoActiveCheckpointIndex + 1 == 0);
static_assert(NextExpectedCheckpointIndex(kNoActiveCheckpointIndex) == 0);
static_assert(NextExpectedCheckpointIndex(0) == 1);
static_assert(NextExpectedCheckpointIndex(1) == 2);
static_assert(!IsValidCheckpointIndex(NextExpectedCheckpointIndex(1)));
}
