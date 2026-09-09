// M49 Correction 6: default-framebuffer greybox immediate-mode state after
// Chest DrawModel. Hidden 1280x720 window, no RenderTexture. Compares
// pre-DrawGrid snapshots and tries single-axis recoveries. Not shipped.

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <cmath>
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
constexpr Color kGround{78, 84, 96, 255};
constexpr int kWidth = 1280;
constexpr int kHeight = 720;

constexpr unsigned int kGlViewport = 0x0BA2;
constexpr unsigned int kGlCurrentProgram = 0x8B8D;
constexpr unsigned int kGlVertexArrayBinding = 0x85B5;
constexpr unsigned int kGlArrayBufferBinding = 0x8894;
constexpr unsigned int kGlElementArrayBufferBinding = 0x8895;
constexpr unsigned int kGlTextureBinding2D = 0x8069;
constexpr unsigned int kGlBlend = 0x0BE2;
constexpr unsigned int kGlDepthTest = 0x0B71;
constexpr unsigned int kGlCullFace = 0x0B44;
constexpr unsigned int kGlDepthWritemask = 0x0B72;
constexpr unsigned int kGlDepthFunc = 0x0B74;
constexpr unsigned int kGlDepthRange = 0x0B70;
constexpr unsigned int kGlColorWritemask = 0x0C23;
constexpr unsigned int kGlCullFaceMode = 0x0B45;
constexpr unsigned int kGlFrontFace = 0x0B46;
constexpr unsigned int kGlVertexAttribArrayEnabled = 0x8622;
constexpr unsigned int kGlActiveTexture = 0x84E0;

using GlGetIntegervFn = void (*)(unsigned int, int*);
using GlGetBooleanvFn = void (*)(unsigned int, unsigned char*);
using GlGetFloatvFn = void (*)(unsigned int, float*);
using GlGetUniformfvFn = void (*)(unsigned int, int, float*);
using GlGetVertexAttribivFn = void (*)(unsigned int, unsigned int, int*);

struct Snapshot
{
    int viewport[4]{};
    int currentProgram = 0;
    unsigned int defaultShaderId = 0;
    int vertexArray = 0;
    int arrayBuffer = 0;
    int elementBuffer = 0;
    int attribEnabled[4]{};
    int activeTexture = 0;
    int textureBinding2d = 0;
    bool blend = false;
    bool depthTest = false;
    bool depthWriteMask = false;
    int depthFunc = 0;
    float depthRange[2]{};
    unsigned char colorWriteMask[4]{};
    bool cullFace = false;
    int cullFaceMode = 0;
    int frontFace = 0;
    float colDiffuse[4]{};
    float mvpUniform[16]{};
    bool mvpUniformAvailable = false;
    float projection[16]{};
    float modelview[16]{};
    float transform[16]{};
    double cullNear = 0.0;
    double cullFar = 0.0;
};

void CopyMatrix(float* dest, const Matrix& source)
{
    dest[0] = source.m0;
    dest[1] = source.m1;
    dest[2] = source.m2;
    dest[3] = source.m3;
    dest[4] = source.m4;
    dest[5] = source.m5;
    dest[6] = source.m6;
    dest[7] = source.m7;
    dest[8] = source.m8;
    dest[9] = source.m9;
    dest[10] = source.m10;
    dest[11] = source.m11;
    dest[12] = source.m12;
    dest[13] = source.m13;
    dest[14] = source.m14;
    dest[15] = source.m15;
}

