// Milestone 71/72/73/74/75: semantic gameplay SFX requests, lethal-hit precedence,
// movement cadence, missing-cue safety, UI/menu feedback, and authored/Dirty isolation.
// No window, raylib, or Jolt.

#include "gameplay/GameplayAudio.h"
#include "gameplay/GameFlowState.h"
#include "gameplay/CollectibleRunState.h"
#include "gameplay/DoorLockRuntime.h"
#include "gameplay/Inventory.h"
#include "gameplay/InventoryUi.h"
#include "gameplay/ItemPickupRuntime.h"
#include "gameplay/LevelCompletionState.h"
#include "gameplay/LevelTransition.h"
#include "gameplay/PlayerDeath.h"
#include "gameplay/PlayerHealth.h"
#include "gameplay/RespawnState.h"
#include "input/InputState.h"
#include "platform/GameplayAudio.h"
#include "world/Door.h"
#include "world/HazardWorld.h"
#include "world/ItemPickup.h"
#include "world/LevelGoal.h"
#include "world/RespawnWorld.h"

#include <cstdio>
#include <cstdint>
#include <span>
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

world::ItemPickupSpec MakePickup(core::Vec3 position)
{
    world::ItemPickupSpec spec{};
    spec.position = position;
    spec.itemId = "key";
    spec.quantity = 1;
    return spec;
}

void MaybeRequestPickupSfx(
    gameplay::Inventory& inventory,
    gameplay::ItemPickupRunState& runState,
    gameplay::GameplaySfxRequestState& sfx,
    std::span<const world::ItemPickupSpec> pickups,
    int index)
{
    if (!gameplay::TryCollectItemPickup(inventory, runState, pickups, index))
    {
        return;
    }
    gameplay::RecordGameplaySfx(sfx, gameplay::PickupCollectionSfx());
}

void MaybeRequestCollectibleSfx(
    gameplay::CollectibleRunState& runState,
    gameplay::GameplaySfxRequestState& sfx,
    std::span<const world::CollectibleSpec> collectibles,
    core::Vec3 visualCenter)
{
    if (gameplay::TryCollectCollectible(runState, collectibles, visualCenter)
        == world::kNoCollectibleIndex)
    {
        return;
    }
    gameplay::RecordGameplaySfx(sfx, gameplay::CollectibleCollectionSfx());
}

gameplay::PlayerMovementSfxInput MakeMovementInput(
    bool allowed,
    bool grounded,
    float horizontalVelocity,
    float deltaSeconds,
    bool jumpAccepted = false,
    bool becameGrounded = false,
    float airborneSecondsAtStart = 0.0f)
{
    gameplay::PlayerMovementSfxInput input{};
    input.allowed = allowed;
    input.grounded = grounded;
    input.becameGrounded = becameGrounded;
    input.jumpAccepted = jumpAccepted;
    input.airborneSecondsAtStart = airborneSecondsAtStart;
    input.horizontalVelocity = horizontalVelocity;
    input.deltaSeconds = deltaSeconds;
    return input;
}

void RecordMovementTick(
    gameplay::PlayerMovementSfxState& tracker,
    gameplay::GameplaySfxRequestState& sfx,
    const gameplay::PlayerMovementSfxInput& input)
{
    gameplay::RecordGameplaySfx(sfx, gameplay::TickPlayerMovementSfx(tracker, input));
}

void StepHazardSfx(
    gameplay::PlayerHealthState& health,
    gameplay::HazardContactState& contact,
    gameplay::PlayerDeathState& death,
    gameplay::GameplaySfxRequestState& sfx,
    bool overlapping,
    float deltaSeconds,
    bool allowed)
{
    const int healthBefore = health.currentHealth;
    const bool applied = gameplay::TickHazardContactDamage(
        health, contact, overlapping, deltaSeconds, allowed);
    const bool beganDeath =
        gameplay::TryBeginPlayerDeath(death, healthBefore, health.currentHealth);
    gameplay::RecordGameplaySfx(
        sfx, gameplay::ResolveHazardOutcomeSfx(applied, beganDeath));
}
}

