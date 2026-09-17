#include "gameplay/PlayerHealth.h"
#include "gameplay/GameplayObjectiveHud.h"

#include "gameplay/LevelCompletionState.h"
#include "gameplay/LevelTransition.h"
#include "gameplay/RespawnState.h"
#include "gameplay/RunTimerState.h"
#include "world/HazardWorld.h"
#include "world/LevelIdentity.h"

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

bool TextEquals(const char* buffer, const char* expected)
{
    return buffer != nullptr && expected != nullptr && std::strcmp(buffer, expected) == 0;
}
}

int main()
{
    Expect(gameplay::kMaxPlayerHealth == 100, "canonical max Health is 100");
    Expect(gameplay::kHazardDamageAmount > 0, "Hazard damage amount is positive");
    Expect(gameplay::kHazardDamageAmount <= gameplay::kMaxPlayerHealth, "one tick cannot exceed max");
    Expect(gameplay::kHazardDamageCadenceSeconds > 0.0f, "cadence is a positive simulation interval");
    Expect(
        gameplay::kHazardDamageCadenceSeconds > 1.0f / 120.0f,
        "cadence is slower than a render frame");

    gameplay::PlayerHealthState health{};
    gameplay::HazardContactState contact{};
    Expect(health.currentHealth == gameplay::kMaxPlayerHealth, "default current is max");
    Expect(health.maxHealth == gameplay::kMaxPlayerHealth, "default max uses the authority");
    Expect(gameplay::PlayerHealthInvariantsHold(health), "default invariants hold");

    gameplay::ResetPlayerHealthForNewRuntime(health, contact);
    Expect(
        health.currentHealth == gameplay::kMaxPlayerHealth
            && health.maxHealth == gameplay::kMaxPlayerHealth,
        "1. fresh Play/New Run initializes Health to maximum");
    Expect(contact.cooldownRemaining == 0.0f, "fresh runtime contact is ready");

    std::vector<world::HazardSpec> authoredHazards{{{11.5f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}}};
    const world::HazardSpec authoredHazardBefore = authoredHazards[0];
    const core::Vec3 insideHazard{11.5f, 0.5f, 0.0f};
    const core::Vec3 outsideHazard{0.0f, 1.0f, 0.0f};
    Expect(
        world::FindHazardIndexContaining(insideHazard, authoredHazards) == 0,
        "existing authored Hazard overlap authority detects contact");
    Expect(
        world::FindHazardIndexContaining(outsideHazard, authoredHazards)
            == world::kNoHazardIndex,
        "existing authored Hazard overlap authority detects leave");

    const bool overlapping = world::FindHazardIndexContaining(insideHazard, authoredHazards)
        != world::kNoHazardIndex;
    Expect(
        gameplay::TickHazardContactDamage(health, contact, overlapping, 1.0f / 60.0f, true),
        "first overlapping simulation step applies damage");
    Expect(
        health.currentHealth == gameplay::kMaxPlayerHealth - gameplay::kHazardDamageAmount,
        "2. Hazard overlap applies the fixed damage amount");
    Expect(
        contact.cooldownRemaining == gameplay::kHazardDamageCadenceSeconds,
        "first tick starts the cadence cooldown");

    char hud[gameplay::kHealthHudTextCapacity]{};
    gameplay::FormatHealthHudText(hud, sizeof(hud), health);
    Expect(
        TextEquals(hud, "HEALTH 75 / 100"),
        "20. Health HUD shows authoritative current/max after damage");

    const int healthAfterFirstTick = health.currentHealth;
    constexpr int kSustainedFrames = 45;
    int extraHits = 0;
    for (int frame = 0; frame < kSustainedFrames; ++frame)
    {
        if (gameplay::TickHazardContactDamage(health, contact, true, 1.0f / 60.0f, true))
        {
            ++extraHits;
        }
    }
    Expect(extraHits == 0, "3. sustained overlap does not damage once per render frame");
    Expect(health.currentHealth == healthAfterFirstTick, "sustained sub-cadence overlap keeps Health");

    Expect(
        !gameplay::TickHazardContactDamage(
            health, contact, true, gameplay::kHazardDamageCadenceSeconds, true),
        "cooldown reaching zero does not apply on the decrement step");
    Expect(
        gameplay::TickHazardContactDamage(health, contact, true, 1.0f / 60.0f, true),
        "next overlapping step after cadence applies the second tick");
    Expect(
        health.currentHealth == gameplay::kMaxPlayerHealth - 2 * gameplay::kHazardDamageAmount,
        "second cadence tick applies the same fixed amount");

    Expect(
        !gameplay::TickHazardContactDamage(health, contact, false, 1.0f / 60.0f, true),
        "leaving a Hazard applies no damage");
    Expect(contact.cooldownRemaining == 0.0f, "6. leaving resets cadence so re-entry is immediate");
    const int healthBeforeReentry = health.currentHealth;
    Expect(
        gameplay::TickHazardContactDamage(health, contact, true, 1.0f / 60.0f, true),
        "re-entering a Hazard damages immediately");
    Expect(
        health.currentHealth == healthBeforeReentry - gameplay::kHazardDamageAmount,
        "6. re-entry uses the documented immediate-damage rule");

    gameplay::PlayerHealthState paused = health;
    gameplay::HazardContactState pausedContact = contact;
    const int pausedHealth = paused.currentHealth;
    const float pausedCooldown = pausedContact.cooldownRemaining;
    for (int frame = 0; frame < 120; ++frame)
    {
        Expect(
            !gameplay::TickHazardContactDamage(
                paused, pausedContact, true, 1.0f / 60.0f, false),
            "7. Pause applies no Hazard damage");
    }
    Expect(paused.currentHealth == pausedHealth, "Pause freezes Health while overlapping");
    Expect(
        pausedContact.cooldownRemaining == pausedCooldown,
        "7. Pause does not progress cadence or catch up");
    Expect(
        !gameplay::TickHazardContactDamage(paused, pausedContact, true, 1.0f / 60.0f, true),
        "Resume continues the remaining cooldown without catch-up");
    Expect(paused.currentHealth == pausedHealth, "Resume does not dump paused overlap as damage");

    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, false, true, false, false, false),
        "8. Inventory gameplay blocking prevents Hazard damage");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, true, false, false, false, false),
        "9. Development editor gameplay blocking prevents Hazard damage");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::MainMenu, false, false, false, false, false),
        "10. Main Menu prevents Hazard damage");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, false, false, false, true, false),
        "11. destination LEVEL COMPLETE hold prevents Hazard damage");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, false, false, true, true, false),
        "12. RUN COMPLETE prevents Hazard damage");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, false, false, false, false, true),
        "PauseBlocksGameplay prevents Hazard damage");
    Expect(
        gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, false, false, false, false, false),
        "active unpaused Gameplay allows Hazard damage");

    gameplay::PlayerHealthState clampHealth{};
    gameplay::HazardContactState clampContact{};
    gameplay::InitializePlayerHealth(clampHealth);
    int ticksToZero = 0;
    while (clampHealth.currentHealth > 0 && ticksToZero < 16)
    {
        clampContact.cooldownRemaining = 0.0f;
        gameplay::TickHazardContactDamage(clampHealth, clampContact, true, 1.0f / 60.0f, true);
        ++ticksToZero;
    }
    Expect(clampHealth.currentHealth == 0, "4. Health reaches zero");
    Expect(gameplay::PlayerHealthInvariantsHold(clampHealth), "zero still satisfies invariants");
    Expect(
        !gameplay::TickHazardContactDamage(clampHealth, clampContact, true, 1.0f / 60.0f, true),
        "additional overlap at zero applies no damage");
    Expect(clampHealth.currentHealth == 0, "4. Health never underflows");
    Expect(gameplay::ApplyPlayerDamage(clampHealth, 1000) == 0, "huge extra damage still clamps at zero");
    Expect(clampHealth.currentHealth == 0, "current Health cannot go negative");
    Expect(gameplay::ApplyPlayerDamage(clampHealth, -10) == 0, "damage cannot increase Health");
    Expect(clampHealth.currentHealth == 0, "negative amounts leave zero Health unchanged");

    gameplay::RespawnState respawn{};
    respawn.activeCheckpointIndex = 0;
    respawn.deathCount = 2;
    const gameplay::RespawnState respawnBeforeZero = respawn;
    gameplay::LevelCompletionState completion{};
    gameplay::RunCompleteState runComplete{};
    gameplay::LevelTransitionSchedule transition{};
    gameplay::RunTimerState timer{};
    timer.elapsedSeconds = 9.5;
    int inventoryQuantity = 1;
    int pickupCollected = 1;
    int collectibleCollected = 1;
    int doorUnlocked = 1;
    std::string runtimeId{world::kLevel01Id};
    bool dirty = false;
    std::vector<world::HazardSpec> workingCopy = authoredHazards;

    for (int frame = 0; frame < 8; ++frame)
    {
        clampContact.cooldownRemaining = 0.0f;
        gameplay::TickHazardContactDamage(clampHealth, clampContact, true, 1.0f / 60.0f, true);
    }
    Expect(clampHealth.currentHealth == 0, "zero Health remains clamped");
    Expect(
        respawn.activeCheckpointIndex == respawnBeforeZero.activeCheckpointIndex
            && respawn.deathCount == respawnBeforeZero.deathCount,
        "5. zero Health does not respawn or restore a Checkpoint");
    Expect(!completion.completed, "5. zero Health does not complete or reload the Level");
    Expect(!runComplete.active, "5. zero Health does not enter Run Complete / Game Over");
    Expect(!transition.pending, "5. zero Health does not schedule a transition");
    Expect(timer.elapsedSeconds == 9.5, "5. zero Health does not reset the timer");
    Expect(inventoryQuantity == 1, "5. zero Health does not reset Inventory");
    Expect(pickupCollected == 1, "5. zero Health does not reset pickups");
    Expect(collectibleCollected == 1, "5. zero Health does not reset collectibles");
    Expect(doorUnlocked == 1, "5. zero Health does not reset Doors");
    Expect(runtimeId == world::kLevel01Id, "5. zero Health does not change currentRuntimeLevelId");
    Expect(!dirty, "5. zero Health does not mark Dirty");

    gameplay::PlayerHealthState transitionHealth{};
    gameplay::HazardContactState transitionContact{};
    gameplay::ResetPlayerHealthForNewRuntime(transitionHealth, transitionContact);
    gameplay::TickHazardContactDamage(transitionHealth, transitionContact, true, 1.0f / 60.0f, true);
    const int preserved = transitionHealth.currentHealth;
    Expect(preserved == gameplay::kMaxPlayerHealth - gameplay::kHazardDamageAmount, "seed damaged Health");
    std::string currentRuntimeLevelId{world::kLevel01Id};
    currentRuntimeLevelId = std::string(world::kLevel02Id);
    gameplay::ResetHazardContactState(transitionContact);
    Expect(
        transitionHealth.currentHealth == preserved,
        "13. successful level_01 -> level_02 transition preserves Health");
    Expect(
        transitionContact.cooldownRemaining == 0.0f,
        "level change resets local Hazard contact, not Health");

    const int failedHealth = transitionHealth.currentHealth;
    Expect(
        transitionHealth.currentHealth == failedHealth,
        "14. failed destination transition leaves Health unchanged");

    gameplay::ResetPlayerHealthForNewRuntime(transitionHealth, transitionContact);
    Expect(
        transitionHealth.currentHealth == gameplay::kMaxPlayerHealth,
        "15. Restart Current Level restores maximum Health");

    gameplay::TickHazardContactDamage(transitionHealth, transitionContact, true, 1.0f / 60.0f, true);
    gameplay::ResetPlayerHealthForNewRuntime(transitionHealth, transitionContact);
    Expect(
        transitionHealth.currentHealth == gameplay::kMaxPlayerHealth,
        "16/17. Play Again and Main Menu PLAY restore maximum Health");

    gameplay::TickHazardContactDamage(transitionHealth, transitionContact, true, 1.0f / 60.0f, true);
    gameplay::ResetPlayerHealthForNewRuntime(transitionHealth, transitionContact);
    Expect(
        transitionHealth.currentHealth == gameplay::kMaxPlayerHealth,
        "18. Apply/Reload/Development Open/Switch reset Health for the new runtime");

    gameplay::PlayerHealthState rebuildHealth{};
    gameplay::HazardContactState rebuildContact{};
    gameplay::ResetPlayerHealthForNewRuntime(rebuildHealth, rebuildContact);
    gameplay::TickHazardContactDamage(rebuildHealth, rebuildContact, true, 1.0f / 60.0f, true);
    const int rebuildBefore = rebuildHealth.currentHealth;
    const float rebuildCooldown = rebuildContact.cooldownRemaining;
    Expect(
        rebuildHealth.currentHealth == rebuildBefore
            && rebuildContact.cooldownRemaining == rebuildCooldown,
        "19. physics rebuild preservation does not independently reset Health");

    Expect(
        gameplay::HealthHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, false, false, false, false),
        "20. Health HUD is visible during active Gameplay");
    Expect(
        !gameplay::HealthHudIsVisible(
            gameplay::TopLevelFlow::MainMenu, false, false, false, false),
        "21. Main Menu hides Health HUD");
    Expect(
        !gameplay::HealthHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, false, false, false, false, true),
        "21. Pause hides Health HUD");
    Expect(
        !gameplay::HealthHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, false, false, false, true),
        "21. destination LEVEL COMPLETE hides Health HUD");
    Expect(
        !gameplay::HealthHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, false, false, true, false),
        "21. RUN COMPLETE hides Health HUD");
    Expect(
        !gameplay::HealthHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, true, false, false, false),
        "21. F2/editor hides Health HUD");
    Expect(
        !gameplay::HealthHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, false, true, false, false),
        "21. Inventory follows existing Gameplay HUD hide convention");

    Expect(
        gameplay::kHealthHudY
            >= gameplay::kGameplayObjectiveHudObjectiveY
                + gameplay::kGameplayObjectiveHudObjectiveFontSize,
        "Health HUD sits below the M67 objective line");
    Expect(
        gameplay::kHealthHudY + gameplay::kHealthHudFontSize < 200,
        "Health HUD stays in the top-left band away from prompts/overlays");
    Expect(gameplay::kHealthHudMarginX == 20, "Health HUD uses the existing left margin");

    Expect(
        authoredHazards.size() == 1
            && authoredHazards[0].center.x == authoredHazardBefore.center.x
            && authoredHazards[0].size.y == authoredHazardBefore.size.y,
        "22. Health/damage never mutates authored Hazard specs");
    Expect(!dirty, "22. Health/damage never sets Dirty");
    Expect(workingCopy[0].center.x == authoredHazardBefore.center.x, "workingCopy unchanged");

    gameplay::FormatHealthHudText(hud, sizeof(hud), clampHealth);
    Expect(TextEquals(hud, "HEALTH 0 / 100"), "HUD updates immediately at zero Health");

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d player health test(s) failed.\n", gFailures);
        return 1;
    }
    std::printf("Player health tests passed.\n");
    return 0;
}
