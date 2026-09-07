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

// M44: the M23 cyan dynamic crate is no longer instantiated in the canonical
// scene. Level Format v1 still stores dynamic_box for round-trip. Do not
// recreate that body here without updating this count.
inline constexpr bool kInstantiateCanonicalDynamicProbeBody = false;

// ground, 2 slopes, kinematic moving platform, CharacterVirtual inner body.
// CharacterVirtual itself is not a Jolt body.
inline constexpr int kPhysicsNonPlatformBodyCount = 5;

inline constexpr int kMaxPhysicsElevatedPlatformCount =
    static_cast<int>(kPhysicsMaxBodies) - kPhysicsNonPlatformBodyCount;

static_assert(kPhysicsNonPlatformBodyCount == 5);
static_assert(kMaxPhysicsElevatedPlatformCount == 59);
}
