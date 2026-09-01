#pragma once

#include "core/Vec3.h"

namespace world
{
// Static test slopes. Renderer and PhysicsWorld both derive from these specs.
//
// CharacterVirtual max slope remains 50 degrees. A surface is walkable when
// the ground-normal vs world-up angle is below that limit.
//
// Walkable 30 degrees: long thin box rotated around Z so the surface rises
// with +X. Center is chosen so the lowest AABB point sits on the ground
// (Y = 0) and the left AABB edge stays to the right of the moving platform
// (solid max X = 8).
//
// Steep 60 degrees: small static box on the far left for OnSteepGround
// classification only. It is not intended as a gameplay path.
struct SlopeSpec
{
    core::Vec3 center;
    core::Vec3 size;
    float rotationZDegrees = 0.0f;
};

inline constexpr float kWalkableSlopeDegrees = 30.0f;
inline constexpr float kSteepSlopeDegrees = 60.0f;

inline constexpr SlopeSpec kWalkableSlope{
    {10.90f, 1.6732f, 0.0f},
    {6.0f, 0.4f, 4.0f},
    kWalkableSlopeDegrees};

inline constexpr SlopeSpec kSteepSlope{
    {-10.80f, 0.9660f, 0.0f},
    {2.0f, 0.4f, 3.0f},
    kSteepSlopeDegrees};
}
