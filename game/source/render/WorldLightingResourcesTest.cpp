// Milestone 85: lighting shader/shadow resource lifetime. Hidden window.
// Stages copies next to the test exe; no source-directory fallback.

#include "platform/RuntimePaths.h"
#include "render/LightingEnvironment.h"
#include "render/TerrainMesh.h"
#include "render/WorldLighting.h"
#include "assets/RuntimePngResolve.h"
#include "world/LocalLight.h"
#include "world/Terrain.h"
#include "world/TerrainPaint.h"
#include "world/TerrainSculpt.h"

#include "raylib.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
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

bool StageSolidIdentity(std::string_view identity, Color color)
{
    const std::filesystem::path dest = platform::RuntimeAssetPath(identity);
    if (dest.empty())
    {
        return false;
    }
    std::error_code error;
    std::filesystem::create_directories(dest.parent_path(), error);
    if (error)
    {
        return false;
    }
    Image image = GenImageColor(8, 8, color);
    const bool ok = ExportImage(image, dest.string().c_str());
    UnloadImage(image);
    return ok && std::filesystem::is_regular_file(dest, error);
}

int ChannelDelta(unsigned char actual, unsigned char expected)
{
    return actual > expected ? actual - expected : expected - actual;
}

bool ColorChannelsClose(Color actual, Color expected, int slack)
{
    return ChannelDelta(actual.r, expected.r) <= slack && ChannelDelta(actual.g, expected.g) <= slack
        && ChannelDelta(actual.b, expected.b) <= slack;
}

void FillTerrainDrawRequest(
    render::TerrainLayerDrawRequest& request,
    const render::TerrainGpuResources& terrainGpu)
{
    request = {};
    request.layerCount = terrainGpu.LayerCount();
    const world::TerrainSpec* spec = terrainGpu.LastSpec();
    if (spec == nullptr)
    {
        return;
    }
    request.originX = spec->origin.x;
    request.originZ = spec->origin.z;
    for (int layer = 0; layer < world::kMaxTerrainMaterialLayers; ++layer)
    {
        request.tiling[layer] = world::TerrainLayerTextureTiling(*spec, layer);
        request.layers[layer] = terrainGpu.GetLayerTexture(layer);
    }
}

bool SampleLitTerrainPixel(
    render::WorldLightingResources& lighting,
    const render::TerrainGpuResources& terrainGpu,
    const render::LightingEnvironment& environment,
    Color& outPixel)
{
    outPixel = {};
    if (!terrainGpu.HasMesh() || terrainGpu.GetMesh() == nullptr)
    {
        return false;
    }
    RenderTexture2D target = LoadRenderTexture(64, 64);
    if (target.id == 0)
    {
        return false;
    }
    BeginTextureMode(target);
    ClearBackground(BLACK);
    Camera3D camera{};
    camera.position = Vector3{0.0f, 28.0f, 0.15f};
    camera.target = Vector3{0.0f, 0.25f, 0.0f};
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy = 35.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    BeginMode3D(camera);
    lighting.BindLitPass(environment);
    render::TerrainLayerDrawRequest request{};
    FillTerrainDrawRequest(request, terrainGpu);
    lighting.DrawWorldTerrain(*terrainGpu.GetMesh(), WHITE, request);
    lighting.UnbindLitPass();
    EndMode3D();
    EndTextureMode();
    Image image = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&image);
    outPixel = GetImageColor(image, 32, 32);
    UnloadImage(image);
    UnloadRenderTexture(target);
    return true;
}

bool FillAllSampleWeights(world::TerrainSpec& terrain, float layer0, float layer1, float layer2, float layer3)
{
    const float weights[world::kMaxTerrainMaterialLayers] = {layer0, layer1, layer2, layer3};
    const int count = world::TerrainSampleCount(terrain);
    if (count <= 0)
    {
        return false;
    }
    for (int sample = 0; sample < count; ++sample)
    {
        world::WriteTerrainSampleWeights(terrain, sample, weights);
    }
    return true;
}

bool ReadVertexWeightColor(
    const render::TerrainGpuResources& terrainGpu,
    int vertexIndex,
    unsigned char& red,
    unsigned char& green,
    unsigned char& blue,
    unsigned char& alpha)
{
    const Mesh* mesh = terrainGpu.GetMesh();
    if (mesh == nullptr || mesh->colors == nullptr || vertexIndex < 0 || vertexIndex >= mesh->vertexCount)
    {
        return false;
    }
    red = mesh->colors[vertexIndex * 4];
    green = mesh->colors[vertexIndex * 4 + 1];
    blue = mesh->colors[vertexIndex * 4 + 2];
    alpha = mesh->colors[vertexIndex * 4 + 3];
    return true;
}
}

