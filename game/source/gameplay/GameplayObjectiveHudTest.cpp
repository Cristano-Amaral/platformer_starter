#include "gameplay/GameFlowState.h"
#include "gameplay/GameplayObjectiveHud.h"
#include "gameplay/PlayerHealth.h"
#include "gameplay/ItemPickupCollectionHud.h"
#include "gameplay/LevelCompletionState.h"
#include "gameplay/LevelTransition.h"
#include "world/LevelGoal.h"
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

world::LevelGoalSpec MakeGoal(std::string_view nextLevelId)
{
    world::LevelGoalSpec spec{{0.0f, 1.0f, 0.0f}, world::kDefaultLevelGoalSize, {}};
    spec.nextLevelId = std::string(nextLevelId);
    return spec;
}
}

int main()
{
    Expect(
        gameplay::kGameplayObjectiveHudLevelY >= 46 + 22 + 8,
        "objective HUD sits below TIME/BEST");
    Expect(
        gameplay::kGameplayObjectiveHudObjectiveY
            >= gameplay::kGameplayObjectiveHudLevelY
                + gameplay::kGameplayObjectiveHudLevelFontSize,
        "objective line sits below the Level label");
    Expect(
        gameplay::kGameplayObjectiveHudObjectiveY
                + gameplay::kGameplayObjectiveHudObjectiveFontSize
            < 160,
        "objective HUD stays in the top-left band");
    Expect(
        gameplay::kGameplayObjectiveHudMarginX == 20,
        "objective HUD uses the existing left safe margin");
    Expect(
        gameplay::kGameplayObjectiveHudLevelY < 200,
        "objective HUD does not occupy the centered Inventory/completion band");
    Expect(
        gameplay::kItemPickupCollectionHudLifetimeSeconds > 0.0f,
        "M62 notifications remain a separate bottom-stack HUD");

    char levelLabel[gameplay::kGameplayObjectiveHudLevelLabelCapacity]{};
    char objectiveLine[gameplay::kGameplayObjectiveHudObjectiveCapacity]{};

    gameplay::FormatGameplayLevelLabel(levelLabel, sizeof(levelLabel), world::kLevel01Id);
    Expect(TextEquals(levelLabel, "LEVEL 01"), "level_01 formats as LEVEL 01");
    gameplay::FormatGameplayObjectiveLine(objectiveLine, sizeof(objectiveLine), {MakeGoal("level_02")});
    Expect(
        TextEquals(objectiveLine, gameplay::kGameplayObjectiveHudDestinationText),
        "level_01 destination wording");
    Expect(
        gameplay::ClassifyGameplayObjective({MakeGoal("level_02")})
            == gameplay::GameplayObjectiveKind::Destination,
        "single destination Goal is destination wording");

    gameplay::FormatGameplayLevelLabel(levelLabel, sizeof(levelLabel), world::kLevel02Id);
    Expect(TextEquals(levelLabel, "LEVEL 02"), "level_02 formats as LEVEL 02");
    gameplay::FormatGameplayObjectiveLine(objectiveLine, sizeof(objectiveLine), {MakeGoal({})});
    Expect(
        TextEquals(objectiveLine, gameplay::kGameplayObjectiveHudTerminalText),
        "level_02 terminal wording");
    Expect(
        gameplay::ClassifyGameplayObjective({MakeGoal({})})
            == gameplay::GameplayObjectiveKind::Terminal,
        "single terminal Goal is terminal wording");

    gameplay::FormatGameplayObjectiveLine(objectiveLine, sizeof(objectiveLine), {});
    Expect(objectiveLine[0] == '\0', "no Level Goal omits the objective line");
    Expect(
        gameplay::ClassifyGameplayObjective({}) == gameplay::GameplayObjectiveKind::None,
        "no-goal classification does not invent an objective");
    gameplay::FormatGameplayLevelLabel(levelLabel, sizeof(levelLabel), "sandbox");
    Expect(TextEquals(levelLabel, "SANDBOX"), "unknown valid ids use a narrow uppercase fallback");
    gameplay::FormatGameplayLevelLabel(levelLabel, sizeof(levelLabel), "levels/level_01.level");
    Expect(
        TextEquals(levelLabel, gameplay::kGameplayObjectiveHudUnknownLevelLabel),
        "paths are not exposed as player-facing Level labels");
    gameplay::FormatGameplayLevelLabel(levelLabel, sizeof(levelLabel), "LEVEL_03");
    Expect(TextEquals(levelLabel, "LEVEL 03"), "friendly pattern is case-insensitive");
    gameplay::FormatGameplayLevelLabel(levelLabel, sizeof(levelLabel), "level_");
    Expect(TextEquals(levelLabel, "LEVEL"), "incomplete friendly pattern uses fallback");

    const std::vector<world::LevelGoalSpec> mixed{
        MakeGoal("level_02"),
        MakeGoal({})};
    Expect(
        gameplay::ClassifyGameplayObjective(mixed) == gameplay::GameplayObjectiveKind::Mixed,
        "mixed Goals do not invent an active route");
    gameplay::FormatGameplayObjectiveLine(objectiveLine, sizeof(objectiveLine), mixed);
    Expect(
        TextEquals(objectiveLine, gameplay::kGameplayObjectiveHudMixedText),
        "mixed Goals use generic wording");

    const std::vector<world::LevelGoalSpec> twoDestinations{
        MakeGoal("level_02"),
        MakeGoal("level_04")};
    Expect(
        gameplay::ClassifyGameplayObjective(twoDestinations)
            == gameplay::GameplayObjectiveKind::Destination,
        "multiple destinations stay destination-class without choosing a next Level");
    gameplay::FormatGameplayObjectiveLine(objectiveLine, sizeof(objectiveLine), twoDestinations);
    Expect(
        TextEquals(objectiveLine, gameplay::kGameplayObjectiveHudDestinationText),
        "multiple destinations keep generic destination wording");
    Expect(
        std::string_view(objectiveLine).find("LEVEL 02") == std::string_view::npos
            && std::string_view(objectiveLine).find("LEVEL 04") == std::string_view::npos,
        "multiple destinations do not advertise a campaign next Level");

    const std::vector<world::LevelGoalSpec> twoTerminals{MakeGoal({}), MakeGoal({})};
    gameplay::FormatGameplayObjectiveLine(objectiveLine, sizeof(objectiveLine), twoTerminals);
    Expect(
        TextEquals(objectiveLine, gameplay::kGameplayObjectiveHudTerminalText),
        "multiple terminals stay terminal wording without ordering");

    Expect(
        !gameplay::ObjectiveHudIsVisible(
            gameplay::TopLevelFlow::MainMenu, false, false, false, false),
        "Main Menu hides objective HUD");
    Expect(
        gameplay::ObjectiveHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, false, false, false, false),
        "active Gameplay shows objective HUD");
    Expect(
        !gameplay::ObjectiveHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, false, true, false, false),
        "Inventory hides objective HUD");
    Expect(
        !gameplay::ObjectiveHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, false, false, false, true),
        "destination completion overlay has priority over objective HUD");
    Expect(
        !gameplay::ObjectiveHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, false, true, false, true),
        "Inventory plus completion still hides objective HUD");
    Expect(
        !gameplay::ObjectiveHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, false, false, true, false),
        "Run Complete hides objective HUD");
    Expect(
        !gameplay::ObjectiveHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, true, false, false, false),
        "F2/editor hides objective HUD");
    Expect(
        !gameplay::ObjectiveHudIsVisible(
            gameplay::TopLevelFlow::MainMenu, true, false, false, false),
        "F2 from Main Menu does not show objective HUD");

    std::string currentRuntimeLevelId{world::kLevel01Id};
    std::vector<world::LevelGoalSpec> activeGoals{MakeGoal("level_02")};
    gameplay::FormatGameplayLevelLabel(
        levelLabel, sizeof(levelLabel), currentRuntimeLevelId);
    gameplay::FormatGameplayObjectiveLine(objectiveLine, sizeof(objectiveLine), activeGoals);
    Expect(TextEquals(levelLabel, "LEVEL 01"), "Play starts on LEVEL 01");
    Expect(
        TextEquals(objectiveLine, gameplay::kGameplayObjectiveHudDestinationText),
        "Play starts with destination objective");

    gameplay::LevelTransitionSchedule failedTransition{};
    gameplay::CaptureCompletedGoalDestination(failedTransition, world::kLevel02Id);
    gameplay::MarkLevelTransitionFailed(failedTransition, "missing destination");
    Expect(failedTransition.destinationId == world::kLevel02Id, "failed schedule still names destination");
    gameplay::FormatGameplayLevelLabel(
        levelLabel, sizeof(levelLabel), currentRuntimeLevelId);
    Expect(
        TextEquals(levelLabel, "LEVEL 01"),
        "failed transition does not display the destination Level");
    Expect(
        gameplay::ClassifyGameplayObjective(activeGoals)
            == gameplay::GameplayObjectiveKind::Destination,
        "failed transition keeps the source Level Goals");

    currentRuntimeLevelId = std::string(world::kLevel02Id);
    activeGoals = {MakeGoal({})};
    gameplay::FormatGameplayLevelLabel(
        levelLabel, sizeof(levelLabel), currentRuntimeLevelId);
    gameplay::FormatGameplayObjectiveLine(objectiveLine, sizeof(objectiveLine), activeGoals);
    Expect(TextEquals(levelLabel, "LEVEL 02"), "successful transition refreshes LEVEL 02");
    Expect(
        TextEquals(objectiveLine, gameplay::kGameplayObjectiveHudTerminalText),
        "successful transition refreshes terminal objective");

    gameplay::LevelCompletionState restartCompletion{};
    restartCompletion.completed = true;
    gameplay::ResetLevelCompletionState(restartCompletion);
    Expect(
        gameplay::ObjectiveHudIsVisible(
            gameplay::TopLevelFlow::Gameplay,
            false,
            false,
            false,
            restartCompletion.completed),
        "Restart Current Level returns objective HUD");
    gameplay::FormatGameplayLevelLabel(
        levelLabel, sizeof(levelLabel), currentRuntimeLevelId);
    Expect(TextEquals(levelLabel, "LEVEL 02"), "Restart stays on LEVEL 02 presentation");

    currentRuntimeLevelId = std::string(world::kLevel01Id);
    activeGoals = {MakeGoal("level_02")};
    gameplay::FormatGameplayLevelLabel(
        levelLabel, sizeof(levelLabel), currentRuntimeLevelId);
    gameplay::FormatGameplayObjectiveLine(objectiveLine, sizeof(objectiveLine), activeGoals);
    Expect(TextEquals(levelLabel, "LEVEL 01"), "Play Again returns LEVEL 01 presentation");
    Expect(
        TextEquals(objectiveLine, gameplay::kGameplayObjectiveHudDestinationText),
        "Play Again returns destination objective");

    gameplay::TopLevelFlow flow = gameplay::TopLevelFlow::Gameplay;
    gameplay::MainMenuState menu{};
    gameplay::RunCompleteState results{};
    results.active = true;
    Expect(
        !gameplay::ObjectiveHudIsVisible(flow, false, false, results.active, false),
        "Run Complete still hides objective HUD before Esc");
    gameplay::PauseMenuState hudPause{};
    gameplay::EnterMainMenu(flow, menu, results, hudPause);
    Expect(
        !gameplay::ObjectiveHudIsVisible(flow, false, false, results.active, false),
        "Run Complete to Main Menu keeps objective HUD hidden");
    gameplay::EnterGameplayFromSuccessfulPlay(flow, menu, hudPause);
    Expect(
        gameplay::ObjectiveHudIsVisible(flow, false, false, false, false),
        "Main Menu Play restores objective HUD");
    Expect(
        !gameplay::ObjectiveHudIsVisible(flow, false, false, false, false, true),
        "Pause suppresses objective HUD");
    Expect(
        gameplay::ObjectiveHudIsVisible(flow, false, false, false, false, false),
        "Resume restores objective HUD");

    Expect(
        gameplay::FlowAfterEditorToggle(gameplay::TopLevelFlow::Gameplay)
            == gameplay::TopLevelFlow::Gameplay,
        "F2 toggle preserves Gameplay for HUD lifecycle");
    Expect(
        gameplay::FlowAfterEditorOpenOrSwitch(gameplay::TopLevelFlow::Gameplay)
            == gameplay::TopLevelFlow::Gameplay,
        "Open/Switch does not fabricate a second HUD Level identity");

    std::vector<world::LevelGoalSpec> authoredGoals{MakeGoal("level_02")};
    const std::string authoredId{world::kLevel01Id};
    const world::LevelGoalSpec authoredGoalBefore = authoredGoals[0];
    gameplay::FormatGameplayLevelLabel(levelLabel, sizeof(levelLabel), authoredId);
    gameplay::FormatGameplayObjectiveLine(objectiveLine, sizeof(objectiveLine), authoredGoals);
    Expect(authoredId == world::kLevel01Id, "HUD formatting does not mutate Level id");
    Expect(
        authoredGoals.size() == 1
            && authoredGoals[0].nextLevelId == authoredGoalBefore.nextLevelId
            && authoredGoals[0].center.x == authoredGoalBefore.center.x
            && authoredGoals[0].size.y == authoredGoalBefore.size.y,
        "HUD formatting does not mutate LevelGoalSpec");

    gameplay::RunCompleteState playFailedMenu{};
    gameplay::TopLevelFlow failedPlayFlow = gameplay::TopLevelFlow::MainMenu;
    gameplay::MainMenuState failedPlayMenu{};
    gameplay::MarkPlayFailed(failedPlayMenu, "Initial staged level is missing.");
    Expect(
        !gameplay::ObjectiveHudIsVisible(
            failedPlayFlow, false, false, playFailedMenu.active, false),
        "failed Play remains on Main Menu without objective HUD");

    gameplay::PlayerHealthState hudHealth{};
    gameplay::InitializePlayerHealth(hudHealth);
    char healthText[gameplay::kHealthHudTextCapacity]{};
    gameplay::FormatHealthHudText(healthText, sizeof(healthText), hudHealth);
    Expect(TextEquals(healthText, "HEALTH 100 / 100"), "Health HUD formats max Health");
    Expect(
        gameplay::kHealthHudY
            >= gameplay::kGameplayObjectiveHudObjectiveY
                + gameplay::kGameplayObjectiveHudObjectiveFontSize,
        "Health HUD sits below LEVEL/OBJECTIVE");
    Expect(
        gameplay::HealthHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, false, false, false, false),
        "active Gameplay shows Health HUD");
    Expect(
        !gameplay::HealthHudIsVisible(
            gameplay::TopLevelFlow::MainMenu, false, false, false, false),
        "Main Menu hides Health HUD");
    Expect(
        !gameplay::HealthHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, false, false, false, false, true),
        "Pause hides Health HUD");
    Expect(
        !gameplay::HealthHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, false, true, false, false),
        "Inventory hides Health HUD with objective HUD");
    Expect(
        !gameplay::HealthHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, false, false, false, true),
        "destination completion hides Health HUD");
    Expect(
        !gameplay::HealthHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, false, false, true, false),
        "Run Complete hides Health HUD");
    Expect(
        !gameplay::HealthHudIsVisible(
            gameplay::TopLevelFlow::Gameplay, true, false, false, false),
        "F2/editor hides Health HUD");
    hudHealth.currentHealth = gameplay::kMaxPlayerHealth - gameplay::kHazardDamageAmount;
    gameplay::FormatHealthHudText(healthText, sizeof(healthText), hudHealth);
    Expect(TextEquals(healthText, "HEALTH 75 / 100"), "Health HUD updates after damage");

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d GameplayObjectiveHudTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("GameplayObjectiveHudTest passed\n");
    return 0;
}
