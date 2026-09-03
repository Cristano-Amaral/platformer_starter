#pragma once

#include "core/Vec3.h"

namespace world
{
// Static slope box in local size, rotated about +Z. Authored instances live in
// Level01.cpp. CharacterVirtual max-slope policy is not level data.
struct SlopeSpec
{
    core::Vec3 center;
    core::Vec3 size;
    float rotationZDegrees = 0.0f;
};
}
