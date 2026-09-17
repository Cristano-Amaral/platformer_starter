// Milestone 63 correction: Editor draws a translucent authored AABB; Gameplay
// and Release draw the marker only. Hidden window. Not shipped.

#include "gameplay/LevelCompletionState.h"
#include "render/LevelGoalVisualization.h"
#include "render/StaticModelScene.h"
#include "world/LevelGoal.h"

#include "raylib.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace
{
int gFailures = 0;

void Expect(bool condition, const char* what)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++gFailures;
    }
}

constexpr unsigned int kGlBlend = 0x0BE2;
constexpr unsigned int kGlDepthTest = 0x0B71;
constexpr unsigned int kGlCullFace = 0x0B44;
constexpr unsigned int kGlDepthWritemask = 0x0B72;

using GlGetBooleanvFn = void (*)(unsigned int, unsigned char*);

struct Snapshot
{
    bool blend = false;
    bool depthTest = false;
    bool depthWriteMask = false;
    bool cullFace = false;
    float transform[16]{};
};

Snapshot QuerySnapshot()
{
    Snapshot state{};
    auto getBooleanv = reinterpret_cast<GlGetBooleanvFn>(rlGetProcAddress("glGetBooleanv"));
    if (getBooleanv != nullptr)
    {
        unsigned char blend = 0;
        unsigned char depth = 0;
        unsigned char cull = 0;
        unsigned char depthMask = 0;
        getBooleanv(kGlBlend, &blend);
        getBooleanv(kGlDepthTest, &depth);
        getBooleanv(kGlCullFace, &cull);
        getBooleanv(kGlDepthWritemask, &depthMask);
        state.blend = blend != 0;
        state.depthTest = depth != 0;
        state.cullFace = cull != 0;
        state.depthWriteMask = depthMask != 0;
    }
    const Matrix transform = rlGetMatrixTransform();
    state.transform[0] = transform.m0;
    state.transform[1] = transform.m5;
    state.transform[2] = transform.m10;
    state.transform[3] = transform.m12;
    state.transform[4] = transform.m13;
    state.transform[5] = transform.m14;
    state.transform[6] = transform.m15;
    return state;
}

bool SnapshotNear(const Snapshot& a, const Snapshot& b)
{
    return a.blend == b.blend && a.depthTest == b.depthTest && a.depthWriteMask == b.depthWriteMask
        && a.cullFace == b.cullFace
        && std::fabs(a.transform[0] - b.transform[0]) < 1.0e-4f
        && std::fabs(a.transform[1] - b.transform[1]) < 1.0e-4f
        && std::fabs(a.transform[2] - b.transform[2]) < 1.0e-4f
        && std::fabs(a.transform[3] - b.transform[3]) < 1.0e-4f
        && std::fabs(a.transform[4] - b.transform[4]) < 1.0e-4f
        && std::fabs(a.transform[5] - b.transform[5]) < 1.0e-4f;
}

Camera3D MakeCamera()
{
    Camera3D camera{};
    camera.position = Vector3{0.0f, 4.0f, 12.0f};
    camera.target = Vector3{0.0f, 1.0f, 0.0f};
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy = 40.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}
}

int main()
{
    const render::LevelGoalVisualPlan editor =
        render::MakeLevelGoalVisualPlan(render::LevelGoalViewKind::Editor);
    const render::LevelGoalVisualPlan gameplay =
        render::MakeLevelGoalVisualPlan(render::LevelGoalViewKind::Gameplay);
    const render::LevelGoalVisualPlan release =
        render::MakeLevelGoalVisualPlan(render::LevelGoalViewKindFromEditor(false));

    Expect(editor.drawAuthoredVolume, "1. Editor includes authored AABB visualization");
    Expect(
        editor.volumeFillAlpha == render::kLevelGoalEditorVolumeFillAlpha
            && editor.volumeFillAlpha < 255 && editor.volumeFillAlpha > 0,
        "2. Editor AABB uses translucent fill alpha");
    Expect(
        editor.volumeWireAlpha == render::kLevelGoalEditorVolumeWireAlpha,
        "2. Editor AABB uses intended wire alpha");
    Expect(editor.drawMarker, "4. Editor still draws the goal marker");

    Expect(!gameplay.drawAuthoredVolume, "3. Gameplay does not draw the authored AABB");
    Expect(gameplay.drawMarker, "4. Gameplay still draws the goal marker");
    Expect(gameplay.volumeFillAlpha == 0, "3. Gameplay volume alpha is unused");

    Expect(!release.drawAuthoredVolume && release.drawMarker,
        "5. Release uses the same gameplay visualization rule");
    Expect(
        gameplay.drawAuthoredVolume == release.drawAuthoredVolume
            && gameplay.drawMarker == release.drawMarker,
        "5. Gameplay and Release share one plan");

    const world::LevelGoalSpec goal{{-21.0f, 3.8f, 0.0f}, {2.0f, 1.6f, 1.8f}};
    Expect(world::PointInsideGoal(goal, goal.center), "6. overlap still uses full authored AABB");
    Expect(
        !world::PointInsideGoal(
            goal,
            {goal.center.x + goal.size.x * 0.5f + 0.1f, goal.center.y, goal.center.z}),
        "6. outside the authored AABB still misses");
    gameplay::LevelCompletionState state{};
    std::vector<world::LevelGoalSpec> goals{goal};
    Expect(
        gameplay::TryCompleteLevelFromPlayerOverlap(state, goals, goal.center),
        "6. completion still uses the full authored AABB");

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(640, 360, "LevelGoalVisualizationTest");

    const Camera3D camera = MakeCamera();
    BeginDrawing();
    ClearBackground(Color{32, 36, 48, 255});
    BeginMode3D(camera);
    rlSetClipPlanes(RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
    render::RestoreGreyboxImmediateState();
    DrawCube(Vector3{0.0f, 0.0f, 0.0f}, 2.0f, 0.4f, 2.0f, Color{78, 84, 96, 255});
    const Snapshot beforeEditor = QuerySnapshot();
    render::DrawLevelGoalPresentation(goal, false, render::LevelGoalViewKind::Editor);
    const Snapshot afterEditor = QuerySnapshot();
    Expect(SnapshotNear(beforeEditor, afterEditor), "7. editor translucent volume restores state");
    Expect(beforeEditor.depthTest && afterEditor.depthTest, "7. depth test restored");
    Expect(beforeEditor.depthWriteMask && afterEditor.depthWriteMask, "7. depth mask restored");
    Expect(beforeEditor.cullFace && afterEditor.cullFace, "7. culling restored");
    Expect(beforeEditor.blend == afterEditor.blend, "7. blend restored");

    const Snapshot beforeGameplay = QuerySnapshot();
    render::DrawLevelGoalPresentation(goal, false, render::LevelGoalViewKind::Gameplay);
    const Snapshot afterGameplay = QuerySnapshot();
    Expect(SnapshotNear(beforeGameplay, afterGameplay), "7. gameplay marker restores state");
    DrawCube(Vector3{2.0f, 1.0f, 0.0f}, 0.5f, 0.5f, 0.5f, Color{216, 96, 72, 255});
    EndMode3D();
    EndDrawing();
    CloseWindow();

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d LevelGoalVisualizationTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("LevelGoalVisualizationTest passed\n");
    return 0;
}
