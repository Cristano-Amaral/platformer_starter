#pragma once

// Milestone 83: player 3D presentation follower. Not gameplay, physics,
// camera, or interaction authority. Not an authored Level object, Actor,
// Character, ECS component, scene graph, or animation state machine.

#include "core/Vec3.h"

#include <cmath>
#include <cstddef>

namespace gameplay
{
inline constexpr const char* kPlayerModelLogicalId = "models/player.glb";

// Horizontal speed below this does not change facing. Vertical velocity is
// never fed into facing.
inline constexpr float kPlayerFacingMoveEpsilon = 0.01f;

// Engine +X. atan2(+X, 0) in degrees. Model local +Z is authored forward, so
// this yaw maps that forward onto world +X when the correction is 0.
inline constexpr float kPlayerInitialFacingYawDegrees = 90.0f;

inline constexpr float kPlayerPresentationPi = 3.14159265358979323846f;
inline constexpr float kPlayerPresentationRadToDeg = 180.0f / kPlayerPresentationPi;

// World-space offset added to the authoritative visual-center position.
// It is not rotated by facing yaw.
struct PlayerPresentationConfig
{
    const char* modelIdentity = kPlayerModelLogicalId;
    core::Vec3 visualOffset{};
    core::Vec3 visualScale{1.0f, 1.0f, 1.0f};
    float modelForwardYawCorrectionDegrees = 0.0f;
};

inline constexpr PlayerPresentationConfig kDefaultPlayerPresentationConfig{};

struct PlayerPresentationState
{
    float facingYawDegrees = kPlayerInitialFacingYawDegrees;
};

struct PlayerVisualTransform
{
    core::Vec3 position{};
    core::Vec3 scale{1.0f, 1.0f, 1.0f};
    float yawDegrees = kPlayerInitialFacingYawDegrees;
};

// Load-once policy for the staged player model. Renderer owns GPU objects;
// this only records whether a load was already attempted this lifetime.
struct PlayerPresentationModelLifetime
{
    bool loadAttempted = false;
    std::size_t loadCount = 0;
};

inline float UpdatePlayerFacingYaw(float previousFacingYawDegrees, float moveX, float moveZ)
{
    const float magnitudeSquared = moveX * moveX + moveZ * moveZ;
    const float epsilonSquared = kPlayerFacingMoveEpsilon * kPlayerFacingMoveEpsilon;
    if (magnitudeSquared <= epsilonSquared)
    {
        return previousFacingYawDegrees;
    }
    return std::atan2(moveX, moveZ) * kPlayerPresentationRadToDeg;
}

// Current controller is X-constrained. Vertical velocity must not face the
// model. Z is 0 until a future controller actually accepts Z movement.
inline void AcceptedPlayerHorizontalMovement(
    float horizontalVelocity,
    float verticalVelocity,
    float& moveX,
    float& moveZ)
{
    (void)verticalVelocity;
    moveX = horizontalVelocity;
    moveZ = 0.0f;
}

inline PlayerVisualTransform BuildPlayerVisualTransform(
    core::Vec3 authoritativePlayerPosition,
    const PlayerPresentationConfig& config,
    float facingYawDegrees)
{
    PlayerVisualTransform visual{};
    visual.position = authoritativePlayerPosition + config.visualOffset;
    visual.scale = config.visualScale;
    visual.yawDegrees = facingYawDegrees + config.modelForwardYawCorrectionDegrees;
    return visual;
}

inline void ResetPlayerPresentationForLifecycle(PlayerPresentationState& state)
{
    state.facingYawDegrees = kPlayerInitialFacingYawDegrees;
}

inline bool PlayerPresentationShouldAttemptModelLoad(
    const PlayerPresentationModelLifetime& lifetime)
{
    return !lifetime.loadAttempted;
}

inline void NotePlayerPresentationLoadAttempt(PlayerPresentationModelLifetime& lifetime)
{
    if (lifetime.loadAttempted)
    {
        return;
    }
    lifetime.loadAttempted = true;
    ++lifetime.loadCount;
}

inline void ClearPlayerPresentationModelLifetime(PlayerPresentationModelLifetime& lifetime)
{
    lifetime = {};
}

inline bool ShouldDrawPlayerPresentationModel(bool modelLoaded)
{
    return modelLoaded;
}

inline bool ShouldDrawPlayerGameplayPrimitive(bool modelLoaded)
{
    return !modelLoaded;
}
}
