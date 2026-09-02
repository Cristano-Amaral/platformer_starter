#include "ui/debug/DebugMetrics.h"

#if defined(PLATFORMER_ENABLE_DEBUG_UI)

#include "imgui.h"

namespace ui
{
namespace
{
const char* BoolText(bool value)
{
    return value ? "true" : "false";
}

float CoyoteRemaining(const DebugMetricsSnapshot& snapshot)
{
    if (!snapshot.coyoteAvailable)
    {
        return 0.0f;
    }
    if (snapshot.grounded)
    {
        return snapshot.coyoteDuration;
    }

    const float remaining = snapshot.coyoteDuration - snapshot.coyoteElapsed;
    return remaining > 0.0f ? remaining : 0.0f;
}
}

void DrawDebugMetrics(const DebugMetricsSnapshot& snapshot)
{
    ImGui::Begin("Platformer3D Metrics");

    if (ImGui::CollapsingHeader("Frame", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("FPS: %.0f", snapshot.fps);
        ImGui::Text("deltaSeconds: %.6f", snapshot.deltaSeconds);
        ImGui::Text("deltaMilliseconds: %.3f", snapshot.deltaSeconds * 1000.0f);
    }

    if (ImGui::CollapsingHeader("Player transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("position X: %.4f", snapshot.playerPosition.x);
        ImGui::Text("position Y: %.4f", snapshot.playerPosition.y);
        ImGui::Text("position Z: %.4f", snapshot.playerPosition.z);
    }

    if (ImGui::CollapsingHeader("Player movement", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("horizontal velocity: %.4f", snapshot.horizontalVelocity);
        ImGui::Text("vertical velocity: %.4f", snapshot.verticalVelocity);
        ImGui::Text("grounded: %s", BoolText(snapshot.grounded));
    }

    if (ImGui::CollapsingHeader("Input", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("moveX: %.0f", snapshot.moveX);
        ImGui::Text("jumpPressed: %s", BoolText(snapshot.jumpPressed));
        ImGui::Text("respawnPressed: %s", BoolText(snapshot.respawnPressed));
        ImGui::Text("restartPressed: %s", BoolText(snapshot.restartPressed));
        ImGui::Text("Restart available: %s", BoolText(snapshot.restartAvailable));
        ImGui::Text("Restarted this frame: %s", BoolText(snapshot.restartedThisFrame));
    }

    if (ImGui::CollapsingHeader("Milestone 07 state", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("coyote elapsed: %.4f", snapshot.coyoteElapsed);
        ImGui::Text("coyote remaining: %.4f", CoyoteRemaining(snapshot));
        ImGui::Text("coyote available: %s", BoolText(snapshot.coyoteAvailable));
        ImGui::Text("jump buffer remaining: %.4f", snapshot.jumpBufferRemaining);
    }

    if (ImGui::CollapsingHeader("Milestone 07 constants", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("max horizontal speed: %.4f", snapshot.maxMoveSpeed);
        ImGui::Text("acceleration: %.4f", snapshot.acceleration);
        ImGui::Text("deceleration: %.4f", snapshot.deceleration);
        ImGui::Text("jump speed: %.4f", snapshot.jumpSpeed);
        ImGui::Text("gravity: %.4f", snapshot.gravity);
        ImGui::Text("coyote duration: %.4f", snapshot.coyoteDuration);
        ImGui::Text("jump buffer duration: %.4f", snapshot.jumpBufferDuration);
    }

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text(
            "desired target: %.4f, %.4f, %.4f",
            snapshot.desiredTarget.x,
            snapshot.desiredTarget.y,
            snapshot.desiredTarget.z);
        ImGui::Text(
            "smoothed target: %.4f, %.4f, %.4f",
            snapshot.smoothedTarget.x,
            snapshot.smoothedTarget.y,
            snapshot.smoothedTarget.z);
        ImGui::Text("horizontal dead zone: %.4f", snapshot.horizontalDeadZone);
        ImGui::Text("vertical dead zone: %.4f", snapshot.verticalDeadZone);
        ImGui::Text("follow sharpness: %.4f", snapshot.followSharpness);
    }

    if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Jolt initialized: %s", BoolText(snapshot.physicsInitialized));
        ImGui::Text("static greybox bodies: %d", snapshot.staticBodyCount);
        ImGui::Text("dynamic test body valid: %s", BoolText(snapshot.dynamicTestBodyValid));
        ImGui::Text("dynamic test box X: %.4f", snapshot.physicsTestBoxPosition.x);
        ImGui::Text("dynamic test box Y: %.4f", snapshot.physicsTestBoxPosition.y);
        ImGui::Text("dynamic test box Z: %.4f", snapshot.physicsTestBoxPosition.z);
        ImGui::Text(
            "dynamic test box active: %s",
            BoolText(snapshot.physicsTestBoxActive));
        ImGui::Text(
            "dynamic test box sleeping: %s",
            BoolText(snapshot.dynamicTestBodyValid && !snapshot.physicsTestBoxActive));
        ImGui::Text(
            "dynamic test box velocity: %.4f, %.4f, %.4f",
            snapshot.physicsTestBoxLinearVelocity.x,
            snapshot.physicsTestBoxLinearVelocity.y,
            snapshot.physicsTestBoxLinearVelocity.z);
    }

    if (ImGui::CollapsingHeader("Player Physics / Character", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text(
            "CharacterVirtual initialized: %s",
            BoolText(snapshot.characterVirtualInitialized));
        ImGui::Text("Character inner body active: %s", BoolText(snapshot.characterInnerBodyActive));
        ImGui::Text("physical Player X: %.4f", snapshot.playerPosition.x);
        ImGui::Text("physical Player Y: %.4f", snapshot.playerPosition.y);
        ImGui::Text("physical Player Z: %.4f", snapshot.playerPosition.z);
        ImGui::Text("horizontal velocity: %.4f", snapshot.horizontalVelocity);
        ImGui::Text("vertical velocity: %.4f", snapshot.verticalVelocity);
        ImGui::Text("supported/grounded: %s", BoolText(snapshot.grounded));
        ImGui::Text("ground support: %s", snapshot.playerGroundSupport);
        ImGui::Text("active contacts: %d", snapshot.playerContactCount);
        ImGui::Text("dynamic contact this frame: %s", BoolText(snapshot.dynamicContact));
        ImGui::Text("support body kind: %s", snapshot.supportBodyKind);
        ImGui::Text(
            "world velocity: %.4f, %.4f, %.4f",
            snapshot.playerWorldVelocity.x,
            snapshot.playerWorldVelocity.y,
            snapshot.playerWorldVelocity.z);
        ImGui::Text("relative horizontal velocity: %.4f", snapshot.horizontalVelocity);
        ImGui::Text("position finite: %s", BoolText(snapshot.playerPositionFinite));
        ImGui::Text("velocity finite: %s", BoolText(snapshot.playerVelocityFinite));
        ImGui::Text(
            "ground velocity: %.4f, %.4f, %.4f",
            snapshot.groundVelocity.x,
            snapshot.groundVelocity.y,
            snapshot.groundVelocity.z);
        ImGui::Text("supporting ground moving: %s", BoolText(snapshot.supportingGroundMoving));
        ImGui::Text(
            "ground normal: %.4f, %.4f, %.4f",
            snapshot.groundNormal.x,
            snapshot.groundNormal.y,
            snapshot.groundNormal.z);
        ImGui::Text("ground slope angle deg: %.2f", snapshot.groundSlopeAngleDegrees);
        ImGui::Text("current support walkable: %s", BoolText(snapshot.currentSupportWalkable));
        ImGui::Text("support classification: %s", snapshot.supportClassification);
    }

    if (ImGui::CollapsingHeader("Moving Platform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("initialized/valid: %s", BoolText(snapshot.movingPlatformValid));
        ImGui::Text("position X: %.4f", snapshot.movingPlatformPosition.x);
        ImGui::Text("position Y: %.4f", snapshot.movingPlatformPosition.y);
        ImGui::Text("position Z: %.4f", snapshot.movingPlatformPosition.z);
        ImGui::Text("velocity X: %.4f", snapshot.movingPlatformVelocity.x);
        ImGui::Text("velocity Y: %.4f", snapshot.movingPlatformVelocity.y);
        ImGui::Text("velocity Z: %.4f", snapshot.movingPlatformVelocity.z);
        ImGui::Text("direction: %.0f", snapshot.movingPlatformDirection);
        ImGui::Text("path min X: %.4f", snapshot.movingPlatformPathMinX);
        ImGui::Text("path max X: %.4f", snapshot.movingPlatformPathMaxX);
        ImGui::Text("configured speed: %.4f", snapshot.movingPlatformSpeed);
    }

    if (ImGui::CollapsingHeader("Assets", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("test texture loaded: %s", BoolText(snapshot.testTextureLoaded));
        ImGui::Text("logical id: %s", snapshot.testTextureLogicalId);
        ImGui::Text("runtime path: %s", snapshot.testTextureRuntimeRelativePath);
        ImGui::Text("fallback active: %s", BoolText(snapshot.testTextureFallbackActive));
        ImGui::Separator();
        ImGui::Text("Static Model");
        ImGui::Text("loaded: %s", BoolText(snapshot.testModelLoaded));
        ImGui::Text("logical id: %s", snapshot.testModelLogicalId);
        ImGui::Text("fallback: %s", BoolText(snapshot.testModelFallbackActive));
        ImGui::Separator();
        ImGui::Text("Blender Authored Model");
        ImGui::Text("loaded: %s", BoolText(snapshot.authoredModelLoaded));
        ImGui::Text("fallback: %s", BoolText(snapshot.authoredModelFallbackActive));
        ImGui::Text("id: %s", snapshot.authoredModelLogicalId);
        ImGui::Separator();
        ImGui::Text("Textured GLB Model");
        ImGui::Text("loaded: %s", BoolText(snapshot.texturedModelLoaded));
        ImGui::Text("fallback: %s", BoolText(snapshot.texturedModelFallbackActive));
        ImGui::Text("id: %s", snapshot.texturedModelLogicalId);
        ImGui::Text("material count: %d", snapshot.texturedModelMaterialCount);
        ImGui::Text("albedo texture: %s", BoolText(snapshot.texturedModelHasAlbedoTexture));
    }

    if (ImGui::CollapsingHeader("Respawn / Checkpoint", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Active checkpoint: %s", snapshot.activeCheckpointLabel);
        ImGui::Text(
            "Respawn position: %.4f, %.4f, %.4f",
            snapshot.respawnPosition.x,
            snapshot.respawnPosition.y,
            snapshot.respawnPosition.z);
        ImGui::Text("Player Y: %.4f", snapshot.playerPosition.y);
        ImGui::Text("Kill plane Y: %.4f", snapshot.killPlaneY);
        ImGui::Text("Death count: %d", snapshot.deathCount);
        ImGui::Text("Last respawn reason: %s", snapshot.lastRespawnReason);
        ImGui::Separator();
        ImGui::Text("Checkpoint 1");
        ImGui::Text("Inside: %s", BoolText(snapshot.checkpoint1Inside));
        ImGui::Text("State: %s", snapshot.checkpoint1VisualState);
        ImGui::Separator();
        ImGui::Text("Checkpoint 2");
        ImGui::Text("Inside: %s", BoolText(snapshot.checkpoint2Inside));
        ImGui::Text("State: %s", snapshot.checkpoint2VisualState);
    }

    if (ImGui::CollapsingHeader("Hazards", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Inside hazard: %s", snapshot.insideHazardLabel);
        ImGui::Text(
            "Hazard contact this frame: %s",
            BoolText(snapshot.hazardContactThisFrame));
    }

    if (ImGui::CollapsingHeader("Collectibles", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Collectibles: %d / %d", snapshot.collectedCount, world::kCollectibleCount);
        for (int index = 0; index < world::kCollectibleCount; ++index)
        {
            ImGui::Separator();
            ImGui::Text("Collectible %d", index + 1);
            ImGui::Text(
                "State: %s",
                snapshot.collectibleCollected[static_cast<std::size_t>(index)]
                    ? "Collected"
                    : "Available");
            ImGui::Text(
                "Inside: %s",
                BoolText(snapshot.collectibleInside[static_cast<std::size_t>(index)]));
        }
        ImGui::Separator();
        ImGui::Text("Collected this frame: %s", snapshot.collectedThisFrameLabel);
    }

    if (ImGui::CollapsingHeader("Level Goal", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Level completed: %s", BoolText(snapshot.levelCompleted));
        ImGui::Text("Goal marker: two-post gate (not a checkpoint)");
        ImGui::Text(
            "Goal center: %.4f, %.4f, %.4f",
            snapshot.goalCenter.x,
            snapshot.goalCenter.y,
            snapshot.goalCenter.z);
        ImGui::Text(
            "Goal size: %.4f, %.4f, %.4f",
            snapshot.goalSize.x,
            snapshot.goalSize.y,
            snapshot.goalSize.z);
        ImGui::Text("Player inside goal: %s", BoolText(snapshot.playerInsideGoal));
    }

    ImGui::End();
}
}

#else

namespace ui
{
void DrawDebugMetrics(const DebugMetricsSnapshot&) {}
}

#endif
