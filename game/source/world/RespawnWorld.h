#pragma once

// Checkpoint types, visual states, and AABB helpers. Authored checkpoint
// instances live in the external level file / LevelDefinition. Player visual
// size is character configuration, not level authoring.

#include "core/Vec3.h"

namespace world
{
inline constexpr core::Vec3 kPlayerVisualSize{0.8f, 1.6f, 0.8f};

inline constexpr int kLevel01CheckpointCount = 2;
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

constexpr bool IsValidCheckpointIndex(int index, int checkpointCount)
{
    return index >= 0 && index < checkpointCount;
}

constexpr int NextExpectedCheckpointIndex(int activeCheckpointIndex)
{
    return activeCheckpointIndex + 1;
}

// If Apply/Reload shrinks the authored list, an old active index must not be
// kept. Count 0 or an out-of-range index returns none.
constexpr int ReconcileActiveCheckpointIndex(int activeCheckpointIndex, int checkpointCount)
{
    if (!IsValidCheckpointIndex(activeCheckpointIndex, checkpointCount))
    {
        return kNoActiveCheckpointIndex;
    }
    return activeCheckpointIndex;
}

constexpr CheckpointVisualState CheckpointVisualStateForIndex(
    int checkpointIndex,
    int activeCheckpointIndex,
    int checkpointCount)
{
    if (!IsValidCheckpointIndex(checkpointIndex, checkpointCount) || activeCheckpointIndex < 0)
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

// Shared by active runtime markers and the Development pending ghost so both
// follow the same post/beacon assembly (trigger XZ + respawn Y).
inline constexpr float kCheckpointMarkerPostWidth = 0.18f;
inline constexpr float kCheckpointMarkerPostHeight = 1.6f;
inline constexpr float kCheckpointMarkerBeaconSize = 0.36f;
inline constexpr float kCheckpointMarkerZOffset = -0.95f;

struct CheckpointMarkerLayout
{
    core::Vec3 postCenter{};
    core::Vec3 postSize{};
    core::Vec3 beaconCenter{};
    core::Vec3 beaconSize{};
};

inline CheckpointMarkerLayout MakeCheckpointMarkerLayout(const CheckpointSpec& spec)
{
    CheckpointMarkerLayout layout{};
    const float supportTopY = spec.respawnPosition.y - kPlayerVisualSize.y * 0.5f;
    layout.postCenter = {
        spec.center.x,
        supportTopY + kCheckpointMarkerPostHeight * 0.5f,
        spec.center.z + kCheckpointMarkerZOffset};
    layout.postSize = {
        kCheckpointMarkerPostWidth, kCheckpointMarkerPostHeight, kCheckpointMarkerPostWidth};
    layout.beaconSize = {
        kCheckpointMarkerBeaconSize, kCheckpointMarkerBeaconSize, kCheckpointMarkerBeaconSize};
    layout.beaconCenter = {
        layout.postCenter.x,
        layout.postCenter.y + kCheckpointMarkerPostHeight * 0.5f
            + kCheckpointMarkerBeaconSize * 0.5f,
        layout.postCenter.z};
    return layout;
}

static_assert(kNoActiveCheckpointIndex + 1 == 0);
static_assert(NextExpectedCheckpointIndex(kNoActiveCheckpointIndex) == 0);
static_assert(NextExpectedCheckpointIndex(0) == 1);
static_assert(NextExpectedCheckpointIndex(1) == 2);
static_assert(!IsValidCheckpointIndex(NextExpectedCheckpointIndex(1), kLevel01CheckpointCount));
static_assert(ReconcileActiveCheckpointIndex(1, 1) == kNoActiveCheckpointIndex);
static_assert(ReconcileActiveCheckpointIndex(0, 1) == 0);
}
