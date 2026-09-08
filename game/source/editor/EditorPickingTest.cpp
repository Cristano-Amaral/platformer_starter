#include "editor/AuthoredObjectLifecycle.h"
#include "editor/EditorCamera.h"
#include "editor/EditorGizmo.h"
#include "editor/EditorHierarchy.h"
#include "editor/EditorPicking.h"
#include "editor/EditorSelection.h"
#include "editor/EditorWorkspace.h"
#include "gameplay/CollectibleRunState.h"
#include "world/LevelDefinition.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

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
    for (const editor::PickingProxy& proxy : set.proxies)
    {
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
    level.elevatedPlatforms.assign(
        static_cast<std::size_t>(world::kLevel01ElevatedPlatformCount),
        world::Box{{0.0f, 1.0f, 0.0f}, {1.0f, 0.5f, 1.0f}});
    level.elevatedPlatforms[0] = {{5.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}};
    level.slopes[0] = {{0.0f, 0.0f, 0.0f}, {4.0f, 0.4f, 2.0f}, 0.0f};
    level.slopes[1] = {{0.0f, 0.0f, 0.0f}, {4.0f, 0.4f, 2.0f}, 90.0f};
    level.movingPlatform.size = {4.0f, 0.4f, 3.0f};
    level.movingPlatform.centerY = 1.3f;
    level.movingPlatform.centerZ = 0.0f;
    level.movingPlatform.startX = 0.0f;
    level.camera = {{2.0f, 3.5f, 12.0f}, 40.0f};
    level.checkpoints.resize(static_cast<std::size_t>(world::kLevel01CheckpointCount));
    level.checkpoints[0] = {{16.5f, 1.8f, 0.0f}, {2.4f, 1.6f, 2.0f}, {16.5f, 1.8f, 0.0f}};
    level.hazards.resize(static_cast<std::size_t>(world::kLevel01HazardCount));
    level.hazards[0] = {{11.5f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}};
    level.collectibles.resize(static_cast<std::size_t>(world::kLevel01CollectibleCount));
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
            editor::IsEditableSelection({editor::EditorObjectKind::Hazard, 0}),
            "hazard is inspector-editable");
        Expect(
            editor::IsEditableSelection({editor::EditorObjectKind::Checkpoint, 0}),
            "checkpoint is inspector-editable");
        Expect(
            editor::IsEditableSelection({editor::EditorObjectKind::Collectible, 0}),
            "collectible is inspector-editable");
        Expect(
            editor::IsEditableSelection({editor::EditorObjectKind::Ground, 0}),
            "ground is editable");
        Expect(
            !editor::IsValidSelection(level, {editor::EditorObjectKind::DynamicBox, 0}),
            "DynamicBox is not a selectable scene object");
        Expect(
            editor::IsEditableSelection({editor::EditorObjectKind::DynamicBox, 0}),
            "DynamicBox is inspector-editable");
    }

    const std::vector<editor::HierarchyEntry> hierarchy =
        editor::BuildHierarchyEntries(MakeStubLevel());
    const std::size_t expectedHierarchy =
        5 + static_cast<std::size_t>(world::kLevel01ElevatedPlatformCount)
        + static_cast<std::size_t>(world::kLevel01SlopeCount)
        + static_cast<std::size_t>(world::kLevel01CheckpointCount)
        + static_cast<std::size_t>(world::kLevel01HazardCount)
        + static_cast<std::size_t>(world::kLevel01CollectibleCount);
    Expect(hierarchy.size() == expectedHierarchy, "hierarchy lists every stub authored scene object");
    bool hierarchyHasDynamicBox = false;
    bool hierarchyHasGoal = false;
    for (const editor::HierarchyEntry& entry : hierarchy)
    {
        hierarchyHasDynamicBox =
            hierarchyHasDynamicBox || entry.selection.kind == editor::EditorObjectKind::DynamicBox;
        hierarchyHasGoal = hierarchyHasGoal || entry.selection.kind == editor::EditorObjectKind::Goal;
    }
    Expect(!hierarchyHasDynamicBox, "empty Dynamic Boxes collection has no hierarchy rows");
    Expect(hierarchyHasGoal, "Hierarchy lists Goal");
    Expect(
        hierarchy.back().selection.kind == editor::EditorObjectKind::Goal,
        "hierarchy ends with Goal");
    Expect(
        hierarchy[0].selection.kind == editor::EditorObjectKind::Spawn,
        "hierarchy starts with Player Spawn");
    Expect(
        hierarchy[3].selection
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

    // ---- runtime pose vs authored start for moving platform / Dynamic Box ----
    {
        world::LevelDefinition level = MakeStubLevel();
        editor::EditorPickingWorldState worldState = editor::AuthoredPickingWorldState(level);
        Expect(NearlyEqual(worldState.movingPlatformCenter.x, 0.0f), "authored startX proxy");

        worldState.movingPlatformCenter = {10.0f, 1.3f, 0.0f};
        const editor::EditorPickingSet set = editor::BuildPickingSet(level, worldState);

        const editor::Ray3 atRuntime{{10.0f, 1.3f, 8.0f}, {0.0f, 0.0f, -1.0f}};
        Expect(
            editor::PickNearest(atRuntime, set).kind == editor::EditorObjectKind::MovingPlatform,
            "moving platform picks the visible runtime center");

        const editor::Ray3 atAuthoredStart{{0.0f, 1.3f, 8.0f}, {0.0f, 0.0f, -1.0f}};
        Expect(
            editor::PickNearest(atAuthoredStart, set).kind != editor::EditorObjectKind::MovingPlatform,
            "authored startX is not picked when the platform has moved");
    }

    {
        world::LevelDefinition level = MakeStubLevel();
        world::DynamicBoxSpec box{};
        box.center = {2.0f, 1.0f, 0.0f};
        box.size = {1.0f, 1.0f, 1.0f};
        box.massKg = 30.0f;
        level.dynamicBoxes.push_back(box);
        editor::EditorPickingWorldState worldState = editor::AuthoredPickingWorldState(level);
        worldState.dynamicBoxCenters[0] = {8.0f, 1.0f, 0.0f};
        worldState.dynamicBoxSizes[0] = box.size;
        const editor::EditorPickingSet set = editor::BuildPickingSet(level, worldState);
        const editor::Ray3 atRuntime{{8.0f, 1.0f, 8.0f}, {0.0f, 0.0f, -1.0f}};
        Expect(
            editor::PickNearest(atRuntime, set).kind == editor::EditorObjectKind::DynamicBox,
            "active Dynamic Box picks the runtime pose");
        const editor::Ray3 atAuthored{{2.0f, 1.0f, 8.0f}, {0.0f, 0.0f, -1.0f}};
        Expect(
            editor::PickNearest(atAuthored, set).kind != editor::EditorObjectKind::DynamicBox,
            "stale authored center does not own active picking");
    }

    // ---- camera is not a world proxy; spawn is ----
    {
        const world::LevelDefinition level = MakeStubLevel();
        const editor::EditorPickingSet set =
            editor::BuildPickingSet(level, editor::AuthoredPickingWorldState(level));
        bool sawCamera = false;
        bool sawSpawn = false;
        bool sawPlayerKind = false;
        bool sawDynamicBox = false;
        for (const editor::PickingProxy& proxy : set.proxies)
        {
            sawCamera = sawCamera || proxy.selection.kind == editor::EditorObjectKind::Camera;
            sawSpawn = sawSpawn || proxy.selection.kind == editor::EditorObjectKind::Spawn;
            sawDynamicBox =
                sawDynamicBox || proxy.selection.kind == editor::EditorObjectKind::DynamicBox;
        }
        Expect(!sawCamera, "authored camera has no world picking proxy");
        Expect(sawSpawn, "player spawn has a world picking proxy");
        Expect(!sawPlayerKind, "runtime Player is not a selectable authored object");
        Expect(!sawDynamicBox, "empty Dynamic Boxes collection has no world picking proxy");
        Expect(
            set.proxies.size() == 19,
            "19 world proxies: hierarchy minus Camera");
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

        const editor::EditorContentViewport menuOnly =
            editor::MakeEditorContentViewport(100.0f, 100.0f, 20.0f);
        const editor::EditorContentViewport withToolbar =
            editor::MakeEditorContentViewport(100.0f, 100.0f, 40.0f);
        const editor::Ray3 menuCenter = editor::ScreenToWorldRayFromWindow(
            view, 50.0f, menuOnly.y + menuOnly.height * 0.5f, menuOnly);
        const editor::Ray3 toolbarCenter = editor::ScreenToWorldRayFromWindow(
            view, 50.0f, withToolbar.y + withToolbar.height * 0.5f, withToolbar);
        Expect(
            Vec3Near(menuCenter.direction, toolbarCenter.direction, 0.01f),
            "toolbar-visible and hidden content-center picks share the look ray");
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

    {
        editor::EditorSelection platformPick{editor::EditorObjectKind::ElevatedPlatform, 2};
        editor::StructuralIndexMap identity{};
        Expect(
            editor::ShouldAcceptActiveWorldPick(platformPick, identity),
            "uninitialized map still maps active pick as identity");
        Expect(
            editor::ShouldAcceptActiveWorldPick({editor::EditorObjectKind::Ground, 0}, identity),
            "ground pick still accepted");
        Expect(
            editor::ShouldAcceptActiveWorldPick(editor::ClearSelection(), identity),
            "empty click still clears");
        const std::vector<editor::HierarchyEntry> afterDelete = editor::BuildHierarchyEntries(
            [] {
                world::LevelDefinition working = MakeStubLevel();
                working.elevatedPlatforms.erase(working.elevatedPlatforms.begin() + 2);
                return working;
            }());
        bool foundPlatform2 = false;
        std::size_t platformCount = 0;
        for (const editor::HierarchyEntry& entry : afterDelete)
        {
            if (entry.selection.kind == editor::EditorObjectKind::ElevatedPlatform)
            {
                ++platformCount;
                if (entry.selection.index == 2)
                {
                    foundPlatform2 = true;
                }
            }
        }
        Expect(platformCount == 5, "hierarchy follows workingCopy count");
        Expect(foundPlatform2, "remaining platforms reindex; old 3 is now 2");
    }

    {
        world::LevelDefinition active{};
        active.checkpoints.push_back({{0.0f, 1.0f, 0.0f}, {2.0f, 1.0f, 2.0f}, {0.0f, 1.0f, 0.0f}});
        active.checkpoints.push_back({{4.0f, 1.0f, 0.0f}, {2.0f, 1.0f, 2.0f}, {4.0f, 1.0f, 0.0f}});
        active.hazards.push_back({{0.0f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        active.hazards.push_back({{2.0f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        active.collectibles.push_back({{0.0f, 2.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        active.collectibles.push_back({{2.0f, 2.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        editor::DeleteSelected(working, {editor::EditorObjectKind::Checkpoint, 0});
        editor::ApplyLifecycleToStructuralMap(map, editor::EditorObjectKind::Checkpoint, true, 0);
        Expect(
            !editor::ShouldAcceptActiveWorldPick(
                {editor::EditorObjectKind::Checkpoint, 0}, map),
            "pending-deleted checkpoint pick is ignored");
        Expect(
            editor::ShouldAcceptActiveWorldPick(
                {editor::EditorObjectKind::Checkpoint, 1}, map),
            "surviving checkpoint remains pickable");
        Expect(
            editor::ShouldAcceptActiveWorldPick(
                {editor::EditorObjectKind::Hazard, 0}, map),
            "hazard pick still accepted while a checkpoint is pending-deleted");
        editor::DeleteSelected(working, {editor::EditorObjectKind::Hazard, 0});
        editor::ApplyLifecycleToStructuralMap(map, editor::EditorObjectKind::Hazard, true, 0);
        Expect(
            !editor::ShouldAcceptActiveWorldPick(
                {editor::EditorObjectKind::Hazard, 0}, map),
            "pending-deleted hazard pick is ignored");
        Expect(
            editor::ShouldAcceptActiveWorldPick(
                {editor::EditorObjectKind::Hazard, 1}, map),
            "surviving hazard remains pickable");
        editor::DeleteSelected(working, {editor::EditorObjectKind::Collectible, 1});
        editor::ApplyLifecycleToStructuralMap(map, editor::EditorObjectKind::Collectible, true, 1);
        Expect(
            !editor::ShouldAcceptActiveWorldPick(
                {editor::EditorObjectKind::Collectible, 1}, map),
            "pending-deleted collectible pick is ignored");
        Expect(
            editor::ShouldAcceptActiveWorldPick(
                {editor::EditorObjectKind::Collectible, 0}, map),
            "surviving collectible remains pickable");
        Expect(
            editor::ShouldAcceptActiveWorldPick(
                {editor::EditorObjectKind::Spawn, 0}, map),
            "spawn pick still accepted while repeatable categories have pending deletes");
    }

    {
        world::LevelDefinition variable{};
        variable.elevatedPlatforms.push_back({{0.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        const std::vector<editor::HierarchyEntry> onePlatform = editor::BuildHierarchyEntries(variable);
        std::size_t platforms = 0;
        std::size_t checkpoints = 0;
        std::size_t hazards = 0;
        std::size_t collectibles = 0;
        for (const editor::HierarchyEntry& entry : onePlatform)
        {
            platforms += entry.selection.kind == editor::EditorObjectKind::ElevatedPlatform ? 1 : 0;
            checkpoints += entry.selection.kind == editor::EditorObjectKind::Checkpoint ? 1 : 0;
            hazards += entry.selection.kind == editor::EditorObjectKind::Hazard ? 1 : 0;
            collectibles += entry.selection.kind == editor::EditorObjectKind::Collectible ? 1 : 0;
        }
        Expect(platforms == 1, "hierarchy 1 platform");
        Expect(checkpoints == 0 && hazards == 0 && collectibles == 0, "hierarchy zero repeatable extras");
        variable.elevatedPlatforms.push_back({{10.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        variable.checkpoints.push_back({{1.0f, 1.0f, 0.0f}, {2.0f, 1.0f, 2.0f}, {1.0f, 1.0f, 0.0f}});
        variable.hazards.push_back({{0.0f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        variable.hazards.push_back({{2.0f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        variable.collectibles.push_back({{0.0f, 2.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        const std::vector<editor::HierarchyEntry> several = editor::BuildHierarchyEntries(variable);
        platforms = checkpoints = hazards = collectibles = 0;
        for (const editor::HierarchyEntry& entry : several)
        {
            platforms += entry.selection.kind == editor::EditorObjectKind::ElevatedPlatform ? 1 : 0;
            checkpoints += entry.selection.kind == editor::EditorObjectKind::Checkpoint ? 1 : 0;
            hazards += entry.selection.kind == editor::EditorObjectKind::Hazard ? 1 : 0;
            collectibles += entry.selection.kind == editor::EditorObjectKind::Collectible ? 1 : 0;
        }
        Expect(platforms == 2, "hierarchy several platforms");
        Expect(checkpoints == 1, "hierarchy several checkpoints");
        Expect(hazards == 2, "hierarchy several hazards");
        Expect(collectibles == 1, "hierarchy several collectibles");
    }

    {
        world::LevelDefinition active = MakeStubLevel();
        gameplay::CollectibleRunState run =
            gameplay::MakeClearedCollectibleRunState(active.collectibles.size());
        run.collected[0] = 1;
        const int collectedBefore = gameplay::CollectedCount(run);
        const editor::EditorPickingSet set =
            editor::BuildPickingSet(active, editor::AuthoredPickingWorldState(active));
        const editor::PickingProxy* collectedItem =
            FindProxy(set, editor::EditorObjectKind::Collectible, 0);
        Expect(collectedItem != nullptr, "collected authored collectible still has a pick proxy");
        Expect(
            collectedItem != nullptr && Vec3Near(collectedItem->center, active.collectibles[0].center),
            "collected pick proxy uses active authored center");
        Expect(
            editor::ShouldAcceptActiveWorldPick(
                {editor::EditorObjectKind::Collectible, 0}, {}),
            "editor visible collected collectible remains pickable");
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        editor::ApplyLifecycleToStructuralMap(map, editor::EditorObjectKind::Collectible, true, 0);
        Expect(
            !editor::ShouldAcceptActiveWorldPick(
                {editor::EditorObjectKind::Collectible, 0}, map),
            "pending-deleted collected collectible pick is ignored");
        Expect(
            editor::ShouldAcceptActiveWorldPick(
                {editor::EditorObjectKind::Collectible, 1}, map),
            "surviving collected-category collectible remains pickable");
        Expect(run.collected[0] == 1 && gameplay::CollectedCount(run) == collectedBefore,
            "editor picking does not mutate CollectibleRunState");
        const std::vector<editor::HierarchyEntry> collectedHierarchy =
            editor::BuildHierarchyEntries(active);
        std::size_t hierarchyCollectibles = 0;
        for (const editor::HierarchyEntry& entry : collectedHierarchy)
        {
            hierarchyCollectibles +=
                entry.selection.kind == editor::EditorObjectKind::Collectible ? 1 : 0;
        }
        Expect(
            hierarchyCollectibles == active.collectibles.size(),
            "Hierarchy lists authored collectibles regardless of collected flags");
    }

    {
        Expect(
            editor::ShouldAttemptEditorViewportPick(true, false, false, false, false),
            "viewport pick allowed without capture");
        Expect(
            !editor::ShouldAttemptEditorViewportPick(true, true, false, false, false),
            "ImGui mouse capture suppresses pending and active pick");
        Expect(
            !editor::ShouldAttemptEditorViewportPick(true, false, true, false, false),
            "RMB look suppresses viewport pick");
        Expect(
            !editor::ShouldAttemptEditorViewportPick(true, false, false, true, false),
            "orientation widget consumes pointer before world pick");
        Expect(
            !editor::ShouldAttemptEditorViewportPick(true, false, false, false, true),
            "gizmo drag consumes pointer before pending/active world pick");
        Expect(
            !editor::ShouldAttemptEditorViewportPick(false, false, false, false, false),
            "no pick without select press");
    }

    {
        world::LevelDefinition active{};
        active.collectibles.push_back({{5.0f, 2.5f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        Expect(editor::AddCollectible(working, {20.0f, 4.0f, 0.0f}).succeeded, "pending Add Collectible");
        editor::ApplyLifecycleToStructuralMap(map, editor::EditorObjectKind::Collectible, false, 0);
        const editor::EditorSelection other{editor::EditorObjectKind::Collectible, 0};
        const std::vector<editor::PendingPickProxy> pending =
            editor::BuildPendingPickProxies(editor::CollectPendingAuthoringVisuals(
                active, working, map, other));
        Expect(pending.size() == 1, "one pending Add pick proxy");
        Expect(
            pending[0].selection.kind == editor::EditorObjectKind::Collectible
                && pending[0].selection.index == 1,
            "pending Add proxy uses working index");
        Expect(
            Vec3Near(pending[0].size, working.collectibles[1].size),
            "Collectible pending pick uses authored collection bounds");
        const editor::Ray3 atGhost{{20.0f, 4.0f, 8.0f}, {0.0f, 0.0f, -1.0f}};
        editor::EditorSelection resolved{};
        Expect(
            editor::TryResolveEditorViewportPick(
                atGhost,
                editor::BuildPickingSet(active, editor::AuthoredPickingWorldState(active)),
                pending,
                map,
                resolved),
            "pending Add Collectible is viewport-pickable");
        Expect(
            resolved.kind == editor::EditorObjectKind::Collectible && resolved.index == 1,
            "pending Add pick selects working Collectible");
        const std::vector<editor::PendingAuthoringVisual> selectedVisuals =
            editor::CollectPendingAuthoringVisuals(active, working, map, resolved);
        const editor::PendingAuthoringVisual* selected =
            editor::FindPendingAuthoringVisual(
                selectedVisuals, editor::EditorObjectKind::Collectible, 1);
        Expect(selected != nullptr && selected->selected, "picked pending Add restores selected emphasis");
    }

    {
        world::LevelDefinition active{};
        active.elevatedPlatforms.push_back({{0.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        active.collectibles.push_back({{5.0f, 2.5f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        Expect(
            editor::DuplicateSelected(working, {editor::EditorObjectKind::ElevatedPlatform, 0})
                .succeeded,
            "pending Duplicate Platform");
        editor::ApplyLifecycleToStructuralMap(map, editor::EditorObjectKind::ElevatedPlatform, false, 0);
        Expect(
            editor::DuplicateSelected(working, {editor::EditorObjectKind::Collectible, 0}).succeeded,
            "pending Duplicate Collectible");
        editor::ApplyLifecycleToStructuralMap(map, editor::EditorObjectKind::Collectible, false, 0);
        const std::vector<editor::PendingPickProxy> pending =
            editor::BuildPendingPickProxies(editor::CollectPendingAuthoringVisuals(
                active, working, map, {}));
        editor::EditorSelection resolved{};
        Expect(
            editor::TryResolveEditorViewportPick(
                editor::Ray3{{1.0f, 0.75f, 8.0f}, {0.0f, 0.0f, -1.0f}},
                editor::BuildPickingSet(active, editor::AuthoredPickingWorldState(active)),
                pending,
                map,
                resolved),
            "pending Duplicate Platform pick");
        Expect(
            resolved.kind == editor::EditorObjectKind::ElevatedPlatform && resolved.index == 1,
            "duplicate Platform working index selected");
        Expect(
            editor::TryResolveEditorViewportPick(
                editor::Ray3{{6.0f, 2.5f, 8.0f}, {0.0f, 0.0f, -1.0f}},
                editor::BuildPickingSet(active, editor::AuthoredPickingWorldState(active)),
                pending,
                map,
                resolved),
            "pending Duplicate Collectible pick");
        Expect(
            resolved.kind == editor::EditorObjectKind::Collectible && resolved.index == 1,
            "duplicate Collectible working index selected");
    }

    {
        world::LevelDefinition active{};
        active.hazards.push_back({{11.5f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        working.hazards[0].center = {30.0f, 0.5f, 0.0f};
        const std::vector<editor::PendingPickProxy> pending =
            editor::BuildPendingPickProxies(editor::CollectPendingAuthoringVisuals(
                active, working, map, {}));
        const editor::EditorPickingSet activeSet =
            editor::BuildPickingSet(active, editor::AuthoredPickingWorldState(active));
        editor::EditorSelection resolved{};
        Expect(
            editor::TryResolveEditorViewportPick(
                editor::Ray3{{30.0f, 0.5f, 8.0f}, {0.0f, 0.0f, -1.0f}},
                activeSet,
                pending,
                map,
                resolved),
            "pending Modify Hazard pick at working location");
        Expect(
            resolved.kind == editor::EditorObjectKind::Hazard && resolved.index == 0,
            "Modify pending pick is working Hazard 0");
        Expect(
            editor::TryResolveEditorViewportPick(
                editor::Ray3{{11.5f, 0.5f, 8.0f}, {0.0f, 0.0f, -1.0f}},
                activeSet,
                pending,
                map,
                resolved),
            "spatially distinct old active Hazard remains pickable");
        Expect(
            resolved.kind == editor::EditorObjectKind::Hazard && resolved.index == 0,
            "old active location maps to the same working Hazard");
    }

    {
        world::LevelDefinition active{};
        active.collectibles.push_back({{0.0f, 2.0f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        active.collectibles.push_back({{4.0f, 2.0f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        Expect(editor::DeleteSelected(working, {editor::EditorObjectKind::Collectible, 0}).succeeded,
            "pending Delete collectible 0");
        editor::ApplyLifecycleToStructuralMap(map, editor::EditorObjectKind::Collectible, true, 0);
        const std::vector<editor::PendingPickProxy> pending =
            editor::BuildPendingPickProxies(editor::CollectPendingAuthoringVisuals(
                active, working, map, {}));
        Expect(pending.empty(), "pending-deleted object is not a pending pick proxy");
        editor::EditorSelection previous{editor::EditorObjectKind::Collectible, 0};
        editor::EditorSelection resolved = previous;
        Expect(
            !editor::TryResolveEditorViewportPick(
                editor::Ray3{{0.0f, 2.0f, 8.0f}, {0.0f, 0.0f, -1.0f}},
                editor::BuildPickingSet(active, editor::AuthoredPickingWorldState(active)),
                pending,
                map,
                resolved),
            "pending-deleted active geometry is ignored");
        Expect(
            resolved.kind == editor::EditorObjectKind::Collectible && resolved.index == 0,
            "ignored pending-delete pick does not rewrite out selection");
        Expect(
            editor::TryResolveEditorViewportPick(
                editor::Ray3{{4.0f, 2.0f, 8.0f}, {0.0f, 0.0f, -1.0f}},
                editor::BuildPickingSet(active, editor::AuthoredPickingWorldState(active)),
                pending,
                map,
                resolved),
            "surviving active Collectible remains pickable");
        Expect(
            resolved.kind == editor::EditorObjectKind::Collectible && resolved.index == 0,
            "survivor maps through StructuralIndexMap to shifted working index 0");
    }

    {
        world::LevelDefinition active{};
        active.hazards.push_back({{0.0f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        Expect(editor::AddCollectible(working, {10.0f, 2.0f, 0.0f}).succeeded, "multi pending A");
        editor::ApplyLifecycleToStructuralMap(map, editor::EditorObjectKind::Collectible, false, 0);
        Expect(editor::AddCollectible(working, {14.0f, 2.0f, 0.0f}).succeeded, "multi pending B");
        editor::ApplyLifecycleToStructuralMap(map, editor::EditorObjectKind::Collectible, false, 0);
        working.hazards[0].center.x = 18.0f;
        const std::vector<editor::PendingPickProxy> pending =
            editor::BuildPendingPickProxies(editor::CollectPendingAuthoringVisuals(
                active, working, map, {}));
        Expect(pending.size() == 3, "three pending pick proxies");
        const editor::EditorPickingSet activeSet =
            editor::BuildPickingSet(active, editor::AuthoredPickingWorldState(active));
        editor::EditorSelection resolved{};
        Expect(
            editor::TryResolveEditorViewportPick(
                editor::Ray3{{10.0f, 2.0f, 8.0f}, {0.0f, 0.0f, -1.0f}},
                activeSet,
                pending,
                map,
                resolved)
                && resolved.kind == editor::EditorObjectKind::Collectible && resolved.index == 0,
            "multi pending pick A");
        Expect(
            editor::TryResolveEditorViewportPick(
                editor::Ray3{{14.0f, 2.0f, 8.0f}, {0.0f, 0.0f, -1.0f}},
                activeSet,
                pending,
                map,
                resolved)
                && resolved.kind == editor::EditorObjectKind::Collectible && resolved.index == 1,
            "multi pending pick B");
        Expect(
            editor::TryResolveEditorViewportPick(
                editor::Ray3{{18.0f, 0.5f, 8.0f}, {0.0f, 0.0f, -1.0f}},
                activeSet,
                pending,
                map,
                resolved)
                && resolved.kind == editor::EditorObjectKind::Hazard && resolved.index == 0,
            "multi pending pick C");
        editor::PendingPickProxy nearer{};
        nearer.selection = {editor::EditorObjectKind::Collectible, 0};
        nearer.center = {10.0f, 2.0f, 2.0f};
        nearer.size = {1.0f, 1.2f, 1.0f};
        editor::PendingPickProxy farther = nearer;
        farther.selection = {editor::EditorObjectKind::Collectible, 1};
        farther.center.z = -2.0f;
        const editor::EditorSelection nearest = editor::PickNearestPending(
            editor::Ray3{{10.0f, 2.0f, 8.0f}, {0.0f, 0.0f, -1.0f}},
            {farther, nearer});
        Expect(
            nearest.kind == editor::EditorObjectKind::Collectible && nearest.index == 0,
            "overlapping pending picks use nearest ray hit, not list order");
    }

    {
        world::LevelDefinition active{};
        active.ground = {{0.0f, -0.25f, 0.0f}, {8.0f, 0.5f, 8.0f}};
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        Expect(editor::AddCollectible(working, {0.0f, 0.0f, 0.0f}).succeeded, "overlap pending Add");
        editor::ApplyLifecycleToStructuralMap(map, editor::EditorObjectKind::Collectible, false, 0);
        working.collectibles.back().size = {4.0f, 2.0f, 4.0f};
        const std::vector<editor::PendingPickProxy> pending =
            editor::BuildPendingPickProxies(editor::CollectPendingAuthoringVisuals(
                active, working, map, {}));
        editor::EditorSelection resolved{};
        Expect(
            editor::TryResolveEditorViewportPick(
                editor::Ray3{{0.0f, 0.0f, 8.0f}, {0.0f, 0.0f, -1.0f}},
                editor::BuildPickingSet(active, editor::AuthoredPickingWorldState(active)),
                pending,
                map,
                resolved),
            "pending vs active overlap still resolves");
        Expect(
            resolved.kind == editor::EditorObjectKind::Collectible && resolved.index == 0,
            "pending workingCopy ghost wins over overlapping active Ground");
    }

    {
        world::LevelDefinition active{};
        active.checkpoints.push_back(
            {{16.5f, 1.8f, 0.0f}, {2.4f, 1.6f, 2.0f}, {18.0f, 0.8f, 1.0f}});
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        Expect(editor::AddCheckpoint(working, {0.0f, 4.0f, 0.0f}).succeeded, "pending Add Checkpoint");
        editor::ApplyLifecycleToStructuralMap(map, editor::EditorObjectKind::Checkpoint, false, 0);
        Expect(editor::AddHazard(working, {8.0f, 0.5f, 0.0f}).succeeded, "pending Add Hazard");
        editor::ApplyLifecycleToStructuralMap(map, editor::EditorObjectKind::Hazard, false, 0);
        Expect(editor::AddPlatform(working, {-8.0f, 1.0f, 0.0f}).succeeded, "pending Add Platform");
        editor::ApplyLifecycleToStructuralMap(map, editor::EditorObjectKind::ElevatedPlatform, false, 0);
        const std::vector<editor::PendingPickProxy> pending =
            editor::BuildPendingPickProxies(editor::CollectPendingAuthoringVisuals(
                active, working, map, {}));
        const editor::EditorPickingSet activeSet =
            editor::BuildPickingSet(active, editor::AuthoredPickingWorldState(active));
        editor::EditorSelection resolved{};
        Expect(
            editor::TryResolveEditorViewportPick(
                editor::Ray3{{0.0f, 4.0f, 8.0f}, {0.0f, 0.0f, -1.0f}},
                activeSet,
                pending,
                map,
                resolved)
                && resolved.kind == editor::EditorObjectKind::Checkpoint,
            "pending Checkpoint trigger AABB is pickable");
        Expect(
            editor::TryResolveEditorViewportPick(
                editor::Ray3{{8.0f, 0.5f, 8.0f}, {0.0f, 0.0f, -1.0f}},
                activeSet,
                pending,
                map,
                resolved)
                && resolved.kind == editor::EditorObjectKind::Hazard,
            "pending Hazard AABB is pickable");
        Expect(
            editor::TryResolveEditorViewportPick(
                editor::Ray3{{-8.0f, 1.0f, 8.0f}, {0.0f, 0.0f, -1.0f}},
                activeSet,
                pending,
                map,
                resolved)
                && resolved.kind == editor::EditorObjectKind::ElevatedPlatform,
            "pending Platform AABB is pickable");
        working = active;
        editor::ResetStructuralIndexMap(map, active);
        Expect(
            editor::BuildPendingPickProxies(editor::CollectPendingAuthoringVisuals(
                                               active, working, map, {}))
                .empty(),
            "Revert/Apply identity clears pending pick proxies");
    }

    {
        world::LevelDefinition active = MakeStubLevel();
        world::LevelDefinition working = active;
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        Expect(
            editor::AddDynamicBoxAt(working, {3.0f, 1.0f, 0.0f}).succeeded,
            "pending add Dynamic Box");
        editor::ApplyLifecycleToStructuralMap(map, editor::EditorObjectKind::DynamicBox, false, 0);
        const std::vector<editor::PendingPickProxy> pending =
            editor::BuildPendingPickProxies(editor::CollectPendingAuthoringVisuals(
                active, working, map, {editor::EditorObjectKind::DynamicBox, 0}));
        Expect(!pending.empty(), "pending Dynamic Box is pickable");
        const editor::Ray3 atPending{
            {working.dynamicBoxes[0].center.x, working.dynamicBoxes[0].center.y, 8.0f},
            {0.0f, 0.0f, -1.0f}};
        Expect(
            editor::PickNearestPending(atPending, pending).kind
                == editor::EditorObjectKind::DynamicBox,
            "pending Dynamic Box pick hits authored ghost");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d editor picking/selection test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Editor picking tests passed.\n");
    return 0;
}
