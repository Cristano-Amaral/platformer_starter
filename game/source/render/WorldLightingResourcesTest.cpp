// Milestone 85: lighting shader/shadow resource lifetime. Hidden window.
// Stages copies next to the test exe; no source-directory fallback.

#include "platform/RuntimePaths.h"
#include "render/LightingEnvironment.h"
#include "render/TerrainMesh.h"
#include "render/WorldLighting.h"
#include "world/LocalLight.h"
#include "world/Terrain.h"
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
