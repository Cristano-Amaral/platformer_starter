#include "gameplay/LevelCompletionState.h"
#include "render/LevelGoalVisualization.h"
#include "world/LevelGoal.h"

#include <cstdio>
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

bool Vec3Equal(core::Vec3 a, core::Vec3 b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
}

int main()
{
    Expect(world::kLevel01LevelGoalCount == 0, "canonical Level 01 has zero Level Goals");
    Expect(world::LevelGoalSpecIsValid({{0.0f, 1.0f, 0.0f}, world::kDefaultLevelGoalSize}),
        "valid default");
    Expect(!world::LevelGoalSpecIsValid({{0.0f, 1.0f, 0.0f}, {0.0f, 1.6f, 1.8f}}),
        "zero extent rejected");
    Expect(!world::LevelGoalSpecIsValid({{0.0f, 1.0f, 0.0f}, {-1.0f, 1.6f, 1.8f}}),
        "negative extent rejected");
    Expect(!world::LevelGoalSpecIsValid({{0.0f, 1.0f, 0.0f}, {0.05f, 1.6f, 1.8f}}),
        "below-minimum extent rejected");

    const world::LevelGoalSpec goalA{{-21.0f, 3.8f, 0.0f}, {2.0f, 1.6f, 1.8f}};
    const world::LevelGoalSpec goalB{{8.0f, 1.0f, 0.0f}, {2.0f, 1.6f, 1.8f}};
    std::vector<world::LevelGoalSpec> goals{goalA, goalB};
    const core::Vec3 outside{0.0f, 0.8f, 0.0f};
    const core::Vec3 insideA = goalA.center;
    const core::Vec3 insideB = goalB.center;

    Expect(!world::PlayerOverlapsAnyLevelGoal({}, outside), "empty goals never overlap");
    Expect(!world::PlayerOverlapsAnyLevelGoal(goals, outside), "9. outside does not complete");
    Expect(world::PlayerOverlapsAnyLevelGoal(goals, insideA), "10. player overlap first goal");
    Expect(world::PlayerOverlapsAnyLevelGoal(goals, insideB), "player overlap second goal");

    gameplay::LevelCompletionState state{};
    Expect(
        !gameplay::TryCompleteLevelFromPlayerOverlap(state, goals, outside),
        "outside does not complete");
    Expect(!state.completed, "outside leaves incomplete");

    Expect(
        gameplay::TryCompleteLevelFromPlayerOverlap(state, goals, insideA),
        "10. first player overlap completes");
    Expect(state.completed, "completed after first overlap");
    Expect(
        !gameplay::TryCompleteLevelFromPlayerOverlap(state, goals, insideA),
        "12. staying inside does not retrigger");
    Expect(
        !gameplay::TryCompleteLevelFromPlayerOverlap(state, goals, insideB),
        "13. second goal does not retrigger");
    Expect(state.completed, "completion remains true");

    const core::Vec3 authoredCenter = goals[0].center;
    const core::Vec3 authoredSize = goals[0].size;
    Expect(Vec3Equal(goals[0].center, authoredCenter) && Vec3Equal(goals[0].size, authoredSize),
        "16. completion does not mutate authored data");

    gameplay::ResetLevelCompletionState(state);
    Expect(!state.completed, "14. Restart/Apply/reload reset");
    Expect(
        gameplay::TryCompleteLevelFromPlayerOverlap(state, goals, insideA),
        "reset allows completion again");

    const core::Vec3 justOutside{
        goalA.center.x + goalA.size.x * 0.5f + 0.1f,
        goalA.center.y,
        goalA.center.z};
    gameplay::LevelCompletionState nonPlayer{};
    Expect(
        !gameplay::TryCompleteLevelFromPlayerOverlap(nonPlayer, goals, justOutside),
        "11. non-player AABB overlap is not a completion test");
    Expect(!nonPlayer.completed, "only the player point is tested");

    gameplay::LevelCompletionState checkpointState{};
    Expect(
        !gameplay::TryCompleteLevelFromPlayerOverlap(checkpointState, goals, outside),
        "15. checkpoint-style outside overlap does not fabricate completion");
    Expect(!checkpointState.completed, "PhysicsWorld rebuild cannot invent completion state");

    gameplay::LevelCompletionState emptyGoals{};
    Expect(
        !gameplay::TryCompleteLevelFromPlayerOverlap(emptyGoals, {}, insideA),
        "no authored goals cannot complete");

    const render::LevelGoalVisualPlan editorPlan =
        render::MakeLevelGoalVisualPlan(render::LevelGoalViewKind::Editor);
    const render::LevelGoalVisualPlan gameplayPlan =
        render::MakeLevelGoalVisualPlan(render::LevelGoalViewKind::Gameplay);
    Expect(editorPlan.drawAuthoredVolume && editorPlan.drawMarker,
        "1. Editor visualization includes authored AABB and marker");
    Expect(
        editorPlan.volumeFillAlpha == render::kLevelGoalEditorVolumeFillAlpha
            && editorPlan.volumeFillAlpha < 255,
        "2. Editor AABB uses translucent presentation alpha");
    Expect(!gameplayPlan.drawAuthoredVolume && gameplayPlan.drawMarker,
        "3/4. Gameplay/Release omit AABB and keep the marker");
    Expect(
        render::MakeLevelGoalVisualPlan(render::LevelGoalViewKindFromEditor(false)).drawMarker
            && !render::MakeLevelGoalVisualPlan(render::LevelGoalViewKindFromEditor(false))
                    .drawAuthoredVolume,
        "5. Release uses the gameplay visualization rule");
    Expect(
        world::PlayerOverlapsAnyLevelGoal(goals, insideA),
        "6. gameplay overlap still uses the full authored AABB");

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d LevelGoalTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("LevelGoalTest passed\n");
    return 0;
}
