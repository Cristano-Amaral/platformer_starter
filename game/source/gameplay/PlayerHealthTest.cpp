#include "gameplay/PlayerHealth.h"
#include "gameplay/PlayerDeath.h"
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

    gameplay::PlayerDeathState lethalDeath{};
    Expect(
        gameplay::TryBeginPlayerDeath(lethalDeath, gameplay::kHazardDamageAmount, 0),
        "5. Health >0 to 0 enters death exactly once");
    Expect(gameplay::PlayerDeathIsActive(lethalDeath), "death phase is active after the transition");
    Expect(
        lethalDeath.remainingSeconds == gameplay::kPlayerDeathDelaySeconds,
        "death delay starts at the named constant");
    Expect(
        !gameplay::TryBeginPlayerDeath(lethalDeath, 0, 0),
        "5. remaining at zero does not re-enter death");
    Expect(gameplay::PlayerDeathIsActive(lethalDeath), "duplicate begin is ignored");
    for (int frame = 0; frame < 8; ++frame)
    {
        clampContact.cooldownRemaining = 0.0f;
        Expect(
            !gameplay::TickHazardContactDamage(clampHealth, clampContact, true, 1.0f / 60.0f, true),
            "Health stays clamped at zero during death");
        Expect(clampHealth.currentHealth == 0, "6. Health remains 0 during the death phase");
        Expect(
            !gameplay::TryBeginPlayerDeath(lethalDeath, 0, 0),
            "zero Health does not duplicate death");
    }

    gameplay::RespawnState respawn{};
    respawn.activeCheckpointIndex = 0;
    respawn.respawnPosition = {16.5f, 1.8f, 0.0f};
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
    int plateActive = 1;
    core::Vec3 boxCenter{4.0f, 1.0f, 0.0f};
    std::string runtimeId{world::kLevel01Id};
    bool dirty = false;
    std::vector<world::HazardSpec> workingCopy = authoredHazards;

    Expect(
        !gameplay::PauseCanBeEntered(
            gameplay::TopLevelFlow::Gameplay, false, false, false, false, false, true),
        "8. Pause cannot be entered during death");
    Expect(
        !gameplay::InventoryUiIsAvailable(
            gameplay::TopLevelFlow::Gameplay, false, false, false, true),
        "8. Inventory cannot open during death");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, false, false, false, false, false, true),
        "7. Hazard damage is blocked during death");
    Expect(
        gameplay::PlayerDeathBlocksGameplay(true, true),
        "7. death blocks ordinary gameplay progression");
    Expect(
        gameplay::PlayerDeathBlocksGameplay(true, false),
        "respawn frame still blocks gameplay input carry-through");

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
        !gameplay::HealthHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, false, false, false, false, false, true),
        "death hides ordinary Health HUD");
    Expect(
        !gameplay::ObjectiveHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, false, false, false, false, false, true),
        "death hides ordinary objective HUD");
    Expect(
        gameplay::DamageVignetteIsVisible(
            gameplay::TopLevelFlow::Gameplay, false, false, false, false, false, true),
        "death may keep the Damage Vignette as initial feedback");

    Expect(
        authoredHazards.size() == 1
            && authoredHazards[0].center.x == authoredHazardBefore.center.x
            && authoredHazards[0].size.y == authoredHazardBefore.size.y,
        "22. Health/damage never mutates authored Hazard specs");
    Expect(!dirty, "22. Health/damage never sets Dirty");
    Expect(workingCopy[0].center.x == authoredHazardBefore.center.x, "workingCopy unchanged");

    gameplay::FormatHealthHudText(hud, sizeof(hud), clampHealth);
    Expect(TextEquals(hud, "HEALTH 0 / 100"), "HUD updates immediately at zero Health");

    Expect(gameplay::kDamageVignetteDurationSeconds == 0.35f, "vignette duration is 0.35 s");
    Expect(gameplay::kDamageVignettePeakOpacity == 0.45f, "vignette peak opacity is 0.45");
    Expect(gameplay::kDamageVignetteEdgeFraction == 0.18f, "vignette edge fraction is 0.18");
    Expect(gameplay::kPlayerDeathDelaySeconds == 1.25f, "9. death delay is 1.25 s");

    gameplay::DamageVignetteState vignette{};
    gameplay::PlayerHealthState vignetteHealth{};
    gameplay::HazardContactState vignetteContact{};
    gameplay::InitializePlayerHealth(vignetteHealth);
    Expect(
        gameplay::TickHazardContactDamage(
            vignetteHealth, vignetteContact, true, 1.0f / 60.0f, true),
        "successful non-lethal damage occurs");
    gameplay::BeginDamageVignette(vignette);
    Expect(
        vignette.remainingSeconds == gameplay::kDamageVignetteDurationSeconds,
        "1. successful damage starts the Damage Vignette");
    Expect(
        gameplay::DamageVignetteOpacity(vignette) == gameplay::kDamageVignettePeakOpacity,
        "vignette starts at peak opacity");

    gameplay::DamageVignetteState blockedVignette{};
    gameplay::PlayerHealthState blockedHealth = vignetteHealth;
    gameplay::HazardContactState blockedContact = vignetteContact;
    Expect(
        !gameplay::TickHazardContactDamage(
            blockedHealth, blockedContact, true, 1.0f / 60.0f, false),
        "blocked Hazard overlap applies no damage");
    Expect(
        blockedVignette.remainingSeconds == 0.0f,
        "2. blocked/no-op damage does not trigger the vignette");
    Expect(
        !gameplay::TickHazardContactDamage(
            clampHealth, clampContact, true, 1.0f / 60.0f, true),
        "zero-Health overlap is a no-op");
    Expect(
        blockedVignette.remainingSeconds == 0.0f,
        "2. zero-Health overlap does not trigger the vignette");

    const float remainingAtStart = vignette.remainingSeconds;
    gameplay::TickDamageVignette(vignette, 0.10f, false);
    Expect(
        vignette.remainingSeconds == remainingAtStart,
        "4. Pause/Inventory/editor blocking does not age the vignette");
    gameplay::TickDamageVignette(vignette, 0.10f, true);
    Expect(
        vignette.remainingSeconds == remainingAtStart - 0.10f,
        "3. vignette fades only while gameplay is allowed to age");
    gameplay::BeginDamageVignette(vignette);
    Expect(
        vignette.remainingSeconds == gameplay::kDamageVignetteDurationSeconds,
        "later successful damage refreshes the vignette");
    gameplay::TickDamageVignette(vignette, gameplay::kDamageVignetteDurationSeconds, true);
    Expect(vignette.remainingSeconds == 0.0f, "3. vignette reaches zero at the named duration");
    Expect(gameplay::DamageVignetteOpacity(vignette) == 0.0f, "elapsed vignette is invisible");

    gameplay::TickPlayerDeathDelay(lethalDeath, 0.25f);
    Expect(
        lethalDeath.remainingSeconds == gameplay::kPlayerDeathDelaySeconds - 0.25f,
        "9. death delay is deterministic");
    Expect(gameplay::PlayerDeathIsActive(lethalDeath), "death stays active before the delay elapses");
    gameplay::TickPlayerDeathDelay(lethalDeath, gameplay::kPlayerDeathDelaySeconds);
    Expect(gameplay::PlayerDeathDelayElapsed(lethalDeath), "delay reaching zero is ready to respawn");

    Expect(
        gameplay::DeathUsesActiveCheckpoint(respawn)
            && gameplay::ResolveDeathRespawnPosition(respawn).x == respawnBeforeZero.respawnPosition.x,
        "10. active Checkpoint is the death respawn destination");
    gameplay::RestorePlayerHealthAfterDeathRespawn(clampHealth);
    Expect(
        clampHealth.currentHealth == gameplay::kMaxPlayerHealth
            && clampHealth.maxHealth == gameplay::kMaxPlayerHealth,
        "12. death respawn restores Health to maximum");
    Expect(
        respawn.activeCheckpointIndex == respawnBeforeZero.activeCheckpointIndex,
        "10. death does not clear the active Checkpoint");
    Expect(runtimeId == world::kLevel01Id, "13. death respawn preserves currentRuntimeLevelId");
    Expect(inventoryQuantity == 1, "14. death respawn preserves Inventory");
    Expect(pickupCollected == 1, "15. death respawn preserves collected pickups");
    Expect(doorUnlocked == 1, "16. death respawn preserves Door unlock state");
    Expect(plateActive == 1, "16. death respawn preserves Pressure Plate runtime");
    Expect(boxCenter.x == 4.0f, "16. death respawn preserves Dynamic Box runtime pose");
    Expect(collectibleCollected == 1, "16. death respawn preserves run-local collectibles");
    Expect(timer.elapsedSeconds == 9.5, "17. death does not reset the run timer");
    Expect(!completion.completed, "death does not complete the Level");
    Expect(!runComplete.active, "death does not enter RUN COMPLETE");
    Expect(!transition.pending, "death does not schedule a Level transition");
    Expect(!dirty, "24. death never marks Dirty");
    gameplay::ClearPlayerDeath(lethalDeath);
    Expect(!gameplay::PlayerDeathIsActive(lethalDeath), "death clears after respawn");

    gameplay::RespawnState spawnOnly{};
    spawnOnly.respawnPosition = {0.0f, 1.0f, 0.0f};
    Expect(
        !gameplay::DeathUsesActiveCheckpoint(spawnOnly),
        "11. no active Checkpoint uses the current Level player spawn");
    Expect(
        gameplay::ResolveDeathRespawnPosition(spawnOnly).x == 0.0f
            && gameplay::ResolveDeathRespawnPosition(spawnOnly).y == 1.0f,
        "11. death respawn destination is the stored Level spawn");

    gameplay::HazardContactState staleContact{};
    staleContact.cooldownRemaining = 0.15f;
    gameplay::ResetHazardContactAfterDeathRespawn(staleContact, false);
    Expect(staleContact.cooldownRemaining == 0.0f, "18. death respawn clears stale Hazard cadence");
    gameplay::PlayerHealthState afterRespawnHealth{};
    gameplay::InitializePlayerHealth(afterRespawnHealth);
    Expect(
        gameplay::TickHazardContactDamage(
            afterRespawnHealth, staleContact, true, 1.0f / 60.0f, true),
        "non-overlapping spawn keeps first-step damage for a later Hazard entry");

    gameplay::HazardContactState overlapContact{};
    overlapContact.cooldownRemaining = 0.0f;
    gameplay::ResetHazardContactAfterDeathRespawn(overlapContact, true);
    Expect(
        overlapContact.cooldownRemaining == gameplay::kHazardDamageCadenceSeconds,
        "18. spawn/Checkpoint overlapping a Hazard arms cadence instead of first-tick damage");
    gameplay::PlayerHealthState overlapHealth{};
    gameplay::InitializePlayerHealth(overlapHealth);
    Expect(
        !gameplay::TickHazardContactDamage(
            overlapHealth, overlapContact, true, 1.0f / 60.0f, true),
        "19. overlapping spawn does not apply catch-up or immediate death damage");
    Expect(
        overlapHealth.currentHealth == gameplay::kMaxPlayerHealth,
        "19. no immediate repeated death loop at an overlapping spawn");
    Expect(
        !gameplay::TickHazardContactDamage(
            overlapHealth, overlapContact, false, 1.0f / 60.0f, true),
        "leaving the overlapping spawn Hazard resets cadence");
    Expect(overlapContact.cooldownRemaining == 0.0f, "leave after spawn-safety restores immediate re-entry");

    gameplay::DamageVignetteState respawnVignette = vignette;
    gameplay::RestorePlayerHealthAfterDeathRespawn(afterRespawnHealth);
    Expect(
        respawnVignette.remainingSeconds == vignette.remainingSeconds,
        "respawn itself does not retrigger the Damage Vignette");

    gameplay::PlayerHealthState fallHealth{};
    gameplay::InitializePlayerHealth(fallHealth);
    gameplay::ApplyPlayerDamage(fallHealth, gameplay::kHazardDamageAmount);
    const int fallHealthBefore = fallHealth.currentHealth;
    Expect(
        fallHealthBefore == gameplay::kMaxPlayerHealth - gameplay::kHazardDamageAmount,
        "Fall/Manual respawn does not restore Health");
    Expect(
        fallHealth.currentHealth == fallHealthBefore,
        "11b. M70 does not redefine Fall/Manual Health restoration");

    gameplay::FormatHealthHudText(hud, sizeof(hud), afterRespawnHealth);
    Expect(TextEquals(hud, "HEALTH 100 / 100"), "after respawn Health HUD shows maximum");

    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::MainMenu, false, false, false, false, false, false),
        "23. Main Menu cannot receive Hazard/death activity");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, false, false, false, true, false, false),
        "23. LEVEL COMPLETE cannot receive Hazard/death activity");
    Expect(
        !gameplay::HazardDamageIsAllowed(
            gameplay::TopLevelFlow::Gameplay, false, false, true, true, false, false),
        "23. RUN COMPLETE cannot receive Hazard/death activity");
    Expect(
        !gameplay::TryBeginPlayerDeath(lethalDeath, 0, 0),
        "23. completion/menu cannot begin death from zero Health");

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d player health test(s) failed.\n", gFailures);
        return 1;
    }
    std::printf("Player health tests passed.\n");
    return 0;
}