int main()
{
    const std::filesystem::path litVs = FixturePath(
#if defined(PLATFORMER_WORLD_LIT_VS)
        PLATFORMER_WORLD_LIT_VS
#else
        ""
#endif
    );
    const std::filesystem::path litFs = FixturePath(
#if defined(PLATFORMER_WORLD_LIT_FS)
        PLATFORMER_WORLD_LIT_FS
#else
        ""
#endif
    );
    const std::filesystem::path depthVs = FixturePath(
#if defined(PLATFORMER_SHADOW_DEPTH_VS)
        PLATFORMER_SHADOW_DEPTH_VS
#else
        ""
#endif
    );
    const std::filesystem::path depthFs = FixturePath(
#if defined(PLATFORMER_SHADOW_DEPTH_FS)
        PLATFORMER_SHADOW_DEPTH_FS
#else
        ""
#endif
    );
    const std::filesystem::path checkerPng = FixturePath(
#if defined(PLATFORMER_TEST_CHECKER_PNG)
        PLATFORMER_TEST_CHECKER_PNG
#else
        ""
#endif
    );

    Expect(std::filesystem::is_regular_file(litVs), "world_lit.vs exists");
    Expect(std::filesystem::is_regular_file(litFs), "world_lit.fs exists");
    Expect(std::filesystem::is_regular_file(depthVs), "shadow_depth.vs exists");
    Expect(std::filesystem::is_regular_file(depthFs), "shadow_depth.fs exists");

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(640, 360, "WorldLightingResourcesTest");

    Expect(
        StageIdentity(platform::kWorldLitVertexShaderLogicalId, litVs), "stage world_lit.vs");
    Expect(
        StageIdentity(platform::kWorldLitFragmentShaderLogicalId, litFs), "stage world_lit.fs");
    Expect(
        StageIdentity(platform::kShadowDepthVertexShaderLogicalId, depthVs),
        "stage shadow_depth.vs");
    Expect(
        StageIdentity(platform::kShadowDepthFragmentShaderLogicalId, depthFs),
        "stage shadow_depth.fs");
    Expect(std::filesystem::is_regular_file(checkerPng), "test_checker.png exists");
    Expect(
        StageIdentity(platform::kTestCheckerLogicalId, checkerPng),
        "stage textures/test_checker.png");

    {
        std::error_code error;
        const std::filesystem::path resolveRoot =
            (std::filesystem::temp_directory_path() / "platformer_m90_png_resolve").lexically_normal();
        std::filesystem::remove_all(resolveRoot, error);
        const std::filesystem::path stagedRoot = resolveRoot / "staged";
        const std::filesystem::path cookedRoot = resolveRoot / "cooked";
        std::filesystem::create_directories(stagedRoot / "textures", error);
        std::filesystem::create_directories(cookedRoot / "textures", error);
        std::filesystem::copy_file(
            checkerPng,
            cookedRoot / "textures" / "test_cooked_only.png",
            std::filesystem::copy_options::overwrite_existing,
            error);
        std::filesystem::copy_file(
            checkerPng,
            stagedRoot / "textures" / "test_checker.png",
            std::filesystem::copy_options::overwrite_existing,
            error);
        std::filesystem::copy_file(
            checkerPng,
            cookedRoot / "textures" / "test_checker.png",
            std::filesystem::copy_options::overwrite_existing,
            error);
        const assets::RuntimePngLoadResolution cookedOnly = assets::ResolveRuntimePngLoadFile(
            "textures/test_cooked_only.png", cookedRoot, stagedRoot);
        Expect(
            cookedOnly.source == assets::RuntimePngLoadSource::Cooked,
            "cooked PNG resolves when staged is absent");
        Expect(
            cookedOnly.path.filename() == "test_cooked_only.png",
            "cooked resolution uses the identity filename");
        const assets::RuntimePngLoadResolution stagedWins = assets::ResolveRuntimePngLoadFile(
            "textures/test_checker.png", cookedRoot, stagedRoot);
        Expect(
            stagedWins.source == assets::RuntimePngLoadSource::Staged,
            "staged PNG wins over cooked");
        const assets::RuntimePngLoadResolution releaseMissing = assets::ResolveRuntimePngLoadFile(
            "textures/test_cooked_only.png", {}, stagedRoot);
        Expect(
            releaseMissing.source == assets::RuntimePngLoadSource::Missing,
            "Release (no cooked root) does not use cooked PNGs");
        const assets::RuntimePngLoadResolution missing = assets::ResolveRuntimePngLoadFile(
            "textures/missing_layer.png", cookedRoot, stagedRoot);
        Expect(missing.source == assets::RuntimePngLoadSource::Missing, "missing PNG stays missing");
        std::filesystem::create_directories(resolveRoot / "source" / "textures", error);
        std::filesystem::copy_file(
            checkerPng,
            resolveRoot / "source" / "textures" / "test_source_only.png",
            std::filesystem::copy_options::overwrite_existing,
            error);
        const assets::RuntimePngLoadResolution sourceOnly = assets::ResolveRuntimePngLoadFile(
            "textures/test_source_only.png",
            cookedRoot,
            stagedRoot,
            resolveRoot / "source");
        Expect(
            sourceOnly.source == assets::RuntimePngLoadSource::Source,
            "Development source last-resort resolves when staged and cooked are absent");
        const assets::RuntimePngLoadResolution releaseIgnoresSource = assets::ResolveRuntimePngLoadFile(
            "textures/test_source_only.png", {}, stagedRoot, {});
        Expect(
            releaseIgnoresSource.source == assets::RuntimePngLoadSource::Missing,
            "Release does not use catalog source PNGs");
        std::filesystem::remove_all(resolveRoot, error);
    }

    const std::filesystem::path stagedVs =
        platform::RuntimeAssetPath(platform::kWorldLitVertexShaderLogicalId);
    Expect(stagedVs.is_absolute(), "runtime shader path is absolute");
    Expect(stagedVs.string().find("game/assets/source") == std::string::npos
            && stagedVs.string().find("game\\assets\\source") == std::string::npos,
        "runtime shader path is not the authoring source tree");

    render::WorldLightingResources lighting;
    Expect(!lighting.IsReady(), "lighting starts unloaded");
    lighting.Load();
    Expect(lighting.IsReady(), "staged shaders and shadow map load");
    Expect(lighting.ShaderLoadCount() == 1, "first load compiles shaders once");
    Expect(lighting.ShadowMapCreateCount() == 1, "first load creates one shadow map");
    Expect(lighting.LitShaderId() != 0, "lit shader id is valid");
    Expect(lighting.DepthShaderId() != 0, "depth shader id is valid");
    Expect(lighting.ShadowMapId() != 0, "shadow map id is valid");
    Expect(lighting.ShadowMapResolution() == 2048, "shadow map uses the default resolution");
    Expect(
        lighting.TerrainExtraAlbedoSamplerLocation(0) >= 0
            && lighting.TerrainExtraAlbedoSamplerLocation(1) >= 0
            && lighting.TerrainExtraAlbedoSamplerLocation(2) >= 0,
        "terrain extra albedo samplers exist");
    Expect(
        render::kWorldLitDiffuseTextureUnit != render::kWorldLitShadowMapTextureUnit,
        "layer 0 unit does not alias the shadow map");
    Expect(
        render::kWorldLitTerrainExtraTextureUnits[0] != render::kWorldLitShadowMapTextureUnit
            && render::kWorldLitTerrainExtraTextureUnits[1] != render::kWorldLitShadowMapTextureUnit
            && render::kWorldLitTerrainExtraTextureUnits[2] != render::kWorldLitShadowMapTextureUnit,
        "extra Terrain albedo units do not alias the shadow map");
    Expect(
        render::kWorldLitTerrainExtraTextureUnits[0] != render::kWorldLitDiffuseTextureUnit
            && render::kWorldLitTerrainExtraTextureUnits[1] != render::kWorldLitDiffuseTextureUnit
            && render::kWorldLitTerrainExtraTextureUnits[2] != render::kWorldLitDiffuseTextureUnit,
        "extra Terrain albedo units do not alias texture0");
    Expect(
        render::kWorldLitTerrainExtraTextureUnits[0] != render::kWorldLitTerrainExtraTextureUnits[1]
            && render::kWorldLitTerrainExtraTextureUnits[1] != render::kWorldLitTerrainExtraTextureUnits[2]
            && render::kWorldLitTerrainExtraTextureUnits[0] != render::kWorldLitTerrainExtraTextureUnits[2],
        "extra Terrain albedo units are distinct");

    const std::size_t shaderLoads = lighting.ShaderLoadCount();
    const std::size_t shadowCreates = lighting.ShadowMapCreateCount();
    lighting.Load();
    Expect(lighting.ShaderLoadCount() == shaderLoads, "second Load does not recreate shaders");
    Expect(lighting.ShadowMapCreateCount() == shadowCreates, "second Load does not recreate the shadow map");

    const render::LightingEnvironment environment = render::MakeDefaultLightingEnvironment();
    lighting.BeginShadowPass(environment);
    lighting.DrawSolidBox({0.0f, 0.5f, 0.0f}, {2.0f, 1.0f, 2.0f}, WHITE);
    lighting.EndShadowPass();
    BeginDrawing();
    ClearBackground(Color{32, 36, 48, 255});
    Camera3D camera{};
    camera.position = Vector3{0.0f, 4.0f, 12.0f};
    camera.target = Vector3{0.0f, 1.0f, 0.0f};
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy = 40.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    BeginMode3D(camera);
    lighting.BindLitPass(environment);
    lighting.DrawSolidBox({0.0f, 0.5f, 0.0f}, {2.0f, 1.0f, 2.0f}, Color{110, 118, 132, 255});
    lighting.UnbindLitPass();
    EndMode3D();
    EndDrawing();
    Expect(lighting.ShaderLoadCount() == shaderLoads, "drawing does not create shaders");
    Expect(lighting.ShadowMapCreateCount() == shadowCreates, "drawing does not create shadow maps");

    render::LightingEnvironment noShadows = environment;
    noShadows.directional.shadowsEnabled = false;
    lighting.BindLitPass(noShadows);
    lighting.UnbindLitPass();
    render::LightingEnvironment disabled = environment;
    disabled.directional.authoredEnabled = false;
    lighting.BindLitPass(disabled);
    lighting.UnbindLitPass();
    render::LightingEnvironment plateOff = environment;
    render::ApplyDirectionalLightActivation(plateOff, true, false);
    lighting.BindLitPass(plateOff);
    lighting.UnbindLitPass();
    render::LightingEnvironment plateOn = environment;
    render::ApplyDirectionalLightActivation(plateOn, true, true);
    lighting.BindLitPass(plateOn);
    lighting.UnbindLitPass();
    Expect(lighting.ShaderLoadCount() == shaderLoads, "enabled edits do not recreate shaders");
    Expect(
        lighting.ShadowMapCreateCount() == shadowCreates,
        "enabled edits do not recreate shadow maps");
    render::LightingEnvironment withLocal = environment;
    std::vector<world::PointLightSpec> packedPoints(
        8, world::MakeDefaultPointLight({0.0f, 2.0f, 0.0f}));
    packedPoints.push_back(world::MakeDefaultPointLight({4.0f, 2.0f, 0.0f}));
    render::ApplyAuthoredLocalLights(withLocal, packedPoints, {});
    Expect(withLocal.localLights.count == 8, "BindLitPass overflow packing stays at 8");
    lighting.BindLitPass(withLocal);
    lighting.UnbindLitPass();
    Expect(lighting.ShaderLoadCount() == shaderLoads, "local-light bind does not recreate shaders");
    Expect(
        lighting.ShadowMapCreateCount() == shadowCreates,
        "local-light bind does not recreate shadow maps");
    Expect(
        !render::ShouldCastDirectionalShadow(render::ShadowParticipant::Thumbnail)
            && !render::ShouldCastDirectionalShadow(render::ShadowParticipant::Preview),
        "thumbnail/preview remain isolated from world shadow casters");
    Expect(
        render::ShouldCastDirectionalShadow(render::ShadowParticipant::Terrain)
            && render::ShouldReceiveDirectionalShadow(render::ShadowParticipant::Terrain),
        "Terrain is a Directional shadow caster and receiver");

    {
        render::TerrainGpuResources terrainGpu;
        Expect(!terrainGpu.HasMesh(), "Terrain GPU starts unloaded");
        Expect(terrainGpu.UploadCount() == 0 && terrainGpu.UnloadCount() == 0,
            "Terrain GPU counters start at zero");
        const world::TerrainSpec flat = world::MakeDefaultTerrain();
        terrainGpu.Sync(&flat);
        Expect(terrainGpu.HasMesh(), "Sync uploads Terrain mesh");
        Expect(terrainGpu.UploadCount() == 1, "first Sync is one upload");
        const std::size_t uploads = terrainGpu.UploadCount();
        const std::size_t unloads = terrainGpu.UnloadCount();
        terrainGpu.Sync(&flat);
        Expect(terrainGpu.UploadCount() == uploads, "unchanged Terrain does not reupload");
        Expect(terrainGpu.UnloadCount() == unloads, "unchanged Terrain does not unload");
        world::TerrainSpec moved = flat;
        moved.origin.x += 1.0f;
        terrainGpu.Sync(&moved);
        Expect(terrainGpu.UploadCount() == uploads + 1, "authored change rebuilds Terrain mesh");
        Expect(terrainGpu.UnloadCount() == unloads + 1, "rebuild unloads the previous mesh");
        lighting.BeginShadowPass(environment);
        lighting.DrawWorldMesh(*terrainGpu.GetMesh(), WHITE);
        lighting.EndShadowPass();
        BeginDrawing();
        Camera3D terrainCamera{};
        terrainCamera.position = Vector3{0.0f, 4.0f, 12.0f};
        terrainCamera.target = Vector3{0.0f, 1.0f, 0.0f};
        terrainCamera.up = Vector3{0.0f, 1.0f, 0.0f};
        terrainCamera.fovy = 40.0f;
        terrainCamera.projection = CAMERA_PERSPECTIVE;
        BeginMode3D(terrainCamera);
        lighting.BindLitPass(environment);
        lighting.DrawWorldMesh(*terrainGpu.GetMesh(), Color{118, 140, 96, 255});
        lighting.UnbindLitPass();
        EndMode3D();
        EndDrawing();
        Expect(terrainGpu.UploadCount() == uploads + 1, "drawing does not rebuild Terrain");
        world::TerrainSpec sculpted = moved;
        const core::Vec3 sample = world::TerrainSamplePosition(sculpted, 4, 2);
        world::TerrainSculptStampRequest raise{};
        raise.operation = world::TerrainSculptOperation::Raise;
        raise.centerX = sample.x;
        raise.centerZ = sample.z;
        raise.radius = 2.0f;
        raise.strength = 0.5f;
        Expect(world::ApplyTerrainSculptStamp(sculpted, raise), "GPU sculpt fixture");
        terrainGpu.Sync(&sculpted);
        Expect(terrainGpu.HasMesh(), "sculpted Terrain uploads");
        Expect(terrainGpu.UploadCount() == uploads + 2, "sculpted heights rebuild Terrain mesh");
        const std::size_t sculptUploads = terrainGpu.UploadCount();
        terrainGpu.Sync(&sculpted);
        Expect(terrainGpu.UploadCount() == sculptUploads, "unchanged sculpted Terrain does not reupload");
        const std::size_t sculptUnloads = terrainGpu.UnloadCount();
        terrainGpu.Sync(nullptr);
        Expect(!terrainGpu.HasMesh(), "removing Terrain unloads the mesh");
        Expect(terrainGpu.UnloadCount() == sculptUnloads + 1, "removal unloads once");
        terrainGpu.Sync(nullptr);
        Expect(terrainGpu.UnloadCount() == sculptUnloads + 1, "second removal is idempotent");

        world::TerrainSpec textured = world::MakeDefaultTerrain();
        Expect(!terrainGpu.HasTexture(), "no assignment has no GPU texture");
        Expect(terrainGpu.TextureLoadCount() == 0, "fallback does not load a texture");
        Expect(
            world::TryAssignTerrainTextureIdentity(textured, "textures/test_checker.png"),
            "GPU texture assignment fixture");
        const std::size_t meshUploadsBeforeTexture = terrainGpu.UploadCount();
        terrainGpu.Sync(&textured);
        Expect(terrainGpu.HasMesh(), "textured Terrain still has mesh");
        Expect(terrainGpu.HasTexture(), "valid staged assignment loads GPU texture");
        Expect(terrainGpu.TextureLoadCount() == 1, "first assignment loads once");
        Expect(
            terrainGpu.UploadCount() == meshUploadsBeforeTexture + 1,
            "first textured Sync after unload uploads mesh");
        const std::size_t textureLoads = terrainGpu.TextureLoadCount();
        const std::size_t textureUnloads = terrainGpu.TextureUnloadCount();
        const std::size_t meshUploads = terrainGpu.UploadCount();
        terrainGpu.Sync(&textured);
        Expect(terrainGpu.TextureLoadCount() == textureLoads, "same assignment does not reload");
        Expect(terrainGpu.UploadCount() == meshUploads, "same assignment does not rebuild mesh");
        world::TerrainSpec heightOnly = textured;
        heightOnly.heights[0] = 1.0f;
        terrainGpu.Sync(&heightOnly);
        Expect(terrainGpu.UploadCount() == meshUploads + 1, "height change rebuilds mesh");
        Expect(terrainGpu.TextureLoadCount() == textureLoads, "height change does not reload texture");
        world::TerrainSpec tilingOnly = heightOnly;
        Expect(world::TrySetTerrainTextureTiling(tilingOnly, 0.5f), "GPU tiling fixture");
        terrainGpu.Sync(&tilingOnly);
        Expect(terrainGpu.UploadCount() == meshUploads + 2, "tiling change rebuilds UVs");
        Expect(terrainGpu.TextureLoadCount() == textureLoads, "tiling change does not reload texture");
        BeginDrawing();
        BeginMode3D(terrainCamera);
        lighting.BindLitPass(environment);
        lighting.DrawWorldMesh(*terrainGpu.GetMesh(), WHITE, terrainGpu.GetTexture());
        lighting.UnbindLitPass();
        EndMode3D();
        EndDrawing();
        Expect(terrainGpu.TextureLoadCount() == textureLoads, "drawing does not reload texture");
        world::TerrainSpec cleared = tilingOnly;
        Expect(world::TryClearTerrainTextureIdentity(cleared), "GPU clear fixture");
        terrainGpu.Sync(&cleared);
        Expect(!terrainGpu.HasTexture(), "cleared assignment unloads texture");
        Expect(
            terrainGpu.TextureUnloadCount() == textureUnloads + 1, "clear unloads the GPU texture");
        Expect(terrainGpu.UploadCount() == meshUploads + 2, "clear does not rebuild mesh");
        terrainGpu.Sync(&textured);
        Expect(terrainGpu.HasTexture(), "reassign loads texture");
        Expect(terrainGpu.TextureLoadCount() == textureLoads + 1, "reassign is one new load");
        const std::size_t f2Loads = terrainGpu.TextureLoadCount();
        const std::size_t f2Unloads = terrainGpu.TextureUnloadCount();
        terrainGpu.Unload();
        Expect(!terrainGpu.HasTexture() && !terrainGpu.HasMesh(), "F2 Unload releases mesh and texture");
        Expect(terrainGpu.TextureUnloadCount() == f2Unloads + 1, "F2 unloads texture once");
        terrainGpu.Sync(&textured);
        Expect(terrainGpu.HasTexture(), "F2 restore reloads staged texture");
        Expect(terrainGpu.TextureLoadCount() == f2Loads + 1, "F2 restore loads once, not a duplicate leftover");
        terrainGpu.Sync(nullptr);
        Expect(!terrainGpu.HasTexture(), "Level without Terrain releases texture");
        world::TerrainSpec nextLevel = world::MakeDefaultTerrain();
        Expect(
            world::TryAssignTerrainTextureIdentity(nextLevel, "textures/test_checker.png"),
            "transition assignment fixture");
        terrainGpu.Sync(&nextLevel);
        Expect(terrainGpu.HasTexture(), "Terrain Level B loads its texture");
        terrainGpu.Sync(nullptr);
        Expect(!terrainGpu.HasTexture(), "none-Terrain transition does not leak texture");
        terrainGpu.Unload();
        terrainGpu.Unload();
        Expect(terrainGpu.HasTexture() == false, "repeated Unload is idempotent");

        Expect(
            StageIdentity("textures/test_textured_basecolor.png", checkerPng),
            "stage second Terrain layer texture");
        world::TerrainSpec layered = world::MakeDefaultTerrain();
        Expect(
            world::TryAssignTerrainTextureIdentity(layered, "textures/test_checker.png"),
            "GPU layer 0 fixture");
        const std::size_t layerLoads = terrainGpu.TextureLoadCount();
        terrainGpu.Sync(&layered);
        Expect(terrainGpu.HasTexture(), "layer 0 loads");
        Expect(terrainGpu.LayerCount() == 1, "single layer count");
        Expect(terrainGpu.GetLayerTexture(0) != nullptr, "layer 0 texture present");
        Expect(terrainGpu.GetLayerTexture(1) == nullptr, "unused layer 1 is empty");
        Expect(
            world::TryAddTerrainMaterialLayer(layered, "textures/test_textured_basecolor.png"),
            "GPU add extra layer");
        const std::size_t meshUploadsBeforeLayer = terrainGpu.UploadCount();
        terrainGpu.Sync(&layered);
        Expect(terrainGpu.LayerCount() == 2, "two GPU layers");
        Expect(terrainGpu.GetLayerTexture(1) != nullptr, "extra layer texture loads");
        Expect(
            terrainGpu.TextureLoadCount() == layerLoads + 2,
            "adding a layer loads one additional texture");
        Expect(
            terrainGpu.UploadCount() == meshUploadsBeforeLayer,
            "unpainted extra layer does not rebuild mesh");
        const core::Vec3 gpuPaint = world::TerrainSamplePosition(layered, 4, 2);
        world::TerrainPaintStampRequest gpuStamp{};
        gpuStamp.layer = 1;
        gpuStamp.centerX = gpuPaint.x;
        gpuStamp.centerZ = gpuPaint.z;
        gpuStamp.radius = 4.0f;
        gpuStamp.strength = 1.0f;
        Expect(world::ApplyTerrainPaintStamp(layered, gpuStamp), "GPU paint fixture");
        terrainGpu.Sync(&layered);
        Expect(
            terrainGpu.UploadCount() == meshUploadsBeforeLayer + 1,
            "paint weights rebuild mesh colors");
        Expect(terrainGpu.GetLayerTexture(0) != nullptr && terrainGpu.GetLayerTexture(1) != nullptr,
            "both layer textures remain after paint");
        BeginDrawing();
        BeginMode3D(terrainCamera);
        lighting.BindLitPass(environment);
        render::TerrainLayerDrawRequest drawRequest{};
        drawRequest.layerCount = terrainGpu.LayerCount();
        drawRequest.originX = layered.origin.x;
        drawRequest.originZ = layered.origin.z;
        drawRequest.layers[0] = terrainGpu.GetLayerTexture(0);
        drawRequest.layers[1] = terrainGpu.GetLayerTexture(1);
        drawRequest.tiling[0] = world::TerrainLayerTextureTiling(layered, 0);
        drawRequest.tiling[1] = world::TerrainLayerTextureTiling(layered, 1);
        lighting.DrawWorldTerrain(*terrainGpu.GetMesh(), WHITE, drawRequest);
        lighting.UnbindLitPass();
        EndMode3D();
        EndDrawing();
        Expect(world::TryRemoveTerrainMaterialLayer(layered, 1), "GPU remove extra layer");
        const std::size_t unloadsBeforeRemove = terrainGpu.TextureUnloadCount();
        terrainGpu.Sync(&layered);
        Expect(terrainGpu.LayerCount() == 1, "remove drops GPU layer count");
        Expect(terrainGpu.GetLayerTexture(1) == nullptr, "removed layer texture is gone");
        Expect(
            terrainGpu.TextureUnloadCount() == unloadsBeforeRemove + 1,
            "removing a layer unloads its texture");
        terrainGpu.Sync(nullptr);
        Expect(terrainGpu.GetLayerTexture(0) == nullptr, "Level transition unloads layer 0");
        terrainGpu.Unload();
        Expect(terrainGpu.GetLayerTexture(0) == nullptr, "F2 unload clears layer textures");

        std::error_code cookedError;
        const std::filesystem::path cookedPreviewRoot =
            (std::filesystem::temp_directory_path() / "platformer_m90_terrain_cooked").lexically_normal();
        std::filesystem::remove_all(cookedPreviewRoot, cookedError);
        std::filesystem::create_directories(cookedPreviewRoot / "textures", cookedError);
        const std::filesystem::path cookedRgb = cookedPreviewRoot / "textures" / "test_cooked_rgb.png";
        Image rgbImage = GenImageColor(600, 400, Color{48, 96, 32, 255});
        ImageResize(&rgbImage, 512, 341);
        ExportImage(rgbImage, cookedRgb.string().c_str());
        UnloadImage(rgbImage);
        Expect(std::filesystem::is_regular_file(cookedRgb), "cooked RGB PNG was written");
        world::TerrainSpec cookedLayer = world::MakeDefaultTerrain();
        Expect(
            world::TryAssignTerrainTextureIdentity(cookedLayer, "textures/test_checker.png"),
            "cooked-preview layer 0 is staged");
        Expect(
            world::TryAddTerrainMaterialLayer(cookedLayer, "textures/test_cooked_rgb.png"),
            "cooked-preview extra layer identity");
        terrainGpu.SetAuthoringCookedRoot({});
        terrainGpu.Sync(&cookedLayer);
        Expect(terrainGpu.GetLayerTexture(0) != nullptr, "layer 0 still loads from staged");
        Expect(
            terrainGpu.GetLayerTexture(1) == nullptr,
            "Release semantics: unstaged extra layer does not load");
        terrainGpu.SetAuthoringCookedRoot(cookedPreviewRoot);
        terrainGpu.Sync(&cookedLayer);
        Expect(
            terrainGpu.GetLayerTexture(1) != nullptr,
            "Development cooked root loads unstaged extra layer");
        std::filesystem::copy_file(
            cookedRgb,
            cookedPreviewRoot / "textures" / "test_cooked_rgb_b.png",
            std::filesystem::copy_options::overwrite_existing,
            cookedError);
        cookedLayer.extraLayers[0].textureIdentity = "textures/test_cooked_rgb_b.png";
        const std::size_t loadsBeforeChange = terrainGpu.TextureLoadCount();
        terrainGpu.Sync(&cookedLayer);
        Expect(
            terrainGpu.GetLayerTexture(1) != nullptr,
            "changing extra-layer identity reloads from cooked root");
        Expect(
            terrainGpu.TextureLoadCount() == loadsBeforeChange + 1,
            "identity change is one new load");
        cookedLayer.extraLayers[0].textureIdentity = "textures/missing_extra.png";
        terrainGpu.Sync(&cookedLayer);
        Expect(
            terrainGpu.GetLayerTexture(1) == nullptr,
            "missing extra layer uses deterministic fallback (no GPU texture)");
        cookedLayer.extraLayers[0].textureIdentity = "textures/test_cooked_rgb.png";
        terrainGpu.Sync(&cookedLayer);
        Expect(
            terrainGpu.GetLayerTexture(1) != nullptr,
            "missing then available extra layer reconciles");

        Expect(
            StageSolidIdentity("textures/m90_blend_green.png", Color{0, 255, 0, 255}),
            "stage representative green layer 0");
        Expect(
            StageSolidIdentity("textures/m90_blend_blue.png", Color{0, 0, 255, 255}),
            "stage representative blue layer 1");
        Expect(
            StageSolidIdentity("textures/m90_blend_yellow.png", Color{255, 255, 0, 255}),
            "stage representative yellow layer 2");
        terrainGpu.SetAuthoringCookedRoot({});
        terrainGpu.SetAuthoringSourceRoot({});
        world::TerrainSpec blend = world::MakeDefaultTerrain();
        Expect(
            world::TryAssignTerrainTextureIdentity(blend, "textures/m90_blend_green.png"),
            "blend layer 0 green");
        Expect(
            world::TryAddTerrainMaterialLayer(blend, "textures/m90_blend_blue.png"),
            "blend layer 1 blue");
        Expect(
            world::TryAddTerrainMaterialLayer(blend, "textures/m90_blend_yellow.png"),
            "blend layer 2 yellow");
        terrainGpu.Sync(&blend);
        Expect(
            terrainGpu.GetLayerTexture(0) != nullptr && terrainGpu.GetLayerTexture(1) != nullptr
                && terrainGpu.GetLayerTexture(2) != nullptr,
            "representative layers load distinct GPU textures");
        Expect(
            terrainGpu.GetLayerTexture(0)->id != terrainGpu.GetLayerTexture(1)->id
                && terrainGpu.GetLayerTexture(1)->id != terrainGpu.GetLayerTexture(2)->id,
            "layer 0/1/2 map to distinct Texture2D ids");
        unsigned char weightR = 0;
        unsigned char weightG = 0;
        unsigned char weightB = 0;
        unsigned char weightA = 0;
        Expect(
            ReadVertexWeightColor(terrainGpu, 0, weightR, weightG, weightB, weightA)
                && weightR == 255 && weightG == 0 && weightB == 0 && weightA == 0,
            "default weights encode R=layer0 G=layer1 B=layer2 A=layer3");
        render::LightingEnvironment albedoOnly{};
        albedoOnly.ambient.color = {1.0f, 1.0f, 1.0f};
        albedoOnly.ambient.intensity = 1.0f;
        albedoOnly.directional.authoredEnabled = false;
        albedoOnly.directional.shadowsEnabled = false;
        Color pixel{};
        Expect(SampleLitTerrainPixel(lighting, terrainGpu, albedoOnly, pixel), "sample unpainted Terrain");
        Expect(
            ColorChannelsClose(pixel, Color{0, 255, 0, 255}, 40),
            "unpainted Terrain samples layer 0 green, not a tinted weight color");
        Expect(pixel.r < 40, "unpainted Terrain is not shadow-red");

        Expect(FillAllSampleWeights(blend, 0.0f, 1.0f, 0.0f, 0.0f), "set all samples to layer 1");
        terrainGpu.Sync(&blend);
        Expect(
            ReadVertexWeightColor(terrainGpu, 0, weightR, weightG, weightB, weightA)
                && weightR == 0 && weightG == 255 && weightB == 0 && weightA == 0,
            "layer 1 weights encode G=255 without using it as visual tint");
        Expect(SampleLitTerrainPixel(lighting, terrainGpu, albedoOnly, pixel), "sample layer 1 Terrain");
        Expect(
            ColorChannelsClose(pixel, Color{0, 0, 255, 255}, 40),
            "painted extra layer samples blue albedo, not shadow-map red");
        Expect(pixel.r < 40, "extra-layer paint is not shadow depth");
        Expect(pixel.b > 180, "extra-layer paint is actually the blue Texture");

        Expect(FillAllSampleWeights(blend, 0.0f, 0.0f, 1.0f, 0.0f), "set all samples to layer 2");
        terrainGpu.Sync(&blend);
        Expect(
            ReadVertexWeightColor(terrainGpu, 0, weightR, weightG, weightB, weightA)
                && weightB == 255 && weightR == 0 && weightG == 0,
            "layer 2 weights encode B=255");
        Expect(SampleLitTerrainPixel(lighting, terrainGpu, albedoOnly, pixel), "sample layer 2 Terrain");
        Expect(
            ColorChannelsClose(pixel, Color{255, 255, 0, 255}, 40),
            "layer 2 samples yellow albedo");

        world::TerrainSpec half = world::MakeDefaultTerrain();
        Expect(
            world::TryAssignTerrainTextureIdentity(half, "textures/m90_blend_green.png"),
            "half blend layer 0");
        Expect(
            world::TryAddTerrainMaterialLayer(half, "textures/m90_blend_blue.png"),
            "half blend layer 1");
        Expect(FillAllSampleWeights(half, 0.5f, 0.5f, 0.0f, 0.0f), "half-weight mixture");
        terrainGpu.Sync(&half);
        Expect(SampleLitTerrainPixel(lighting, terrainGpu, albedoOnly, pixel), "sample 0.5/0.5 blend");
        Expect(pixel.r < 50, "50/50 blend is not shadow-red");
        Expect(pixel.g > 80 && pixel.b > 80, "50/50 blend mixes green and blue albedos");

        world::TerrainSpec swap = world::MakeDefaultTerrain();
        Expect(
            world::TryAssignTerrainTextureIdentity(swap, "textures/m90_blend_green.png"),
            "identity A Layer 0");
        terrainGpu.Sync(&swap);
        const std::size_t loadsBeforeSwap = terrainGpu.TextureLoadCount();
        Expect(SampleLitTerrainPixel(lighting, terrainGpu, albedoOnly, pixel), "sample identity A");
        Expect(ColorChannelsClose(pixel, Color{0, 255, 0, 255}, 40), "Layer 0 identity A is green");
        Expect(
            world::TryAssignTerrainTextureIdentity(swap, "textures/m90_blend_yellow.png"),
            "identity A->B on layer 0");
        terrainGpu.Sync(&swap);
        Expect(terrainGpu.GetLayerTexture(0) != nullptr, "identity B loads");
        Expect(
            terrainGpu.TextureLoadCount() == loadsBeforeSwap + 1,
            "A->B is one new Layer 0 load");
        Expect(SampleLitTerrainPixel(lighting, terrainGpu, albedoOnly, pixel), "sample identity B");
        Expect(ColorChannelsClose(pixel, Color{255, 255, 0, 255}, 40), "Layer 0 identity B is yellow");
        Expect(
            world::TryAssignTerrainTextureIdentity(swap, "textures/m90_blend_green.png"),
            "identity B->A on layer 0");
        const std::size_t loadsBeforeReturn = terrainGpu.TextureLoadCount();
        terrainGpu.Sync(&swap);
        Expect(
            terrainGpu.TextureLoadCount() == loadsBeforeReturn + 1,
            "B->A reloads Layer 0 instead of keeping a stale Texture");
        Expect(SampleLitTerrainPixel(lighting, terrainGpu, albedoOnly, pixel), "sample identity A again");
        Expect(
            ColorChannelsClose(pixel, Color{0, 255, 0, 255}, 40),
            "returning to identity A is not a stale yellow/fallback");

        std::error_code sourceError;
        const std::filesystem::path sourcePreviewRoot =
            (std::filesystem::temp_directory_path() / "platformer_m90_terrain_source").lexically_normal();
        std::filesystem::remove_all(sourcePreviewRoot, sourceError);
        std::filesystem::create_directories(sourcePreviewRoot / "textures", sourceError);
        Image sourceGreen = GenImageColor(8, 8, Color{0, 220, 40, 255});
        const std::filesystem::path sourceOnlyPng =
            sourcePreviewRoot / "textures" / "m90_source_only_layer0.png";
        ExportImage(sourceGreen, sourceOnlyPng.string().c_str());
        UnloadImage(sourceGreen);
        world::TerrainSpec sourceLayer0 = world::MakeDefaultTerrain();
        Expect(
            world::TryAssignTerrainTextureIdentity(
                sourceLayer0, "textures/m90_source_only_layer0.png"),
            "source-only Layer 0 identity");
        terrainGpu.SetAuthoringCookedRoot({});
        terrainGpu.SetAuthoringSourceRoot({});
        terrainGpu.Sync(&sourceLayer0);
        Expect(
            terrainGpu.GetLayerTexture(0) == nullptr,
            "Release semantics: unstaged uncooked Layer 0 does not load");
        terrainGpu.SetAuthoringSourceRoot(sourcePreviewRoot);
        terrainGpu.Sync(&sourceLayer0);
        Expect(
            terrainGpu.GetLayerTexture(0) != nullptr,
            "Development source last-resort loads assigned Layer 0");
        Expect(SampleLitTerrainPixel(lighting, terrainGpu, albedoOnly, pixel), "sample source Layer 0");
        Expect(pixel.g > 150 && pixel.r < 80, "source-only Layer 0 shows the assigned Texture");
        terrainGpu.SetAuthoringSourceRoot({});
        std::filesystem::remove_all(sourcePreviewRoot, sourceError);

        terrainGpu.SetAuthoringCookedRoot({});
        terrainGpu.Unload();
        std::filesystem::remove_all(cookedPreviewRoot, cookedError);
    }

    lighting.Unload();
    lighting.Unload();
    Expect(!lighting.IsReady(), "unload leaves lighting unready");
    Expect(lighting.LitShaderId() == 0, "unload clears the lit shader");
    Expect(lighting.ShadowMapId() == 0, "unload clears the shadow map");

    lighting.Load();
    Expect(lighting.IsReady(), "load after unload succeeds");
    Expect(lighting.ShaderLoadCount() == shaderLoads + 1, "reload is a new deterministic load");
    lighting.Unload();

    CloseWindow();
    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d WorldLightingResourcesTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("WorldLightingResourcesTest passed\n");
    return 0;
}
