#pragma once

#include "core/Vec3.h"
#include "world/GreyboxWorld.h"

namespace world
{
// Single test kinematic platform. PhysicsWorld and Renderer both derive from
// this spec so visual size/path and Jolt collision stay in sync.
//
// Reachability with jump speed 8 and gravity 20 (peak height 1.6 from rest):
// top Y = 1.5 is reachable from the ground and easy from the right static
// platform (top Y = 1.0). Bottom Y = 1.1 stays above the right static
// platform (top Y = 1.0) and the ground (top Y = 0). The left static
// platform underside is Y = 2.0, so the path does not intersect it.
// Center X travels [-6, 6]; with width 4 the solid occupies [-8, 8],
// inside the ground extents [-28, 28] and clear of the cyan test-box spawn.
struct MovingPlatformSpec
{
    core::Vec3 size;
    float centerY = 0.0f;
    float centerZ = 0.0f;
    float pathMinX = 0.0f;
    float pathMaxX = 0.0f;
    float speed = 0.0f;
    float startX = 0.0f;
};

inline constexpr MovingPlatformSpec kTestMovingPlatform{
    {4.0f, 0.4f, 3.0f},
    1.3f,
    0.0f,
    -6.0f,
    6.0f,
    2.5f,
    0.0f};

// Axis-aligned occupancy of the solid over the full path. Checkpoint respawn
// player volumes must not intersect this box.
inline constexpr Box kMovingPlatformSweptAabb{
    {
        (kTestMovingPlatform.pathMinX + kTestMovingPlatform.pathMaxX) * 0.5f,
        kTestMovingPlatform.centerY,
        kTestMovingPlatform.centerZ},
    {
        (kTestMovingPlatform.pathMaxX - kTestMovingPlatform.pathMinX) + kTestMovingPlatform.size.x,
        kTestMovingPlatform.size.y,
        kTestMovingPlatform.size.z}};

static_assert(kMovingPlatformSweptAabb.center.x == 0.0f);
static_assert(kMovingPlatformSweptAabb.center.y == 1.3f);
static_assert(kMovingPlatformSweptAabb.size.x == 16.0f);
static_assert(kMovingPlatformSweptAabb.size.y == 0.4f);
static_assert(kMovingPlatformSweptAabb.size.z == 3.0f);
}
