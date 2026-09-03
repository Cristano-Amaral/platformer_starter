#pragma once

#include "core/Vec3.h"
#include "world/CollectibleWorld.h"

#include <array>

namespace ui
{
struct DebugMetricsSnapshot
{
    float fps = 0.0f;
    float deltaSeconds = 0.0f;
    double runTimeSeconds = 0.0;
    bool runTimerFrozen = false;
    bool hasSessionBest = false;
    double sessionBestSeconds = 0.0;

    core::Vec3 playerPosition{};
    float horizontalVelocity = 0.0f;
    float verticalVelocity = 0.0f;
    bool grounded = false;

    float moveX = 0.0f;
    bool jumpPressed = false;
    bool respawnPressed = false;
    bool restartPressed = false;
    bool restartAvailable = false;
    bool restartedThisFrame = false;

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
    core::Vec3 physicsTestBoxLinearVelocity{};

    bool characterVirtualInitialized = false;
    const char* playerGroundSupport = "InAir";
    int playerContactCount = 0;
    int staticBodyCount = 0;
    bool dynamicTestBodyValid = false;
    bool playerPositionFinite = true;
    bool playerVelocityFinite = true;

    core::Vec3 groundVelocity{};
    bool supportingGroundMoving = false;
    core::Vec3 groundNormal{};
    float groundSlopeAngleDegrees = 0.0f;
    bool currentSupportWalkable = false;
    const char* supportClassification = "Unsupported";
    const char* supportBodyKind = "None";
    bool dynamicContact = false;
    core::Vec3 playerWorldVelocity{};
    bool characterInnerBodyActive = false;

    bool movingPlatformValid = false;
    core::Vec3 movingPlatformPosition{};
    core::Vec3 movingPlatformVelocity{};
    float movingPlatformDirection = 1.0f;
    float movingPlatformPathMinX = 0.0f;
    float movingPlatformPathMaxX = 0.0f;
    float movingPlatformSpeed = 0.0f;

    bool testTextureLoaded = false;
    bool testTextureFallbackActive = false;
    const char* testTextureLogicalId = "";
    const char* testTextureRuntimeRelativePath = "";

    bool testModelLoaded = false;
    bool testModelFallbackActive = false;
    const char* testModelLogicalId = "";

    bool authoredModelLoaded = false;
    bool authoredModelFallbackActive = false;
    const char* authoredModelLogicalId = "";

    bool texturedModelLoaded = false;
    bool texturedModelFallbackActive = false;
    const char* texturedModelLogicalId = "";
    int texturedModelMaterialCount = 0;
    bool texturedModelHasAlbedoTexture = false;

    core::Vec3 respawnPosition{};
    float killPlaneY = 0.0f;
    int deathCount = 0;
    const char* lastRespawnReason = "None";
    const char* activeCheckpointLabel = "None";
    bool checkpoint1Inside = false;
    const char* checkpoint1VisualState = "Future";
    bool checkpoint2Inside = false;
    const char* checkpoint2VisualState = "Future";
    const char* insideHazardLabel = "None";
    bool hazardContactThisFrame = false;

    int collectedCount = 0;
    std::array<bool, world::kCollectibleCount> collectibleCollected{};
    std::array<bool, world::kCollectibleCount> collectibleInside{};
    const char* collectedThisFrameLabel = "None";

    bool levelCompleted = false;
    core::Vec3 goalCenter{};
    core::Vec3 goalSize{};
    bool playerInsideGoal = false;
};

void DrawDebugMetrics(const DebugMetricsSnapshot& snapshot);
}
