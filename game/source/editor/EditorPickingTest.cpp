#include "editor/EditorCamera.h"
#include "editor/EditorHierarchy.h"
#include "editor/EditorPicking.h"
#include "editor/EditorSelection.h"
#include "world/LevelDefinition.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>

namespace
{
int gFailures = 0;

void Expect(bool condition, const std::string& name)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL %s\n", name.c_str());
        ++gFailures;
    }
}

bool NearlyEqual(float a, float b, float epsilon = 0.001f)
{
    return std::fabs(a - b) <= epsilon;
}

bool Vec3Near(core::Vec3 a, core::Vec3 b, float epsilon = 0.001f)
{
    return NearlyEqual(a.x, b.x, epsilon) && NearlyEqual(a.y, b.y, epsilon)
        && NearlyEqual(a.z, b.z, epsilon);
}

const editor::PickingProxy* FindProxy(
    const editor::EditorPickingSet& set,
    editor::EditorObjectKind kind,
    std::size_t index)
{
    for (int proxyIndex = 0; proxyIndex < set.count; ++proxyIndex)
    {
        const editor::PickingProxy& proxy = set.proxies[static_cast<std::size_t>(proxyIndex)];
        if (proxy.selection.kind == kind && proxy.selection.index == index)
        {
            return &proxy;
        }
    }
    return nullptr;
}

world::LevelDefinition MakeStubLevel()
{
    world::LevelDefinition level{};
    level.id = "level_01";
    level.initialSpawnVisualCenter = {0.0f, 0.8f, 0.0f};
    level.ground = {{0.0f, -0.25f, 0.0f}, {10.0f, 0.5f, 8.0f}};
    for (world::Box& platform : level.elevatedPlatforms)
    {
        platform = {{0.0f, 1.0f, 0.0f}, {1.0f, 0.5f, 1.0f}};
    }
    level.elevatedPlatforms[0] = {{5.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}};
    level.slopes[0] = {{0.0f, 0.0f, 0.0f}, {4.0f, 0.4f, 2.0f}, 0.0f};
    level.slopes[1] = {{0.0f, 0.0f, 0.0f}, {4.0f, 0.4f, 2.0f}, 90.0f};
    level.movingPlatform.size = {4.0f, 0.4f, 3.0f};
    level.movingPlatform.centerY = 1.3f;
    level.movingPlatform.centerZ = 0.0f;
    level.movingPlatform.startX = 0.0f;
    level.dynamicBox = {{0.0f, 5.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 30.0f};
    level.camera = {{2.0f, 3.5f, 12.0f}, 40.0f};
    level.checkpoints[0] = {{16.5f, 1.8f, 0.0f}, {2.4f, 1.6f, 2.0f}, {16.5f, 1.8f, 0.0f}};
    level.hazards[0] = {{11.5f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}};
    level.collectibles[0] = {{5.0f, 2.5f, 0.0f}, {1.0f, 1.2f, 1.0f}};
    level.goal = {{-21.0f, 3.8f, 0.0f}, {2.0f, 1.6f, 1.8f}};
    return level;
}
}

