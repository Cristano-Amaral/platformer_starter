#pragma once

#include "core/Vec3.h"

namespace world
{
struct SupportContact
{
    bool grounded = false;
    float positionY = 0.0f;
    float verticalVelocity = 0.0f;
};

SupportContact ResolveGroundContact(
    core::Vec3 previousPosition,
    core::Vec3 currentPosition,
    core::Vec3 size,
    float verticalVelocity);
}
