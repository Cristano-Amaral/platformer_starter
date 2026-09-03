#pragma once

// Project-owned axis-aligned box. Authored Level 01 instances live in the
// external level file / LevelDefinition, not here.

#include "core/Vec3.h"

namespace world
{
struct Box
{
    core::Vec3 center;
    core::Vec3 size;
};

constexpr float TopY(const Box& box)
{
    return box.center.y + box.size.y * 0.5f;
}

constexpr float BottomY(const Box& box)
{
    return box.center.y - box.size.y * 0.5f;
}
}