Snapshot QuerySnapshot()
{
    Snapshot state{};
    auto getIntegerv = reinterpret_cast<GlGetIntegervFn>(rlGetProcAddress("glGetIntegerv"));
    auto getBooleanv = reinterpret_cast<GlGetBooleanvFn>(rlGetProcAddress("glGetBooleanv"));
    auto getFloatv = reinterpret_cast<GlGetFloatvFn>(rlGetProcAddress("glGetFloatv"));
    auto getUniformfv = reinterpret_cast<GlGetUniformfvFn>(rlGetProcAddress("glGetUniformfv"));
    auto getVertexAttribiv =
        reinterpret_cast<GlGetVertexAttribivFn>(rlGetProcAddress("glGetVertexAttribiv"));
    if (getIntegerv != nullptr)
    {
        getIntegerv(kGlViewport, state.viewport);
        getIntegerv(kGlCurrentProgram, &state.currentProgram);
        getIntegerv(kGlVertexArrayBinding, &state.vertexArray);
        getIntegerv(kGlArrayBufferBinding, &state.arrayBuffer);
        getIntegerv(kGlElementArrayBufferBinding, &state.elementBuffer);
        getIntegerv(kGlTextureBinding2D, &state.textureBinding2d);
        getIntegerv(kGlActiveTexture, &state.activeTexture);
        getIntegerv(kGlDepthFunc, &state.depthFunc);
        getIntegerv(kGlCullFaceMode, &state.cullFaceMode);
        getIntegerv(kGlFrontFace, &state.frontFace);
    }
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
        getBooleanv(kGlColorWritemask, state.colorWriteMask);
        state.blend = blend != 0;
        state.depthTest = depth != 0;
        state.cullFace = cull != 0;
        state.depthWriteMask = depthMask != 0;
    }
    if (getFloatv != nullptr)
    {
        getFloatv(kGlDepthRange, state.depthRange);
    }
    if (getVertexAttribiv != nullptr)
    {
        for (unsigned int attrib = 0; attrib < 4; ++attrib)
        {
            getVertexAttribiv(attrib, kGlVertexAttribArrayEnabled, &state.attribEnabled[attrib]);
        }
    }
    state.defaultShaderId = rlGetShaderIdDefault();
    CopyMatrix(state.projection, rlGetMatrixProjection());
    CopyMatrix(state.modelview, rlGetMatrixModelview());
    CopyMatrix(state.transform, rlGetMatrixTransform());
    state.cullNear = rlGetCullDistanceNear();
    state.cullFar = rlGetCullDistanceFar();
    const int* locs = rlGetShaderLocsDefault();
    if (getUniformfv != nullptr && state.defaultShaderId != 0 && locs != nullptr)
    {
        if (locs[SHADER_LOC_COLOR_DIFFUSE] >= 0)
        {
            getUniformfv(state.defaultShaderId, locs[SHADER_LOC_COLOR_DIFFUSE], state.colDiffuse);
        }
        if (locs[SHADER_LOC_MATRIX_MVP] >= 0)
        {
            getUniformfv(state.defaultShaderId, locs[SHADER_LOC_MATRIX_MVP], state.mvpUniform);
            state.mvpUniformAvailable = true;
        }
    }
    return state;
}

void PrintMatrix(const char* name, const float* m)
{
    std::printf("  %s:", name);
    for (int i = 0; i < 16; ++i)
    {
        std::printf(" %.6g", static_cast<double>(m[i]));
    }
    std::printf("\n");
}

bool MatrixHasNan(const float* m)
{
    for (int i = 0; i < 16; ++i)
    {
        if (!std::isfinite(m[i]))
        {
            return true;
        }
    }
    return false;
}

bool MatricesDiffer(const float* a, const float* b, float epsilon)
{
    for (int i = 0; i < 16; ++i)
    {
        if (std::fabs(a[i] - b[i]) > epsilon)
        {
            return true;
        }
    }
    return false;
}

