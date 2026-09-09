#pragma once

// Milestone 51: Dynamic Box Grab / Carry tuning and runtime-only query state.
// Not a generic interaction framework. Not Level Format. Not authored.

#include "core/Vec3.h"

namespace physics
{
inline constexpr int kNoDynamicBoxGrabIndex = -1;

// Maximum center-to-center distance for targeting. Finite so a box across the
// level cannot be grabbed.
inline constexpr float kDynamicBoxMaxGrabDistance = 2.5f;

// Cosine of the maximum angle from the facing axis. ~70 degrees in the XY
// plane so a box on the ground stays eligible without matching a pixel-perfect
// ray.
inline constexpr float kDynamicBoxMinFacingDot = 0.35f;

// Desired hold distance from the player visual center along facing X.
inline constexpr float kDynamicBoxCarryDistance = 1.35f;
inline constexpr float kDynamicBoxCarryHeightOffset = 0.15f;

// When a world hit shortens the hold point, keep the box outside the player
// capsule (radius 0.4) plus a small gap.
inline constexpr float kDynamicBoxCarryMinDistance = 0.7f;

// Velocity drive toward the carry target. Mass-independent (SetLinearVelocity),
// so authored Dynamic Boxes remain grabbable regardless of massKg.
inline constexpr float kDynamicBoxCarryMaxSpeed = 10.0f;
inline constexpr float kDynamicBoxCarryGain = 16.0f;
inline constexpr float kDynamicBoxCarrySafetyMargin = 0.08f;

struct DynamicBoxGrabState
{
    bool carrying = false;
    int carriedIndex = kNoDynamicBoxGrabIndex;
    bool hasTarget = false;
    int targetIndex = kNoDynamicBoxGrabIndex;
    core::Vec3 carryTarget{};
    bool carryTargetValid = false;
};
}
