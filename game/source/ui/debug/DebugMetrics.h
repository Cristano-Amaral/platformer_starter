#pragma once

#include "core/Vec3.h"

namespace ui
{
struct DebugMetricsSnapshot
{
    float fps = 0.0f;
    float deltaSeconds = 0.0f;

    core::Vec3 playerPosition{};
    float horizontalVelocity = 0.0f;
    float verticalVelocity = 0.0f;
    bool grounded = false;

    float moveX = 0.0f;
    bool jumpPressed = false;

    float coyoteElapsed = 0.0f;
    bool coyoteAvailable = false;
    float jumpBufferRemaining = 0.0f;

    float maxMoveSpeed = 0.0f;
    float acceleration = 0.0f;
    float deceleration = 0.0f;
    float jumpSpeed = 0.0f;
    float gravity = 0.0f;
    float coyoteDuration = 0.0f;
    float jumpBufferDuration = 0.0f;

    core::Vec3 desiredTarget{};
    core::Vec3 smoothedTarget{};
    float horizontalDeadZone = 0.0f;
    float verticalDeadZone = 0.0f;
    float followSharpness = 0.0f;

    bool physicsInitialized = false;
    core::Vec3 physicsTestBoxPosition{};
    bool physicsTestBoxActive = false;

    bool characterVirtualInitialized = false;
    const char* playerGroundSupport = "InAir";
    int playerContactCount = 0;
};

void DrawDebugMetrics(const DebugMetricsSnapshot& snapshot);
}
