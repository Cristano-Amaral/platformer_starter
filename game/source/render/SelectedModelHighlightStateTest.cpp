// Milestone 58.2: editor selected-model highlight restores rlgl state.
// Hidden window. Placeholder cube path does not need staged assets.

#include "render/StaticModelScene.h"
#include "world/StaticProp.h"

#include "raylib.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>

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

world::StaticPropSpec MakeSpec()
{
    world::StaticPropSpec spec{};
    spec.modelIdentity = "models/missing_highlight.glb";
    spec.position = {1.0f, 1.0f, 0.0f};
    spec.rotationDegrees = {15.0f, 30.0f, 5.0f};
    spec.scale = {1.25f, 0.8f, 1.1f};
    return spec;
}
}

int main()
{
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(640, 360, "SelectedModelHighlightStateTest");

    render::StaticModelSceneStore store;
    const world::StaticPropSpec spec = MakeSpec();
    Expect(!store.HasModel(spec.modelIdentity), "missing identity does not fabricate a model");
    const std::size_t loadsBefore = store.LoadCount();

    const Camera3D camera = MakeCamera();
    BeginDrawing();
    ClearBackground(Color{32, 36, 48, 255});
    BeginMode3D(camera);
    rlSetClipPlanes(RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
    render::RestoreGreyboxImmediateState();
    DrawCube(Vector3{0.0f, 0.0f, 0.0f}, 2.0f, 0.4f, 2.0f, Color{78, 84, 96, 255});
    const Snapshot before = QuerySnapshot();
    store.ResetDrawStats();
    store.DrawSelectionHighlight(spec);
    Expect(store.HighlightSubmissionCount() == 1, "ghost records one highlight submission");
    Expect(store.DrawSubmissionCount() == 2, "depth pass plus x-ray pass reuse DrawProp");
    Expect(store.LoadCount() == loadsBefore, "ghost does not LoadModel");
    Expect(!store.HasModel(spec.modelIdentity), "ghost does not create a second model cache");
    const Snapshot after = QuerySnapshot();
    Expect(SnapshotNear(before, after), "ghost pass restores depth/blend/cull/matrix");
    Expect(before.depthTest && after.depthTest, "depth test restored");
    Expect(before.depthWriteMask && after.depthWriteMask, "depth mask restored");
    Expect(before.cullFace && after.cullFace, "culling restored");

    store.ResetDrawStats();
    const Snapshot gameplayBefore = QuerySnapshot();
    store.DrawGameplayTargetHighlight(spec);
    Expect(
        store.GameplayHighlightSubmissionCount() == 1,
        "gameplay target records one highlight submission");
    Expect(store.HighlightSubmissionCount() == 0, "gameplay highlight is not the editor ghost counter");
    Expect(store.DrawSubmissionCount() == 1, "gameplay highlight is a single tinted pass");
    Expect(store.LoadCount() == loadsBefore, "gameplay highlight does not LoadModel");
    Expect(!store.HasModel(spec.modelIdentity), "gameplay highlight does not create a model cache");
    const Snapshot gameplayAfter = QuerySnapshot();
    Expect(
        SnapshotNear(gameplayBefore, gameplayAfter),
        "41-44. gameplay highlight restores matrix/depth/blend/cull");
    Expect(gameplayBefore.depthTest && gameplayAfter.depthTest, "gameplay depth test restored");
    Expect(
        gameplayBefore.depthWriteMask && gameplayAfter.depthWriteMask,
        "gameplay depth mask restored");
    Expect(gameplayBefore.cullFace && gameplayAfter.cullFace, "gameplay culling restored");

    store.ResetDrawStats();
    const Snapshot skipBefore = QuerySnapshot();
    store.DrawGameplayTargetHighlight(spec, 0);
    Expect(store.GameplayHighlightSubmissionCount() == 0, "46. alpha 0 skips gameplay highlight");
    Expect(store.DrawSubmissionCount() == 0, "alpha 0 does not DrawProp");
    Expect(store.LoadCount() == loadsBefore, "alpha 0 does not LoadModel");
    const Snapshot skipAfter = QuerySnapshot();
    Expect(SnapshotNear(skipBefore, skipAfter), "zero-intensity skip leaves renderer state valid");

    store.ResetDrawStats();
    const Snapshot fullBefore = QuerySnapshot();
    store.DrawGameplayTargetHighlight(spec, 255);
    Expect(store.GameplayHighlightSubmissionCount() == 1, "24. intensity 1 still uses one tinted pass");
    Expect(store.DrawSubmissionCount() == 1, "full intensity reuses DrawPropTinted");
    const Snapshot fullAfter = QuerySnapshot();
    Expect(SnapshotNear(fullBefore, fullAfter), "47. non-zero path restores renderer state");
    store.ResetDrawStats();
    const Snapshot goldBefore = QuerySnapshot();
    store.DrawGameplayTargetHighlight(spec, 255, 255, 255, 180);
    Expect(store.GameplayHighlightSubmissionCount() == 1, "Gold Amount 0 uses the same extra pass");
    const Snapshot goldAfter = QuerySnapshot();
    Expect(SnapshotNear(goldBefore, goldAfter), "47. Gold Amount RGB path restores renderer state");
    DrawCube(Vector3{2.0f, 1.0f, 0.0f}, 0.5f, 0.5f, 0.5f, Color{216, 96, 72, 255});
    DrawCubeWires(Vector3{2.0f, 1.0f, 0.0f}, 0.5f, 0.5f, 0.5f, Color{24, 26, 32, 255});
    EndMode3D();
    DrawText("ok", 8, 8, 16, RAYWHITE);
    EndDrawing();

    CloseWindow();
    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d selected-model highlight state test(s) failed.\n", gFailures);
        return 1;
    }
    std::printf("Selected-model highlight state tests passed.\n");
    return 0;
}
