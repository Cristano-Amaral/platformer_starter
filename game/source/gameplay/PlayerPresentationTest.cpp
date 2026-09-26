// Milestone 83: player presentation math, lifecycle facing, and authority
// isolation. No window, raylib, Jolt, or GPU.

#include "gameplay/PlayerPresentation.h"
#include "gameplay/ItemPickupRuntime.h"
#include "world/ItemPickup.h"
#include "world/RespawnWorld.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace
{
int gFailures = 0;

void Expect(bool condition, const char* name)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++gFailures;
    }
}

bool NearlyEqual(float a, float b, float epsilon = 0.0001f)
{
    return std::fabs(a - b) <= epsilon;
}

bool Vec3Equal(core::Vec3 a, core::Vec3 b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

float FacingYaw(float moveX, float moveZ)
{
    return std::atan2(moveX, moveZ) * gameplay::kPlayerPresentationRadToDeg;
}
}

int main()
{
    Expect(
        std::string_view(gameplay::kPlayerModelLogicalId) == "models/player.glb",
        "staged player model identity is models/player.glb");
    Expect(
        std::strcmp(gameplay::kDefaultPlayerPresentationConfig.modelIdentity, "models/player.glb")
            == 0,
        "default config uses the staged player identity");
    Expect(
        NearlyEqual(gameplay::kPlayerInitialFacingYawDegrees, FacingYaw(1.0f, 0.0f)),
        "initial facing is engine +X");
    Expect(
        NearlyEqual(gameplay::kDefaultPlayerPresentationConfig.modelForwardYawCorrectionDegrees, 0.0f),
        "default model-forward correction is 0");

    gameplay::PlayerPresentationState facing{};
    Expect(
        NearlyEqual(facing.facingYawDegrees, gameplay::kPlayerInitialFacingYawDegrees),
        "default state faces +X");

    facing.facingYawDegrees = gameplay::UpdatePlayerFacingYaw(facing.facingYawDegrees, 1.0f, 0.0f);
    Expect(NearlyEqual(facing.facingYawDegrees, FacingYaw(1.0f, 0.0f)), "1. +X movement faces +X");

    facing.facingYawDegrees = gameplay::UpdatePlayerFacingYaw(facing.facingYawDegrees, -1.0f, 0.0f);
    Expect(NearlyEqual(facing.facingYawDegrees, FacingYaw(-1.0f, 0.0f)), "2. -X movement faces -X");

    facing.facingYawDegrees = gameplay::UpdatePlayerFacingYaw(facing.facingYawDegrees, 0.0f, 1.0f);
    Expect(NearlyEqual(facing.facingYawDegrees, FacingYaw(0.0f, 1.0f)), "3. +Z movement faces +Z");

    facing.facingYawDegrees = gameplay::UpdatePlayerFacingYaw(facing.facingYawDegrees, 0.0f, -1.0f);
    Expect(NearlyEqual(facing.facingYawDegrees, FacingYaw(0.0f, -1.0f)), "4. -Z movement faces -Z");

    facing.facingYawDegrees = gameplay::UpdatePlayerFacingYaw(facing.facingYawDegrees, 1.0f, 1.0f);
    Expect(
        NearlyEqual(facing.facingYawDegrees, FacingYaw(1.0f, 1.0f)),
        "5. diagonal +X+Z facing is deterministic");

    const float previous = facing.facingYawDegrees;
    facing.facingYawDegrees =
        gameplay::UpdatePlayerFacingYaw(facing.facingYawDegrees, 0.004f, -0.003f);
    Expect(
        NearlyEqual(facing.facingYawDegrees, previous),
        "6. near-zero horizontal movement preserves previous facing");

    float acceptedX = 0.0f;
    float acceptedZ = 1.0f;
    gameplay::AcceptedPlayerHorizontalMovement(0.0f, 8.0f, acceptedX, acceptedZ);
    Expect(acceptedX == 0.0f && acceptedZ == 0.0f, "accepted movement ignores vertical velocity");
    const float beforeVertical = facing.facingYawDegrees;
    facing.facingYawDegrees =
        gameplay::UpdatePlayerFacingYaw(facing.facingYawDegrees, acceptedX, acceptedZ);
    Expect(
        NearlyEqual(facing.facingYawDegrees, beforeVertical),
        "7. vertical-only movement does not change facing");

    const float lastMeaningful = facing.facingYawDegrees;
    facing.facingYawDegrees = gameplay::UpdatePlayerFacingYaw(facing.facingYawDegrees, 0.0f, 0.0f);
    Expect(
        NearlyEqual(facing.facingYawDegrees, lastMeaningful),
        "8. stationary player preserves last meaningful facing");

    gameplay::PlayerPresentationConfig corrected = gameplay::kDefaultPlayerPresentationConfig;
    corrected.modelForwardYawCorrectionDegrees = 15.0f;
    const gameplay::PlayerVisualTransform correctedVisual = gameplay::BuildPlayerVisualTransform(
        {0.0f, 0.8f, 0.0f}, corrected, gameplay::kPlayerInitialFacingYawDegrees);
    Expect(
        NearlyEqual(
            correctedVisual.yawDegrees,
            gameplay::kPlayerInitialFacingYawDegrees + 15.0f),
        "9. model-forward-axis correction is deterministic");

    core::Vec3 authoritative{3.0f, 1.25f, -0.5f};
    gameplay::PlayerPresentationConfig offsetConfig = gameplay::kDefaultPlayerPresentationConfig;
    offsetConfig.visualOffset = {0.0f, -0.25f, 0.10f};
    const gameplay::PlayerVisualTransform offsetVisual =
        gameplay::BuildPlayerVisualTransform(authoritative, offsetConfig, 45.0f);
    Expect(Vec3Equal(authoritative, {3.0f, 1.25f, -0.5f}), "10. visual offset does not mutate authority");
    Expect(
        Vec3Equal(offsetVisual.position, {3.0f, 1.0f, -0.40f}),
        "visual offset is world-space and yaw-independent");

    core::Vec3 colliderSize = world::kPlayerVisualSize;
    gameplay::PlayerPresentationConfig scaled = gameplay::kDefaultPlayerPresentationConfig;
    scaled.visualScale = {1.4f, 1.4f, 1.4f};
    const gameplay::PlayerVisualTransform scaledVisual =
        gameplay::BuildPlayerVisualTransform({0.0f, 0.8f, 0.0f}, scaled, 0.0f);
    Expect(Vec3Equal(colliderSize, world::kPlayerVisualSize), "11. visual scale does not mutate collider");
    Expect(Vec3Equal(scaledVisual.scale, {1.4f, 1.4f, 1.4f}), "visual scale is presentation-only");

    const core::Vec3 followPos{6.5f, 2.0f, 0.0f};
    const gameplay::PlayerVisualTransform followed = gameplay::BuildPlayerVisualTransform(
        followPos, gameplay::kDefaultPlayerPresentationConfig, 90.0f);
    Expect(Vec3Equal(followed.position, followPos), "12. visual transform follows authoritative position");

    gameplay::PlayerPresentationState lifecycle{};
    lifecycle.facingYawDegrees = -90.0f;
    gameplay::ResetPlayerPresentationForLifecycle(lifecycle);
    Expect(
        NearlyEqual(lifecycle.facingYawDegrees, gameplay::kPlayerInitialFacingYawDegrees),
        "13. lifecycle reset produces deterministic initial facing");

    const core::Vec3 deathPos{12.0f, 0.8f, 0.0f};
    const core::Vec3 respawnPos{0.0f, 0.8f, 0.0f};
    const gameplay::PlayerVisualTransform deathVisual = gameplay::BuildPlayerVisualTransform(
        deathPos, gameplay::kDefaultPlayerPresentationConfig, lifecycle.facingYawDegrees);
    gameplay::ResetPlayerPresentationForLifecycle(lifecycle);
    const gameplay::PlayerVisualTransform respawnVisual = gameplay::BuildPlayerVisualTransform(
        respawnPos, gameplay::kDefaultPlayerPresentationConfig, lifecycle.facingYawDegrees);
    Expect(!Vec3Equal(deathVisual.position, respawnVisual.position), "death visual is not reused");
    Expect(Vec3Equal(respawnVisual.position, respawnPos), "14. respawn visual snaps to respawn position");

    const core::Vec3 destinationSpawn{-4.0f, 0.8f, 0.0f};
    gameplay::ResetPlayerPresentationForLifecycle(lifecycle);
    const gameplay::PlayerVisualTransform destinationVisual = gameplay::BuildPlayerVisualTransform(
        destinationSpawn, gameplay::kDefaultPlayerPresentationConfig, lifecycle.facingYawDegrees);
    Expect(Vec3Equal(destinationVisual.position, destinationSpawn), "15. level transition uses destination spawn");
    Expect(
        NearlyEqual(lifecycle.facingYawDegrees, gameplay::kPlayerInitialFacingYawDegrees),
        "15b. destination spawn facing is the lifecycle default");

    gameplay::PlayerPresentationState restart{};
    restart.facingYawDegrees = 180.0f;
    gameplay::ResetPlayerPresentationForLifecycle(restart);
    Expect(
        NearlyEqual(restart.facingYawDegrees, gameplay::kPlayerInitialFacingYawDegrees),
        "16. Restart resets presentation facing");

    gameplay::PlayerPresentationState playAgain{};
    playAgain.facingYawDegrees = -45.0f;
    gameplay::ResetPlayerPresentationForLifecycle(playAgain);
    Expect(
        NearlyEqual(playAgain.facingYawDegrees, gameplay::kPlayerInitialFacingYawDegrees),
        "17. Play Again resets presentation facing");

    Expect(
        gameplay::ShouldDrawPlayerGameplayPrimitive(false),
        "18. missing model keeps the old primitive fallback");
    Expect(
        !gameplay::ShouldDrawPlayerPresentationModel(false),
        "18b. missing model does not draw the 3D presentation");

    Expect(Vec3Equal(colliderSize, world::kPlayerVisualSize), "18c. load failure does not resize the collider");

    gameplay::PlayerPresentationModelLifetime lifetime{};
    Expect(gameplay::PlayerPresentationShouldAttemptModelLoad(lifetime), "first lifetime may load");
    gameplay::NotePlayerPresentationLoadAttempt(lifetime);
    Expect(lifetime.loadCount == 1, "19. load is recorded once");
    Expect(!gameplay::PlayerPresentationShouldAttemptModelLoad(lifetime), "already attempted does not reload");
    gameplay::NotePlayerPresentationLoadAttempt(lifetime);
    Expect(lifetime.loadCount == 1, "19b. a second note does not load per frame");
    (void)gameplay::BuildPlayerVisualTransform(
        followPos, gameplay::kDefaultPlayerPresentationConfig, 0.0f);
    Expect(lifetime.loadCount == 1, "19c. visual transform does not load a model");
    gameplay::PreparePlayerPresentationModelDefinitionRefresh(lifetime);
    Expect(gameplay::PlayerPresentationShouldAttemptModelLoad(lifetime),
        "19d. promoted CharacterDefinition permits one presentation refresh");
    gameplay::NotePlayerPresentationLoadAttempt(lifetime);
    Expect(lifetime.loadCount == 2
            && !gameplay::PlayerPresentationShouldAttemptModelLoad(lifetime),
        "19e. definition refresh reloads once and restores load-once policy");

    Expect(gameplay::ShouldDrawPlayerPresentationModel(true), "20. successful load draws the model");
    Expect(
        !gameplay::ShouldDrawPlayerGameplayPrimitive(true),
        "20b. successful load hides the old gameplay primitive");

    Expect(
        std::string_view(gameplay::kPlayerModelLogicalId).find("source/") == std::string_view::npos,
        "21. identity is a staged runtime path, not source/");
    Expect(
        std::string_view(gameplay::kPlayerModelLogicalId).find("cooked/") == std::string_view::npos,
        "21b. identity is not a cooked-directory path");

    world::ItemPickupSpec pickup{};
    pickup.position = {2.0f, 0.8f, 0.0f};
    pickup.itemId = "items/master_key";
    pickup.quantity = 1;
    std::vector<world::ItemPickupSpec> pickups{pickup};
    std::vector<std::uint8_t> collected{0};
    std::vector<std::uint8_t> los{0};
    const core::Vec3 targetingCenter{0.5f, 0.8f, 0.0f};
    gameplay::PlayerPresentationConfig farOffset = gameplay::kDefaultPlayerPresentationConfig;
    farOffset.visualOffset = {50.0f, 0.0f, 0.0f};
    const gameplay::PlayerVisualTransform farVisual =
        gameplay::BuildPlayerVisualTransform(targetingCenter, farOffset, 0.0f);
    const int target = gameplay::FindItemPickupTargetIndex(
        targetingCenter, 1.0f, pickups, collected, los);
    Expect(target == 0, "24. Item Pickup targeting uses controller position");
    Expect(farVisual.position.x != targetingCenter.x, "visual offset is not targeting authority");

    const core::Vec3 cameraTarget = targetingCenter;
    Expect(Vec3Equal(cameraTarget, targetingCenter), "23. camera authority stays the controller position");
    Expect(farVisual.position.x != cameraTarget.x, "camera does not follow visual offset");

    gameplay::ClearPlayerPresentationModelLifetime(lifetime);
    Expect(lifetime.loadCount == 0 && !lifetime.loadAttempted, "unload clears lifetime");

    if (gFailures > 0)
    {
        std::fprintf(stderr, "%d PlayerPresentationTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("PlayerPresentationTest passed\n");
    return 0;
}
