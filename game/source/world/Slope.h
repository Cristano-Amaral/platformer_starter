#pragma once

#include "core/Vec3.h"
#include "world/GreyboxWorld.h"

namespace world
{
// Static test slopes. Renderer and PhysicsWorld both derive from these specs.
//
// CharacterVirtual max slope remains 50 degrees. A surface is walkable when
// the ground-normal vs world-up angle is below that limit.
//
// Walkable 30 degrees: long thin box rotated +Z so the top surface rises with
// +X. Lowest corner sits on the ground (Y = 0). This is an optional
// classification/traversal TEST, not a gate for Checkpoint 1.
//
// Phase B.1: the M14 center {10.90, 1.6732, 0} placed the high end (Y ~ 3.35)
// immediately left of the CP1 platform. Outbound (walk up, drop onto CP1)
// worked; return could not jump the 2.35 m face (max rise 1.6) and walking
// under the underside wedged the Player. The slope now sits past CP1 as a
// dead-end to the right. Center <-> CP1 uses open ground.
//
// Steep 60 degrees: classification-only dead-end past the 30-degree test.
struct SlopeSpec
{
    core::Vec3 center;
    core::Vec3 size;
    float rotationZDegrees = 0.0f;
};

inline constexpr float kWalkableSlopeDegrees = 30.0f;
inline constexpr float kSteepSlopeDegrees = 60.0f;

// Local size {6, 0.4, 4}, +30 deg about Z. Approximate world ends (Z = 0):
//   low  (left)  top:    X ~ 19.00, Y ~ 0.35
//   high (right) top:    X ~ 24.20, Y ~ 3.35
//   low  (left)  bottom: X ~ 19.20, Y ~ 0.00
//   high (right) bottom: X ~ 24.40, Y ~ 3.00
inline constexpr SlopeSpec kWalkableSlope{
    {21.70f, 1.6732f, 0.0f},
    {6.0f, 0.4f, 4.0f},
    kWalkableSlopeDegrees};

// Local size {2, 0.4, 3}, +60 deg about Z. Past the 30-degree high end.
inline constexpr SlopeSpec kSteepSlope{
    {25.60f, 0.9660f, 0.0f},
    {2.0f, 0.4f, 3.0f},
    kSteepSlopeDegrees};

// Conservative AABB half-extents of the +30 deg / +60 deg boxes.
inline constexpr float kWalkableSlopeAabbHalfX = 2.70f;
inline constexpr float kSteepSlopeAabbHalfX = 0.68f;

static_assert(
    kWalkableSlope.center.x - kWalkableSlopeAabbHalfX
    > kElevatedPlatforms[kCheckpoint1PlatformIndex].center.x
        + kElevatedPlatforms[kCheckpoint1PlatformIndex].size.x * 0.5f);
static_assert(
    kSteepSlope.center.x - kSteepSlopeAabbHalfX
    > kWalkableSlope.center.x + kWalkableSlopeAabbHalfX);
}
