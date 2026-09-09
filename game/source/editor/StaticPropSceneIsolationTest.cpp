// Correction 3: Static Prop DrawModel isolation against later greybox draws.
// Hidden window + render texture. Uses repository test_static.glb always, and
// the imported Chest/Barrel GLBs when present. Not a screenshot test: it
// counts non-background and player-cube pixels so a model cannot erase the
// world or leave renderer state dirty for the next frame.

#include "raylib.h"
#include "rlgl.h"

#include <cstdio>
#include <filesystem>
#include <string>

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

constexpr Color kBackground{32, 36, 48, 255};
constexpr Color kPlayer{220, 96, 64, 255};

struct PixelCounts
{
    int nonBackground = 0;
    int player = 0;
    int bright = 0;
};

void RestoreAfterModel()
{
    rlDrawRenderBatchActive();
    rlEnableShader(rlGetShaderIdDefault());
    const int* locs = rlGetShaderLocsDefault();
    if (locs != nullptr)
    {
        if (locs[SHADER_LOC_COLOR_DIFFUSE] >= 0)
        {
            const float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
            rlSetUniform(locs[SHADER_LOC_COLOR_DIFFUSE], white, SHADER_UNIFORM_VEC4, 1);
        }
        if (locs[SHADER_LOC_MAP_DIFFUSE] >= 0)
        {
            const int slot0 = 0;
            rlSetUniform(locs[SHADER_LOC_MAP_DIFFUSE], &slot0, SHADER_UNIFORM_INT, 1);
        }
    }
    rlActiveTextureSlot(0);
    rlEnableTexture(rlGetTextureIdDefault());
}

PixelCounts CountPixels(const Image& image)
{
    PixelCounts counts{};
    if (image.data == nullptr)
    {
        return counts;
    }
    Color* pixels = LoadImageColors(image);
    const int total = image.width * image.height;
    for (int i = 0; i < total; ++i)
    {
        const Color pixel = pixels[i];
        if (pixel.r != kBackground.r || pixel.g != kBackground.g || pixel.b != kBackground.b)
        {
            ++counts.nonBackground;
        }
        if (pixel.r >= 100 && pixel.r > pixel.b + 20)
        {
            ++counts.player;
        }
        if (static_cast<int>(pixel.r) + pixel.g + pixel.b >= 400)
        {
            ++counts.bright;
        }
    }
    UnloadImageColors(pixels);
    return counts;
}

