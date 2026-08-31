#pragma once

#include "core/Vec3.h"

namespace gameplay
{
// Tunable view: mostly side-on (large +Z), slightly off-axis (+X) so box sides read as 3D.
struct PlatformerCamera
{
    core::Vec3 offset{2.0f, 3.5f, 12.0f};
    float fieldOfViewY = 40.0f;
};
}
