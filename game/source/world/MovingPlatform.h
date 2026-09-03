#pragma once

#include "core/Vec3.h"
#include "world/GreyboxWorld.h"

namespace world
{
// Immutable kinematic-platform authoring. Runtime pose lives in PhysicsWorld.
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

constexpr Box MovingPlatformSweptAabb(const MovingPlatformSpec& spec)
{
    return {
        {(spec.pathMinX + spec.pathMaxX) * 0.5f, spec.centerY, spec.centerZ},
        {(spec.pathMaxX - spec.pathMinX) + spec.size.x, spec.size.y, spec.size.z}};
}
}