int main()
{
    Expect(
        platform::kGameplaySfxVolume == 1.0f, "fixed gameplay SFX volume is 1.0");
    Expect(
        platform::kGameplaySfxCueCount == 19,
        "fixed cue set includes M71-M74 plus six M75 UI cues");

    {
        world::ItemPickupSpec pickup = MakePickup({1.55f, 1.0f, 0.0f});
        std::vector<world::ItemPickupSpec> pickups{pickup};
        gameplay::Inventory inventory;
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        gameplay::GameplaySfxRequestState sfx{};
        MaybeRequestPickupSfx(inventory, run, sfx, pickups, 0);
        Expect(sfx.pickupCount == 1, "1. successful pickup emits exactly one pickup-audio request");
        Expect(
            sfx.collectibleCount == 0 && sfx.damageCount == 0 && sfx.deathCount == 0
                && sfx.respawnCount == 0 && sfx.footstepCount == 0 && sfx.jumpCount == 0
                && sfx.landingCount == 0,
            "successful pickup does not emit Collectible or survival/movement cues");
        Expect(inventory.GetQuantity("key") == 1, "pickup collection still mutates Inventory");
        Expect(run.collected[0] == 1, "pickup collection still marks collected");
    }

    {
        world::ItemPickupSpec pickup = MakePickup({1.55f, 1.0f, 0.0f});
        std::vector<world::ItemPickupSpec> pickups{pickup};
        gameplay::Inventory inventory;
        Expect(inventory.TryAdd("key", gameplay::kMaxItemQuantity), "seed inventory at cap");
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        gameplay::GameplaySfxRequestState sfx{};
        MaybeRequestPickupSfx(inventory, run, sfx, pickups, 0);
        Expect(sfx.pickupCount == 0, "2. failed pickup emits no pickup audio");
        Expect(run.collected[0] == 0, "failed pickup leaves the world item");
        Expect(
            inventory.GetQuantity("key") == gameplay::kMaxItemQuantity,
            "failed pickup does not change Inventory");
    }

    {
        world::CollectibleSpec collectible{};
        collectible.center = {2.0f, 1.0f, 0.0f};
        collectible.size = {1.0f, 1.2f, 1.0f};
        std::vector<world::CollectibleSpec> collectibles{collectible};
        gameplay::CollectibleRunState run =
            gameplay::MakeClearedCollectibleRunState(collectibles.size());
        gameplay::GameplaySfxRequestState sfx{};
        MaybeRequestCollectibleSfx(run, sfx, collectibles, collectible.center);
        Expect(
            sfx.collectibleCount == 1,
            "1. successful Collectible collection emits exactly one Collectible SFX request");
        Expect(
            sfx.pickupCount == 0 && sfx.checkpointActivateCount == 0 && sfx.doorUnlockCount == 0,
            "Collectible SFX is distinct from Item Pickup and M73 cues");
        Expect(run.collected[0] == 1, "successful collection still marks the Collectible collected");
        Expect(gameplay::CollectedCount(run) == 1, "Collectible count/progression remains 1");
        MaybeRequestCollectibleSfx(run, sfx, collectibles, collectible.center);
        Expect(
            sfx.collectibleCount == 1,
            "2. continued overlap after collection emits no repeated Collectible request");
        MaybeRequestCollectibleSfx(
            run, sfx, collectibles, {collectible.center.x + 4.0f, collectible.center.y, 0.0f});
        Expect(
            sfx.collectibleCount == 1,
            "3. proximity without collection emits no Collectible request");
    }

    {
        world::CollectibleSpec collectible{};
        collectible.center = {2.0f, 1.0f, 0.0f};
        collectible.size = {1.0f, 1.2f, 1.0f};
        std::vector<world::CollectibleSpec> collectibles{collectible};
        gameplay::CollectibleRunState already =
            gameplay::MakeClearedCollectibleRunState(collectibles.size());
        already.collected[0] = 1;
        gameplay::GameplaySfxRequestState sfx{};
        MaybeRequestCollectibleSfx(already, sfx, collectibles, collectible.center);
        Expect(sfx.collectibleCount == 0, "already-collected Collectible emits no SFX");
        Expect(already.collected[0] == 1, "already-collected flag is preserved");
    }

    {
        world::CollectibleSpec collectible{};
        collectible.center = {2.0f, 1.0f, 0.0f};
        collectible.size = {1.0f, 1.2f, 1.0f};
        std::vector<world::CollectibleSpec> collectibles{collectible};
        const char* lifecycleNames[] = {
            "initialization",
            "Restart reconstruction",
            "Apply/Reload",
            "Open/Switch",
            "physics rebuild",
            "Checkpoint respawn",
            "Health-death respawn",
            "Fall/Manual respawn",
            "Level transition load",
        };
        gameplay::GameplaySfxRequestState lifecycleSfx{};
        for (const char* name : lifecycleNames)
        {
            gameplay::CollectibleRunState reconstructed =
                gameplay::MakeClearedCollectibleRunState(collectibles.size());
            Expect(
                reconstructed.collected.size() == 1 && reconstructed.collected[0] == 0,
                "lifecycle reconstruction clears Collectible flags without collecting");
            Expect(
                lifecycleSfx.collectibleCount == 0 && lifecycleSfx.pickupCount == 0,
                "4. reconstruction/lifecycle does not synthesize Collectible audio");
            (void)name;
        }
        gameplay::CollectibleRunState paused =
            gameplay::MakeClearedCollectibleRunState(collectibles.size());
        paused.collected[0] = 1;
        gameplay::GameplaySfxRequestState blockedSfx{};
        MaybeRequestCollectibleSfx(paused, blockedSfx, collectibles, collectible.center);
        Expect(
            blockedSfx.collectibleCount == 0,
            "Pause/Inventory/F2 do not catch up Collectible audio");
        world::CollectibleSpec authoredBefore = collectible;
        bool dirty = false;
        gameplay::CollectibleRunState working =
            gameplay::MakeClearedCollectibleRunState(collectibles.size());
        MaybeRequestCollectibleSfx(working, blockedSfx, collectibles, collectible.center);
        Expect(
            collectible.center.x == authoredBefore.center.x && !dirty
                && collectibles[0].center.x == authoredBefore.center.x,
            "Collectible audio never mutates authored data, workingCopy, or Dirty");
    }

    gameplay::PlayerHealthState health{};
    gameplay::HazardContactState contact{};
    gameplay::PlayerDeathState death{};
    gameplay::GameplaySfxRequestState sfx{};
    gameplay::InitializePlayerHealth(health);

    StepHazardSfx(health, contact, death, sfx, true, 1.0f / 60.0f, true);
    Expect(sfx.damageCount == 1, "3. successful non-lethal damage emits exactly one damage request");
    Expect(sfx.deathCount == 0, "non-lethal damage does not emit death");
    Expect(
        health.currentHealth == gameplay::kMaxPlayerHealth - gameplay::kHazardDamageAmount,
        "damage audio follows actual Health loss");

    const int damageAfterFirst = sfx.damageCount;
    for (int frame = 0; frame < 45; ++frame)
    {
        StepHazardSfx(health, contact, death, sfx, true, 1.0f / 60.0f, true);
    }
    Expect(sfx.damageCount == damageAfterFirst, "4. Hazard cooldown frames emit no damage audio");
    Expect(!gameplay::PlayerDeathIsActive(death), "cooldown frames do not enter death");

    gameplay::PlayerHealthState blocked = health;
    gameplay::HazardContactState blockedContact = contact;
    gameplay::PlayerDeathState blockedDeath{};
    gameplay::GameplaySfxRequestState blockedSfx = sfx;
    StepHazardSfx(blocked, blockedContact, blockedDeath, blockedSfx, true, 1.0f / 60.0f, false);
    Expect(
        blockedSfx.damageCount == sfx.damageCount && blockedSfx.deathCount == 0,
        "5. blocked Hazard damage emits no damage audio");
    Expect(blocked.currentHealth == health.currentHealth, "blocked overlap does not change Health");

    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, false, false, false, false, true),
        "11. Pause blocks Hazard damage, so no damage audio");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, false, true, false, false, false),
        "11. Inventory blocks Hazard damage, so no damage audio");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, true, false, false, false, false),
        "11. F2/editor blocks Hazard damage, so no damage audio");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::MainMenu, false, false, false, false, false),
        "11. Main Menu blocks Hazard damage, so no damage audio");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, false, false, false, true, false),
        "11. LEVEL COMPLETE blocks Hazard damage, so no damage audio");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, false, false, true, true, false),
        "11. RUN COMPLETE blocks Hazard damage, so no damage audio");

    gameplay::PlayerHealthState lethalHealth{};
    gameplay::HazardContactState lethalContact{};
    gameplay::PlayerDeathState lethalDeath{};
    gameplay::GameplaySfxRequestState lethalSfx{};
    gameplay::InitializePlayerHealth(lethalHealth);
    lethalHealth.currentHealth = gameplay::kHazardDamageAmount;
    StepHazardSfx(lethalHealth, lethalContact, lethalDeath, lethalSfx, true, 1.0f / 60.0f, true);
    Expect(lethalHealth.currentHealth == 0, "lethal tick reaches zero Health");
    Expect(
        gameplay::PlayerDeathIsActive(lethalDeath),
        "7. successful death-phase entry happens on the lethal tick");
    Expect(
        lethalSfx.damageCount == 0 && lethalSfx.deathCount == 1,
        "6. lethal-hit rule B: death cue takes precedence, damage cue suppressed");
    Expect(lethalSfx.respawnCount == 0, "death entry does not emit the respawn cue");

    for (int frame = 0; frame < 8; ++frame)
    {
        StepHazardSfx(
            lethalHealth, lethalContact, lethalDeath, lethalSfx, true, 1.0f / 60.0f, true);
    }
    Expect(lethalSfx.deathCount == 1, "8. remaining in the death phase does not replay death audio");
    Expect(lethalSfx.damageCount == 0, "zero-Health overlap during death emits no damage audio");

    gameplay::RecordGameplaySfx(
        lethalSfx,
        gameplay::ResolveRespawnSfx(gameplay::GameplayRespawnAudioKind::HealthDeath));
    Expect(lethalSfx.respawnCount == 1, "9. completed Health-death respawn emits exactly one respawn cue");
    gameplay::RestorePlayerHealthAfterDeathRespawn(lethalHealth);
    gameplay::ClearPlayerDeath(lethalDeath);
    Expect(
        lethalHealth.currentHealth == gameplay::kMaxPlayerHealth,
        "Health-death respawn still restores maximum Health");

    gameplay::GameplaySfxRequestState fallSfx{};
    gameplay::RecordGameplaySfx(
        fallSfx, gameplay::ResolveRespawnSfx(gameplay::GameplayRespawnAudioKind::Fall));
    gameplay::RecordGameplaySfx(
        fallSfx, gameplay::ResolveRespawnSfx(gameplay::GameplayRespawnAudioKind::Manual));
    Expect(
        fallSfx.respawnCount == 0 && fallSfx.deathCount == 0 && fallSfx.damageCount == 0,
        "10. Fall/Manual R do not emit the Health-death respawn cue");

    gameplay::PlayerHealthState fallHealth{};
    gameplay::InitializePlayerHealth(fallHealth);
    gameplay::ApplyPlayerDamage(fallHealth, gameplay::kHazardDamageAmount);
    const int fallHealthBefore = fallHealth.currentHealth;
    Expect(
        fallHealth.currentHealth == fallHealthBefore,
        "Fall/Manual R still do not restore Health");

    gameplay::GameplaySfxRequestState missingSfx{};
    gameplay::PlayerHealthState missingHealth{};
    gameplay::InitializePlayerHealth(missingHealth);
    const int healthBeforeMissing = missingHealth.currentHealth;
    gameplay::RecordGameplaySfx(
        missingSfx, gameplay::ResolveHazardOutcomeSfx(true, false));
    Expect(missingSfx.damageCount == 1, "13. unavailable playback still records the semantic request");
    Expect(
        missingHealth.currentHealth == healthBeforeMissing,
        "13. missing audio does not change Health");
    gameplay::GameplaySfxEmit missingMove{};
    missingMove.footstep = true;
    missingMove.jump = true;
    missingMove.landing = true;
    gameplay::RecordGameplaySfx(missingSfx, missingMove);
    Expect(
        missingSfx.footstepCount == 1 && missingSfx.jumpCount == 1 && missingSfx.landingCount == 1,
        "19. unavailable movement playback still records the semantic request");

    std::vector<world::HazardSpec> authoredHazards{{{11.5f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}}};
    const world::HazardSpec authoredBefore = authoredHazards[0];
    bool dirty = false;
    std::vector<world::HazardSpec> workingCopy = authoredHazards;
    gameplay::RecordGameplaySfx(sfx, gameplay::PickupCollectionSfx());
    gameplay::RecordGameplaySfx(
        sfx, gameplay::ResolveHazardOutcomeSfx(true, false));
    Expect(
        authoredHazards[0].center.x == authoredBefore.center.x
            && authoredHazards[0].size.y == authoredBefore.size.y,
        "17. audio activity never mutates authored Hazard specs");
    Expect(!dirty, "17. audio activity never sets Dirty");
    Expect(
        workingCopy[0].center.x == authoredBefore.center.x,
        "17. audio activity never mutates workingCopy");

    gameplay::GameplaySfxRequestState lifecycle{};
    Expect(lifecycle.pickupCount == 0, "12. fresh Play starts with no SFX requests");
    gameplay::ClearGameplaySfxRequests(lifecycle);
    Expect(
        lifecycle.pickupCount == 0 && lifecycle.damageCount == 0 && lifecycle.deathCount == 0
            && lifecycle.respawnCount == 0 && lifecycle.footstepCount == 0
            && lifecycle.jumpCount == 0 && lifecycle.landingCount == 0
            && lifecycle.checkpointActivateCount == 0 && lifecycle.plateActivateCount == 0
            && lifecycle.plateDeactivateCount == 0 && lifecycle.doorUnlockCount == 0
            && lifecycle.levelGoalCompleteCount == 0,
        "12. Restart/Play/transition helpers do not fabricate SFX requests");

    const gameplay::GameplaySfxEmit emptyReload{};
    gameplay::RecordGameplaySfx(lifecycle, emptyReload);
    Expect(
        lifecycle.pickupCount == 0 && lifecycle.damageCount == 0 && lifecycle.footstepCount == 0
            && lifecycle.jumpCount == 0 && lifecycle.landingCount == 0,
        "12. Level transition/Restart/Apply/Reload emit no implicit gameplay SFX");

    Expect(
        gameplay::kFootstepStrideDistance == 2.0f
            && gameplay::kFootstepMinHorizontalSpeed == 1.0f
            && gameplay::kLandingMinAirborneSeconds == 0.12f,
        "M72 cadence/landing thresholds are the documented named constants");

    const float dt = 1.0f / 60.0f;
    const float walkSpeed = 6.0f;
    gameplay::PlayerMovementSfxState walkTracker{};
    gameplay::GameplaySfxRequestState walkSfx{};
    gameplay::ReanchorPlayerMovementSfx(walkTracker, true);
    int firstFootstepFrame = -1;
    for (int frame = 0; frame < 40; ++frame)
    {
        RecordMovementTick(
            walkTracker, walkSfx, MakeMovementInput(true, true, walkSpeed, dt));
        if (firstFootstepFrame < 0 && walkSfx.footstepCount > 0)
        {
            firstFootstepFrame = frame;
        }
        Expect(walkSfx.footstepCount <= frame + 1, "4. footsteps never emit every frame");
    }
    Expect(walkSfx.footstepCount >= 1, "1. grounded meaningful movement emits Footstep requests");
    Expect(firstFootstepFrame == 19, "1. first footstep is at the 2.0 stride distance");
    Expect(walkSfx.footstepCount == 2, "1. cadence is one footstep per 2.0 units at 6 m/s");
    Expect(
        walkSfx.jumpCount == 0 && walkSfx.landingCount == 0,
        "grounded walking does not emit Jump/Landing");

    gameplay::PlayerMovementSfxState idleTracker{};
    gameplay::GameplaySfxRequestState idleSfx{};
    gameplay::ReanchorPlayerMovementSfx(idleTracker, true);
    for (int frame = 0; frame < 30; ++frame)
    {
        RecordMovementTick(idleTracker, idleSfx, MakeMovementInput(true, true, 0.0f, dt));
    }
    Expect(idleSfx.footstepCount == 0, "2. stationary grounded player emits no Footstep requests");

    gameplay::PlayerMovementSfxState airTracker{};
    gameplay::GameplaySfxRequestState airSfx{};
    gameplay::ReanchorPlayerMovementSfx(airTracker, false);
    for (int frame = 0; frame < 30; ++frame)
    {
        RecordMovementTick(airTracker, airSfx, MakeMovementInput(true, false, walkSpeed, dt));
    }
    Expect(airSfx.footstepCount == 0, "3. airborne player emits no Footstep requests");

    gameplay::PlayerMovementSfxState pauseTracker{};
    gameplay::GameplaySfxRequestState pauseSfx{};
    gameplay::ReanchorPlayerMovementSfx(pauseTracker, true);
    for (int frame = 0; frame < 19; ++frame)
    {
        RecordMovementTick(
            pauseTracker, pauseSfx, MakeMovementInput(true, true, walkSpeed, dt));
    }
    Expect(pauseSfx.footstepCount == 0, "almost one stride accumulated before Pause");
    for (int frame = 0; frame < 60; ++frame)
    {
        RecordMovementTick(
            pauseTracker, pauseSfx, MakeMovementInput(false, true, walkSpeed, dt));
    }
    Expect(pauseSfx.footstepCount == 0, "5. Pause/Inventory/F2/death blocking emits no Footstep");
    RecordMovementTick(
        pauseTracker, pauseSfx, MakeMovementInput(true, true, walkSpeed, dt));
    Expect(
        pauseSfx.footstepCount == 0,
        "5. Footstep cadence does not catch up after blocking re-anchor");

    gameplay::PlayerMovementSfxState jumpTracker{};
    gameplay::GameplaySfxRequestState jumpSfx{};
    gameplay::ReanchorPlayerMovementSfx(jumpTracker, true);
    RecordMovementTick(
        jumpTracker,
        jumpSfx,
        MakeMovementInput(true, false, 0.0f, dt, true, false, 0.0f));
    Expect(jumpSfx.jumpCount == 1, "6. a real accepted jump emits exactly one Jump request");
    RecordMovementTick(
        jumpTracker, jumpSfx, MakeMovementInput(true, false, 0.0f, dt, false));
    Expect(jumpSfx.jumpCount == 1, "accepted jump does not retrigger without a new acceptance");

    gameplay::PlayerMovementSfxState rejectedJump{};
    gameplay::GameplaySfxRequestState rejectedJumpSfx{};
    gameplay::ReanchorPlayerMovementSfx(rejectedJump, false);
    RecordMovementTick(
        rejectedJump, rejectedJumpSfx, MakeMovementInput(true, false, 0.0f, dt, false));
    Expect(rejectedJumpSfx.jumpCount == 0, "7. rejected/airborne jump input emits no Jump request");

    gameplay::PlayerMovementSfxState blockedJump{};
    gameplay::GameplaySfxRequestState blockedJumpSfx{};
    gameplay::ReanchorPlayerMovementSfx(blockedJump, true);
    RecordMovementTick(
        blockedJump, blockedJumpSfx, MakeMovementInput(false, true, 0.0f, dt, true));
    Expect(blockedJumpSfx.jumpCount == 0, "8. blocked jump input emits no Jump request");

    gameplay::PlayerMovementSfxState groundedLand{};
    gameplay::GameplaySfxRequestState groundedLandSfx{};
    gameplay::ReanchorPlayerMovementSfx(groundedLand, true);
    for (int frame = 0; frame < 10; ++frame)
    {
        RecordMovementTick(groundedLand, groundedLandSfx, MakeMovementInput(true, true, 0.0f, dt));
    }
    Expect(groundedLandSfx.landingCount == 0, "9. ordinary grounded frames emit no Landing request");

    gameplay::PlayerMovementSfxState landTracker{};
    gameplay::GameplaySfxRequestState landSfx{};
    gameplay::ReanchorPlayerMovementSfx(landTracker, false);
    RecordMovementTick(
        landTracker,
        landSfx,
        MakeMovementInput(true, true, 0.0f, dt, false, true, 0.20f));
    Expect(landSfx.landingCount == 1, "10. meaningful airborne-to-grounded emits one Landing");
    RecordMovementTick(landTracker, landSfx, MakeMovementInput(true, true, 0.0f, dt));
    Expect(landSfx.landingCount == 1, "landing does not repeat on following grounded frames");

    gameplay::PlayerMovementSfxState jitterTracker{};
    gameplay::GameplaySfxRequestState jitterSfx{};
    gameplay::ReanchorPlayerMovementSfx(jitterTracker, false);
    RecordMovementTick(
        jitterTracker,
        jitterSfx,
        MakeMovementInput(true, true, 0.0f, dt, false, true, 0.05f));
    Expect(
        jitterSfx.landingCount == 0,
        "11. tiny jitter/insufficient airborne movement emits no Landing");

    gameplay::PlayerMovementSfxState spawnTracker{};
    gameplay::GameplaySfxRequestState spawnSfx{};
    RecordMovementTick(
        spawnTracker,
        spawnSfx,
        MakeMovementInput(true, true, 0.0f, dt, false, true, 1.0f));
    Expect(spawnSfx.landingCount == 0, "12. startup/spawn does not synthesize Landing");
    Expect(spawnTracker.anchored, "unanchored first tick re-anchors presentation state");

    gameplay::PlayerMovementSfxState deathRespawnTracker{};
    gameplay::GameplaySfxRequestState deathRespawnSfx{};
    deathRespawnTracker.strideDistanceAccumulated = 1.9f;
    deathRespawnTracker.previousGrounded = false;
    deathRespawnTracker.anchored = true;
    gameplay::RecordGameplaySfx(
        deathRespawnSfx,
        gameplay::ResolveRespawnSfx(gameplay::GameplayRespawnAudioKind::HealthDeath));
    gameplay::ReanchorPlayerMovementSfx(deathRespawnTracker, true);
    RecordMovementTick(
        deathRespawnTracker,
        deathRespawnSfx,
        MakeMovementInput(true, true, walkSpeed, dt));
    Expect(deathRespawnSfx.respawnCount == 1, "Health-death still emits the M71 respawn cue");
    Expect(
        deathRespawnSfx.jumpCount == 0 && deathRespawnSfx.landingCount == 0
            && deathRespawnSfx.footstepCount == 0,
        "13. Health-death respawn re-anchor synthesizes no Jump/Landing/Footstep");

    gameplay::PlayerMovementSfxState fallTracker{};
    gameplay::GameplaySfxRequestState fallAfter{};
    gameplay::RecordGameplaySfx(
        fallAfter, gameplay::ResolveRespawnSfx(gameplay::GameplayRespawnAudioKind::Fall));
    gameplay::ReanchorPlayerMovementSfx(fallTracker, true);
    RecordMovementTick(
        fallTracker, fallAfter, MakeMovementInput(true, true, walkSpeed, dt));
    Expect(
        fallAfter.respawnCount == 0 && fallAfter.jumpCount == 0 && fallAfter.landingCount == 0
            && fallAfter.footstepCount == 0,
        "14. Fall respawn synthesizes no Jump/Landing/Footstep or Health-death Respawn");

    const char* lifecycleNames[] = {
        "Restart",
        "Play",
        "Play Again",
        "Level transition",
        "Apply/Reload",
        "Open/Switch",
        "physics rebuild",
    };
    for (const char* name : lifecycleNames)
    {
        gameplay::PlayerMovementSfxState rebuilt{};
        gameplay::GameplaySfxRequestState rebuiltSfx{};
        rebuilt.strideDistanceAccumulated = 8.0f;
        rebuilt.previousGrounded = false;
        rebuilt.anchored = true;
        gameplay::ReanchorPlayerMovementSfx(rebuilt, true);
        RecordMovementTick(
            rebuilt, rebuiltSfx, MakeMovementInput(true, true, walkSpeed, dt));
        Expect(
            rebuiltSfx.jumpCount == 0 && rebuiltSfx.landingCount == 0
                && rebuiltSfx.footstepCount == 0,
            "15. lifecycle re-anchor synthesizes no movement cue");
        (void)name;
    }

    gameplay::PlayerMovementSfxState blockedFlow{};
    gameplay::GameplaySfxRequestState blockedFlowSfx{};
    gameplay::ReanchorPlayerMovementSfx(blockedFlow, true);
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, false, false, false, false, true),
        "16. Pause uses existing gameplay blocker, not an audio pause authority");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, false, true, false, false, false),
        "16. Inventory uses existing gameplay blocker");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, true, false, false, false, false),
        "16. F2/editor uses existing gameplay blocker");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::MainMenu, false, false, false, false, false),
        "16. Main Menu uses existing gameplay blocker");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, false, false, false, true, false),
        "16. LEVEL COMPLETE uses existing gameplay blocker");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, false, false, true, true, false),
        "16. RUN COMPLETE uses existing gameplay blocker");
    RecordMovementTick(
        blockedFlow, blockedFlowSfx, MakeMovementInput(false, true, walkSpeed, dt, true, true, 1.0f));
    Expect(
        blockedFlowSfx.footstepCount == 0 && blockedFlowSfx.jumpCount == 0
            && blockedFlowSfx.landingCount == 0,
        "16. Pause/Inventory/F2/MainMenu/LEVEL COMPLETE/RUN COMPLETE emit no movement SFX");

    gameplay::PlayerMovementSfxState dirtyTracker{};
    gameplay::ReanchorPlayerMovementSfx(dirtyTracker, true);
    RecordMovementTick(
        dirtyTracker, sfx, MakeMovementInput(true, true, walkSpeed, dt));
    Expect(
        authoredHazards[0].center.x == authoredBefore.center.x && !dirty
            && workingCopy[0].center.x == authoredBefore.center.x,
        "21. movement audio never mutates authored data, workingCopy, or Dirty");

    world::CheckpointSpec checkpointA{};
    checkpointA.center = {0.0f, 1.0f, 0.0f};
    checkpointA.size = {2.0f, 2.0f, 2.0f};
    checkpointA.respawnPosition = {0.0f, 1.0f, 0.0f};
    world::CheckpointSpec checkpointB = checkpointA;
    checkpointB.center.x = 8.0f;
    checkpointB.respawnPosition.x = 8.0f;
    std::vector<world::CheckpointSpec> checkpoints{checkpointA, checkpointB};

    gameplay::RespawnState checkpointRespawn{};
    gameplay::GameplaySfxRequestState checkpointSfx{};
    Expect(
        world::TryActivateExpectedCheckpoint(
            checkpointRespawn.activeCheckpointIndex,
            checkpointRespawn.respawnPosition,
            checkpoints,
            checkpointA.center),
        "genuine sequential Checkpoint activation succeeds");
    gameplay::RecordGameplaySfx(checkpointSfx, gameplay::CheckpointActivatedSfx());
    Expect(
        checkpointSfx.checkpointActivateCount == 1,
        "1. genuine Checkpoint activation emits exactly one Checkpoint SFX request");
    Expect(
        !world::TryActivateExpectedCheckpoint(
            checkpointRespawn.activeCheckpointIndex,
            checkpointRespawn.respawnPosition,
            checkpoints,
            checkpointA.center),
        "continued overlap of the current Checkpoint is not a new activation");
    Expect(
        checkpointSfx.checkpointActivateCount == 1,
        "2. continued overlap with the same active Checkpoint emits no repeat");

    gameplay::RespawnState restoredCheckpoint{};
    restoredCheckpoint.activeCheckpointIndex = 0;
    restoredCheckpoint.respawnPosition = checkpointA.respawnPosition;
    gameplay::GameplaySfxRequestState restoredCheckpointSfx{};
    Expect(
        !world::TryActivateExpectedCheckpoint(
            restoredCheckpoint.activeCheckpointIndex,
            restoredCheckpoint.respawnPosition,
            checkpoints,
            checkpointA.center),
        "preserved active Checkpoint does not reactivate from overlap");
    Expect(
        restoredCheckpointSfx.checkpointActivateCount == 0,
        "3. restored/preserved active Checkpoint does not synthesize activation audio");

    gameplay::RespawnState laterCheckpoint = restoredCheckpoint;
    Expect(
        world::TryActivateExpectedCheckpoint(
            laterCheckpoint.activeCheckpointIndex,
            laterCheckpoint.respawnPosition,
            checkpoints,
            checkpointB.center),
        "a later sequential Checkpoint can still activate");
    gameplay::RecordGameplaySfx(checkpointSfx, gameplay::CheckpointActivatedSfx());
    Expect(
        checkpointSfx.checkpointActivateCount == 2,
        "returning to a previous Checkpoint is not invented; next expected still activates");

    std::vector<std::uint8_t> plateInactive{0};
    std::vector<std::uint8_t> plateActive{1};
    gameplay::PressurePlateSfxState plateTracker{};
    gameplay::GameplaySfxRequestState plateSfx{};
    gameplay::SynchronizePressurePlateSfx(plateTracker, plateInactive);
    gameplay::RecordGameplaySfx(
        plateSfx, gameplay::TickPressurePlateSfx(plateTracker, plateActive));
    Expect(
        plateSfx.plateActivateCount == 1 && plateSfx.plateDeactivateCount == 0,
        "4. Pressure Plate inactive -> active emits exactly one Activate request");
    gameplay::RecordGameplaySfx(
        plateSfx, gameplay::TickPressurePlateSfx(plateTracker, plateActive));
    Expect(
        plateSfx.plateActivateCount == 1,
        "5. held-active Pressure Plate emits no repeated Activate request");
    gameplay::RecordGameplaySfx(
        plateSfx, gameplay::TickPressurePlateSfx(plateTracker, plateInactive));
    Expect(
        plateSfx.plateDeactivateCount == 1 && plateSfx.plateActivateCount == 1,
        "6. Pressure Plate active -> inactive emits exactly one Deactivate request");
    gameplay::RecordGameplaySfx(
        plateSfx, gameplay::TickPressurePlateSfx(plateTracker, plateInactive));
    Expect(
        plateSfx.plateDeactivateCount == 1,
        "7. held-inactive Pressure Plate emits no repeated Deactivate request");
    Expect(
        plateSfx.doorUnlockCount == 0,
        "11. Plate-driven Door movement emits no Door Unlock request");

    world::DoorSpec lockedDoor{};
    lockedDoor.center = {2.0f, 1.5f, 0.0f};
    lockedDoor.size = world::kDefaultDoorSize;
    lockedDoor.openDistance = world::kDefaultDoorOpenDistance;
    lockedDoor.requiredItemId = "key";
    std::vector<world::DoorSpec> doors{lockedDoor};
    gameplay::DoorLockRunState locks = gameplay::MakeDoorLockRunState(doors);
    gameplay::Inventory doorInventory;
    gameplay::GameplaySfxRequestState doorSfx{};
    Expect(
        !gameplay::TryUnlockLockedDoor(doorInventory, locks, doors, 0),
        "missing item fails unlock");
    Expect(doorSfx.doorUnlockCount == 0, "9. missing-item / failed unlock emits no Door Unlock request");
    Expect(doorInventory.TryAdd("key", 1), "seed required item");
    Expect(gameplay::TryUnlockLockedDoor(doorInventory, locks, doors, 0), "successful item-gated unlock");
    gameplay::RecordGameplaySfx(doorSfx, gameplay::DoorUnlockSfx());
    Expect(
        doorSfx.doorUnlockCount == 1 && doorInventory.GetQuantity("key") == 0
            && gameplay::DoorIsRuntimeUnlocked(locks, 0),
        "8. successful item-gated Door unlock emits exactly one Door Unlock request");
    Expect(
        !gameplay::TryUnlockLockedDoor(doorInventory, locks, doors, 0),
        "already-unlocked Door does not unlock again");
    Expect(doorSfx.doorUnlockCount == 1, "10. already-unlocked Door interaction emits no replay");
    Expect(doors[0].requiredItemId == "key", "Door unlock audio does not mutate authored requiredItemId");

    world::LevelGoalSpec destinationGoal{{-21.0f, 3.8f, 0.0f}, {2.0f, 1.6f, 1.8f}, "level_02"};
    world::LevelGoalSpec terminalGoal{{-21.0f, 3.8f, 0.0f}, {2.0f, 1.6f, 1.8f}, {}};
    gameplay::LevelCompletionState destinationCompletion{};
    gameplay::GameplaySfxRequestState destinationSfx{};
    std::string destinationId;
    Expect(
        gameplay::TryCompleteLevelFromPlayerOverlap(
            destinationCompletion, {destinationGoal}, destinationGoal.center, &destinationId),
        "destination goal completes once");
    gameplay::RecordGameplaySfx(destinationSfx, gameplay::LevelGoalCompleteSfx());
    Expect(
        destinationSfx.levelGoalCompleteCount == 1,
        "12. destination Level Goal first completion emits exactly one Goal Complete request");
    gameplay::LevelTransitionSchedule hold{};
    gameplay::CaptureCompletedGoalDestination(hold, destinationId);
    for (int frame = 0; frame < 30; ++frame)
    {
        Expect(
            !gameplay::TryCompleteLevelFromPlayerOverlap(
                destinationCompletion, {destinationGoal}, destinationGoal.center, &destinationId),
            "LEVEL COMPLETE overlap does not complete again");
        gameplay::TickDestinationTransitionHold(hold, 1.0f / 60.0f);
    }
    Expect(
        destinationSfx.levelGoalCompleteCount == 1,
        "14. LEVEL COMPLETE hold emits no repeated Goal request");
    gameplay::SkipDestinationTransitionHold(hold);
    Expect(
        destinationSfx.levelGoalCompleteCount == 1,
        "15. deferred destination transition emits no second Goal request");

    gameplay::LevelCompletionState terminalCompletion{};
    gameplay::GameplaySfxRequestState terminalSfx{};
    std::string terminalId = "stale";
    Expect(
        gameplay::TryCompleteLevelFromPlayerOverlap(
            terminalCompletion, {terminalGoal}, terminalGoal.center, &terminalId),
        "terminal goal completes once");
    gameplay::RecordGameplaySfx(terminalSfx, gameplay::LevelGoalCompleteSfx());
    gameplay::RunCompleteState runComplete{};
    Expect(
        gameplay::TryEnterRunCompleteFromTerminalGoal(
            runComplete, terminalId, 12.5),
        "terminal completion enters RUN COMPLETE");
    Expect(
        terminalSfx.levelGoalCompleteCount == 1,
        "13. terminal Level Goal first completion emits exactly one Goal Complete request");
    for (int frame = 0; frame < 16; ++frame)
    {
        Expect(
            !gameplay::TryCompleteLevelFromPlayerOverlap(
                terminalCompletion, {terminalGoal}, terminalGoal.center, &terminalId),
            "RUN COMPLETE overlap does not complete again");
        Expect(
            !gameplay::TryEnterRunCompleteFromTerminalGoal(runComplete, terminalId, 99.0),
            "RUN COMPLETE does not re-enter");
    }
    Expect(
        terminalSfx.levelGoalCompleteCount == 1,
        "16. RUN COMPLETE emits no repeated Goal request");

    gameplay::PressurePlateSfxState rebuiltPlates{};
    rebuiltPlates.previousActive = {0, 0};
    gameplay::GameplaySfxRequestState rebuiltWorldSfx{};
    const char* worldLifecycleNames[] = {
        "initialization",
        "Restart",
        "transition",
        "Apply/Reload",
        "Open/Switch",
        "physics rebuild",
        "respawn",
    };
    for (const char* name : worldLifecycleNames)
    {
        std::vector<std::uint8_t> reconstructed{1, 0};
        gameplay::SynchronizePressurePlateSfx(rebuiltPlates, reconstructed);
        gameplay::RecordGameplaySfx(
            rebuiltWorldSfx, gameplay::TickPressurePlateSfx(rebuiltPlates, reconstructed));
        Expect(
            rebuiltWorldSfx.plateActivateCount == 0 && rebuiltWorldSfx.plateDeactivateCount == 0
                && rebuiltWorldSfx.checkpointActivateCount == 0
                && rebuiltWorldSfx.doorUnlockCount == 0
                && rebuiltWorldSfx.levelGoalCompleteCount == 0
                && rebuiltWorldSfx.collectibleCount == 0,
            "17. reconstruction/lifecycle does not synthesize world-interaction SFX");
        (void)name;
    }

    gameplay::PressurePlateSfxState blockedPlates{};
    gameplay::GameplaySfxRequestState blockedWorldSfx{};
    gameplay::SynchronizePressurePlateSfx(blockedPlates, plateInactive);
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, false, false, false, false, true),
        "18. Pause uses existing gameplay blocker, not an audio pause authority");
    gameplay::SynchronizePressurePlateSfx(blockedPlates, plateActive);
    gameplay::RecordGameplaySfx(
        blockedWorldSfx, gameplay::TickPressurePlateSfx(blockedPlates, plateActive));
    Expect(
        blockedWorldSfx.plateActivateCount == 0 && blockedWorldSfx.plateDeactivateCount == 0
            && blockedWorldSfx.checkpointActivateCount == 0,
        "18. Pause/Inventory/F2 blockers do not create catch-up/replay");

    Expect(
        checkpointSfx.pickupCount == 0 && checkpointSfx.footstepCount == 0
            && plateSfx.jumpCount == 0 && doorSfx.landingCount == 0,
        "19. existing M71/M72 cues remain independent of world-interaction requests");

    gameplay::RecordGameplaySfx(sfx, gameplay::CheckpointActivatedSfx());
    gameplay::RecordGameplaySfx(sfx, gameplay::DoorUnlockSfx());
    gameplay::RecordGameplaySfx(sfx, gameplay::LevelGoalCompleteSfx());
    Expect(
        authoredHazards[0].center.x == authoredBefore.center.x && !dirty
            && workingCopy[0].center.x == authoredBefore.center.x
            && doors[0].requiredItemId == "key",
        "23. M73 audio never mutates authored data, workingCopy, or Dirty");

    gameplay::TopLevelFlow menuFlow = gameplay::TopLevelFlow::MainMenu;
    gameplay::MainMenuState menu{};
    gameplay::GameplaySfxRequestState menuSfx{};
    const gameplay::MainMenuItem menuBeforeDown = menu.selected;
    Expect(
        gameplay::ResolveMainMenuInput(true, false, true, false, menu, menuFlow)
            == gameplay::MainMenuInputAction::None,
        "Main Menu Down navigates without activating");
    Expect(menu.selected == gameplay::MainMenuItem::Quit, "Down selects QUIT");
    if (menuBeforeDown != menu.selected)
    {
        gameplay::RecordGameplaySfx(menuSfx, gameplay::UiNavigateSfx());
    }
    Expect(menuSfx.uiNavigateCount == 1, "1. genuine Main Menu selection change emits one UiNavigate");
    Expect(
        menuSfx.uiConfirmCount == 0 && menuSfx.pauseOpenCount == 0,
        "Navigate does not emit Confirm or Pause cues");

    const gameplay::MainMenuItem menuBeforeBoth = menu.selected;
    Expect(
        gameplay::ResolveMainMenuInput(true, true, true, false, menu, menuFlow)
            == gameplay::MainMenuInputAction::None,
        "Up+Down does not activate");
    if (menuBeforeBoth != menu.selected)
    {
        gameplay::RecordGameplaySfx(menuSfx, gameplay::UiNavigateSfx());
    }
    Expect(menu.selected == gameplay::MainMenuItem::Quit, "Up+Down leaves QUIT selected");
    Expect(menuSfx.uiNavigateCount == 1, "2. input that does not change selection emits no UiNavigate");

    const gameplay::MainMenuItem menuBeforePlay = menu.selected;
    Expect(
        gameplay::ResolveMainMenuInput(true, false, true, true, menu, menuFlow)
            == gameplay::MainMenuInputAction::Quit,
        "Enter activates the captured QUIT item without also moving");
    if (menuBeforePlay != menu.selected)
    {
        gameplay::RecordGameplaySfx(menuSfx, gameplay::UiNavigateSfx());
    }
    Expect(menuSfx.uiNavigateCount == 1, "activate does not emit a false Navigate");
    gameplay::RequestQuitFromMainMenu(menu, menuFlow);
    gameplay::RecordGameplaySfx(menuSfx, gameplay::UiConfirmSfx());
    Expect(menu.quitRequested, "QUIT remains the existing close request");
    Expect(menuSfx.uiConfirmCount == 1, "QUIT activation emits exactly one UiConfirm");

    gameplay::ResetMainMenuState(menu);
    gameplay::GameplaySfxRequestState playSfx{};
    Expect(
        gameplay::ResolveMainMenuInput(true, false, false, true, menu, menuFlow)
            == gameplay::MainMenuInputAction::Play,
        "Enter activates PLAY");
    gameplay::RequestPlayFromMainMenu(menu, menuFlow);
    gameplay::RecordGameplaySfx(playSfx, gameplay::UiConfirmSfx());
    Expect(gameplay::PlayIsInFlight(menu), "3. PLAY confirm preserves fresh-run pending");
    Expect(playSfx.uiConfirmCount == 1, "PLAY activation emits exactly one UiConfirm");
    Expect(playSfx.uiNavigateCount == 0, "PLAY activation does not emit Navigate");
    gameplay::RequestPlayFromMainMenu(menu, menuFlow);
    Expect(playSfx.uiConfirmCount == 1, "in-flight PLAY does not emit a second Confirm");
    gameplay::PauseMenuState playPause{};
    gameplay::EnterGameplayFromSuccessfulPlay(menuFlow, menu, playPause);
    Expect(menuFlow == gameplay::TopLevelFlow::Gameplay, "deferred PLAY still enters Gameplay");
    Expect(playSfx.pauseOpenCount == 0 && playSfx.pauseCloseCount == 0,
        "PLAY deferred transition synthesizes no Pause cues");

    gameplay::PauseMenuState pause{};
    gameplay::GameplaySfxRequestState uiPauseSfx{};
    Expect(
        gameplay::PauseCanBeEntered(
            gameplay::TopLevelFlow::Gameplay, false, false, false, false, false),
        "Gameplay can enter Pause");
    Expect(
        gameplay::ResolvePauseInput(
            false, true, false, false, false, true, pause, gameplay::TopLevelFlow::Gameplay)
            == gameplay::PauseMenuInputAction::EnterPause,
        "Gameplay Esc is the EnterPause action");
    const bool pauseWasActive = pause.active;
    gameplay::EnterPause(pause);
    if (!pauseWasActive && pause.active)
    {
        gameplay::RecordGameplaySfx(uiPauseSfx, gameplay::PauseOpenSfx());
    }
    Expect(uiPauseSfx.pauseOpenCount == 1, "5. successful Gameplay -> Pause emits one PauseOpen");
    Expect(uiPauseSfx.pauseCloseCount == 0, "entering Pause does not emit PauseClose");

    const gameplay::PauseMenuItem pauseBeforeDown = pause.selected;
    Expect(
        gameplay::ResolvePauseInput(
            true, false, false, true, false, false, pause, gameplay::TopLevelFlow::Gameplay)
            == gameplay::PauseMenuInputAction::None,
        "Pause Down navigates without activating");
    if (pauseBeforeDown != pause.selected)
    {
        gameplay::RecordGameplaySfx(uiPauseSfx, gameplay::UiNavigateSfx());
    }
    Expect(pause.selected == gameplay::PauseMenuItem::MainMenu, "Down selects MAIN MENU");
    Expect(uiPauseSfx.uiNavigateCount == 1, "4. genuine Pause selection change emits one UiNavigate");

    const gameplay::PauseMenuItem pauseBeforeBoth = pause.selected;
    Expect(
        gameplay::ResolvePauseInput(
            true, false, true, true, false, false, pause, gameplay::TopLevelFlow::Gameplay)
            == gameplay::PauseMenuInputAction::None,
        "Pause Up+Down does not activate");
    if (pauseBeforeBoth != pause.selected)
    {
        gameplay::RecordGameplaySfx(uiPauseSfx, gameplay::UiNavigateSfx());
    }
    Expect(uiPauseSfx.uiNavigateCount == 1, "Pause input that does not change selection is silent");

    Expect(
        gameplay::ResolvePauseInput(
            true, false, false, false, true, false, pause, gameplay::TopLevelFlow::Gameplay)
            == gameplay::PauseMenuInputAction::ReturnToMainMenu,
        "Enter on MAIN MENU is the accepted selected action");
    gameplay::RecordGameplaySfx(uiPauseSfx, gameplay::UiConfirmSfx());
    gameplay::MainMenuState pauseMenu{};
    gameplay::RunCompleteState pauseResults{};
    gameplay::TopLevelFlow pauseFlow = gameplay::TopLevelFlow::Gameplay;
    gameplay::EnterMainMenu(pauseFlow, pauseMenu, pauseResults, pause);
    Expect(pauseFlow == gameplay::TopLevelFlow::MainMenu, "Pause MAIN MENU still enters Main Menu");
    Expect(!pause.active, "Pause is cleared by EnterMainMenu reconstruction");
    Expect(
        uiPauseSfx.uiConfirmCount == 1 && uiPauseSfx.pauseCloseCount == 0,
        "9. Pause MAIN MENU emits Confirm and does not synthesize PauseClose");

    gameplay::EnterPause(pause);
    gameplay::GameplaySfxRequestState escResumeSfx{};
    Expect(
        gameplay::ResolvePauseInput(
            true, false, false, false, false, true, pause, gameplay::TopLevelFlow::Gameplay)
            == gameplay::PauseMenuInputAction::Resume,
        "Esc while paused is Resume, not ActivateResume");
    const bool escWasActive = pause.active;
    gameplay::ResumePause(pause);
    if (escWasActive && !pause.active)
    {
        gameplay::RecordGameplaySfx(escResumeSfx, gameplay::PauseEscResumeSfx());
    }
    Expect(
        escResumeSfx.pauseCloseCount == 1 && escResumeSfx.uiConfirmCount == 0,
        "7. Pause Esc -> Gameplay emits PauseClose only");

    gameplay::EnterPause(pause);
    Expect(pause.selected == gameplay::PauseMenuItem::Resume, "RESUME is the default Pause selection");
    gameplay::GameplaySfxRequestState enterResumeSfx{};
    Expect(
        gameplay::ResolvePauseInput(
            true, false, false, false, true, false, pause, gameplay::TopLevelFlow::Gameplay)
            == gameplay::PauseMenuInputAction::ActivateResume,
        "Enter on RESUME is ActivateResume");
    const bool enterWasActive = pause.active;
    gameplay::ResumePause(pause);
    if (enterWasActive && !pause.active)
    {
        gameplay::RecordGameplaySfx(enterResumeSfx, gameplay::PauseActivatedResumeSfx());
    }
    Expect(
        enterResumeSfx.uiConfirmCount == 1 && enterResumeSfx.pauseCloseCount == 1,
        "8. Pause RESUME activation emits UiConfirm plus PauseClose exactly once");
    Expect(enterResumeSfx.pauseOpenCount == 0, "RESUME does not emit PauseOpen");

    gameplay::PauseMenuState blockedPause{};
    gameplay::GameplaySfxRequestState blockedPauseSfx{};
    Expect(
        !gameplay::PauseCanBeEntered(
            gameplay::TopLevelFlow::Gameplay, false, true, false, false, false),
        "Inventory blocks Pause entry");
    Expect(
        gameplay::ResolvePauseInput(
            false,
            false,
            false,
            false,
            false,
            true,
            blockedPause,
            gameplay::TopLevelFlow::Gameplay)
            == gameplay::PauseMenuInputAction::None,
        "blocked Esc is not EnterPause");
    Expect(!blockedPause.active, "blocked Pause entry leaves Pause inactive");
    Expect(blockedPauseSfx.pauseOpenCount == 0, "6. blocked Pause entry emits no PauseOpen");
    Expect(
        !gameplay::PauseCanBeEntered(
            gameplay::TopLevelFlow::Gameplay, false, false, false, true, false),
        "destination LEVEL COMPLETE blocks Pause");
    Expect(
        !gameplay::PauseCanBeEntered(
            gameplay::TopLevelFlow::Gameplay, false, false, true, true, false),
        "Run Complete blocks Pause");
    Expect(
        !gameplay::PauseCanBeEntered(
            gameplay::TopLevelFlow::Gameplay, false, false, false, false, false, true),
        "death blocks Pause");
    Expect(
        gameplay::FlowAfterEditorToggle(gameplay::TopLevelFlow::Gameplay)
            == gameplay::TopLevelFlow::Gameplay,
        "F2 keeps Gameplay flow");
    Expect(blockedPauseSfx.pauseOpenCount == 0 && blockedPauseSfx.pauseCloseCount == 0,
        "F2/lifecycle blockers synthesize no Pause UI cues");

    gameplay::Inventory inventory;
    gameplay::InventoryUiState inventoryUi{};
    gameplay::GameplaySfxRequestState inventorySfx{};
    input::InputState toggle{};
    toggle.toggleInventoryPressed = true;
    Expect(
        gameplay::HandleInventoryUiInput(inventoryUi, inventory, toggle)
            == gameplay::InventoryUiInputAction::Open,
        "available Tab is the Inventory Open action");
    gameplay::RecordGameplaySfx(inventorySfx, gameplay::InventoryOpenSfx());
    Expect(inventoryUi.open, "Inventory is open");
    Expect(inventorySfx.inventoryOpenCount == 1, "10. closed -> open emits one InventoryOpen");
    Expect(
        gameplay::HandleInventoryUiInput(inventoryUi, inventory, toggle)
            == gameplay::InventoryUiInputAction::Close,
        "available Tab while open is the Inventory Close action");
    gameplay::RecordGameplaySfx(inventorySfx, gameplay::InventoryCloseSfx());
    Expect(!inventoryUi.open, "Inventory is closed");
    Expect(inventorySfx.inventoryCloseCount == 1, "11. open -> closed emits one InventoryClose");

    input::InputState cancel{};
    cancel.cancelPressed = true;
    Expect(
        gameplay::HandleInventoryUiInput(inventoryUi, inventory, cancel)
            == gameplay::InventoryUiInputAction::None,
        "Esc while Inventory is closed is not an Inventory action");
    Expect(inventorySfx.inventoryOpenCount == 1 && inventorySfx.inventoryCloseCount == 1,
        "closed Esc synthesizes no Inventory cue");

    Expect(
        !gameplay::InventoryUiIsAvailable(
            gameplay::TopLevelFlow::Gameplay, false, false, true),
        "Pause consumes Tab before Inventory");
    Expect(
        !gameplay::InventoryUiIsAvailable(
            gameplay::TopLevelFlow::Gameplay, false, true, false),
        "Run Complete consumes Tab");
    Expect(
        !gameplay::InventoryUiIsAvailable(
            gameplay::TopLevelFlow::Gameplay, false, false, false, true),
        "death consumes Tab");
    Expect(inventorySfx.inventoryOpenCount == 1 && inventorySfx.inventoryCloseCount == 1,
        "12. blocked/consumed Tab/Esc paths synthesize no Inventory cues");

    gameplay::OpenInventoryUi(inventoryUi, inventory);
    gameplay::GameplaySfxRequestState lifecycleUiSfx{};
    gameplay::CloseInventoryUi(inventoryUi);
    gameplay::ApplyInventoryUiLifecycle(
        inventoryUi, gameplay::InventoryLifecycleEvent::NewRun, inventory);
    gameplay::ApplyInventoryUiLifecycle(
        inventoryUi, gameplay::InventoryLifecycleEvent::RestartRun, inventory);
    gameplay::ApplyInventoryUiLifecycle(
        inventoryUi, gameplay::InventoryLifecycleEvent::LevelTransition, inventory);
    gameplay::ApplyInventoryUiLifecycle(
        inventoryUi, gameplay::InventoryLifecycleEvent::ApplyCommittedLevel, inventory);
    Expect(
        lifecycleUiSfx.inventoryOpenCount == 0 && lifecycleUiSfx.inventoryCloseCount == 0
            && lifecycleUiSfx.uiNavigateCount == 0 && lifecycleUiSfx.uiConfirmCount == 0
            && lifecycleUiSfx.pauseOpenCount == 0 && lifecycleUiSfx.pauseCloseCount == 0,
        "13. F2/lifecycle/reconstruction CloseInventoryUi synthesizes no M75 UI cues");

    Expect(
        playSfx.pickupCount == 0 && uiPauseSfx.collectibleCount == 0 && inventorySfx.damageCount == 0
            && menuSfx.footstepCount == 0 && enterResumeSfx.jumpCount == 0
            && escResumeSfx.landingCount == 0 && uiPauseSfx.checkpointActivateCount == 0
            && uiPauseSfx.doorUnlockCount == 0 && uiPauseSfx.levelGoalCompleteCount == 0,
        "14. M75 UI cues remain distinct from M71-M74 gameplay cues");

    gameplay::GameplaySfxRequestState missingUiSfx{};
    gameplay::RecordGameplaySfx(missingUiSfx, gameplay::UiNavigateSfx());
    gameplay::RecordGameplaySfx(missingUiSfx, gameplay::UiConfirmSfx());
    gameplay::RecordGameplaySfx(missingUiSfx, gameplay::PauseOpenSfx());
    gameplay::RecordGameplaySfx(missingUiSfx, gameplay::PauseEscResumeSfx());
    gameplay::RecordGameplaySfx(missingUiSfx, gameplay::InventoryOpenSfx());
    gameplay::RecordGameplaySfx(missingUiSfx, gameplay::InventoryCloseSfx());
    Expect(
        missingUiSfx.uiNavigateCount == 1 && missingUiSfx.uiConfirmCount == 1
            && missingUiSfx.pauseOpenCount == 1 && missingUiSfx.pauseCloseCount == 1
            && missingUiSfx.inventoryOpenCount == 1 && missingUiSfx.inventoryCloseCount == 1,
        "16. unavailable playback still records the semantic M75 request");
    Expect(
        authoredHazards[0].center.x == authoredBefore.center.x && !dirty
            && workingCopy[0].center.x == authoredBefore.center.x,
        "18. M75 UI audio never mutates authored data, workingCopy, or Dirty");

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d GameplayAudioTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("GameplayAudioTest passed\n");
    return 0;
}