int main()
{
    // ---- selection model ----
    {
        const world::LevelDefinition level = MakeStubLevel();
        Expect(editor::IsValidSelection(level, editor::ClearSelection()), "None is valid");
        Expect(editor::ClearSelection().kind == editor::EditorObjectKind::None, "clear is None");
        Expect(
            std::strcmp(editor::SelectionDisplayName(editor::ClearSelection()), "(none)") == 0,
            "None display name");

        editor::EditorSelection platform0{editor::EditorObjectKind::ElevatedPlatform, 0};
        editor::EditorSelection platform5{editor::EditorObjectKind::ElevatedPlatform, 5};
        editor::EditorSelection platform6{editor::EditorObjectKind::ElevatedPlatform, 6};
        Expect(editor::IsValidSelection(level, platform0), "platform 0 valid");
        Expect(editor::IsValidSelection(level, platform5), "platform 5 valid");
        Expect(!editor::IsValidSelection(level, platform6), "platform 6 invalid");
        Expect(platform0 != platform5, "platform index participates in equality");
        Expect(
            std::strcmp(editor::SelectionDisplayName(platform0), "Platform 0") == 0,
            "platform 0 name");

        editor::EditorSelection collectible2{editor::EditorObjectKind::Collectible, 2};
        editor::EditorSelection collectible3{editor::EditorObjectKind::Collectible, 3};
        Expect(editor::IsValidSelection(level, collectible2), "collectible 2 valid");
        Expect(!editor::IsValidSelection(level, collectible3), "collectible 3 invalid");

        editor::EditorSelection camera{editor::EditorObjectKind::Camera, 0};
        Expect(editor::IsValidSelection(level, camera), "camera identity is valid");
        Expect(editor::IsEditableSelection(camera), "camera is M32-editable");
        Expect(
            !editor::IsEditableSelection({editor::EditorObjectKind::Slope, 0}),
            "slope is not editable");
        Expect(
            !editor::IsEditableSelection({editor::EditorObjectKind::Hazard, 0}),
            "hazard is not editable");
        Expect(
            editor::IsEditableSelection({editor::EditorObjectKind::Ground, 0}),
            "ground is editable");
    }

    Expect(editor::kHierarchyEntryCount == 21, "hierarchy lists every v1 authored object");
    Expect(
        editor::kHierarchyEntries[0].selection.kind == editor::EditorObjectKind::Spawn,
        "hierarchy starts with Player Spawn");
    Expect(
        editor::kHierarchyEntries[3].selection
            == editor::EditorSelection{editor::EditorObjectKind::ElevatedPlatform, 0},
        "hierarchy includes Platform 0");

    // ---- ray vs AABB ----
    {
        const editor::Ray3 front{{0.0f, 0.0f, 10.0f}, {0.0f, 0.0f, -1.0f}};
        const editor::RayHit hit = editor::IntersectRayAabb(front, {}, {2.0f, 2.0f, 2.0f});
        Expect(hit.hit, "front hit");
        Expect(NearlyEqual(hit.distance, 9.0f), "front hit distance to z=+1 face");

        const editor::RayHit miss =
            editor::IntersectRayAabb({{5.0f, 0.0f, 10.0f}, {0.0f, 0.0f, -1.0f}}, {}, {2.0f, 2.0f, 2.0f});
        Expect(!miss.hit, "clear miss");

        const editor::RayHit inside =
            editor::IntersectRayAabb({{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}, {}, {2.0f, 2.0f, 2.0f});
        Expect(inside.hit, "ray starts inside");
        Expect(NearlyEqual(inside.distance, 0.0f), "inside distance is zero");

        const editor::RayHit parallelMiss =
            editor::IntersectRayAabb({{5.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}, {}, {2.0f, 2.0f, 2.0f});
        Expect(!parallelMiss.hit, "parallel miss");

        const editor::Ray3 backward{{0.0f, 0.0f, -10.0f}, {0.0f, 0.0f, -1.0f}};
        const editor::RayHit behind = editor::IntersectRayAabb(backward, {}, {2.0f, 2.0f, 2.0f});
        Expect(!behind.hit, "box behind ray is a miss");

        const editor::Ray3 toward{{0.0f, 0.0f, -10.0f}, {0.0f, 0.0f, 1.0f}};
        const editor::RayHit negativeDir = editor::IntersectRayAabb(toward, {}, {2.0f, 2.0f, 2.0f});
        Expect(negativeDir.hit, "negative-axis direction still hits");
        Expect(NearlyEqual(negativeDir.distance, 9.0f), "negative-dir distance");
    }

    // ---- rotated slope / local AABB ----
    {
        const core::Vec3 center{};
        const core::Vec3 size{4.0f, 0.4f, 2.0f};
        const editor::Ray3 down{{1.5f, 5.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
        Expect(
            editor::IntersectRayOrientedAabb(down, center, size, 0.0f).hit,
            "unrotated slope hit from above");
        Expect(
            !editor::IntersectRayOrientedAabb(down, center, size, 90.0f).hit,
            "90-degree slope misses the same downward ray");

        const editor::Ray3 alongX{{-5.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
        Expect(
            editor::IntersectRayOrientedAabb(alongX, center, size, 90.0f).hit,
            "90-degree slope hit along +X");
        Expect(
            !editor::IntersectRayOrientedAabb({{0.0f, 5.0f, 3.0f}, {0.0f, -1.0f, 0.0f}}, center, size, 30.0f)
                 .hit,
            "obvious miss past Z extent");
    }

    // ---- nearest selection ----
    {
        world::LevelDefinition level = MakeStubLevel();
        level.initialSpawnVisualCenter = {40.0f, 0.8f, 0.0f};
        level.ground = {{40.0f, -0.25f, 0.0f}, {1.0f, 0.5f, 1.0f}};
        for (world::Box& platform : level.elevatedPlatforms)
        {
            platform = {{40.0f, 1.0f, 0.0f}, {1.0f, 0.5f, 1.0f}};
        }
        level.slopes[0].center = {40.0f, 0.0f, 0.0f};
        level.slopes[1].center = {40.0f, 0.0f, 0.0f};
        level.movingPlatform.centerY = 40.0f;
        level.dynamicBox.center = {40.0f, 5.0f, 0.0f};
        level.checkpoints[0].center = {40.0f, 1.8f, 0.0f};
        level.hazards[0].center = {40.0f, 0.5f, 0.0f};
        level.goal.center = {40.0f, 3.8f, 0.0f};
        level.elevatedPlatforms[0] = {{0.0f, 0.0f, -4.0f}, {2.0f, 2.0f, 2.0f}};
        level.collectibles[0] = {{0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
        level.collectibles[1].center = {40.0f, 4.0f, 0.0f};
        level.collectibles[2].center = {40.0f, 3.75f, 0.0f};
        const editor::EditorPickingSet set =
            editor::BuildPickingSet(level, editor::AuthoredPickingWorldState(level));
        const editor::Ray3 ray{{0.0f, 0.0f, 5.0f}, {0.0f, 0.0f, -1.0f}};
        const editor::EditorSelection picked = editor::PickNearest(ray, set);
        Expect(picked.kind == editor::EditorObjectKind::Collectible, "nearer collectible wins");
        Expect(picked.index == 0, "collectible 0 is the nearer candidate");

        const editor::Ray3 missRay{{40.0f, 40.0f, 40.0f}, {0.0f, 1.0f, 0.0f}};
        Expect(
            editor::PickNearest(missRay, set).kind == editor::EditorObjectKind::None,
            "empty space is None");
    }

    // ---- stable tie: overlapping equal-distance candidates keep hierarchy order ----
    {
        world::LevelDefinition level = MakeStubLevel();
        level.ground = {{0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f}};
        level.elevatedPlatforms[0] = {{0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f}};
        const editor::EditorPickingSet set =
            editor::BuildPickingSet(level, editor::AuthoredPickingWorldState(level));
        const editor::Ray3 ray{{0.0f, 0.0f, 10.0f}, {0.0f, 0.0f, -1.0f}};
        const editor::EditorSelection picked = editor::PickNearest(ray, set);
        Expect(picked.kind == editor::EditorObjectKind::Ground, "tie keeps earlier hierarchy proxy");
    }

    // ---- runtime pose vs authored start for moving platform / cyan box ----
    {
        world::LevelDefinition level = MakeStubLevel();
        editor::EditorPickingWorldState worldState = editor::AuthoredPickingWorldState(level);
        Expect(NearlyEqual(worldState.movingPlatformCenter.x, 0.0f), "authored startX proxy");

        worldState.movingPlatformCenter = {10.0f, 1.3f, 0.0f};
        worldState.dynamicBoxCenter = {8.0f, 1.0f, 0.0f};
        const editor::EditorPickingSet set = editor::BuildPickingSet(level, worldState);

        const editor::Ray3 atRuntime{{10.0f, 1.3f, 8.0f}, {0.0f, 0.0f, -1.0f}};
        Expect(
            editor::PickNearest(atRuntime, set).kind == editor::EditorObjectKind::MovingPlatform,
            "moving platform picks the visible runtime center");

        const editor::Ray3 atAuthoredStart{{0.0f, 1.3f, 8.0f}, {0.0f, 0.0f, -1.0f}};
        Expect(
            editor::PickNearest(atAuthoredStart, set).kind != editor::EditorObjectKind::MovingPlatform,
            "authored startX is not picked when the platform has moved");

        const editor::Ray3 atCyan{{8.0f, 1.0f, 8.0f}, {0.0f, 0.0f, -1.0f}};
        Expect(
            editor::PickNearest(atCyan, set).kind == editor::EditorObjectKind::DynamicBox,
            "cyan box picks the visible runtime center");
    }

    // ---- camera is not a world proxy; spawn is ----
    {
        const world::LevelDefinition level = MakeStubLevel();
        const editor::EditorPickingSet set =
            editor::BuildPickingSet(level, editor::AuthoredPickingWorldState(level));
        bool sawCamera = false;
        bool sawSpawn = false;
        bool sawPlayerKind = false;
        for (int index = 0; index < set.count; ++index)
        {
            sawCamera = sawCamera
                || set.proxies[static_cast<std::size_t>(index)].selection.kind
                    == editor::EditorObjectKind::Camera;
            sawSpawn = sawSpawn
                || set.proxies[static_cast<std::size_t>(index)].selection.kind
                    == editor::EditorObjectKind::Spawn;
        }
        Expect(!sawCamera, "authored camera has no world picking proxy");
        Expect(sawSpawn, "player spawn has a world picking proxy");
        Expect(!sawPlayerKind, "runtime Player is not a selectable authored object");
        Expect(set.count == 20, "20 world proxies: hierarchy minus Camera");
    }

    // ---- screen-to-world ray through editor camera view ----
    {
        render::CameraView view{};
        view.position = {0.0f, 0.0f, 10.0f};
        view.target = {0.0f, 0.0f, 0.0f};
        view.up = {0.0f, 1.0f, 0.0f};
        view.fieldOfViewY = 90.0f;
        const editor::Ray3 center = editor::ScreenToWorldRay(view, 50.0f, 50.0f, 100.0f, 100.0f);
        Expect(Vec3Near(center.origin, view.position), "ray origin is camera position");
        Expect(Vec3Near(center.direction, {0.0f, 0.0f, -1.0f}, 0.01f), "center pixel looks along -Z");

        const editor::Ray3 right = editor::ScreenToWorldRay(view, 100.0f, 50.0f, 100.0f, 100.0f);
        Expect(right.direction.x > 0.1f, "right-edge pixel has +X");
        Expect(right.direction.z < 0.0f, "right-edge pixel still looks forward");
    }

    // ---- editor camera does not touch authored framing ----
    {
        world::LevelDefinition level = MakeStubLevel();
        const core::Vec3 authoredOffset = level.camera.offset;
        const float authoredFov = level.camera.fieldOfViewY;

        editor::EditorCamera editorCamera{};
        editor::SeedEditorCameraFromGameplay(
            editorCamera, {0.0f, 0.8f, 0.0f}, authoredOffset, authoredFov);
        Expect(editorCamera.initialized, "first seed initializes");
        Expect(Vec3Near(editorCamera.position, authoredOffset + core::Vec3{0.0f, 0.8f, 0.0f}), "seed pose");

        editorCamera.yawDegrees = 45.0f;
        editor::SeedEditorCameraFromGameplay(
            editorCamera, {0.0f, 0.8f, 0.0f}, authoredOffset, authoredFov);
        Expect(NearlyEqual(editorCamera.yawDegrees, 45.0f), "second seed preserves session pose");
        Expect(Vec3Near(level.camera.offset, authoredOffset), "authored offset unchanged");
        Expect(NearlyEqual(level.camera.fieldOfViewY, authoredFov), "authored FOV unchanged");

        const render::CameraView view = editor::MakeCameraView(editorCamera);
        Expect(Vec3Near(view.position, editorCamera.position), "CameraView position");
        Expect(view.fieldOfViewY == editorCamera.fieldOfViewY, "CameraView FOV");
    }

    // ---- UpdateEditorCamera never writes LevelDefinition.camera ----
    {
        world::LevelDefinition level = MakeStubLevel();
        const core::Vec3 authoredOffset = level.camera.offset;
        const float authoredFov = level.camera.fieldOfViewY;

        editor::EditorCamera camera{};
        camera.initialized = true;
        camera.position = {0.0f, 2.0f, 10.0f};
        camera.movementSpeed = 8.0f;
        camera.fieldOfViewY = 40.0f;

        editor::EditorInputState input{};
        input.moveForward = 1.0f;
        input.moveRight = 1.0f;
        input.moveUp = 1.0f;
        input.lookHeld = true;
        input.mouseDeltaX = 10.0f;
        input.mouseDeltaY = 4.0f;
        input.wheelDelta = 3.0f;
        editor::UpdateEditorCamera(camera, input, 0.25f, true, true, true);
        Expect(camera.yawDegrees > 0.0f, "look yaw changes");
        Expect(camera.position.z < 10.0f, "forward moves along -Z");
        Expect(camera.movementSpeed > 8.0f, "wheel raises speed");
        Expect(Vec3Near(level.camera.offset, authoredOffset), "nav does not write offset");
        Expect(NearlyEqual(level.camera.fieldOfViewY, authoredFov), "nav does not write FOV");

        const float yaw = camera.yawDegrees;
        const core::Vec3 position = camera.position;
        const float speed = camera.movementSpeed;
        editor::UpdateEditorCamera(camera, input, 0.25f, false, false, false);
        Expect(NearlyEqual(camera.yawDegrees, yaw), "gated look does not rotate");
        Expect(Vec3Near(camera.position, position), "gated move does not translate");
        Expect(NearlyEqual(camera.movementSpeed, speed), "gated wheel does not change speed");
    }

    Expect(
        NearlyEqual(editor::kCollectiblePickingSize, world::kCollectibleVisualSize),
        "collectible pick size matches visual cube");

    // ---- highlight request follows the same proxy as picking ----
    {
        const world::LevelDefinition level = MakeStubLevel();
        const editor::EditorPickingSet set =
            editor::BuildPickingSet(level, editor::AuthoredPickingWorldState(level));
        const editor::EditorHighlightRequest none =
            editor::MakeHighlightRequest(editor::ClearSelection(), set);
        Expect(!none.visible, "None has no highlight");

        const editor::EditorHighlightRequest ground = editor::MakeHighlightRequest(
            {editor::EditorObjectKind::Ground, 0}, set);
        Expect(ground.visible, "ground highlight visible");
        Expect(Vec3Near(ground.center, level.ground.center), "ground highlight uses authored center");
        Expect(NearlyEqual(ground.rotationZDegrees, 0.0f), "ground highlight is axis-aligned");

        const editor::EditorHighlightRequest slope = editor::MakeHighlightRequest(
            {editor::EditorObjectKind::Slope, 1}, set);
        Expect(slope.visible, "slope 1 highlight visible");
        Expect(NearlyEqual(slope.rotationZDegrees, 90.0f), "slope highlight keeps rotation");
    }

    // ---- unapplied working-copy transforms must not move pick/highlight ----
    {
        world::LevelDefinition active = MakeStubLevel();
        active.ground = {{40.0f, -0.25f, 0.0f}, {1.0f, 0.5f, 1.0f}};
        active.initialSpawnVisualCenter = {1.0f, 0.8f, 0.0f};
        for (world::Box& platform : active.elevatedPlatforms)
        {
            platform = {{40.0f, 1.0f, 0.0f}, {1.0f, 0.5f, 1.0f}};
        }
        active.elevatedPlatforms[0] = {{5.0f, 1.0f, 0.0f}, {2.0f, 1.0f, 2.0f}};
        active.slopes[0].center = {40.0f, 0.0f, 0.0f};
        active.slopes[1].center = {40.0f, 0.0f, 0.0f};
        active.movingPlatform.centerY = 40.0f;
        active.dynamicBox.center = {40.0f, 5.0f, 0.0f};
        active.checkpoints[0].center = {40.0f, 1.8f, 0.0f};
        active.checkpoints[1].center = {40.0f, 1.8f, 0.0f};
        active.hazards[0].center = {40.0f, 0.5f, 0.0f};
        active.hazards[1].center = {40.0f, 0.5f, 0.0f};
        active.collectibles[0].center = {40.0f, 2.5f, 0.0f};
        active.collectibles[1].center = {40.0f, 2.5f, 0.0f};
        active.collectibles[2].center = {40.0f, 2.5f, 0.0f};
        active.goal.center = {40.0f, 3.8f, 0.0f};

        world::LevelDefinition working = active;
        working.elevatedPlatforms[0].center.x = 20.0f;
        working.initialSpawnVisualCenter.x = 9.0f;

        const editor::EditorPickingSet set =
            editor::BuildPickingSet(active, editor::AuthoredPickingWorldState(active));
        const editor::PickingProxy* platform = FindProxy(
            set, editor::EditorObjectKind::ElevatedPlatform, 0);
        Expect(platform != nullptr, "active platform 0 has a proxy");
        Expect(
            platform != nullptr && NearlyEqual(platform->center.x, 5.0f),
            "platform pick proxy stays at applied X=5");
        Expect(
            working.elevatedPlatforms[0].center.x == 20.0f,
            "working copy X=7-style edit remains 20");

        const editor::EditorHighlightRequest highlight = editor::MakeHighlightRequest(
            {editor::EditorObjectKind::ElevatedPlatform, 0}, set);
        Expect(highlight.visible, "platform 0 highlight visible from active set");
        Expect(NearlyEqual(highlight.center.x, 5.0f), "highlight stays at applied X=5");

        const editor::PickingProxy* spawn =
            FindProxy(set, editor::EditorObjectKind::Spawn, 0);
        Expect(spawn != nullptr && NearlyEqual(spawn->center.x, 1.0f), "spawn proxy is applied");

        const editor::Ray3 atVisible{{5.0f, 1.0f, 8.0f}, {0.0f, 0.0f, -1.0f}};
        const editor::EditorSelection visibleHit = editor::PickNearest(atVisible, set);
        Expect(visibleHit.kind == editor::EditorObjectKind::ElevatedPlatform, "visible X=5 picks");
        Expect(visibleHit.index == 0, "visible hit is platform 0");

        const editor::Ray3 atUnapplied{{20.0f, 1.0f, 8.0f}, {0.0f, 0.0f, -1.0f}};
        Expect(
            editor::PickNearest(atUnapplied, set).kind == editor::EditorObjectKind::None,
            "unapplied X=20 is not selectable");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d editor picking/selection test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Editor picking tests passed.\n");
    return 0;
}