PixelCounts RenderScene(const Model* model, Vector3 position)
{
    RenderTexture2D target = LoadRenderTexture(640, 360);
    BeginTextureMode(target);
    ClearBackground(kBackground);
    Camera3D camera{};
    camera.target = Vector3{0.0f, 0.8f, 0.0f};
    camera.position = Vector3{2.0f, 4.3f, 12.0f};
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy = 40.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    BeginMode3D(camera);
    DrawGrid(20, 1.0f);
    DrawCube(Vector3{0.0f, -0.25f, 0.0f}, 56.0f, 0.5f, 8.0f, Color{90, 96, 108, 255});
    rlDrawRenderBatchActive();
    if (model != nullptr)
    {
        rlPushMatrix();
        rlTranslatef(position.x, position.y, position.z);
        DrawModel(*model, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
        rlPopMatrix();
    }
    RestoreAfterModel();
    DrawCube(Vector3{0.0f, 0.8f, 0.0f}, 1.0f, 1.6f, 1.0f, kPlayer);
    EndMode3D();
    EndTextureMode();
    Image image = LoadImageFromTexture(target.texture);
    UnloadRenderTexture(target);
    const PixelCounts counts = CountPixels(image);
    UnloadImage(image);
    return counts;
}

void TestIsolation(const char* path, const char* label, float maxExtent)
{
    Model model = LoadModel(path);
    const BoundingBox box = GetModelBoundingBox(model);
    const float width = box.max.x - box.min.x;
    const float height = box.max.y - box.min.y;
    const float depth = box.max.z - box.min.z;
    Expect(model.meshCount > 0, "model has meshes");
    Expect(
        model.transform.m0 == 1.0f && model.transform.m5 == 1.0f && model.transform.m10 == 1.0f
            && model.transform.m12 == 0.0f && model.transform.m13 == 0.0f
            && model.transform.m14 == 0.0f,
        "shared Model.transform stays identity after LoadModel");
    Expect(width > 0.05f && height > 0.05f && depth > 0.05f, "loaded bounds are real");
    Expect(width < maxExtent && height < maxExtent && depth < maxExtent, label);

    const PixelCounts none = RenderScene(nullptr, {});
    const PixelCounts nearby = RenderScene(&model, {0.42f, 1.54f, 0.0f});
    const PixelCounts atCamera = RenderScene(&model, {2.0f, 4.3f, 12.0f});
    const PixelCounts farAway = RenderScene(&model, {80.0f, 0.0f, 80.0f});
    Expect(none.nonBackground > 10000, "baseline world submits visible pixels");
    Expect(none.player > 100, "baseline player cube is visible");
    Expect(
        nearby.nonBackground >= none.nonBackground - 64,
        "nearby model does not erase later world pixels");
    Expect(nearby.player > 0, "nearby model cannot prevent the later player cube from drawing");
    Expect(
        farAway.player >= none.player - 16,
        "after a covering pose, a later far pose still draws the player cube");
    Expect(
        farAway.nonBackground >= none.nonBackground - 64,
        "a later frame with the model out of view keeps the world");

    std::printf(
        "%s isolation: none=%d/%d nearby=%d/%d camera=%d/%d far=%d/%d size=%.3fx%.3fx%.3f\n",
        label,
        none.nonBackground,
        none.player,
        nearby.nonBackground,
        nearby.player,
        atCamera.nonBackground,
        atCamera.player,
        farAway.nonBackground,
        farAway.player,
        width,
        height,
        depth);
    UnloadModel(model);
}

void TestUnloadLifecycle(const char* path, const char* label)
{
    RenderTexture2D target = LoadRenderTexture(640, 360);
    Model model = LoadModel(path);
    Expect(model.meshCount > 0, "lifecycle model has meshes");

    const auto render = [&](const Model* drawn) -> PixelCounts {
        BeginTextureMode(target);
        ClearBackground(kBackground);
        Camera3D camera{};
        camera.target = Vector3{0.0f, 0.8f, 0.0f};
        camera.position = Vector3{2.0f, 4.3f, 12.0f};
        camera.up = Vector3{0.0f, 1.0f, 0.0f};
        camera.fovy = 40.0f;
        camera.projection = CAMERA_PERSPECTIVE;
        BeginMode3D(camera);
        RestoreAfterModel();
        DrawGrid(20, 1.0f);
        DrawCube(Vector3{0.0f, -0.25f, 0.0f}, 56.0f, 0.5f, 8.0f, Color{90, 96, 108, 255});
        rlDrawRenderBatchActive();
        if (drawn != nullptr)
        {
            rlPushMatrix();
            rlTranslatef(0.42f, 1.54f, 0.0f);
            DrawModel(*drawn, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
            rlPopMatrix();
            RestoreAfterModel();
        }
        DrawCube(Vector3{0.0f, 0.8f, 0.0f}, 1.0f, 1.6f, 1.0f, kPlayer);
        EndMode3D();
        EndTextureMode();
        Image image = LoadImageFromTexture(target.texture);
        const PixelCounts counts = CountPixels(image);
        UnloadImage(image);
        return counts;
    };

    const PixelCounts baseline = render(nullptr);
    const PixelCounts withModel = render(&model);
    {
        BeginTextureMode(target);
        ClearBackground(kBackground);
        Camera3D camera{};
        camera.target = Vector3{0.0f, 0.8f, 0.0f};
        camera.position = Vector3{2.0f, 4.3f, 12.0f};
        camera.up = Vector3{0.0f, 1.0f, 0.0f};
        camera.fovy = 40.0f;
        camera.projection = CAMERA_PERSPECTIVE;
        BeginMode3D(camera);
        RestoreAfterModel();
        DrawGrid(20, 1.0f);
        DrawCube(Vector3{0.0f, -0.25f, 0.0f}, 56.0f, 0.5f, 8.0f, Color{90, 96, 108, 255});
        for (int pass = 0; pass < 8; ++pass)
        {
            rlPushMatrix();
            rlTranslatef(0.42f, 1.54f, 0.0f);
            DrawModel(model, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, Color{96, 220, 236, 160});
            rlPopMatrix();
            RestoreAfterModel();
        }
        DrawCube(Vector3{0.0f, 0.8f, 0.0f}, 1.0f, 1.6f, 1.0f, kPlayer);
        EndMode3D();
        EndTextureMode();
        Image image = LoadImageFromTexture(target.texture);
        const PixelCounts repeated = CountPixels(image);
        UnloadImage(image);
        Expect(
            repeated.player > 0,
            "placement-tint DrawModel eight times keeps the later player cube (no per-frame LoadModel)");
    }
    UnloadModel(model);
    model = {};
    const PixelCounts afterUnload = render(nullptr);
    const PixelCounts afterUnloadAgain = render(nullptr);
    UnloadRenderTexture(target);

    Expect(baseline.nonBackground > 10000, "lifecycle baseline has world pixels");
    Expect(withModel.player > 0, "model frame still submits the later player cube");
    Expect(
        afterUnload.nonBackground >= baseline.nonBackground - 64
            && afterUnload.player >= baseline.player - 16,
        "UnloadModel then a zero-instance frame matches the baseline world");
    Expect(
        afterUnloadAgain.nonBackground >= baseline.nonBackground - 64
            && afterUnloadAgain.player >= baseline.player - 16,
        "a second zero-instance frame stays at baseline (no accumulated state)");
    std::printf(
        "%s unload lifecycle: base=%d/%d with=%d/%d after=%d/%d again=%d/%d\n",
        label,
        baseline.nonBackground,
        baseline.player,
        withModel.nonBackground,
        withModel.player,
        afterUnload.nonBackground,
        afterUnload.player,
        afterUnloadAgain.nonBackground,
        afterUnloadAgain.player);
}

void TestOptional(const char* macroPath, const char* label, float maxExtent)
{
    const std::filesystem::path path{macroPath};
    if (!std::filesystem::is_regular_file(path))
    {
        std::printf("%s not present; skipped.\n", label);
        return;
    }
    TestIsolation(path.string().c_str(), label, maxExtent);
    TestUnloadLifecycle(path.string().c_str(), label);
}
}

int main()
{
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(64, 64, "StaticPropSceneIsolationTest");

#if defined(PLATFORMER_TEST_STATIC_GLB)
    TestIsolation(PLATFORMER_TEST_STATIC_GLB, "test_static.glb remains < 8 units", 8.0f);
    TestUnloadLifecycle(PLATFORMER_TEST_STATIC_GLB, "test_static.glb");
#endif
#if defined(PLATFORMER_CHEST_GLB)
    TestOptional(PLATFORMER_CHEST_GLB, "Chest remains < 2.5 units after node bake", 2.5f);
#endif
#if defined(PLATFORMER_BARREL_GLB)
    TestOptional(PLATFORMER_BARREL_GLB, "Barrel remains < 8 units", 8.0f);
#endif

    CloseWindow();
    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d Static Prop scene isolation test(s) failed.\n", gFailures);
        return 1;
    }
    std::printf("Static Prop scene isolation tests passed.\n");
    return 0;
}