void PrintSnapshot(const char* label, const Snapshot& state)
{
    std::printf("snapshot %s\n", label);
    std::printf(
        "  viewport: %d %d %d %d\n",
        state.viewport[0],
        state.viewport[1],
        state.viewport[2],
        state.viewport[3]);
    std::printf(
        "  current program: %d default shader: %u\n",
        state.currentProgram,
        state.defaultShaderId);
    std::printf(
        "  VAO/VBO/EBO: %d %d %d\n",
        state.vertexArray,
        state.arrayBuffer,
        state.elementBuffer);
    std::printf(
        "  attrib enabled pos/uv/n/col: %d %d %d %d\n",
        state.attribEnabled[0],
        state.attribEnabled[1],
        state.attribEnabled[2],
        state.attribEnabled[3]);
    std::printf("  active texture: %d texture2D: %d\n", state.activeTexture, state.textureBinding2d);
    std::printf(
        "  blend=%d depthTest=%d depthMask=%d depthFunc=%d depthRange=%.4g %.4g\n",
        static_cast<int>(state.blend),
        static_cast<int>(state.depthTest),
        static_cast<int>(state.depthWriteMask),
        state.depthFunc,
        static_cast<double>(state.depthRange[0]),
        static_cast<double>(state.depthRange[1]));
    std::printf(
        "  color mask: %d %d %d %d cull=%d mode=%d front=%d\n",
        static_cast<int>(state.colorWriteMask[0]),
        static_cast<int>(state.colorWriteMask[1]),
        static_cast<int>(state.colorWriteMask[2]),
        static_cast<int>(state.colorWriteMask[3]),
        static_cast<int>(state.cullFace),
        state.cullFaceMode,
        state.frontFace);
    std::printf(
        "  colDiffuse: %.4g %.4g %.4g %.4g\n",
        static_cast<double>(state.colDiffuse[0]),
        static_cast<double>(state.colDiffuse[1]),
        static_cast<double>(state.colDiffuse[2]),
        static_cast<double>(state.colDiffuse[3]));
    PrintMatrix("rlgl projection", state.projection);
    PrintMatrix("rlgl modelview", state.modelview);
    PrintMatrix("rlgl transform", state.transform);
    std::printf(
        "  rlgl cull distance near/far: %.6g %.6g\n",
        state.cullNear,
        state.cullFar);
    if (state.mvpUniformAvailable)
    {
        PrintMatrix("default shader MVP", state.mvpUniform);
    }
    else
    {
        std::printf("  default shader MVP: unavailable\n");
    }
}

void PrintDiff(const char* label, const Snapshot& healthy, const Snapshot& broken)
{
    std::printf("diff %s\n", label);
    if (healthy.currentProgram != broken.currentProgram)
    {
        std::printf("  current program %d -> %d\n", healthy.currentProgram, broken.currentProgram);
    }
    if (healthy.vertexArray != broken.vertexArray)
    {
        std::printf("  VAO %d -> %d\n", healthy.vertexArray, broken.vertexArray);
    }
    if (healthy.arrayBuffer != broken.arrayBuffer)
    {
        std::printf("  VBO %d -> %d\n", healthy.arrayBuffer, broken.arrayBuffer);
    }
    if (healthy.elementBuffer != broken.elementBuffer)
    {
        std::printf("  EBO %d -> %d\n", healthy.elementBuffer, broken.elementBuffer);
    }
    for (int i = 0; i < 4; ++i)
    {
        if (healthy.attribEnabled[i] != broken.attribEnabled[i])
        {
            std::printf(
                "  attrib %d enabled %d -> %d\n",
                i,
                healthy.attribEnabled[i],
                broken.attribEnabled[i]);
        }
    }
    if (healthy.depthTest != broken.depthTest || healthy.depthWriteMask != broken.depthWriteMask
        || healthy.depthFunc != broken.depthFunc)
    {
        std::printf(
            "  depth test/mask/func %d/%d/%d -> %d/%d/%d\n",
            static_cast<int>(healthy.depthTest),
            static_cast<int>(healthy.depthWriteMask),
            healthy.depthFunc,
            static_cast<int>(broken.depthTest),
            static_cast<int>(broken.depthWriteMask),
            broken.depthFunc);
    }
    if (healthy.cullFace != broken.cullFace || healthy.frontFace != broken.frontFace)
    {
        std::printf(
            "  cull/front %d/%d -> %d/%d\n",
            static_cast<int>(healthy.cullFace),
            healthy.frontFace,
            static_cast<int>(broken.cullFace),
            broken.frontFace);
    }
    if (MatricesDiffer(healthy.projection, broken.projection, 0.0001f))
    {
        std::printf("  projection differs (nan healthy=%d broken=%d)\n",
                    static_cast<int>(MatrixHasNan(healthy.projection)),
                    static_cast<int>(MatrixHasNan(broken.projection)));
    }
    else
    {
        std::printf("  projection: identical\n");
    }
    if (MatricesDiffer(healthy.modelview, broken.modelview, 0.0001f))
    {
        std::printf("  modelview differs (nan healthy=%d broken=%d)\n",
                    static_cast<int>(MatrixHasNan(healthy.modelview)),
                    static_cast<int>(MatrixHasNan(broken.modelview)));
    }
    else
    {
        std::printf("  modelview: identical\n");
    }
    if (MatricesDiffer(healthy.transform, broken.transform, 0.0001f))
    {
        std::printf("  transform differs\n");
    }
    else
    {
        std::printf("  transform: identical\n");
    }
    if (std::fabs(healthy.cullNear - broken.cullNear) > 0.0001
        || std::fabs(healthy.cullFar - broken.cullFar) > 0.0001)
    {
        std::printf(
            "  cull near/far %.6g/%.6g -> %.6g/%.6g\n",
            healthy.cullNear,
            healthy.cullFar,
            broken.cullNear,
            broken.cullFar);
    }
    else
    {
        std::printf("  cull near/far: identical\n");
    }
}

