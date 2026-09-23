// Milestone 84: model/material/texture ownership, reload, highlight restore.
// Hidden window. Stages copies next to the test exe; no source fallback.

#include "assets/ModelMaterialPresentation.h"
#include "platform/RuntimePaths.h"
#include "render/LoadedModelMaterials.h"
#include "render/StaticModelScene.h"
#include "world/LevelDefinition.h"
#include "world/StaticProp.h"
#include "world/Terrain.h"
#include "world/TerrainVegetation.h"

#include "raylib.h"
#include "rlgl.h"

#include <cstdio>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

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

std::filesystem::path FixturePath(const char* macro)
{
    if (macro == nullptr || macro[0] == '\0')
    {
        return {};
    }
    return std::filesystem::path{macro}.lexically_normal();
}

bool StageIdentity(std::string_view identity, const std::filesystem::path& source)
{
    const std::filesystem::path dest = platform::RuntimeAssetPath(identity);
    if (dest.empty() || source.empty())
    {
        return false;
    }
    std::error_code error;
    std::filesystem::create_directories(dest.parent_path(), error);
    if (error)
    {
        return false;
    }
    std::filesystem::copy_file(
        source, dest, std::filesystem::copy_options::overwrite_existing, error);
    return !error && std::filesystem::is_regular_file(dest);
}

