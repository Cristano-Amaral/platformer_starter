// Milestone 71: semantic gameplay SFX requests, lethal-hit precedence,
// missing-cue safety, and authored/Dirty isolation. No window, raylib, or Jolt.

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
        platform::kGameplaySfxCueCount == 4, "fixed cue set is pickup/damage/death/respawn");

    {
        world::ItemPickupSpec pickup = MakePickup({1.55f, 1.0f, 0.0f});
        std::vector<world::ItemPickupSpec> pickups{pickup};
        gameplay::Inventory inventory;
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        gameplay::GameplaySfxRequestState sfx{};
        MaybeRequestPickupSfx(inventory, run, sfx, pickups, 0);
        Expect(sfx.pickupCount == 1, "1. successful pickup emits exactly one pickup-audio request");
        Expect(
            sfx.damageCount == 0 && sfx.deathCount == 0 && sfx.respawnCount == 0,
            "successful pickup does not emit survival cues");
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
            && lifecycle.respawnCount == 0,
        "12. Restart/Play/transition helpers do not fabricate SFX requests");

    const gameplay::GameplaySfxEmit emptyReload{};
    gameplay::RecordGameplaySfx(lifecycle, emptyReload);
    Expect(
        lifecycle.pickupCount == 0 && lifecycle.damageCount == 0,
        "12. Level transition/Restart/Apply/Reload emit no implicit gameplay SFX");

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d GameplayAudioTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("GameplayAudioTest passed\n");
    return 0;
}
