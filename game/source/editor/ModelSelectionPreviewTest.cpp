// Milestone 58.2: model-backed selection ghost, live workingCopy transform,
// oriented picking bounds, and gameplay targeting isolation. No window.

#include "editor/AuthoredObjectLifecycle.h"
#include "editor/EditorGizmo.h"
#include "editor/EditorMath.h"
#include "editor/EditorPicking.h"
#include "editor/EditorSelection.h"
#include "editor/EditorWorkspace.h"
#include "editor/SelectedModelHighlight.h"
#include "editor/StaticPropTransform.h"
#include "gameplay/ItemPickupRuntime.h"
#include "world/ItemPickup.h"
#include "world/LevelDefinition.h"
#include "world/StaticProp.h"

#include <cmath>
#include <cstdio>
#include <cstring>
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

bool NearlyEqual(float a, float b, float tolerance = 1.0e-4f)
{
    return std::fabs(a - b) <= tolerance;
}

bool Vec3Near(core::Vec3 a, core::Vec3 b, float tolerance = 1.0e-4f)
{
    return NearlyEqual(a.x, b.x, tolerance) && NearlyEqual(a.y, b.y, tolerance)
        && NearlyEqual(a.z, b.z, tolerance);
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
    level.camera = {{2.0f, 3.5f, 12.0f}, 40.0f};
    return level;
}

world::StaticPropSpec MakeProp(
    const char* identity,
    core::Vec3 position,
    core::Vec3 rotation,
    core::Vec3 scale)
{
    world::StaticPropSpec prop{};
    prop.modelIdentity = identity;
    prop.position = position;
    prop.rotationDegrees = rotation;
    prop.scale = scale;
    return prop;
}

world::ItemPickupSpec MakePickup(
    core::Vec3 position,
    const char* itemId,
    const char* modelIdentity = "")
{
    world::ItemPickupSpec pickup{};
    pickup.position = position;
    pickup.itemId = itemId;
    pickup.quantity = 1;
    pickup.modelIdentity = modelIdentity;
    return pickup;
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

constexpr core::Vec3 kBarrelMin{-1.7175f, -0.6327f, -1.7372f};
constexpr core::Vec3 kBarrelMax{1.7090f, 2.3821f, 1.7421f};
}