world::StaticPropSpec MakeProp(std::string identity)
{
    world::StaticPropSpec spec{};
    spec.modelIdentity = std::move(identity);
    spec.position = {0.0f, 1.0f, 0.0f};
    spec.rotationDegrees = {};
    spec.scale = {1.0f, 1.0f, 1.0f};
    return spec;
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
    const std::filesystem::path testStatic = FixturePath(
#if defined(PLATFORMER_TEST_STATIC_GLB)
        PLATFORMER_TEST_STATIC_GLB
#else
        ""
#endif
    );
    const std::filesystem::path testTextured = FixturePath(
#if defined(PLATFORMER_TEST_TEXTURED_GLB)
        PLATFORMER_TEST_TEXTURED_GLB
#else
        ""
#endif
    );
    const std::filesystem::path player = FixturePath(
#if defined(PLATFORMER_PLAYER_GLB)
        PLATFORMER_PLAYER_GLB
#else
        ""
#endif
    );

    Expect(std::filesystem::is_regular_file(testStatic), "test_static.glb exists");
    Expect(std::filesystem::is_regular_file(testTextured), "test_textured.glb exists");
    Expect(std::filesystem::is_regular_file(player), "player.glb exists");

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(640, 360, "LoadedModelMaterialsTest");

    Expect(StageIdentity("models/test_static.glb", testStatic), "stage test_static");
    Expect(StageIdentity("models/test_textured.glb", testTextured), "stage test_textured");
    Expect(StageIdentity("models/player.glb", player), "stage player");

    world::LevelDefinition level{};
    level.staticProps.push_back(MakeProp("models/test_static.glb"));
    level.staticProps.push_back(MakeProp("models/test_textured.glb"));

    render::StaticModelSceneStore store;
    store.Sync(level);
    Expect(store.HasModel("models/test_static.glb"), "untextured GLB loads");
    Expect(store.HasModel("models/test_textured.glb"), "textured GLB loads");
    Expect(!store.IsFailed("models/test_static.glb"), "untextured did not fail");
    Expect(!store.IsFailed("models/test_textured.glb"), "textured did not fail");
    const std::size_t loadsAfterFirst = store.LoadCount();
    Expect(loadsAfterFirst == 2, "two identities load once each");

    store.Sync(level);
    Expect(store.LoadCount() == loadsAfterFirst, "second Sync does not reload unchanged files");
    Expect(store.UniqueLoadedCount() == 2, "both models stay cached");

    Model staticModel = LoadModel(testStatic.string().c_str());
    Model texturedModel = LoadModel(testTextured.string().c_str());
    Model playerModel = LoadModel(player.string().c_str());
    render::PrepareLoadedModelMaterials(staticModel);
    render::PrepareLoadedModelMaterials(texturedModel);
    render::PrepareLoadedModelMaterials(playerModel);

    Expect(staticModel.materialCount >= 1, "untextured model has a material");
    Expect(texturedModel.materialCount >= 1, "textured model has a material");
    Expect(playerModel.materialCount >= 2, "player model keeps two materials");
    unsigned int texturedId = 0;
    unsigned int texturedWidth = 0;
    for (int i = 0; i < texturedModel.materialCount; ++i)
    {
        if (texturedModel.materials == nullptr || texturedModel.materials[i].maps == nullptr)
        {
            continue;
        }
        const Texture2D texture = texturedModel.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture;
        if (texture.id != 0 && texture.id != rlGetTextureIdDefault() && texture.width > 1)
        {
            texturedId = texture.id;
            texturedWidth = static_cast<unsigned int>(texture.width);
        }
    }
    const unsigned int defaultTextureId = rlGetTextureIdDefault();
    Expect(texturedId != 0 && texturedId != defaultTextureId, "textured GLB uploaded an embedded Base Color Texture");
    Expect(texturedWidth > 1, "embedded base-color texture is not the 1x1 default");
    Expect(texturedId != 0 && texturedId != defaultTextureId, "textured GLB uploaded an embedded Base Color Texture");
    Expect(texturedWidth > 1, "embedded base-color texture is not the 1x1 default");
    bool playerHasUniqueTexture = false;
    for (int i = 0; i < playerModel.materialCount; ++i)
    {
        if (playerModel.materials == nullptr || playerModel.materials[i].maps == nullptr)
        {
            continue;
        }
        const Texture2D texture = playerModel.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture;
        if (texture.id != 0 && texture.id != defaultTextureId && texture.width > 1)
        {
            playerHasUniqueTexture = true;
        }
    }
    Expect(!playerHasUniqueTexture, "player.glb remains untextured after prepare");

    const render::ModelMaterialGpuSnapshot beforeStatic =
        render::CaptureModelMaterialGpuState(staticModel);
    const render::ModelMaterialGpuSnapshot beforeTextured =
        render::CaptureModelMaterialGpuState(texturedModel);

    const Camera3D camera = MakeCamera();
    BeginDrawing();
    ClearBackground(Color{32, 36, 48, 255});
    BeginMode3D(camera);
    store.ResetDrawStats();
    store.DrawProp(level.staticProps[0]);
    store.DrawProp(level.staticProps[1]);
    Expect(store.DrawSubmissionCount() == 2, "two ordinary draws");
    Expect(store.LoadCount() == loadsAfterFirst, "draws do not LoadModel");
    const std::size_t loadsBeforeHighlight = store.LoadCount();
    store.DrawSelectionHighlight(level.staticProps[1]);
    store.DrawGameplayTargetHighlight(level.staticProps[1], 180);
    Expect(store.LoadCount() == loadsBeforeHighlight, "highlights do not LoadModel");
    render::DrawModelPreservingMaterials(staticModel, Vector3{2.0f, 0.0f, 0.0f}, 1.0f, Color{255, 220, 72, 150});
    render::DrawModelPreservingMaterials(texturedModel, Vector3{-2.0f, 0.0f, 0.0f}, 1.0f, Color{255, 220, 72, 180});
    EndMode3D();
    EndDrawing();

    const render::ModelMaterialGpuSnapshot afterStatic =
        render::CaptureModelMaterialGpuState(staticModel);
    const render::ModelMaterialGpuSnapshot afterTextured =
        render::CaptureModelMaterialGpuState(texturedModel);
    Expect(
        render::ModelMaterialGpuStateEqual(beforeStatic, afterStatic),
        "untextured highlight restores imported material color/texture");
    Expect(
        render::ModelMaterialGpuStateEqual(beforeTextured, afterTextured),
        "textured highlight restores imported material color/texture");

    world::LevelDefinition texturedOnly{};
    texturedOnly.staticProps.push_back(MakeProp("models/test_textured.glb"));
    store.Sync(texturedOnly);
    Expect(!store.HasModel("models/test_static.glb"), "unused model unloads");
    Expect(store.HasModel("models/test_textured.glb"), "kept model is not unloaded");
    Expect(store.UniqueLoadedCount() == 1, "one loaded identity remains");
    Expect(store.LoadCount() == loadsAfterFirst, "keeping a model does not reload it");

    world::LevelDefinition missingAndKept{};
    missingAndKept.staticProps.push_back(MakeProp("models/test_textured.glb"));
    missingAndKept.staticProps.push_back(MakeProp("models/missing_m84.glb"));
    store.Sync(missingAndKept);
    Expect(store.IsFailed("models/missing_m84.glb"), "missing model fails safely");
    Expect(store.HasModel("models/test_textured.glb"), "failed identity does not unload others");
    Expect(!store.HasModel("models/missing_m84.glb"), "missing identity has no model");

    store.Sync(texturedOnly);
    store.Sync(texturedOnly);
    Expect(store.LoadCount() == loadsAfterFirst, "reload of unchanged stamp is not a double load");

    {
        world::LevelDefinition vegetation{};
        vegetation.hasTerrain = true;
        vegetation.terrain = world::MakeDefaultTerrain();
        Expect(
            world::TryAddTerrainVegetationEntry(vegetation.terrain, "models/test_static.glb"),
            "vegetation palette accepts the staged model");
        Expect(
            world::TryAddTerrainVegetationEntry(vegetation.terrain, "models/test_textured.glb"),
            "vegetation palette accepts a second staged model");
        vegetation.terrain.vegetationResolutionX = world::kMaxTerrainVegetationResolution;
        vegetation.terrain.vegetationResolutionZ = world::kMaxTerrainVegetationResolution;
        const int cells = vegetation.terrain.vegetationResolutionX * vegetation.terrain.vegetationResolutionZ;
        vegetation.terrain.vegetationCells.assign(static_cast<std::size_t>(cells), 3);
        vegetation.terrain.vegetationDensityQuanta.assign(
            static_cast<std::size_t>(cells * world::kMaxTerrainVegetationEntries), 0);
        vegetation.terrain.vegetationPaintParams.assign(
            static_cast<std::size_t>(cells * world::kMaxTerrainVegetationEntries), 0);
        const unsigned char density = world::QuantizeTerrainVegetationDensity(world::kMaxTerrainVegetationDensity);
        for (int entryIndex = 0; entryIndex < 2; ++entryIndex)
        {
            const std::uint16_t paint = world::PackTerrainVegetationPaintFromEntry(
                vegetation.terrain.vegetationEntries[static_cast<std::size_t>(entryIndex)]);
            for (int cellIndex = 0; cellIndex < cells; ++cellIndex)
            {
                const int slot = world::TerrainVegetationDensitySlot(cellIndex, entryIndex);
                vegetation.terrain.vegetationDensityQuanta[static_cast<std::size_t>(slot)] = density;
                vegetation.terrain.vegetationPaintParams[static_cast<std::size_t>(slot)] = paint;
            }
        }
        store.Sync(vegetation);
        const std::size_t loadsBeforeVegetationDraw = store.LoadCount();
        Expect(store.UniqueLoadedCount() == 2, "two vegetation models stay one resource each");
        Expect(store.HasModel("models/test_static.glb"), "first vegetation model is loaded once");
        Expect(store.HasModel("models/test_textured.glb"), "second vegetation model is loaded once");

        const char* vertexShader = R"(#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
in mat4 instanceTransform;
uniform mat4 mvp;
uniform mat4 matModel;
uniform int vegetationInstanced;
void main()
{
    mat4 model = vegetationInstanced != 0 ? instanceTransform : matModel;
    gl_Position = vegetationInstanced != 0
        ? mvp * model * vec4(vertexPosition, 1.0)
        : mvp * vec4(vertexPosition, 1.0);
}
)";
        const char* fragmentShader = R"(#version 330
out vec4 finalColor;
void main()
{
    finalColor = vec4(1.0);
}
)";
        const Shader shader = LoadShaderFromMemory(vertexShader, fragmentShader);
        Expect(shader.id != 0, "vegetation instanced shader compiles");
        Expect(
            shader.locs != nullptr && shader.locs[SHADER_LOC_VERTEX_INSTANCETRANSFORM] >= 0,
            "vegetation shader binds instanceTransform");
        render::ModelDrawOverride vegetationOverride{};
        vegetationOverride.shader = shader;
        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode3D(camera);
        store.ResetDrawStats();
        store.DrawTerrainVegetation(vegetation.terrain, &vegetationOverride);
        EndMode3D();
        EndDrawing();
        Expect(store.VegetationInstanceCount() >= 512, "dense vegetation generates many instances");
        Expect(store.VegetationRenderGroupCount() == 2, "two model identities are two render groups");
        Expect(store.VegetationInstancedSubmissionCount() >= 2, "each model submits at least one instanced draw");
        Expect(
            store.VegetationInstancedSubmissionCount() < store.VegetationInstanceCount(),
            "instanced submissions are not one ordinary draw per instance");
        Expect(store.VegetationOrdinarySubmissionCount() == 0, "loaded models do not fall back to DrawModel");
        Expect(store.LoadCount() == loadsBeforeVegetationDraw, "drawing vegetation does not load a model per instance");
        Expect(store.UniqueLoadedCount() == 2, "drawing vegetation does not duplicate model resources");
        UnloadShader(shader);
    }

    store.Shutdown();
    store.Shutdown();
    Expect(store.UniqueLoadedCount() == 0, "shutdown unloads models");
    Expect(!store.HasModel("models/test_textured.glb"), "shutdown leaves no dangling model");

    UnloadModel(staticModel);
    UnloadModel(texturedModel);
    UnloadModel(playerModel);

    CloseWindow();
    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d LoadedModelMaterialsTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("LoadedModelMaterialsTest passed\n");
    return 0;
}
