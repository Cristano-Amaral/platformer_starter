#pragma once

// Project-owned spawn, kill-plane, and exactly two ordered checkpoints.
// Renderer and gameplay consume these values; PhysicsWorld does not interpret
// checkpoint meaning. No Jolt sensor, registry, or heap.

#include "core/Vec3.h"
#include "world/GreyboxWorld.h"
#include "world/MovingPlatform.h"
#include "world/Slope.h"

#include <array>

namespace world
{
inline constexpr core::Vec3 kPlayerVisualSize{0.8f, 1.6f, 0.8f};

inline constexpr float kKillPlaneY = -8.0f;

inline constexpr core::Vec3 kInitialSpawnVisualCenter{
    0.0f,
    GroundSurfaceY() + kPlayerVisualSize.y * 0.5f,
    0.0f};

struct CheckpointSpec
{
    core::Vec3 center;
    core::Vec3 size;
    core::Vec3 respawnPosition;
};

inline constexpr int kCheckpointCount = 2;
inline constexpr int kNoActiveCheckpointIndex = -1;

// Application-owned visual identity for Renderer. Renderer must not infer
// these from overlap, BodyID, or RespawnState.
enum class CheckpointVisualState
{
    Future,
    Current,
    PreviouslyActivated,
};

// Index 0 = Checkpoint 1, index 1 = Checkpoint 2. Ordered progression only.
inline constexpr std::array<CheckpointSpec, kCheckpointCount> kCheckpoints{{
    {
        {16.5f, 1.8f, 0.0f},
        {2.4f, 1.6f, 2.0f},
        {16.5f,
         TopY(kElevatedPlatforms[kCheckpoint1PlatformIndex]) + kPlayerVisualSize.y * 0.5f,
         0.0f}},
    {
        {-15.5f, 2.8f, 0.0f},
        {2.4f, 1.6f, 2.0f},
        {-15.5f,
         TopY(kElevatedPlatforms[kCheckpoint2PlatformIndex]) + kPlayerVisualSize.y * 0.5f,
         0.0f}},
}};

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

// Test the project-facing visual center (Player::Position), not CharacterVirtual feet.
constexpr bool PointInsideCheckpoint(const CheckpointSpec& spec, core::Vec3 visualCenter)
{
    return PointInsideAabb(spec.center, spec.size, visualCenter);
}

constexpr bool IsValidCheckpointIndex(int index)
{
    return index >= 0 && index < kCheckpointCount;
}

// expectedIndex = activeCheckpointIndex + 1. Only that volume may activate.
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

static_assert(kCheckpoints.size() == kCheckpointCount);
static_assert(kNoActiveCheckpointIndex + 1 == 0);
static_assert(NextExpectedCheckpointIndex(kNoActiveCheckpointIndex) == 0);
static_assert(NextExpectedCheckpointIndex(0) == 1);
static_assert(NextExpectedCheckpointIndex(1) == 2);
static_assert(!IsValidCheckpointIndex(NextExpectedCheckpointIndex(1)));
static_assert(CheckpointVisualStateForIndex(0, kNoActiveCheckpointIndex)
              == CheckpointVisualState::Future);
static_assert(CheckpointVisualStateForIndex(1, kNoActiveCheckpointIndex)
              == CheckpointVisualState::Future);
static_assert(
    CheckpointVisualStateForIndex(0, 0) == CheckpointVisualState::Current);
static_assert(
    CheckpointVisualStateForIndex(1, 0) == CheckpointVisualState::Future);
static_assert(
    CheckpointVisualStateForIndex(0, 1)
    == CheckpointVisualState::PreviouslyActivated);
static_assert(
    CheckpointVisualStateForIndex(1, 1) == CheckpointVisualState::Current);

static_assert(kCheckpoints[0].respawnPosition.x == 16.5f);
static_assert(kCheckpoints[0].respawnPosition.y == 1.8f);
static_assert(kCheckpoints[1].respawnPosition.x == -15.5f);
static_assert(kCheckpoints[1].respawnPosition.y == 2.8f);

// Respawn player visual AABB must not be occupiable by the moving platform.
static_assert(!AabbOverlaps(
    kCheckpoints[0].respawnPosition,
    kPlayerVisualSize,
    kMovingPlatformSweptAabb.center,
    kMovingPlatformSweptAabb.size));
static_assert(!AabbOverlaps(
    kCheckpoints[1].respawnPosition,
    kPlayerVisualSize,
    kMovingPlatformSweptAabb.center,
    kMovingPlatformSweptAabb.size));

// Respawn standing pose rests on the support top (touching is not penetration).
static_assert(!AabbOverlaps(
    kCheckpoints[0].respawnPosition,
    kPlayerVisualSize,
    kElevatedPlatforms[kCheckpoint1PlatformIndex].center,
    kElevatedPlatforms[kCheckpoint1PlatformIndex].size));
static_assert(!AabbOverlaps(
    kCheckpoints[1].respawnPosition,
    kPlayerVisualSize,
    kElevatedPlatforms[kCheckpoint2PlatformIndex].center,
    kElevatedPlatforms[kCheckpoint2PlatformIndex].size));

// Phase B.1: CP1 respawn stays clear of the relocated slope tests (AABB bounds).
static_assert(!AabbOverlaps(
    kCheckpoints[0].respawnPosition,
    kPlayerVisualSize,
    kWalkableSlope.center,
    {kWalkableSlopeAabbHalfX * 2.0f, 3.35f, kWalkableSlope.size.z}));
static_assert(!AabbOverlaps(
    kCheckpoints[0].respawnPosition,
    kPlayerVisualSize,
    kSteepSlope.center,
    {kSteepSlopeAabbHalfX * 2.0f, 1.94f, kSteepSlope.size.z}));
}
