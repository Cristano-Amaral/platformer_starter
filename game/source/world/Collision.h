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

struct CeilingContact
{
    float positionY = 0.0f;
    float verticalVelocity = 0.0f;
};

float ResolveHorizontalPosition(
    core::Vec3 previousPosition,
    core::Vec3 proposedPosition,
    core::Vec3 size);

SupportContact ResolveGroundContact(
    core::Vec3 previousPosition,
    core::Vec3 proposedPosition,
    core::Vec3 size,
    float verticalVelocity);

CeilingContact ResolveCeilingContact(
    core::Vec3 previousPosition,
    core::Vec3 proposedPosition,
    core::Vec3 size,
    float verticalVelocity);
}
