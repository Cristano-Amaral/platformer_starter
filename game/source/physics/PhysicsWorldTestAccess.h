#pragma once

// Test-only accessor for PhysicsRebuildTest. Not a gameplay/editor API.
// Arranges a Dynamic Box Jolt pose so tests can exercise production recovery
// without exposing a general teleport/velocity mutator on PhysicsWorld.

#include "core/Vec3.h"

#include <cstddef>

namespace physics
{
class PhysicsWorld;

struct PhysicsWorldTestAccess
{
    static void SetDynamicBoxRuntimeMotion(
        PhysicsWorld& world,
        std::size_t index,
        core::Vec3 center,
        core::Vec3 linearVelocity,
        core::Vec3 angularVelocity,
        float rotationX = 0.0f,
        float rotationY = 0.0f,
        float rotationZ = 0.0f,
        float rotationW = 1.0f);
};
}
