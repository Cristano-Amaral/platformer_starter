#pragma once

// Project-owned spawn, kill-plane, and single-checkpoint layout.
// Renderer and gameplay consume these values; PhysicsWorld does not interpret
// checkpoint meaning. No Jolt sensor.

#include "core/Vec3.h"
#include "world/GreyboxWorld.h"

namespace world
{
inline constexpr core::Vec3 kPlayerVisualSize{0.8f, 1.6f, 0.8f};

inline constexpr float kKillPlaneY = -8.0f;

inline constexpr core::Vec3 kInitialSpawnVisualCenter{
    0.0f,
    GroundSurfaceY() + kPlayerVisualSize.y * 0.5f,
    0.0f};

// Right elevated platform in kElevatedPlatforms[0]: center (5, 0.75, 0),
// size (4, 0.5, 3), top Y = 1.0. Visual-center standing height is 1.8.
struct CheckpointSpec
{
    core::Vec3 center;
    core::Vec3 size;
    core::Vec3 respawnPosition;
};

inline constexpr CheckpointSpec kCheckpoint{
    {5.0f, 1.8f, 0.0f},
    {2.4f, 1.6f, 2.0f},
    {5.0f, TopY(kElevatedPlatforms[0]) + kPlayerVisualSize.y * 0.5f, 0.0f}};

constexpr bool PointInsideAabb(core::Vec3 center, core::Vec3 size, core::Vec3 point)
{
    const float halfX = size.x * 0.5f;
    const float halfY = size.y * 0.5f;
    const float halfZ = size.z * 0.5f;
    return point.x >= center.x - halfX && point.x <= center.x + halfX
        && point.y >= center.y - halfY && point.y <= center.y + halfY
        && point.z >= center.z - halfZ && point.z <= center.z + halfZ;
}

// Test the project-facing visual center (Player::Position), not CharacterVirtual feet.
constexpr bool PointInsideCheckpoint(const CheckpointSpec& spec, core::Vec3 visualCenter)
{
    return PointInsideAabb(spec.center, spec.size, visualCenter);
}
}