// Current production restore (Correction 4/5). Experiments replace this.
void ProductionRestore()
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

void RestoreMatricesOnly(const Camera3D& camera)
{
    rlMatrixMode(RL_PROJECTION);
    rlLoadIdentity();
    const double aspect = static_cast<double>(GetScreenWidth()) / static_cast<double>(GetScreenHeight());
    const double top = RL_CULL_DISTANCE_NEAR * std::tan(static_cast<double>(camera.fovy) * 0.5 * DEG2RAD);
    const double right = top * aspect;
    rlFrustum(-right, right, -top, top, RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
    const Matrix matView = MatrixLookAt(camera.position, camera.target, camera.up);
    rlMultMatrixf(MatrixToFloat(matView));
}

void RestoreVertexInputOnly()
{
    rlDrawRenderBatchActive();
    rlDisableVertexArray();
    rlDisableVertexBuffer();
    rlDisableVertexBufferElement();
    rlSetShader(rlGetShaderIdDefault(), rlGetShaderLocsDefault());
}

void RestoreShaderTextureOnly()
{
    ProductionRestore();
}

Camera3D MakeCamera()
{
    Camera3D camera{};
    camera.target = Vector3{0.0f, 0.8f, 0.0f};
    camera.position = Vector3{2.0f, 4.3f, 12.0f};
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy = 40.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}

void DrawGreyboxWorld()
{
    DrawGrid(20, 1.0f);
    DrawCube(Vector3{0.0f, -0.25f, 0.0f}, 56.0f, 0.5f, 8.0f, kGround);
    DrawCube(Vector3{0.0f, 0.8f, 0.0f}, 1.0f, 1.6f, 1.0f, Color{216, 96, 72, 255});
}

void DrawHud()
{
    DrawText("HUD", 20, 20, 28, WHITE);
}

int CountNonBackground(const Image& image)
{
    if (image.data == nullptr)
    {
        return 0;
    }
    Color* pixels = LoadImageColors(image);
    int count = 0;
    const int total = image.width * image.height;
    for (int i = 0; i < total; ++i)
    {
        const Color pixel = pixels[i];
        if (pixel.r != kBackground.r || pixel.g != kBackground.g || pixel.b != kBackground.b)
        {
            ++count;
        }
    }
    UnloadImageColors(pixels);
    return count;
}

void DrawPropLike(const Model& model)
{
    rlDrawRenderBatchActive();
    rlPushMatrix();
    rlTranslatef(-6.5f, 1.54f, 0.0f);
    DrawModel(model, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
    rlPopMatrix();
    ProductionRestore();
}

void DescribeModel(const char* label, const Model& model)
{
    std::printf("%s meshes=%d materials=%d\n", label, model.meshCount, model.materialCount);
    for (int i = 0; i < model.meshCount; ++i)
    {
        const Mesh& mesh = model.meshes[i];
        std::printf(
            "  mesh[%d] verts=%d vao=%u texcoords=%s indices=%s\n",
            i,
            mesh.vertexCount,
            mesh.vaoId,
            mesh.texcoords != nullptr ? "yes" : "null",
            mesh.indices != nullptr ? "yes" : "null");
    }
    for (int i = 0; i < model.materialCount; ++i)
    {
        const Material& material = model.materials[i];
        std::printf(
            "  mat[%d] shader=%u locs=%s diffuse tex=%u color=%d %d %d\n",
            i,
            material.shader.id,
            material.shader.locs != nullptr ? "yes" : "null",
            material.maps != nullptr ? material.maps[MATERIAL_MAP_DIFFUSE].texture.id : 0,
            material.maps != nullptr ? material.maps[MATERIAL_MAP_DIFFUSE].color.r : 0,
            material.maps != nullptr ? material.maps[MATERIAL_MAP_DIFFUSE].color.g : 0,
            material.maps != nullptr ? material.maps[MATERIAL_MAP_DIFFUSE].color.b : 0);
    }
}

enum class Experiment
{
    Production,
    Matrices,
    VertexInput,
    ShaderTexture
};

const char* ExperimentName(Experiment experiment)
{
    switch (experiment)
    {
    case Experiment::Matrices:
        return "A_matrices";
    case Experiment::VertexInput:
        return "B_vertex_input";
    case Experiment::ShaderTexture:
        return "C_shader_texture";
    case Experiment::Production:
    default:
        return "production_restore";
    }
}

int RenderGreyboxFrame(
    Experiment experiment,
    Snapshot* preGrid,
    bool drawDiagnosticTriangle,
    bool restoreClipPlanes = true)
{
    const Camera3D camera = MakeCamera();
    BeginDrawing();
    ClearBackground(kBackground);
    if (restoreClipPlanes)
    {
        rlSetClipPlanes(RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
    }
    BeginMode3D(camera);
    switch (experiment)
    {
    case Experiment::Matrices:
        RestoreMatricesOnly(camera);
        break;
    case Experiment::VertexInput:
        RestoreVertexInputOnly();
        break;
    case Experiment::ShaderTexture:
        RestoreShaderTextureOnly();
        break;
    case Experiment::Production:
        ProductionRestore();
        break;
    }
    if (preGrid != nullptr)
    {
        *preGrid = QuerySnapshot();
    }
    if (drawDiagnosticTriangle)
    {
        DrawTriangle3D(
            Vector3{-2.0f, 0.5f, 0.0f},
            Vector3{2.0f, 0.5f, 0.0f},
            Vector3{0.0f, 3.0f, 0.0f},
            Color{255, 64, 64, 255});
    }
    DrawGreyboxWorld();
    EndMode3D();
    DrawHud();
    rlDrawRenderBatchActive();
    const Image image = LoadImageFromScreen();
    const int pixels = CountNonBackground(image);
    UnloadImage(image);
    EndDrawing();
    return pixels;
}

void RenderModelFrame(const Model& model, Snapshot* beforeDraw, Snapshot* afterDraw)
{
    const Camera3D camera = MakeCamera();
    BeginDrawing();
    ClearBackground(kBackground);
    BeginMode3D(camera);
    ProductionRestore();
    DrawGreyboxWorld();
    if (beforeDraw != nullptr)
    {
        *beforeDraw = QuerySnapshot();
    }
    DrawPropLike(model);
    if (afterDraw != nullptr)
    {
        *afterDraw = QuerySnapshot();
    }
    EndMode3D();
    DrawHud();
    EndDrawing();
}

void RenderEditorThenGameplay(const Model& model)
{
    const Camera3D camera = MakeCamera();
    BeginDrawing();
    ClearBackground(kBackground);
    rlDrawRenderBatchActive();
    rlViewport(0, 0, 900, 520);
    rlMatrixMode(RL_PROJECTION);
    rlPushMatrix();
    rlLoadIdentity();
    const double aspect = 900.0 / 520.0;
    const double top = RL_CULL_DISTANCE_NEAR * std::tan(40.0 * 0.5 * DEG2RAD);
    const double right = top * aspect;
    rlFrustum(-right, right, -top, top, RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
    rlMultMatrixf(MatrixToFloat(MatrixLookAt(camera.position, camera.target, camera.up)));
    rlEnableDepthTest();
    ProductionRestore();
    DrawGreyboxWorld();
    DrawPropLike(model);
    EndMode3D();
    rlViewport(0, 0, GetScreenWidth(), GetScreenHeight());
    DrawHud();
    EndDrawing();
}

int RunModelSequence(const char* label, const Model& model, const Snapshot& healthyPreGrid)
{
    Snapshot beforeDraw{};
    Snapshot afterDraw{};
    RenderModelFrame(model, &beforeDraw, &afterDraw);
    std::printf("--- %s DrawModel before ---\n", label);
    PrintSnapshot("before_DrawModel", beforeDraw);
    std::printf("--- %s DrawModel after ---\n", label);
    PrintSnapshot("after_DrawModel", afterDraw);
    PrintDiff(label, beforeDraw, afterDraw);

    Snapshot afterPreGrid{};
    const int afterPixels = RenderGreyboxFrame(Experiment::Production, &afterPreGrid, true);
    std::printf("--- %s next Gameplay pre-DrawGrid ---\n", label);
    PrintSnapshot("next_frame_pre_grid", afterPreGrid);
    PrintDiff((std::string(label) + " vs healthy pre-grid").c_str(), healthyPreGrid, afterPreGrid);
    std::printf("%s next-frame greybox non-bg pixels=%d\n", label, afterPixels);
    return afterPixels;
}

Model LoadIfPresent(const char* path)
{
    Model model{};
    if (path == nullptr || !std::filesystem::is_regular_file(std::filesystem::path{path}))
    {
        return model;
    }
    model = LoadModel(path);
    return model;
}
}

int main()
{
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(kWidth, kHeight, "GreyboxImmediateStateTest");

    Snapshot healthyPreGrid{};
    const int baselinePixels = RenderGreyboxFrame(Experiment::Production, &healthyPreGrid, true);
    PrintSnapshot("healthy_baseline_pre_grid", healthyPreGrid);
    std::printf("baseline greybox non-bg pixels=%d\n", baselinePixels);
    Expect(baselinePixels > 10000, "baseline default-FBO greybox produces pixels");
    Expect(
        std::fabs(healthyPreGrid.cullNear - RL_CULL_DISTANCE_NEAR) < 0.0001
            && std::fabs(healthyPreGrid.cullFar - RL_CULL_DISTANCE_FAR) < 0.001,
        "baseline uses default raylib clip planes");

    {
        // Chest-like Model Preview framing (~1.2 unit mesh): far plane ~6, while
        // the gameplay camera sits ~13 units from the origin. BeginMode3D reads
        // these leaked distances and clips the whole greybox world.
        rlSetClipPlanes(1.52, 5.84);
        Snapshot clippedSnap{};
        const int clippedPixels =
            RenderGreyboxFrame(Experiment::Production, &clippedSnap, true, false);
        std::printf("Chest-like clip leak pixels=%d near/far=%.4g/%.4g\n",
                    clippedPixels,
                    clippedSnap.cullNear,
                    clippedSnap.cullFar);
        PrintSnapshot("chest_like_clip_pre_grid", clippedSnap);
        Expect(clippedPixels < baselinePixels / 10, "tight Chest-like far plane clips greybox 3D");
        Expect(clippedSnap.cullFar < 10.0, "leaked far plane is the preview frame");

        Snapshot restoredSnap{};
        const int restoredPixels =
            RenderGreyboxFrame(Experiment::Production, &restoredSnap, true, true);
        std::printf("restored default clip pixels=%d near/far=%.4g/%.4g\n",
                    restoredPixels,
                    restoredSnap.cullNear,
                    restoredSnap.cullFar);
        Expect(
            restoredPixels > baselinePixels / 2,
            "restoring default clip planes before BeginMode3D recovers greybox");
        Expect(
            std::fabs(restoredSnap.cullNear - RL_CULL_DISTANCE_NEAR) < 0.0001
                && std::fabs(restoredSnap.cullFar - RL_CULL_DISTANCE_FAR) < 0.001,
            "DrawWorld-like restore writes default clip planes");
    }

    {
        rlSetClipPlanes(5.15, 19.77);
        Snapshot barrelSnap{};
        const int barrelClipPixels =
            RenderGreyboxFrame(Experiment::Production, &barrelSnap, false, false);
        std::printf("Barrel-like clip leak pixels=%d near/far=%.4g/%.4g\n",
                    barrelClipPixels,
                    barrelSnap.cullNear,
                    barrelSnap.cullFar);
        Expect(
            barrelClipPixels > baselinePixels / 4,
            "Barrel-like preview far plane still includes the gameplay camera");
        rlSetClipPlanes(RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
    }

#if defined(PLATFORMER_CHEST_GLB)
    Model chest = LoadIfPresent(PLATFORMER_CHEST_GLB);
    if (chest.meshCount > 0)
    {
        DescribeModel("Chest", chest);
        {
            RenderTexture2D preview = LoadRenderTexture(256, 256);
            const Camera3D camera = MakeCamera();
            BeginTextureMode(preview);
            ClearBackground(kBackground);
            rlSetClipPlanes(1.52, 5.84);
            BeginMode3D(camera);
            DrawModel(chest, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
            EndMode3D();
            EndTextureMode();
            Snapshot leaked{};
            const int leakedPixels =
                RenderGreyboxFrame(Experiment::Production, &leaked, false, false);
            std::printf(
                "preview Chest clip leak into Gameplay pixels=%d far=%.4g\n",
                leakedPixels,
                leaked.cullFar);
            Expect(
                leakedPixels < baselinePixels / 10,
                "Model Preview clip planes leak into the next Gameplay BeginMode3D");
            rlSetClipPlanes(RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
            Snapshot restoredAfterPreview{};
            const int restoredAfterPreviewPixels =
                RenderGreyboxFrame(Experiment::Production, &restoredAfterPreview, false, false);
            Expect(
                restoredAfterPreviewPixels > baselinePixels / 2,
                "Preview restore lets Gameplay greybox draw again");
            Expect(
                std::fabs(restoredAfterPreview.cullNear - RL_CULL_DISTANCE_NEAR) < 0.0001
                    && std::fabs(restoredAfterPreview.cullFar - RL_CULL_DISTANCE_FAR) < 0.001,
                "Preview restore writes default clip planes");
            UnloadRenderTexture(preview);
        }
        const int chestAfter = RunModelSequence("Chest", chest, healthyPreGrid);
        Expect(
            chestAfter > baselinePixels / 4,
            "Chest DrawModel then next default-FBO frame still draws greybox");

        if (chestAfter <= baselinePixels / 4)
        {
            UnloadModel(chest);
            chest = LoadIfPresent(PLATFORMER_CHEST_GLB);
            RenderModelFrame(chest, nullptr, nullptr);
            Snapshot matricesSnap{};
            const int matricesPixels =
                RenderGreyboxFrame(Experiment::Matrices, &matricesSnap, false);
                std::printf(
                "experiment %s pixels=%d\n", ExperimentName(Experiment::Matrices), matricesPixels);
            PrintSnapshot("experiment_A_pre_grid", matricesSnap);

            UnloadModel(chest);
            chest = LoadIfPresent(PLATFORMER_CHEST_GLB);
            RenderModelFrame(chest, nullptr, nullptr);
            Snapshot vertexSnap{};
            const int vertexPixels =
                RenderGreyboxFrame(Experiment::VertexInput, &vertexSnap, false);
            std::printf(
                "experiment %s pixels=%d\n", ExperimentName(Experiment::VertexInput), vertexPixels);
            PrintSnapshot("experiment_B_pre_grid", vertexSnap);

            UnloadModel(chest);
            chest = LoadIfPresent(PLATFORMER_CHEST_GLB);
            RenderModelFrame(chest, nullptr, nullptr);
            Snapshot shaderSnap{};
            const int shaderPixels =
                RenderGreyboxFrame(Experiment::ShaderTexture, &shaderSnap, false);
            std::printf(
                "experiment %s pixels=%d\n", ExperimentName(Experiment::ShaderTexture), shaderPixels);
            PrintSnapshot("experiment_C_pre_grid", shaderSnap);

            const bool matricesHelped = matricesPixels > baselinePixels / 4;
            const bool vertexHelped = vertexPixels > baselinePixels / 4;
            const bool shaderHelped = shaderPixels > baselinePixels / 4;
            std::printf(
                "experiment recovery matrices=%d vertex=%d shader=%d\n",
                static_cast<int>(matricesHelped),
                static_cast<int>(vertexHelped),
                static_cast<int>(shaderHelped));
            Expect(
                (matricesHelped && !vertexHelped && !shaderHelped)
                    || (vertexHelped && !matricesHelped && !shaderHelped)
                    || (shaderHelped && !matricesHelped && !vertexHelped)
                    || (vertexHelped || matricesHelped || shaderHelped),
                "at least one single-axis experiment recovers greybox");
        }

        UnloadModel(chest);
        chest = LoadIfPresent(PLATFORMER_CHEST_GLB);
        RenderEditorThenGameplay(chest);
        Snapshot editorFollow{};
        const int editorFollowPixels =
            RenderGreyboxFrame(Experiment::Production, &editorFollow, false);
        std::printf(
            "after editor-subviewport Chest, Gameplay greybox pixels=%d\n", editorFollowPixels);
        Expect(
            editorFollowPixels > baselinePixels / 4,
            "editor sub-viewport Chest then Gameplay still draws greybox");
        UnloadModel(chest);
    }
    else
    {
        std::printf("Chest GLB not present; skipped.\n");
    }
#else
    std::printf("Chest GLB macro absent; skipped.\n");
#endif

#if defined(PLATFORMER_BARREL_GLB)
    Model barrel = LoadIfPresent(PLATFORMER_BARREL_GLB);
    if (barrel.meshCount > 0)
    {
        DescribeModel("Barrel", barrel);
        const int barrelAfter = RunModelSequence("Barrel", barrel, healthyPreGrid);
        Expect(
            barrelAfter > baselinePixels / 4,
            "Barrel DrawModel then next default-FBO frame still draws greybox");
        UnloadModel(barrel);
    }
    else
    {
        std::printf("Barrel GLB not present; skipped.\n");
    }
#endif

    CloseWindow();
    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d greybox immediate-state test(s) failed.\n", gFailures);
        return 1;
    }
    std::printf("Greybox immediate-state tests passed.\n");
    return 0;
}
