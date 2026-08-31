#pragma once

#include "core/Vec3.h"

namespace world
{
struct Box
{
    core::Vec3 center;
    core::Vec3 size;
};

inline constexpr Box kGround{{0.0f, -0.25f, 0.0f}, {24.0f, 0.5f, 8.0f}};

inline constexpr Box kElevatedPlatforms[] = {
    {{5.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}},
    {{-4.5f, 1.5f, 0.0f}, {3.0f, 0.5f, 2.5f}},
};

constexpr float TopY(const Box& box)
{
    return box.center.y + box.size.y * 0.5f;
}

constexpr float GroundSurfaceY()
{
    return TopY(kGround);
}
}
