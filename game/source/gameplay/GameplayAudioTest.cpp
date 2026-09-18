// Milestone 71/72: semantic gameplay SFX requests, lethal-hit precedence,
// movement cadence, missing-cue safety, and authored/Dirty isolation.
// No window, raylib, or Jolt.

#include "gameplay/GameplayAudio.h"
#include "gameplay/GameFlowState.h"
#include "gameplay/Inventory.h"
#include "gameplay/ItemPickupRuntime.h"
#include "gameplay/PlayerDeath.h"
#include "gameplay/PlayerHealth.h"
#include "platform/GameplayAudio.h"
#include "world/HazardWorld.h"
#include "world/ItemPickup.h"

#include <cstdio>
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
        platform::kGameplaySfxCueCount == 7,
        "fixed cue set is pickup/damage/death/respawn/footstep/jump/landing");

    {
        world::ItemPickupSpec pickup = MakePickup({1.55f, 1.0f, 0.0f});
        std::vector<world::ItemPickupSpec> pickups{pickup};
        gameplay::Inventory inventory;
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        gameplay::GameplaySfxRequestState sfx{};
        MaybeRequestPickupSfx(inventory, run, sfx, pickups, 0);
        Expect(sfx.pickupCount == 1, "1. successful pickup emits exactly one pickup-audio request");
        Expect(
            sfx.damageCount == 0 && sfx.deathCount == 0 && sfx.respawnCount == 0
                && sfx.footstepCount == 0 && sfx.jumpCount == 0 && sfx.landingCount == 0,
            "successful pickup does not emit survival or movement cues");
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
            && lifecycle.jumpCount == 0 && lifecycle.landingCount == 0,
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

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d GameplayAudioTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("GameplayAudioTest passed\n");
    return 0;
}