int main()
{
    // Selected Static Prop gets a model ghost from workingCopy.
    {
        world::LevelDefinition working = MakeStubLevel();
        working.staticProps.push_back(
            MakeProp("models/test_static.glb", {4.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}));
        working.staticProps.push_back(
            MakeProp("models/test_authored.glb", {8.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}));
        const editor::SelectedModelGhostRequest selected =
            editor::MakeSelectedModelGhostRequest(
                {editor::EditorObjectKind::StaticProp, 0}, working);
        Expect(selected.visible, "selected Static Prop receives model ghost");
        Expect(selected.demotePrimaryBox, "model ghost demotes the generic box");
        Expect(selected.drawOrientedBounds, "oriented bounds remain as a diagnostic");
        Expect(selected.visual.modelIdentity == working.staticProps[0].modelIdentity,
            "ghost reuses the authored identity");
        Expect(Vec3Near(selected.visual.position, working.staticProps[0].position),
            "ghost uses workingCopy position");
        Expect(Vec3Near(selected.visual.rotationDegrees, working.staticProps[0].rotationDegrees),
            "ghost uses workingCopy rotation");
        Expect(Vec3Near(selected.visual.scale, working.staticProps[0].scale),
            "ghost uses workingCopy scale");

        const editor::SelectedModelGhostRequest other =
            editor::MakeSelectedModelGhostRequest(
                {editor::EditorObjectKind::StaticProp, 1}, working);
        Expect(other.visible, "second selected Static Prop can receive a ghost");
        Expect(other.visual.modelIdentity == working.staticProps[1].modelIdentity,
            "ghost follows the currently selected identity");

        const editor::SelectedModelGhostRequest none =
            editor::MakeSelectedModelGhostRequest(editor::ClearSelection(), working);
        Expect(!none.visible, "unselected has no model ghost");
        Expect(
            !editor::MakeSelectedModelGhostRequest(
                 {editor::EditorObjectKind::StaticProp, 1}, working)
                 .visual.modelIdentity.empty(),
            "unselected instance is not highlighted by a leftover request");
        Expect(
            editor::MakeSelectedModelGhostRequest(
                {editor::EditorObjectKind::Ground, 0}, working)
                .visible
                == false,
            "primitive Ground does not receive a model ghost");
    }

    // Selected model-backed Item Pickup uses M58 visual transform.
    {
        world::LevelDefinition working = MakeStubLevel();
        world::ItemPickupSpec pickup = MakePickup({2.0f, 1.0f, 0.0f}, "key", "models/test_static.glb");
        pickup.visualOffset = {0.5f, 0.25f, -0.1f};
        pickup.visualRotationDegrees = {10.0f, 20.0f, 30.0f};
        pickup.visualScale = {2.0f, 0.5f, 1.5f};
        working.itemPickups.push_back(pickup);
        working.itemPickups.push_back(MakePickup({9.0f, 1.0f, 0.0f}, "coin", "models/test_authored.glb"));
        const editor::SelectedModelGhostRequest selected =
            editor::MakeSelectedModelGhostRequest(
                {editor::EditorObjectKind::ItemPickup, 0}, working);
        Expect(selected.visible, "selected model-backed Item Pickup receives ghost");
        Expect(
            Vec3Near(selected.visual.position, world::ItemPickupVisualPosition(pickup)),
            "ghost origin is position + visualOffset");
        Expect(Vec3Near(selected.visual.rotationDegrees, pickup.visualRotationDegrees),
            "ghost uses visualRotationDegrees");
        Expect(Vec3Near(selected.visual.scale, pickup.visualScale),
            "ghost uses visualScale");
        Expect(
            editor::MakeSelectedModelGhostRequest(
                {editor::EditorObjectKind::ItemPickup, 1}, working)
                    .visual.modelIdentity
                == working.itemPickups[1].modelIdentity,
            "selecting pickup 1 uses that instance identity");
        Expect(
            editor::MakeSelectedModelGhostRequest(editor::ClearSelection(), working).visible
                == false,
            "unselected Item Pickup has no ghost");
        working.itemPickups[0].showInteractionBounds = false;
        Expect(
            editor::MakeSelectedModelGhostRequest(
                {editor::EditorObjectKind::ItemPickup, 0}, working)
                .visible,
            "showInteractionBounds does not disable M58.2 editor ghost");
        working.itemPickups[0].targetHighlightIntensity = 0.0f;
        Expect(
            editor::MakeSelectedModelGhostRequest(
                {editor::EditorObjectKind::ItemPickup, 0}, working)
                .visible,
            "targetHighlightIntensity does not disable M58.2 editor ghost");
        working.itemPickups[0].idleAnimationEnabled = true;
        working.itemPickups[0].idleBobAmplitude = 1.5f;
        working.itemPickups[0].idleSpinSpeedDegrees = 180.0f;
        const editor::SelectedModelGhostRequest idleGhost =
            editor::MakeSelectedModelGhostRequest(
                {editor::EditorObjectKind::ItemPickup, 0}, working);
        Expect(idleGhost.visible, "idle animation does not disable M58.2 editor ghost");
        Expect(
            Vec3Near(idleGhost.visual.position, world::ItemPickupVisualPosition(working.itemPickups[0])),
            "48. editor ghost stays on authored visual origin");
        Expect(
            Vec3Near(idleGhost.visual.rotationDegrees, working.itemPickups[0].visualRotationDegrees),
            "48. editor ghost stays on authored visual rotation");
        Expect(
            !Vec3Near(
                idleGhost.visual.position,
                world::ItemPickupPresentedVisualPosition(working.itemPickups[0], 0.25)),
            "35. editor ghost does not chase runtime bob");
        Expect(
            !Vec3Near(
                idleGhost.visual.rotationDegrees,
                world::ItemPickupPresentedVisualRotationDegrees(working.itemPickups[0], 1.0)),
            "35. editor ghost does not chase runtime spin");
    }

    // Fallback Item Pickup (empty modelIdentity) stays on the cube path.
    {
        world::LevelDefinition working = MakeStubLevel();
        working.itemPickups.push_back(MakePickup({1.0f, 1.0f, 0.0f}, "key"));
        const editor::SelectedModelGhostRequest ghost =
            editor::MakeSelectedModelGhostRequest(
                {editor::EditorObjectKind::ItemPickup, 0}, working);
        Expect(!ghost.visible, "fallback Item Pickup has no model ghost");
        Expect(!ghost.demotePrimaryBox, "fallback keeps the generic selection box");
        const editor::EditorPickingSet set =
            editor::BuildPickingSet(working, editor::AuthoredPickingWorldState(working));
        const editor::PickingProxy* proxy =
            FindProxy(set, editor::EditorObjectKind::ItemPickup, 0);
        Expect(proxy != nullptr && !proxy->usesStaticPropTransform,
            "fallback Item Pickup keeps cube picking");
        Expect(
            editor::PickNearest({{1.0f, 1.0f, 8.0f}, {0.0f, 0.0f, -1.0f}}, set).kind
                == editor::EditorObjectKind::ItemPickup,
            "fallback Item Pickup remains pickable");
    }

    // Missing staged model still produces a selectable ghost (placeholder path).
    {
        world::LevelDefinition working = MakeStubLevel();
        working.staticProps.push_back(
            MakeProp("models/missing_fixture.glb", {0.0f, 1.0f, 0.0f}, {}, {1.0f, 1.0f, 1.0f}));
        editor::SelectedModelGhostRequest ghost =
            editor::MakeSelectedModelGhostRequest(
                {editor::EditorObjectKind::StaticProp, 0}, working);
        Expect(ghost.visible, "missing staged model remains selectable via ghost request");
        Expect(ghost.visual.modelIdentity == "models/missing_fixture.glb",
            "missing model does not fabricate a replacement identity");
        editor::ApplyLoadedLocalBoundsToGhost(ghost, false, {}, {});
        Expect(Vec3Near(ghost.localMin, editor::kStaticPropDefaultLocalMin),
            "missing model keeps the local placeholder bounds");
    }

    // Live Translate/Scale/Rotate update the ghost from workingCopy only.
    {
        world::LevelDefinition working = MakeStubLevel();
        working.staticProps.push_back(
            MakeProp("models/test_static.glb", {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}));
        const editor::EditorSelection prop0{editor::EditorObjectKind::StaticProp, 0};
        working.staticProps[0].position.x = 3.0f;
        Expect(
            NearlyEqual(
                editor::MakeSelectedModelGhostRequest(prop0, working).visual.position.x, 3.0f),
            "Static Prop Translate updates highlight live");
        working.staticProps[0].scale = {2.0f, 0.5f, 3.0f};
        Expect(
            Vec3Near(
                editor::MakeSelectedModelGhostRequest(prop0, working).visual.scale,
                {2.0f, 0.5f, 3.0f}),
            "Static Prop Scale updates highlight live");
        working.staticProps[0].rotationDegrees = {15.0f, 45.0f, 90.0f};
        Expect(
            Vec3Near(
                editor::MakeSelectedModelGhostRequest(prop0, working).visual.rotationDegrees,
                {15.0f, 45.0f, 90.0f}),
            "Static Prop Rotate updates highlight live");

        world::ItemPickupSpec pickup =
            MakePickup({1.0f, 2.0f, 0.0f}, "key", "models/test_static.glb");
        pickup.visualOffset = {0.2f, 0.4f, 0.1f};
        working.itemPickups.push_back(pickup);
        const editor::EditorSelection pickup0{editor::EditorObjectKind::ItemPickup, 0};
        working.itemPickups[0].position.x = 5.0f;
        Expect(
            NearlyEqual(
                editor::MakeSelectedModelGhostRequest(pickup0, working).visual.position.x, 5.2f),
            "Item Pickup Translate updates ghost because visualPosition changes");
        working.itemPickups[0].visualScale = {3.0f, 1.0f, 0.25f};
        Expect(
            Vec3Near(
                editor::MakeSelectedModelGhostRequest(pickup0, working).visual.scale,
                {3.0f, 1.0f, 0.25f}),
            "Item Pickup Scale updates highlight live");
        working.itemPickups[0].visualRotationDegrees = {0.0f, 90.0f, 0.0f};
        Expect(
            NearlyEqual(
                editor::MakeSelectedModelGhostRequest(pickup0, working).visual.rotationDegrees.y,
                90.0f),
            "Item Pickup Rotate updates highlight live");
        working.itemPickups[0].visualOffset.y = 1.5f;
        Expect(
            NearlyEqual(
                editor::MakeSelectedModelGhostRequest(pickup0, working).visual.position.y, 3.5f),
            "Item Pickup visualOffset affects ghost origin");
        Expect(
            NearlyEqual(working.itemPickups[0].position.y, 2.0f),
            "no second preview transform authority exists");
    }

    // Transformed picking uses oriented local bounds, not a disconnected AABB.
    {
        world::LevelDefinition level = MakeStubLevel();
        world::StaticPropSpec prop =
            MakeProp("models/test_static.glb", {0.0f, 0.0f, 0.0f}, {0.0f, 90.0f, 0.0f}, {2.0f, 1.0f, 1.0f});
        level.staticProps.push_back(prop);
        editor::EditorPickingSet set =
            editor::BuildPickingSet(level, editor::AuthoredPickingWorldState(level));
        bool applied = false;
        for (editor::PickingProxy& proxy : set.proxies)
        {
            if (proxy.selection.kind == editor::EditorObjectKind::StaticProp)
            {
                Expect(proxy.usesStaticPropTransform, "Static Prop uses transformed picking");
                editor::ApplyLoadedLocalBounds(proxy, kBarrelMin, kBarrelMax);
                applied = true;
            }
        }
        Expect(applied, "applied loaded model-local bounds to Static Prop proxy");
        const editor::Ray3 atRotatedExtent{{0.0f, 2.0f, 8.0f}, {0.0f, 0.0f, -1.0f}};
        Expect(
            editor::IntersectRayStaticProp(atRotatedExtent, prop, kBarrelMin, kBarrelMax)
                .hit,
            "transformed Static Prop remains pickable after Rotate using model bounds");
        world::StaticPropSpec scaled = prop;
        scaled.rotationDegrees = {};
        scaled.scale = {0.25f, 0.25f, 0.25f};
        Expect(
            editor::IntersectRayStaticProp(
                {{0.0f, 0.0f, 8.0f}, {0.0f, 0.0f, -1.0f}},
                scaled,
                kBarrelMin,
                kBarrelMax)
                .hit,
            "transformed Static Prop remains pickable after Scale");
        Expect(
            !editor::IntersectRayStaticProp(
                 {{4.0f, 0.0f, 8.0f}, {0.0f, 0.0f, -1.0f}},
                 scaled,
                 kBarrelMin,
                 kBarrelMax)
                 .hit,
            "scaled Static Prop does not keep the unit-cube pick volume");
    }

    {
        world::ItemPickupSpec pickup =
            MakePickup({0.0f, 0.0f, 0.0f}, "key", "models/test_static.glb");
        pickup.visualRotationDegrees = {0.0f, 90.0f, 0.0f};
        pickup.visualScale = {2.0f, 1.0f, 1.0f};
        pickup.visualOffset = {0.0f, 1.0f, 0.0f};
        Expect(
            editor::IntersectRayStaticProp(
                {{0.0f, 1.0f, 8.0f}, {0.0f, 0.0f, -1.0f}},
                world::ItemPickupVisualProp(pickup),
                kBarrelMin,
                kBarrelMax)
                .hit,
            "transformed Item Pickup remains pickable after visual Rotate");
        pickup.visualScale = {0.2f, 0.2f, 0.2f};
        Expect(
            editor::IntersectRayStaticProp(
                {{0.0f, 1.0f, 8.0f}, {0.0f, 0.0f, -1.0f}},
                world::ItemPickupVisualProp(pickup),
                editor::kStaticPropDefaultLocalMin,
                editor::kStaticPropDefaultLocalMax)
                .hit,
            "transformed Item Pickup remains pickable after visual Scale");
    }

    // Oriented diagnostic corners follow the authored transform.
    {
        const world::StaticPropSpec prop =
            MakeProp("models/test_static.glb", {3.0f, 1.0f, -2.0f}, {0.0f, 90.0f, 0.0f}, {2.0f, 1.0f, 1.0f});
        core::Vec3 corners[8]{};
        editor::StaticPropWorldCorners(
            prop, editor::kStaticPropDefaultLocalMin, editor::kStaticPropDefaultLocalMax, corners);
        Expect(Vec3Near(corners[0], editor::StaticPropWorldFromLocal(prop, {-0.5f, -0.5f, -0.5f})),
            "retained secondary bounds follow transformed corners");
        Expect(!Vec3Near(corners[7], {3.5f, 1.5f, -1.5f}),
            "rotated corners are not a disconnected world-axis unit box");
        const core::Vec3 worldX = editor::Sub(corners[1], corners[0]);
        Expect(!NearlyEqual(worldX.z, 0.0f) || NearlyEqual(std::fabs(worldX.z), 2.0f),
            "Yaw 90 maps local X into world Z");
    }

    // Pending picking uses the oriented Static Prop transform.
    {
        world::LevelDefinition active = MakeStubLevel();
        world::LevelDefinition working = active;
        working.staticProps.push_back(
            MakeProp("models/test_static.glb", {10.0f, 1.0f, 0.0f}, {0.0f, 90.0f, 0.0f}, {1.0f, 1.0f, 1.0f}));
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        const std::vector<editor::PendingAuthoringVisual> visuals =
            editor::CollectPendingAuthoringVisuals(
                active, working, map, {editor::EditorObjectKind::StaticProp, 0});
        Expect(!visuals.empty() && visuals[0].usesStaticPropTransform,
            "pending Static Prop exposes the authored transform for picking");
        const std::vector<editor::PendingPickProxy> pending =
            editor::BuildPendingPickProxies(visuals);
        Expect(
            editor::PickNearestPending(
                {{10.0f, 1.0f, 8.0f}, {0.0f, 0.0f, -1.0f}}, pending)
                    .kind
                == editor::EditorObjectKind::StaticProp,
            "pending transformed Static Prop remains pickable");
    }

    // Gameplay targeting still uses logical position.
    {
        const core::Vec3 spawn{0.0f, 0.8f, 0.0f};
        world::ItemPickupSpec pickup = MakePickup({spawn.x + 1.5f, spawn.y, spawn.z}, "key", "models/test_static.glb");
        pickup.visualOffset = {8.0f, 4.0f, 3.0f};
        pickup.visualRotationDegrees = {90.0f, 45.0f, 10.0f};
        pickup.visualScale = {6.0f, 6.0f, 6.0f};
        const std::vector<world::ItemPickupSpec> pickups{pickup};
        gameplay::ItemPickupRunState run = gameplay::MakeClearedItemPickupRunState(1);
        const std::vector<std::uint8_t> los{0};
        Expect(
            gameplay::FindItemPickupTargetIndex(spawn, 1.0f, pickups, run.collected, los) == 0,
            "Item Pickup gameplay targeting still uses logical position");
        Expect(gameplay::kItemPickupMaxDistance == 2.5f, "Item Pickup range unchanged");
        world::ItemPickupSpec behind = pickup;
        behind.position = {spawn.x - 1.5f, spawn.y, spawn.z};
        behind.visualOffset = {3.0f, 0.0f, 0.0f};
        Expect(
            gameplay::FindItemPickupTargetIndex(
                spawn, 1.0f, std::vector<world::ItemPickupSpec>{behind}, run.collected, los)
                == gameplay::kNoItemPickupIndex,
            "facing unchanged: visualOffset does not retarget");
        const std::vector<std::uint8_t> blocked{1};
        Expect(
            gameplay::FindItemPickupTargetIndex(spawn, 1.0f, pickups, run.collected, blocked)
                == gameplay::kNoItemPickupIndex,
            "LOS unchanged");
        gameplay::Inventory inventory;
        Expect(gameplay::TryCollectItemPickup(inventory, run, pickups, 0), "collection unchanged");
        Expect(inventory.GetQuantity("key") == 1, "Inventory unchanged");
        Expect(pickups[0].position.x == spawn.x + 1.5f, "collection does not rewrite gameplay position");
        Expect(pickups[0].visualOffset.x == 8.0f, "collection does not rewrite visualOffset");
    }

    // No authored schema / Level Format fields were added.
    {
        world::StaticPropSpec prop{};
        Expect(prop.modelIdentity.empty(), "StaticPropSpec has no extra default identity");
        Expect(Vec3Near(prop.scale, world::kDefaultStaticPropScale), "Static Prop scale default unchanged");
        world::ItemPickupSpec pickup{};
        Expect(pickup.modelIdentity.empty(), "ItemPickupSpec empty modelIdentity unchanged");
        Expect(Vec3Near(pickup.visualOffset, world::kDefaultItemPickupVisualOffset),
            "no new Item Pickup authored visual field");
        Expect(Vec3Near(pickup.visualScale, world::kDefaultItemPickupVisualScale),
            "Item Pickup visualScale default unchanged");
        Expect(world::kItemPickupVisualKeyword == "visual", "Level Format visual marker unchanged");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d model-selection preview test(s) failed.\n", gFailures);
        return 1;
    }
    std::printf("Model selection preview tests passed.\n");
    return 0;
}
