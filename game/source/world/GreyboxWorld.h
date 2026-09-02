#pragma once

// Project-owned greybox geometry. Renderer and PhysicsWorld both derive from
// these boxes so visual solids and Jolt static collision stay in sync.

#include "core/Vec3.h"

namespace world
{
struct Box
{
    core::Vec3 center;
    core::Vec3 size;
};

inline constexpr Box kGround{{0.0f, -0.25f, 0.0f}, {56.0f, 0.5f, 8.0f}};

inline constexpr int kCheckpoint1PlatformIndex = 2;
inline constexpr int kCheckpoint2PlatformIndex = 4;
inline constexpr int kGoalPlatformIndex = 5;

inline constexpr Box kElevatedPlatforms[] = {
    {{5.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}},
    {{-4.5f, 2.25f, 0.0f}, {3.0f, 0.5f, 2.5f}},
    {{16.5f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}},
    {{-10.0f, 2.0f, 0.0f}, {3.0f, 0.5f, 2.5f}},
    {{-15.5f, 1.75f, 0.0f}, {4.0f, 0.5f, 3.0f}},
    {{-21.0f, 2.75f, 0.0f}, {3.0f, 0.5f, 2.5f}},
};

constexpr float TopY(const Box& box)
{
    return box.center.y + box.size.y * 0.5f;
}

constexpr float BottomY(const Box& box)
{
    return box.center.y - box.size.y * 0.5f;
}

constexpr float GroundSurfaceY()
{
    return TopY(kGround);
}

static_assert(TopY(kGround) == 0.0f);
static_assert(kGround.size.x == 56.0f);
static_assert(kGoalPlatformIndex + 1 == sizeof(kElevatedPlatforms) / sizeof(kElevatedPlatforms[0]));
}
