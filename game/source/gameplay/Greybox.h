#pragma once

#include "core/Vec3.h"

namespace gameplay
{
struct GreyboxBox
{
    core::Vec3 center;
    core::Vec3 size;
};

// Shared main ground. Gameplay contact and rendering must use this same surface.
inline constexpr GreyboxBox kGround{{0.0f, -0.25f, 0.0f}, {24.0f, 0.5f, 8.0f}};

constexpr float GroundSurfaceY()
{
    return kGround.center.y + kGround.size.y * 0.5f;
}
}
