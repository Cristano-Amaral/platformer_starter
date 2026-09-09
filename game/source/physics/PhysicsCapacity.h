#pragma once

// Jolt PhysicsSystem::Init allocator size. This is a runtime body budget,
// not a gameplay-design object cap. Checkpoints, hazards, and collectibles
// are not physics bodies.

namespace physics
{
// Raised from 32 so authoring is not stuck near 16 platforms. 64 bodies is
// still small versus the existing 1 MiB temp allocator. Not a Pi Zero W
// streaming/LOD system.
inline constexpr unsigned int kPhysicsMaxBodies = 64;

// ground, 2 slopes, kinematic moving platform, CharacterVirtual inner body.
// CharacterVirtual itself is not a Jolt body. Elevated platforms, Dynamic
// Boxes, and Doors share the leftover. Pressure Plates and Static Props
// do not consume body slots.
inline constexpr int kPhysicsFixedBodyCount = 5;
inline constexpr int kPhysicsNonPlatformBodyCount = kPhysicsFixedBodyCount;

inline constexpr int kMaxAuthoredPhysicsBodies =
    static_cast<int>(kPhysicsMaxBodies) - kPhysicsFixedBodyCount;

// Historical name: max platforms when Dynamic Box and Door counts are 0.
inline constexpr int kMaxPhysicsElevatedPlatformCount = kMaxAuthoredPhysicsBodies;

inline constexpr bool AuthoredPhysicsBodiesWithinBudget(
    int elevatedPlatformCount,
    int dynamicBoxCount,
    int doorCount)
{
    if (elevatedPlatformCount < 0 || dynamicBoxCount < 0 || doorCount < 0)
    {
        return false;
    }
    if (elevatedPlatformCount > kMaxAuthoredPhysicsBodies
        || dynamicBoxCount > kMaxAuthoredPhysicsBodies
        || doorCount > kMaxAuthoredPhysicsBodies)
    {
        return false;
    }
    return elevatedPlatformCount + dynamicBoxCount + doorCount <= kMaxAuthoredPhysicsBodies;
}

static_assert(kPhysicsFixedBodyCount == 5);
static_assert(kMaxAuthoredPhysicsBodies == 59);
static_assert(kMaxPhysicsElevatedPlatformCount == 59);
}
