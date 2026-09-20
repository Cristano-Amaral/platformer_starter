#include "editor/EditorGizmo.h"
#include "editor/AuthoredObjectLifecycle.h"
#include "editor/DirectionalLightAuthoring.h"
#include "editor/EditorLayout.h"
#include "editor/EditorMath.h"
#include "editor/EditorNudge.h"
#include "editor/EditorSelection.h"
#include "editor/EditorSelectionSet.h"
#include "editor/EditorGroupRotate.h"
#include "editor/EditorGroupTranslate.h"
#include "editor/EditorSnap.h"
#include "editor/EditorViewportGrid.h"
#include "editor/StaticPropTransform.h"
#include "world/ItemPickup.h"
#include "world/LevelDefinition.h"

#include <cstdint>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
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

bool NearlyEqual(float a, float b, float epsilon = 0.05f)
{
    return std::fabs(a - b) <= epsilon;
}

bool Vec3Near(core::Vec3 a, core::Vec3 b, float epsilon = 0.05f)
{
    return NearlyEqual(a.x, b.x, epsilon) && NearlyEqual(a.y, b.y, epsilon)
        && NearlyEqual(a.z, b.z, epsilon);
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
    level.camera = {{2.0f, 3.5f, 12.0f}, 40.0f};
    return level;
}

render::CameraView MakeView(core::Vec3 position, core::Vec3 target)
{
    render::CameraView view{};
    view.position = position;
    view.target = target;
    view.up = {0.0f, 1.0f, 0.0f};
    view.fieldOfViewY = 60.0f;
    return view;
}

editor::Ray3 RayThrough(const render::CameraView& view, core::Vec3 worldPoint)
{
    editor::Ray3 ray{};
    ray.origin = view.position;
    ray.direction = editor::NormalizeOr(
        {worldPoint.x - view.position.x,
         worldPoint.y - view.position.y,
         worldPoint.z - view.position.z},
        {0.0f, 0.0f, -1.0f});
    return ray;
}
}

int main()
{
    using editor::EditorAxis;
    using editor::EditorObjectKind;
    using editor::EditorSelection;

    // ---- supported vs unsupported ----
    {
        Expect(editor::IsGizmoSelection({EditorObjectKind::Spawn, 0}), "spawn is gizmo");
        Expect(editor::IsGizmoSelection({EditorObjectKind::Ground, 0}), "ground is gizmo");
        Expect(
            editor::IsGizmoSelection({EditorObjectKind::ElevatedPlatform, 5}),
            "platform 5 is gizmo");
        Expect(
            editor::IsGizmoSelection({EditorObjectKind::ElevatedPlatform, 6}),
            "platform kind remains gizmo; index is resolved against the definition");
        Expect(
            editor::GetEditablePosition(MakeStubLevel(), {EditorObjectKind::ElevatedPlatform, 6})
                == nullptr,
            "out-of-range platform has no gizmo origin");
        Expect(!editor::IsGizmoSelection({EditorObjectKind::Camera, 0}), "camera has no gizmo");
            Expect(
            !editor::IsGizmoSelection({EditorObjectKind::Environment, 0}),
            "Environment has no Translate gizmo");
        Expect(
            editor::IsGizmoSelection({EditorObjectKind::DirectionalLight, 0}),
            "Directional Light Translate moves the editor visualization");
        Expect(
            editor::IsScaleSelection({EditorObjectKind::DirectionalLight, 0}),
            "Directional Light Scale changes visualization size");
        Expect(
            !editor::IsResizeSelection({EditorObjectKind::DirectionalLight, 0}),
            "Directional Light is not resizable");
        Expect(
            editor::IsRotateSelection({EditorObjectKind::DirectionalLight, 0}),
            "Directional Light uses the Rotate gizmo");
        Expect(!editor::IsGizmoSelection({EditorObjectKind::Slope, 0}), "slope has no gizmo");
        Expect(
            !editor::IsGizmoSelection({EditorObjectKind::MovingPlatform, 0}),
            "moving platform has no gizmo");
        Expect(editor::IsGizmoSelection({EditorObjectKind::Checkpoint, 0}), "checkpoint translate gizmo");
        Expect(editor::IsGizmoSelection({EditorObjectKind::Hazard, 0}), "hazard translate gizmo");
        Expect(editor::IsGizmoSelection({EditorObjectKind::Collectible, 0}), "collectible translate gizmo");
        Expect(editor::IsGizmoSelection({EditorObjectKind::Goal, 0}), "goal has translate gizmo");
        Expect(editor::IsGizmoSelection({EditorObjectKind::DynamicBox, 0}), "Dynamic Box has gizmo");
        Expect(editor::IsGizmoSelection({EditorObjectKind::PressurePlate, 0}), "Pressure Plate has gizmo");
        Expect(editor::IsGizmoSelection({EditorObjectKind::Door, 0}), "Door has gizmo");
        Expect(editor::IsGizmoSelection({EditorObjectKind::ItemPickup, 0}), "Item Pickup has Translate gizmo");
        Expect(editor::IsGizmoSelection({EditorObjectKind::StaticProp, 0}), "Static Prop has Translate gizmo");
        Expect(!editor::IsGizmoSelection({EditorObjectKind::None, 0}), "none is not gizmo");
    }

    // ---- working position lookup ----
    {
        world::LevelDefinition level = MakeStubLevel();
        EditorSelection platform0{EditorObjectKind::ElevatedPlatform, 0};
        core::Vec3* position = editor::GetEditablePosition(level, platform0);
        Expect(position != nullptr, "platform 0 has a mutable position");
        Expect(position != nullptr && NearlyEqual(position->x, 5.0f), "lookup reads working X");
        if (position != nullptr)
        {
            position->x = 7.0f;
        }
        Expect(NearlyEqual(level.elevatedPlatforms[0].center.x, 7.0f), "lookup mutates working copy");
        Expect(
            editor::GetEditablePosition(level, {EditorObjectKind::Camera, 0}) == nullptr,
            "camera has no world position pointer");
        Expect(
            editor::GetEditablePosition(level, {EditorObjectKind::Environment, 0}) == nullptr,
            "Environment has no world position");
        Expect(
            editor::GetEditablePosition(level, {EditorObjectKind::DirectionalLight, 0}) == nullptr,
            "Directional Light has no lighting position");
        core::Vec3 lightCenter{};
        core::Vec3 lightSize{};
        Expect(
            editor::GetGizmoPreviewBox(
                level, {EditorObjectKind::DirectionalLight, 0}, lightCenter, lightSize)
                && NearlyEqual(lightCenter.y, editor::kDirectionalLightAuthoringAnchor.y),
            "Directional Light rotate origin is the authoring anchor");
        const core::Vec3 startRay = level.environment.directionalRayDirection;
        const core::Vec3 rotated = editor::RotateAuthoredDirectionalRay(
            startRay, {0.0f, 1.0f, 0.0f}, 25.0f);
        Expect(NearlyEqual(std::sqrt(rotated.x * rotated.x + rotated.y * rotated.y + rotated.z * rotated.z), 1.0f),
            "gizmo rotation stays normalized");
        Expect(
            !NearlyEqual(rotated.x, startRay.x) || !NearlyEqual(rotated.z, startRay.z),
            "gizmo rotation changes direction");
        Expect(editor::GetEditableRotation(level, {EditorObjectKind::DirectionalLight, 0})
                == nullptr,
            "Directional Light does not store a gameplay Euler transform");
        Expect(
            editor::GetEditablePosition(level, {EditorObjectKind::Slope, 0}) == nullptr,
            "slope has no mutable gizmo position");
        level.checkpoints.push_back({{16.5f, 1.8f, 0.0f}, {2.4f, 1.6f, 2.0f}, {16.5f, 1.8f, 0.0f}});
        level.hazards.push_back({{11.5f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
        level.collectibles.push_back({{5.0f, 2.5f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        Expect(
            editor::GetEditablePosition(level, {EditorObjectKind::Checkpoint, 0}) != nullptr,
            "checkpoint has translate origin");
        Expect(
            editor::GetEditablePosition(level, {EditorObjectKind::Hazard, 0}) != nullptr,
            "hazard has translate origin");
        Expect(
            editor::GetEditablePosition(level, {EditorObjectKind::Collectible, 0}) != nullptr,
            "collectible has translate origin");
        level.levelGoals.push_back({{-21.0f, 3.8f, 0.0f}, world::kDefaultLevelGoalSize});
        Expect(
            editor::GetEditablePosition(level, {EditorObjectKind::Goal, 0}) != nullptr,
            "level goal has translate origin");
        Expect(
            editor::GetEditableSize(level, {EditorObjectKind::Goal, 0}) != nullptr,
            "level goal has resize gizmo size");
        Expect(
            editor::GetEditableSize(level, {EditorObjectKind::Checkpoint, 0}) == nullptr,
            "checkpoint has no resize gizmo size");
        level.dynamicBoxes.push_back(
            {{2.0f, 1.0f, 0.0f}, world::kDefaultDynamicBoxSize, world::kDefaultDynamicBoxMassKg});
        core::Vec3* boxCenter =
            editor::GetEditablePosition(level, {EditorObjectKind::DynamicBox, 0});
        Expect(boxCenter != nullptr, "Dynamic Box has translate origin");
        if (boxCenter != nullptr)
        {
            boxCenter->x = 4.0f;
        }
        Expect(NearlyEqual(level.dynamicBoxes[0].center.x, 4.0f), "Translate mutates working center");
        core::Vec3* boxSize = editor::GetEditableSize(level, {EditorObjectKind::DynamicBox, 0});
        Expect(boxSize != nullptr, "Dynamic Box has resize size");
        if (boxSize != nullptr)
        {
            boxSize->x = 2.0f;
        }
        Expect(NearlyEqual(level.dynamicBoxes[0].size.x, 2.0f), "Resize mutates working size");
        Expect(level.dynamicBoxes[0].massKg == 30.0f, "gizmo does not rewrite mass");
        level.pressurePlates.push_back({{4.0f, 0.1f, 0.0f}, world::kDefaultPressurePlateSize});
        core::Vec3* plateCenter =
            editor::GetEditablePosition(level, {EditorObjectKind::PressurePlate, 0});
        Expect(plateCenter != nullptr, "Pressure Plate has translate origin");
        if (plateCenter != nullptr)
        {
            plateCenter->x = 5.0f;
        }
        Expect(NearlyEqual(level.pressurePlates[0].center.x, 5.0f), "Translate mutates plate center");
        core::Vec3* plateSize = editor::GetEditableSize(level, {EditorObjectKind::PressurePlate, 0});
        Expect(plateSize != nullptr, "Pressure Plate has resize size");
        if (plateSize != nullptr)
        {
            plateSize->x = 3.0f;
        }
        Expect(NearlyEqual(level.pressurePlates[0].size.x, 3.0f), "Resize mutates plate size");
        Expect(
            !editor::IsScaleSelection({EditorObjectKind::PressurePlate, 0}),
            "Pressure Plate is not Static Prop Scale");
        level.doors.push_back({{4.0f, 1.5f, 0.0f}, world::kDefaultDoorSize, world::kDefaultDoorOpenDistance});
        core::Vec3* doorCenter = editor::GetEditablePosition(level, {EditorObjectKind::Door, 0});
        Expect(doorCenter != nullptr, "Door has translate origin");
        if (doorCenter != nullptr)
        {
            doorCenter->x = 6.0f;
        }
        Expect(NearlyEqual(level.doors[0].center.x, 6.0f), "Translate mutates Door center");
        Expect(NearlyEqual(level.doors[0].openDistance, world::kDefaultDoorOpenDistance),
            "Translate leaves openDistance");
        core::Vec3* doorSize = editor::GetEditableSize(level, {EditorObjectKind::Door, 0});
        Expect(doorSize != nullptr, "Door has resize size");
        if (doorSize != nullptr)
        {
            doorSize->x = 2.0f;
        }
        Expect(NearlyEqual(level.doors[0].size.x, 2.0f), "Resize mutates Door size");
        Expect(
            !editor::IsScaleSelection({EditorObjectKind::Door, 0}),
            "Door is not Static Prop Scale");
        world::ItemPickupSpec pickup{};
        pickup.position = {5.0f, 1.0f, 0.0f};
        pickup.itemId = "key";
        pickup.quantity = 1;
        level.itemPickups.push_back(pickup);
        core::Vec3* pickupPosition =
            editor::GetEditablePosition(level, {EditorObjectKind::ItemPickup, 0});
        Expect(pickupPosition != nullptr, "Item Pickup has translate origin");
        if (pickupPosition != nullptr)
        {
            pickupPosition->x = 7.0f;
        }
        Expect(NearlyEqual(level.itemPickups[0].position.x, 7.0f), "Translate mutates Item Pickup position");
        Expect(
            editor::GetEditableSize(level, {EditorObjectKind::ItemPickup, 0}) == nullptr,
            "Item Pickup has no Resize size");
        Expect(
            !editor::IsResizeSelection({EditorObjectKind::ItemPickup, 0}),
            "Item Pickup is not resize");
        Expect(
            editor::IsScaleSelection({EditorObjectKind::ItemPickup, 0}),
            "Item Pickup is Scale for visualScale");
        Expect(
            editor::IsRotateSelection({EditorObjectKind::ItemPickup, 0}),
            "Item Pickup is Rotate for visualRotationDegrees");
        core::Vec3* pickupScale =
            editor::GetEditableScale(level, {EditorObjectKind::ItemPickup, 0});
        Expect(pickupScale != nullptr, "Item Pickup has visualScale pointer");
        Expect(
            pickupScale == &level.itemPickups[0].visualScale,
            "Scale gizmo edits visualScale not position");
        core::Vec3* pickupRotation =
            editor::GetEditableRotation(level, {EditorObjectKind::ItemPickup, 0});
        Expect(pickupRotation != nullptr, "Item Pickup has visualRotationDegrees pointer");
        Expect(
            pickupRotation == &level.itemPickups[0].visualRotationDegrees,
            "Rotate gizmo edits visualRotationDegrees not position");
        if (pickupScale != nullptr)
        {
            pickupScale->x = 0.25f;
        }
        Expect(NearlyEqual(level.itemPickups[0].visualScale.x, 0.25f), "Scale mutates visualScale");
        Expect(NearlyEqual(level.itemPickups[0].position.x, 7.0f), "Scale leaves gameplay position");
        world::StaticPropSpec prop{};
        prop.modelIdentity = "models/test_static.glb";
        prop.position = {3.0f, 1.0f, 0.0f};
        prop.rotationDegrees = {0.0f, 15.0f, 0.0f};
        prop.scale = {1.0f, 2.0f, 1.0f};
        level.staticProps.push_back(prop);
        core::Vec3* propPosition =
            editor::GetEditablePosition(level, {EditorObjectKind::StaticProp, 0});
        Expect(propPosition != nullptr, "Static Prop has translate origin");
        if (propPosition != nullptr)
        {
            propPosition->x = 9.0f;
        }
        Expect(NearlyEqual(level.staticProps[0].position.x, 9.0f), "Translate mutates Static Prop position");
        Expect(NearlyEqual(level.staticProps[0].rotationDegrees.y, 15.0f), "Translate leaves rotation");
        Expect(NearlyEqual(level.staticProps[0].scale.y, 2.0f), "Translate leaves scale");
        Expect(
            editor::GetEditableSize(level, {EditorObjectKind::StaticProp, 0}) == nullptr,
            "Static Prop has no primitive Resize size");
        Expect(editor::IsScaleSelection({EditorObjectKind::StaticProp, 0}), "Static Prop is Scale");
        Expect(editor::IsRotateSelection({EditorObjectKind::StaticProp, 0}), "Static Prop is Rotate");
        Expect(
            !editor::IsScaleSelection({EditorObjectKind::Ground, 0}),
            "Ground is not Static Prop Scale");
        Expect(
            !editor::IsRotateSelection({EditorObjectKind::Ground, 0}),
            "Ground is not rotatable");
        Expect(
            !editor::IsScaleSelection({EditorObjectKind::DynamicBox, 0}),
            "Dynamic Box is not Static Prop Scale");
        Expect(
            !editor::IsRotateSelection({EditorObjectKind::DynamicBox, 0}),
            "Dynamic Box is not rotatable");
        core::Vec3* propScale = editor::GetEditableScale(level, {EditorObjectKind::StaticProp, 0});
        Expect(propScale != nullptr, "Static Prop has editable scale");
        core::Vec3* propRotation =
            editor::GetEditableRotation(level, {EditorObjectKind::StaticProp, 0});
        Expect(propRotation != nullptr, "Static Prop has editable rotation");
        Expect(
            propRotation == &level.staticProps[0].rotationDegrees,
            "Rotate edits authored Static Prop rotation");
        if (propScale != nullptr)
        {
            propScale->x = 3.0f;
        }
        Expect(NearlyEqual(level.staticProps[0].scale.x, 3.0f), "Scale lookup mutates working scale");
        Expect(NearlyEqual(level.staticProps[0].position.x, 9.0f), "Scale lookup leaves position");
        Expect(NearlyEqual(level.staticProps[0].rotationDegrees.y, 15.0f), "Scale lookup leaves rotation");
    }

    // ---- X/Y/Z constrained drag ----
    {
        const render::CameraView view = MakeView({0.0f, 1.0f, 10.0f}, {0.0f, 1.0f, 0.0f});
        const core::Vec3 start{0.0f, 1.0f, 0.0f};
        editor::GizmoInteractionState state{};

        Expect(
            editor::BeginGizmoDrag(
                state,
                {EditorObjectKind::Ground, 0},
                EditorAxis::X,
                start,
                RayThrough(view, start),
                view),
            "X drag starts");
        const core::Vec3 movedX = editor::GizmoDragPosition(
            state, RayThrough(view, {3.0f, 1.0f, 0.0f}), view);
        Expect(NearlyEqual(movedX.x, 3.0f), "X drag changes X");
        Expect(NearlyEqual(movedX.y, start.y), "X drag keeps Y");
        Expect(NearlyEqual(movedX.z, start.z), "X drag keeps Z");
        Expect(Vec3Near(state.dragStartPosition, start), "drag start position is captured");
        editor::EndGizmoDrag(state);

        Expect(
            editor::BeginGizmoDrag(
                state,
                {EditorObjectKind::Ground, 0},
                EditorAxis::Y,
                start,
                RayThrough(view, start),
                view),
            "Y drag starts");
        const core::Vec3 movedY = editor::GizmoDragPosition(
            state, RayThrough(view, {0.0f, 4.0f, 0.0f}), view);
        Expect(NearlyEqual(movedY.x, start.x), "Y drag keeps X");
        Expect(NearlyEqual(movedY.y, 4.0f), "Y drag changes Y");
        Expect(NearlyEqual(movedY.z, start.z), "Y drag keeps Z");
        editor::EndGizmoDrag(state);
    }

    {
        const render::CameraView view = MakeView({10.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
        const core::Vec3 start{0.0f, 1.0f, 0.0f};
        editor::GizmoInteractionState state{};
        Expect(
            editor::BeginGizmoDrag(
                state,
                {EditorObjectKind::Spawn, 0},
                EditorAxis::Z,
                start,
                RayThrough(view, start),
                view),
            "Z drag starts");
        const core::Vec3 movedZ = editor::GizmoDragPosition(
            state, RayThrough(view, {0.0f, 1.0f, 2.5f}), view);
        Expect(NearlyEqual(movedZ.x, start.x), "Z drag keeps X");
        Expect(NearlyEqual(movedZ.y, start.y), "Z drag keeps Y");
        Expect(NearlyEqual(movedZ.z, 2.5f), "Z drag changes Z");
    }

    // ---- drag continuity from start, not incremental accumulation ----
    {
        const render::CameraView view = MakeView({0.0f, 1.0f, 10.0f}, {0.0f, 1.0f, 0.0f});
        const core::Vec3 start{1.0f, 1.0f, 0.0f};
        editor::GizmoInteractionState state{};
        editor::BeginGizmoDrag(
            state,
            {EditorObjectKind::Ground, 0},
            EditorAxis::X,
            start,
            RayThrough(view, start),
            view);
        const core::Vec3 mid =
            editor::GizmoDragPosition(state, RayThrough(view, {2.0f, 1.0f, 0.0f}), view);
        const core::Vec3 end =
            editor::GizmoDragPosition(state, RayThrough(view, {4.0f, 1.0f, 0.0f}), view);
        Expect(NearlyEqual(mid.x, 2.0f), "mid drag is absolute from start");
        Expect(NearlyEqual(end.x, 4.0f), "end drag is absolute from start, not mid+delta");
        Expect(Vec3Near(state.dragStartPosition, start), "start pose never mutates during drag");
    }

    // ---- near-parallel remains finite ----
    {
        const render::CameraView view = MakeView({12.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
        const core::Vec3 start{0.0f, 0.0f, 0.0f};
        editor::GizmoInteractionState state{};
        const bool began = editor::BeginGizmoDrag(
            state,
            {EditorObjectKind::Ground, 0},
            EditorAxis::X,
            start,
            RayThrough(view, {0.0f, 1.0f, 0.0f}),
            view);
        Expect(began, "near-parallel X drag can start via fallback");
        const core::Vec3 moved = editor::GizmoDragPosition(
            state, RayThrough(view, {0.0f, 2.0f, 1.0f}), view);
        Expect(editor::IsFiniteVec3(moved), "near-parallel drag stays finite");
        Expect(NearlyEqual(moved.y, start.y), "near-parallel X keeps Y");
        Expect(NearlyEqual(moved.z, start.z), "near-parallel X keeps Z");
    }

    // ---- unusable ray does not produce NaN ----
    {
        const render::CameraView view = MakeView({0.0f, 0.0f, 10.0f}, {0.0f, 0.0f, 0.0f});
        editor::GizmoInteractionState state{};
        const core::Vec3 start{0.0f, 0.0f, 0.0f};
        editor::BeginGizmoDrag(
            state,
            {EditorObjectKind::Ground, 0},
            EditorAxis::X,
            start,
            RayThrough(view, start),
            view);
        editor::Ray3 zero{};
        const core::Vec3 held = editor::GizmoDragPosition(state, zero, view);
        Expect(Vec3Near(held, start), "zero-direction ray keeps start");
        Expect(editor::IsFiniteVec3(held), "zero-direction ray is finite");
    }

    // ---- handle pick prefers the aimed axis ----
    {
        const core::Vec3 origin{};
        const float length = 2.0f;
        const float hitRadius = editor::GizmoHitRadius(length);
        editor::Ray3 alongX{{ -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }};
        Expect(
            editor::PickGizmoHandle(alongX, origin, length, hitRadius) == EditorAxis::X,
            "ray along X hits X");
        editor::Ray3 alongY{{ 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }};
        Expect(
            editor::PickGizmoHandle(alongY, origin, length, hitRadius) == EditorAxis::Y,
            "ray along Y hits Y");
        editor::Ray3 miss{{ 8.0f, 8.0f, 8.0f }, { 0.0f, 1.0f, 0.0f }};
        Expect(
            editor::PickGizmoHandle(miss, origin, length, hitRadius) == EditorAxis::None,
            "far ray misses handles");
        const render::CameraView sideView = MakeView({0.0f, 0.0f, 10.0f}, {});
        const float sideLength = editor::GizmoWorldLength(sideView, origin);
        Expect(
            editor::PickGizmoHandle(
                RayThrough(sideView, {sideLength * 0.5f, 0.0f, 0.0f}),
                origin,
                sideLength,
                editor::GizmoHitRadius(sideLength))
                == EditorAxis::X,
            "camera ray hits X handle");
    }

    // ---- pending preview is working geometry, not active ----
    {
        world::LevelDefinition active = MakeStubLevel();
        world::LevelDefinition working = active;
        working.elevatedPlatforms[0].center.x = 7.0f;
        const EditorSelection platform0{EditorObjectKind::ElevatedPlatform, 0};
        Expect(
            editor::AuthoredGeometryDiffers(active, working, platform0),
            "working X=7 differs from active X=5");
        const editor::EditorPendingTransformPreview preview =
            editor::MakePendingTransformPreview(platform0, active, working);
        Expect(preview.visible, "pending preview visible while unapplied");
        Expect(NearlyEqual(preview.center.x, 7.0f), "preview uses working center");
        Expect(NearlyEqual(preview.size.x, working.elevatedPlatforms[0].size.x), "preview uses working size");

        const editor::GizmoDrawRequest gizmo = editor::MakeGizmoDrawRequest(
            platform0, working, MakeView({0.0f, 5.0f, 20.0f}, {7.0f, 0.75f, 0.0f}), {});
        Expect(gizmo.visible, "gizmo visible for platform");
        Expect(NearlyEqual(gizmo.origin.x, 7.0f), "gizmo origin follows working copy");

        working = active;
        const editor::EditorPendingTransformPreview gone =
            editor::MakePendingTransformPreview(platform0, active, working);
        Expect(!gone.visible, "preview hidden when active == working");

        const editor::EditorPendingTransformPreview cameraPreview =
            editor::MakePendingTransformPreview({EditorObjectKind::Camera, 0}, active, working);
        Expect(!cameraPreview.visible, "camera has no pending world preview");
    }

    // ---- spawn preview uses player visual size ----
    {
        world::LevelDefinition active = MakeStubLevel();
        world::LevelDefinition working = active;
        working.initialSpawnVisualCenter.x = 4.0f;
        const editor::EditorPendingTransformPreview preview = editor::MakePendingTransformPreview(
            {EditorObjectKind::Spawn, 0}, active, working);
        Expect(preview.visible, "spawn preview visible");
        Expect(NearlyEqual(preview.center.x, 4.0f), "spawn preview at working spawn");
        Expect(NearlyEqual(preview.size.y, 1.6f), "spawn preview uses kPlayerVisualSize");
    }

    // ---- gizmo scale stays in range ----
    {
        const float nearLength = editor::GizmoWorldLength(
            MakeView({0.0f, 0.0f, 0.6f}, {0.0f, 0.0f, 0.0f}), {0.0f, 0.0f, 0.0f});
        const float farLength = editor::GizmoWorldLength(
            MakeView({0.0f, 0.0f, 400.0f}, {0.0f, 0.0f, 0.0f}), {0.0f, 0.0f, 0.0f});
        Expect(nearLength >= editor::kGizmoMinWorldLength, "near camera clamps min length");
        Expect(farLength <= editor::kGizmoMaxWorldLength, "far camera clamps max length");
        Expect(
            editor::GizmoHitRadius(2.0f) > editor::GizmoVisualRadius(2.0f),
            "hit radius is larger than visual shaft");
    }

    // ---- layout path is user-data, not CWD ----
    {
        const std::filesystem::path made = editor::MakeEditorLayoutPath("C:/Users/dev/AppData/Local");
        Expect(made.filename() == "editor_layout.ini", "layout file name");
        Expect(
            made.parent_path().filename() == "Platformer3D",
            "layout lives under Platformer3D");
        Expect(
            made.string().find("AppData") != std::string::npos,
            "path join keeps user-data root");
        Expect(made.is_absolute(), "joined layout path is absolute");
        Expect(editor::MakeEditorLayoutPath({}).empty(), "empty user-data yields empty layout path");
        Expect(
            editor::MakeEditorLayoutPath("relative").empty(),
            "relative user-data is rejected");

        const editor::EditorLayoutDefaults defaults =
            editor::ComputeDefaultEditorLayout(1280.0f, 720.0f);
        Expect(std::strcmp(defaults.metrics.name, editor::kMetricsWindowName) == 0, "metrics name");
        Expect(
            std::strcmp(defaults.hierarchy.name, editor::kHierarchyWindowName) == 0,
            "hierarchy name");
        Expect(
            std::strcmp(defaults.inspector.name, editor::kInspectorWindowName) == 0,
            "inspector name");
        Expect(
            std::strcmp(defaults.levelEditor.name, editor::kLevelEditorWindowName) == 0,
            "level editor name");
        Expect(
            std::strcmp(defaults.toolOutput.name, editor::kToolOutputWindowName) == 0,
            "tool output name");
        Expect(
            std::strcmp(defaults.objectPalette.name, editor::kObjectPaletteWindowName) == 0,
            "object palette name");
        Expect(
            std::strcmp(defaults.contentBrowser.name, editor::kContentBrowserWindowName) == 0,
            "content browser name");
        Expect(
            std::strcmp(defaults.levels.name, editor::kLevelsWindowName) == 0,
            "levels name");
        Expect(editor::FindDefaultPlacement(defaults, editor::kObjectPaletteWindowName) != nullptr,
            "object palette has a default placement");
        Expect(editor::FindDefaultPlacement(defaults, editor::kContentBrowserWindowName) != nullptr,
            "content browser has a default placement");
        Expect(editor::FindDefaultPlacement(defaults, editor::kLevelsWindowName) != nullptr,
            "levels has a default placement");
        Expect(editor::FindDefaultPlacement(defaults, editor::kModelPreviewWindowName) != nullptr,
            "model preview has a default placement");
        Expect(defaults.contentBrowser.y > defaults.hierarchy.y, "content browser sits below hierarchy");
        Expect(defaults.levels.y > defaults.hierarchy.y, "levels sits below hierarchy");
        Expect(defaults.contentBrowser.y > defaults.levels.y, "content browser sits below levels");
        Expect(defaults.toolOutput.y > defaults.contentBrowser.y, "tool output sits below content browser");
        Expect(defaults.inspector.x > defaults.metrics.x, "inspector is on the right");
        Expect(defaults.hierarchy.y > defaults.metrics.y, "hierarchy sits below metrics");
        Expect(defaults.toolOutput.y > defaults.inspector.y, "tool output sits lower than Inspector");
        Expect(defaults.toolOutput.width > defaults.inspector.width, "tool output is wider than Inspector");

        editor::EditorWindowPlacement offscreen{
            editor::kInspectorWindowName, 8000.0f, -4000.0f, 340.0f, 360.0f};
        const editor::EditorWindowPlacement clamped =
            editor::ClampEditorWindowPlacement(offscreen, 1280.0f, 720.0f);
        Expect(clamped.x < 1280.0f, "off-screen X is pulled into the viewport");
        Expect(clamped.y + clamped.height > 0.0f, "off-screen Y keeps a visible slice");
        Expect(
            editor::EditorWindowNeedsClamp(offscreen, 1280.0f, 720.0f),
            "off-screen placement needs clamp");
        Expect(
            !editor::EditorWindowNeedsClamp(defaults.metrics, 1280.0f, 720.0f),
            "default metrics does not need clamp");
    }

    // ---- pending preview from numeric size edit ----
    {
        world::LevelDefinition active = MakeStubLevel();
        world::LevelDefinition working = active;
        working.elevatedPlatforms[0].size.x = 6.0f;
        const EditorSelection platform0{EditorObjectKind::ElevatedPlatform, 0};
        const editor::EditorPendingTransformPreview preview =
            editor::MakePendingTransformPreview(platform0, active, working);
        Expect(preview.visible, "size-only edit shows pending preview");
        Expect(NearlyEqual(preview.center.x, 5.0f), "size-only preview keeps working center");
        Expect(NearlyEqual(preview.size.x, 6.0f), "size-only preview uses working size");
    }

    // ---- Apply/Revert clear drag ----
    {
        editor::GizmoInteractionState state{};
        const render::CameraView view = MakeView({0.0f, 1.0f, 10.0f}, {0.0f, 1.0f, 0.0f});
        Expect(
            editor::BeginGizmoDrag(
                state,
                {EditorObjectKind::Ground, 0},
                EditorAxis::X,
                {0.0f, 1.0f, 0.0f},
                RayThrough(view, {0.0f, 1.0f, 0.0f}),
                view),
            "clear-drag test can begin");
        Expect(state.dragging, "drag is active before Apply-equivalent clear");
        editor::ClearGizmoInteraction(state);
        Expect(!state.dragging, "Apply/Revert clear ends drag");
        Expect(state.active == EditorAxis::None, "Apply/Revert clear drops active axis");
    }

    // ---- live tick: ImGui capture does not start drag ----
    {
        world::LevelDefinition working = MakeStubLevel();
        working.elevatedPlatforms[0].center = {};
        editor::GizmoInteractionState state{};
        const render::CameraView view = MakeView({0.0f, 1.0f, 10.0f}, {});
        editor::Ray3 alongX{{-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
        const bool consumed = editor::UpdateGizmoInteraction(
            state,
            {EditorObjectKind::ElevatedPlatform, 0},
            working,
            view,
            alongX,
            true,
            false,
            true,
            true,
            false);
        Expect(!consumed, "imgui capture does not consume the pointer");
        Expect(!state.dragging, "imgui capture does not start gizmo drag");
        Expect(NearlyEqual(working.elevatedPlatforms[0].center.x, 0.0f), "imgui click does not move");
    }

    // ---- live tick writes working copy, X only ----
    {
        world::LevelDefinition working = MakeStubLevel();
        working.elevatedPlatforms[0].center = {0.0f, 1.0f, 0.0f};
        editor::GizmoInteractionState state{};
        const render::CameraView view = MakeView({0.0f, 1.0f, 10.0f}, {0.0f, 1.0f, 0.0f});
        const EditorSelection platform0{EditorObjectKind::ElevatedPlatform, 0};
        Expect(
            editor::UpdateGizmoInteraction(
                state,
                platform0,
                working,
                view,
                RayThrough(view, {0.5f, 1.0f, 0.0f}),
                false,
                false,
                true,
                true,
                false),
            "handle press starts live drag");
        Expect(state.dragging, "live drag is active");
        Expect(state.dragTarget == platform0, "drag target is captured");
        editor::UpdateGizmoInteraction(
            state,
            {EditorObjectKind::Ground, 0},
            working,
            view,
            RayThrough(view, {3.0f, 1.0f, 0.0f}),
            false,
            false,
            false,
            true,
            false);
        Expect(working.elevatedPlatforms[0].center.x > 1.5f, "live drag writes working X");
        Expect(NearlyEqual(working.elevatedPlatforms[0].center.y, 1.0f), "live drag keeps working Y");
        Expect(NearlyEqual(working.elevatedPlatforms[0].center.z, 0.0f), "live drag keeps working Z");
        Expect(
            NearlyEqual(working.ground.center.x, MakeStubLevel().ground.center.x),
            "incidental selection does not redirect drag");
    }

    // ---- unsupported selection has no gizmo or preview ----
    {
        world::LevelDefinition active = MakeStubLevel();
        const editor::GizmoDrawRequest cameraGizmo = editor::MakeGizmoDrawRequest(
            {EditorObjectKind::Camera, 0},
            active,
            MakeView({0.0f, 5.0f, 20.0f}, {}),
            {});
        Expect(!cameraGizmo.visible, "camera has no live gizmo request");
        Expect(
            !editor::MakeGizmoDrawRequest(
                 {EditorObjectKind::Slope, 0}, active, MakeView({0.0f, 5.0f, 20.0f}, {}), {})
                 .visible,
            "slope has no live gizmo request");
    }

    // ---- M35 Phase A resize support / lookup ----
    {
        Expect(!editor::IsResizeSelection({EditorObjectKind::Spawn, 0}), "spawn is not resize");
        Expect(editor::IsResizeSelection({EditorObjectKind::Ground, 0}), "ground is resize");
        Expect(
            editor::IsResizeSelection({EditorObjectKind::ElevatedPlatform, 0}),
            "platform 0 is resize");
        Expect(
            !editor::IsResizeSelection({EditorObjectKind::Camera, 0}), "camera is not resize");
        Expect(!editor::IsResizeSelection({EditorObjectKind::Slope, 0}), "slope is not resize");
        Expect(!editor::IsResizeSelection({EditorObjectKind::Checkpoint, 0}), "checkpoint is not resize");
        Expect(!editor::IsResizeSelection({EditorObjectKind::Hazard, 0}), "hazard is not resize");
        Expect(!editor::IsResizeSelection({EditorObjectKind::Collectible, 0}), "collectible is not resize");
        Expect(editor::IsResizeSelection({EditorObjectKind::Goal, 0}), "Level Goal is resize");
        Expect(editor::IsResizeSelection({EditorObjectKind::DynamicBox, 0}), "Dynamic Box is resize");
        Expect(editor::IsResizeSelection({EditorObjectKind::PressurePlate, 0}), "Pressure Plate is resize");
        Expect(editor::IsResizeSelection({EditorObjectKind::Door, 0}), "Door is resize");
        Expect(!editor::IsResizeSelection({EditorObjectKind::ItemPickup, 0}), "Item Pickup is not resize");
        Expect(!editor::IsResizeSelection({EditorObjectKind::StaticProp, 0}), "Static Prop is not primitive Resize");
        Expect(editor::IsScaleSelection({EditorObjectKind::StaticProp, 0}), "Static Prop is Scale selection");
        Expect(!editor::IsScaleSelection({EditorObjectKind::Spawn, 0}), "spawn is not Scale");
        Expect(!editor::IsScaleSelection({EditorObjectKind::Goal, 0}), "Level Goal is not Scale");
        world::LevelDefinition level = MakeStubLevel();
        Expect(
            editor::GetEditableSize(level, {EditorObjectKind::Spawn, 0}) == nullptr,
            "spawn has no mutable size");
        core::Vec3* size = editor::GetEditableSize(level, {EditorObjectKind::ElevatedPlatform, 0});
        Expect(size != nullptr, "platform 0 has mutable size");
        if (size != nullptr)
        {
            size->x = 6.0f;
        }
        Expect(NearlyEqual(level.elevatedPlatforms[0].size.x, 6.0f), "size lookup mutates working");
        Expect(NearlyEqual(level.elevatedPlatforms[0].center.x, 5.0f), "size lookup leaves center");
    }

    // ---- X/Y/Z resize, center preserved, continuity, min clamp ----
    {
        world::LevelDefinition working = MakeStubLevel();
        const render::CameraView view = MakeView({20.0f, 8.0f, 20.0f}, {5.0f, 0.75f, 0.0f});
        EditorSelection platform0{EditorObjectKind::ElevatedPlatform, 0};
        const core::Vec3 origin = working.elevatedPlatforms[0].center;
        const core::Vec3 startSize = working.elevatedPlatforms[0].size;
        const float length = editor::GizmoWorldLength(view, origin);

        editor::GizmoInteractionState state{};
        Expect(
            editor::BeginResizeDrag(
                state,
                platform0,
                EditorAxis::X,
                1,
                origin,
                startSize,
                RayThrough(view, {origin.x + length, origin.y, origin.z}),
                view),
            "begin +X resize");
        const core::Vec3 sizeX = editor::GizmoResizeSize(
            state, RayThrough(view, {origin.x + length + 1.0f, origin.y, origin.z}), view);
        Expect(sizeX.x > startSize.x, "X resize grows size.x");
        Expect(NearlyEqual(sizeX.y, startSize.y), "X resize keeps size.y");
        Expect(NearlyEqual(sizeX.z, startSize.z), "X resize keeps size.z");
        Expect(Vec3Near(working.elevatedPlatforms[0].center, origin), "X resize does not move center");

        const core::Vec3 again = editor::GizmoResizeSize(
            state, RayThrough(view, {origin.x + length + 1.0f, origin.y, origin.z}), view);
        Expect(Vec3Near(sizeX, again), "resize is absolute from drag start");

        editor::EndGizmoDrag(state);
        Expect(
            editor::BeginResizeDrag(
                state,
                platform0,
                EditorAxis::Y,
                1,
                origin,
                startSize,
                RayThrough(view, {origin.x, origin.y + length, origin.z}),
                view),
            "begin +Y resize");
        const core::Vec3 sizeY = editor::GizmoResizeSize(
            state, RayThrough(view, {origin.x, origin.y + length + 0.5f, origin.z}), view);
        Expect(sizeY.y > startSize.y, "Y resize grows size.y");
        Expect(NearlyEqual(sizeY.x, startSize.x), "Y resize keeps size.x");
        Expect(NearlyEqual(sizeY.z, startSize.z), "Y resize keeps size.z");

        editor::EndGizmoDrag(state);
        Expect(
            editor::BeginResizeDrag(
                state,
                platform0,
                EditorAxis::Z,
                1,
                origin,
                startSize,
                RayThrough(view, {origin.x, origin.y, origin.z + length}),
                view),
            "begin +Z resize");
        const core::Vec3 sizeZ = editor::GizmoResizeSize(
            state, RayThrough(view, {origin.x, origin.y, origin.z + length + 0.5f}), view);
        Expect(sizeZ.z > startSize.z, "Z resize grows size.z");
        Expect(NearlyEqual(sizeZ.x, startSize.x), "Z resize keeps size.x");
        Expect(NearlyEqual(sizeZ.y, startSize.y), "Z resize keeps size.y");

        editor::EndGizmoDrag(state);
        Expect(
            editor::BeginResizeDrag(
                state,
                platform0,
                EditorAxis::X,
                1,
                origin,
                startSize,
                RayThrough(view, {origin.x + length, origin.y, origin.z}),
                view),
            "begin shrink X");
        const core::Vec3 shrunk = editor::GizmoResizeSize(
            state, RayThrough(view, {origin.x - 50.0f, origin.y, origin.z}), view);
        Expect(NearlyEqual(shrunk.x, editor::kMinAuthoredBoxExtent, 0.001f), "size.x clamps to min");
        Expect(shrunk.x > 0.0f, "clamped size is positive");
        Expect(std::isfinite(shrunk.x) && std::isfinite(shrunk.y) && std::isfinite(shrunk.z),
            "clamped size is finite");
    }

    // ---- negative handle grows when pulled outward; near-parallel finite ----
    {
        world::LevelDefinition working = MakeStubLevel();
        const render::CameraView view = MakeView({20.0f, 8.0f, 20.0f}, {5.0f, 0.75f, 0.0f});
        EditorSelection platform0{EditorObjectKind::ElevatedPlatform, 0};
        const core::Vec3 origin = working.elevatedPlatforms[0].center;
        const core::Vec3 startSize = working.elevatedPlatforms[0].size;
        const float length = editor::GizmoWorldLength(view, origin);
        editor::GizmoInteractionState state{};
        Expect(
            editor::BeginResizeDrag(
                state,
                platform0,
                EditorAxis::X,
                -1,
                origin,
                startSize,
                RayThrough(view, {origin.x - length, origin.y, origin.z}),
                view),
            "begin -X resize");
        const core::Vec3 grown = editor::GizmoResizeSize(
            state, RayThrough(view, {origin.x - length - 1.0f, origin.y, origin.z}), view);
        Expect(grown.x > startSize.x, "pulling -X handle outward grows size.x");

        // Same relative geometry as the M34 translation fallback: camera looks along X
        // from ~12 m, ray is offset in Y so closest-points can start.
        const render::CameraView parallel =
            MakeView({origin.x + 12.0f, origin.y, origin.z}, origin);
        editor::EndGizmoDrag(state);
        Expect(
            editor::BeginResizeDrag(
                state,
                platform0,
                EditorAxis::X,
                1,
                origin,
                startSize,
                RayThrough(parallel, {origin.x, origin.y + 1.0f, origin.z}),
                parallel),
            "near-parallel resize can begin");
        const core::Vec3 parallelSize = editor::GizmoResizeSize(
            state, RayThrough(parallel, {origin.x, origin.y + 2.0f, origin.z + 1.0f}), parallel);
        Expect(
            std::isfinite(parallelSize.x) && std::isfinite(parallelSize.y)
                && std::isfinite(parallelSize.z),
            "near-parallel resize stays finite");
    }

    // ---- live resize writes working size, not a stub cache ----
    {
        world::LevelDefinition working = MakeStubLevel();
        const world::LevelDefinition active = working;
        const render::CameraView view = MakeView({20.0f, 8.0f, 20.0f}, {5.0f, 0.75f, 0.0f});
        EditorSelection platform0{EditorObjectKind::ElevatedPlatform, 0};
        const core::Vec3 origin = working.elevatedPlatforms[0].center;
        const float length = editor::GizmoWorldLength(view, origin);
        editor::GizmoInteractionState state{};
        Expect(
            editor::UpdateResizeInteraction(
                state,
                platform0,
                working,
                view,
                RayThrough(view, {origin.x + length, origin.y, origin.z}),
                false,
                false,
                true,
                true,
                false),
            "resize press consumes pointer");
        Expect(
            editor::UpdateResizeInteraction(
                state,
                {EditorObjectKind::Ground, 0},
                working,
                view,
                RayThrough(view, {origin.x + length + 1.0f, origin.y, origin.z}),
                false,
                false,
                false,
                true,
                false),
            "resize drag ignores incidental selection");
        Expect(working.elevatedPlatforms[0].size.x > 4.0f, "resize writes working size.x");
        Expect(NearlyEqual(working.elevatedPlatforms[0].center.x, 5.0f), "resize keeps working center");
        Expect(NearlyEqual(active.elevatedPlatforms[0].size.x, 4.0f), "active size unchanged");
        const editor::EditorPendingTransformPreview preview =
            editor::MakePendingTransformPreview(platform0, active, working);
        Expect(preview.visible, "size-only working edit shows pending preview");
        Expect(NearlyEqual(preview.size.x, working.elevatedPlatforms[0].size.x),
            "preview uses working size");
        Expect(NearlyEqual(preview.center.x, 5.0f), "preview keeps working center");
        Expect(
            !editor::MakeResizeGizmoDrawRequest(
                 {EditorObjectKind::Spawn, 0}, working, view, {})
                 .visible,
            "spawn has no resize gizmo request");
        Expect(
            editor::MakeResizeGizmoDrawRequest(platform0, working, view, {}).visible,
            "platform has resize gizmo request");
    }

    // ---- M49 Static Prop Scale gizmo ----
    {
        world::LevelDefinition working = MakeStubLevel();
        world::StaticPropSpec prop{};
        prop.modelIdentity = "models/test_static.glb";
        prop.position = {3.0f, 1.0f, 0.0f};
        prop.rotationDegrees = {5.0f, 15.0f, 25.0f};
        prop.scale = {1.0f, 2.0f, 1.0f};
        working.staticProps.push_back(prop);
        world::StaticPropSpec other = prop;
        other.scale = {1.0f, 1.0f, 1.0f};
        working.staticProps.push_back(other);
        const world::LevelDefinition active = working;
        const render::CameraView view = MakeView({20.0f, 8.0f, 20.0f}, {3.0f, 1.0f, 0.0f});
        EditorSelection prop0{EditorObjectKind::StaticProp, 0};
        const core::Vec3 origin = working.staticProps[0].position;
        const core::Vec3 startScale = working.staticProps[0].scale;
        const core::Vec3 startRotation = working.staticProps[0].rotationDegrees;
        const float length = editor::GizmoWorldLength(view, origin);

        Expect(
            editor::MakeScaleGizmoDrawRequest(prop0, working, view, {}).visible,
            "Static Prop has Scale gizmo request");
        Expect(
            !editor::MakeScaleGizmoDrawRequest(
                 {EditorObjectKind::ElevatedPlatform, 0}, working, view, {})
                 .visible,
            "platform has no Scale gizmo request");
        editor::GizmoInteractionState rejected{};
        Expect(
            !editor::BeginScaleDrag(
                rejected,
                {EditorObjectKind::Ground, 0},
                EditorAxis::X,
                1,
                origin,
                startScale,
                RayThrough(view, {origin.x + length, origin.y, origin.z}),
                view),
            "non-Static-Prop cannot begin Scale drag");

        editor::GizmoInteractionState state{};
        Expect(
            editor::BeginScaleDrag(
                state,
                prop0,
                EditorAxis::X,
                1,
                origin,
                startScale,
                RayThrough(view, {origin.x + length, origin.y, origin.z}),
                view),
            "begin +X Scale");
        const core::Vec3 scaleX = editor::GizmoScaleSize(
            state, RayThrough(view, {origin.x + length + 1.0f, origin.y, origin.z}), view);
        Expect(scaleX.x > startScale.x, "Scale X changes scale.x");
        Expect(NearlyEqual(scaleX.y, startScale.y), "Scale X keeps scale.y");
        Expect(NearlyEqual(scaleX.z, startScale.z), "Scale X keeps scale.z");
        Expect(world::StaticPropScaleIsValid(scaleX), "Scale X result is valid");

        editor::EndGizmoDrag(state);
        Expect(
            editor::BeginScaleDrag(
                state,
                prop0,
                EditorAxis::Y,
                1,
                origin,
                startScale,
                RayThrough(view, {origin.x, origin.y + length, origin.z}),
                view),
            "begin +Y Scale");
        const core::Vec3 scaleY = editor::GizmoScaleSize(
            state, RayThrough(view, {origin.x, origin.y + length + 1.0f, origin.z}), view);
        Expect(NearlyEqual(scaleY.x, startScale.x), "Scale Y keeps scale.x");
        Expect(scaleY.y > startScale.y, "Scale Y changes scale.y");
        Expect(NearlyEqual(scaleY.z, startScale.z), "Scale Y keeps scale.z");

        editor::EndGizmoDrag(state);
        Expect(
            editor::BeginScaleDrag(
                state,
                prop0,
                EditorAxis::Z,
                1,
                origin,
                startScale,
                RayThrough(view, {origin.x, origin.y, origin.z + length}),
                view),
            "begin +Z Scale");
        const core::Vec3 scaleZ = editor::GizmoScaleSize(
            state, RayThrough(view, {origin.x, origin.y, origin.z + length + 1.0f}), view);
        Expect(NearlyEqual(scaleZ.x, startScale.x), "Scale Z keeps scale.x");
        Expect(NearlyEqual(scaleZ.y, startScale.y), "Scale Z keeps scale.y");
        Expect(scaleZ.z > startScale.z, "Scale Z changes scale.z");

        editor::EndGizmoDrag(state);
        Expect(
            editor::BeginScaleDrag(
                state,
                prop0,
                EditorAxis::X,
                1,
                origin,
                startScale,
                RayThrough(view, {origin.x + length, origin.y, origin.z}),
                view),
            "begin shrink Scale X");
        const core::Vec3 shrunk = editor::GizmoScaleSize(
            state,
            RayThrough(view, {origin.x + length - 100.0f, origin.y, origin.z}),
            view);
        Expect(shrunk.x >= world::kMinStaticPropScale, "Scale respects minimum clamp");
        Expect(world::StaticPropScaleIsValid(shrunk), "clamped Scale stays valid");
        Expect(!(shrunk.x <= 0.0f), "Scale drag never reaches zero");

        editor::EndGizmoDrag(state);
        Expect(
            editor::UpdateScaleInteraction(
                state,
                prop0,
                working,
                view,
                RayThrough(view, {origin.x + length, origin.y, origin.z}),
                false,
                false,
                true,
                true,
                false),
            "Scale press consumes pointer");
        Expect(
            editor::UpdateScaleInteraction(
                state,
                {EditorObjectKind::Ground, 0},
                working,
                view,
                RayThrough(view, {origin.x + length + 1.0f, origin.y, origin.z}),
                false,
                false,
                false,
                true,
                false),
            "Scale drag ignores incidental selection");
        Expect(working.staticProps[0].scale.x > startScale.x, "Scale writes workingCopy scale.x");
        Expect(Vec3Near(working.staticProps[0].position, origin), "Scale leaves position");
        Expect(
            Vec3Near(working.staticProps[0].rotationDegrees, startRotation),
            "Scale leaves rotation");
        Expect(Vec3Near(active.staticProps[0].scale, startScale), "Scale does not mutate active");
        Expect(
            Vec3Near(working.staticProps[1].scale, {1.0f, 1.0f, 1.0f}),
            "shared-asset sibling keeps independent Scale");
        Expect(
            editor::AuthoredGeometryDiffers(active, working, prop0),
            "Scale drag is a pending authored edit");
    }

    {
        world::LevelDefinition working = MakeStubLevel();
        world::ItemPickupSpec pickup{};
        pickup.position = {5.0f, 1.0f, 0.0f};
        pickup.itemId = "key";
        pickup.quantity = 1;
        pickup.modelIdentity = "models/test_static.glb";
        pickup.visualOffset = {0.0f, 0.5f, 0.0f};
        pickup.visualScale = {1.0f, 1.0f, 1.0f};
        working.itemPickups.push_back(pickup);
        const world::LevelDefinition active = working;
        const render::CameraView view = MakeView({20.0f, 8.0f, 20.0f}, {5.0f, 1.5f, 0.0f});
        EditorSelection pickup0{EditorObjectKind::ItemPickup, 0};
        const core::Vec3 visualOrigin = world::ItemPickupVisualPosition(working.itemPickups[0]);
        const core::Vec3 startScale = working.itemPickups[0].visualScale;
        const float length = editor::GizmoWorldLength(view, visualOrigin);
        const editor::GizmoDrawRequest draw =
            editor::MakeScaleGizmoDrawRequest(pickup0, working, view, {});
        Expect(draw.visible, "Item Pickup has Scale gizmo request");
        Expect(Vec3Near(draw.origin, visualOrigin), "Item Pickup Scale gizmo sits at visual position");
        working.itemPickups[0].idleAnimationEnabled = true;
        working.itemPickups[0].idleBobAmplitude = 1.5f;
        working.itemPickups[0].idleSpinSpeedDegrees = 180.0f;
        const editor::GizmoDrawRequest idleDraw =
            editor::MakeScaleGizmoDrawRequest(pickup0, working, view, {});
        Expect(
            Vec3Near(idleDraw.origin, world::ItemPickupVisualPosition(working.itemPickups[0])),
            "35. Scale gizmo origin does not chase runtime animation");
        Expect(
            !Vec3Near(
                idleDraw.origin,
                world::ItemPickupPresentedVisualPosition(working.itemPickups[0], 0.25)),
            "35. Scale gizmo is not the bobbed visual");

        editor::GizmoInteractionState state{};
        Expect(
            editor::BeginScaleDrag(
                state,
                pickup0,
                EditorAxis::Y,
                1,
                visualOrigin,
                startScale,
                RayThrough(view, {visualOrigin.x, visualOrigin.y + length, visualOrigin.z}),
                view),
            "begin Item Pickup +Y Scale");
        const core::Vec3 scaled = editor::GizmoScaleSize(
            state,
            RayThrough(view, {visualOrigin.x, visualOrigin.y + length + 1.0f, visualOrigin.z}),
            view);
        Expect(scaled.y > startScale.y, "Item Pickup Scale Y changes visualScale.y");
        Expect(NearlyEqual(scaled.x, startScale.x), "Item Pickup Scale Y keeps visualScale.x");
        working.itemPickups[0].visualScale = scaled;
        Expect(NearlyEqual(working.itemPickups[0].position.x, 5.0f), "Scale leaves gameplay X");
        Expect(NearlyEqual(working.itemPickups[0].position.y, 1.0f), "Scale leaves gameplay Y");
        Expect(NearlyEqual(working.itemPickups[0].visualOffset.y, 0.5f), "Scale leaves visualOffset");
        Expect(Vec3Near(active.itemPickups[0].visualScale, startScale), "Scale does not mutate active pickup");
        Expect(
            editor::GetEditablePosition(working, pickup0) == &working.itemPickups[0].position,
            "Translate origin remains gameplay position");
    }

    // ---- M58.1 Rotate gizmo ----
    {
        world::LevelDefinition working = MakeStubLevel();
        world::StaticPropSpec prop{};
        prop.modelIdentity = "models/test_static.glb";
        prop.position = {3.0f, 1.0f, 0.0f};
        prop.rotationDegrees = {5.0f, 15.0f, 25.0f};
        prop.scale = {1.0f, 2.0f, 1.0f};
        working.staticProps.push_back(prop);
        world::ItemPickupSpec pickup{};
        pickup.position = {5.0f, 1.0f, 0.0f};
        pickup.itemId = "key";
        pickup.quantity = 1;
        pickup.modelIdentity = "models/test_static.glb";
        pickup.visualOffset = {0.0f, 0.5f, 0.0f};
        pickup.visualRotationDegrees = {10.0f, 20.0f, 30.0f};
        pickup.visualScale = {0.4f, 0.5f, 0.6f};
        working.itemPickups.push_back(pickup);
        const world::LevelDefinition active = working;
        EditorSelection prop0{EditorObjectKind::StaticProp, 0};
        EditorSelection pickup0{EditorObjectKind::ItemPickup, 0};
        const core::Vec3 propOrigin = working.staticProps[0].position;
        const core::Vec3 pickupOrigin = world::ItemPickupVisualPosition(working.itemPickups[0]);
        const render::CameraView propView = MakeView({20.0f, 8.0f, 20.0f}, propOrigin);
        const render::CameraView pickupView = MakeView({20.0f, 8.0f, 20.0f}, pickupOrigin);
        const float propLength = editor::GizmoWorldLength(propView, propOrigin);
        const float pickupLength = editor::GizmoWorldLength(pickupView, pickupOrigin);
        const float ringHit = propLength * editor::kRotateHitRadiusFraction;
        const float unique = 0.70710678f;

        Expect(editor::EditorTransformMode::Rotate != editor::EditorTransformMode::Scale,
            "Rotate is a distinct central transform mode");
        Expect(editor::IsRotateSelection(prop0), "Static Prop supports Rotate");
        Expect(editor::IsRotateSelection(pickup0), "Item Pickup supports Rotate");
        Expect(
            !editor::IsRotateSelection({EditorObjectKind::ElevatedPlatform, 0}),
            "Platform is inert under Rotate");
        Expect(
            !editor::IsRotateSelection({EditorObjectKind::Ground, 0}),
            "Ground is inert under Rotate");
        Expect(
            !editor::IsRotateSelection({EditorObjectKind::Door, 0}),
            "Door is inert under Rotate");
        Expect(
            !editor::IsRotateSelection({EditorObjectKind::PressurePlate, 0}),
            "Pressure Plate is inert under Rotate");
        Expect(
            !editor::IsRotateSelection({EditorObjectKind::Goal, 0}),
            "Level Goal is inert under Rotate");
        Expect(
            editor::MakeRotateGizmoDrawRequest(prop0, working, propView, {}).visible,
            "Static Prop has Rotate gizmo request");
        const editor::GizmoDrawRequest pickupDraw =
            editor::MakeRotateGizmoDrawRequest(pickup0, working, pickupView, {});
        Expect(pickupDraw.visible, "Item Pickup has Rotate gizmo request");
        Expect(
            Vec3Near(pickupDraw.origin, pickupOrigin),
            "Item Pickup Rotate origin is position + visualOffset");
        Expect(
            Vec3Near(
                editor::MakeRotateGizmoDrawRequest(prop0, working, propView, {}).origin, propOrigin),
            "Static Prop Rotate origin is authored position");
        Expect(
            !editor::MakeRotateGizmoDrawRequest(
                 {EditorObjectKind::ElevatedPlatform, 0}, working, propView, {})
                 .visible,
            "unsupported object has no Rotate gizmo");

        const core::Vec3 xRing{
            propOrigin.x,
            propOrigin.y + propLength * unique,
            propOrigin.z + propLength * unique};
        const core::Vec3 yRing{
            propOrigin.x + propLength * unique,
            propOrigin.y,
            propOrigin.z + propLength * unique};
        const core::Vec3 zRing{
            propOrigin.x + propLength * unique,
            propOrigin.y + propLength * unique,
            propOrigin.z};
        Expect(
            editor::PickRotateHandle(RayThrough(propView, xRing), propOrigin, propLength, ringHit)
                == EditorAxis::X,
            "X ring pick");
        Expect(
            editor::PickRotateHandle(RayThrough(propView, yRing), propOrigin, propLength, ringHit)
                == EditorAxis::Y,
            "Y ring pick");
        Expect(
            editor::PickRotateHandle(RayThrough(propView, zRing), propOrigin, propLength, ringHit)
                == EditorAxis::Z,
            "Z ring pick");
        Expect(
            editor::PickRotateHandle(
                RayThrough(propView, {propOrigin.x + 8.0f, propOrigin.y + 8.0f, propOrigin.z + 8.0f}),
                propOrigin,
                propLength,
                ringHit)
                == EditorAxis::None,
            "missed rotation ring");

        editor::GizmoInteractionState rejected{};
        Expect(
            !editor::BeginRotateDrag(
                rejected,
                {EditorObjectKind::Ground, 0},
                EditorAxis::X,
                propOrigin,
                prop.rotationDegrees,
                RayThrough(propView, xRing)),
            "unsupported object cannot begin Rotate drag");
        Expect(!rejected.dragging, "rejected Rotate leaves drag inactive");

        editor::GizmoInteractionState parallel{};
        editor::Ray3 parallelRay{{10.0f, 5.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
        Expect(
            !editor::BeginRotateDrag(
                parallel,
                prop0,
                EditorAxis::Y,
                {},
                {0.0f, 0.0f, 0.0f},
                parallelRay),
            "parallel ray does not begin Y Rotate");
        Expect(!parallel.dragging, "degenerate begin does not capture axis");
        const core::Vec3 parallelResult = editor::GizmoRotateDegrees(parallel, parallelRay);
        Expect(
            std::isfinite(parallelResult.x) && std::isfinite(parallelResult.y)
                && std::isfinite(parallelResult.z),
            "degenerate Rotate does not write Inf");
        Expect(
            !(parallelResult.x != parallelResult.x) && !(parallelResult.y != parallelResult.y)
                && !(parallelResult.z != parallelResult.z),
            "degenerate Rotate does not write NaN");

        const core::Vec3 startRotation = working.staticProps[0].rotationDegrees;
        editor::GizmoInteractionState state{};
        Expect(
            editor::BeginRotateDrag(
                state, prop0, EditorAxis::X, propOrigin, startRotation, RayThrough(propView, xRing)),
            "mouse-down captures X ring");
        Expect(state.dragging && state.active == EditorAxis::X, "active Rotate axis is X");
        const core::Vec3 noJump =
            editor::GizmoRotateDegrees(state, RayThrough(propView, xRing));
        Expect(Vec3Near(noJump, startRotation, 0.25f), "same-ray Rotate has no initial jump");

        const core::Vec3 xSwept{
            propOrigin.x,
            propOrigin.y - propLength * unique,
            propOrigin.z + propLength * unique};
        const core::Vec3 rotatedX = editor::GizmoRotateDegrees(state, RayThrough(propView, xSwept));
        Expect(std::fabs(rotatedX.x - startRotation.x) > 1.0f, "X ring changes authored X");
        Expect(NearlyEqual(rotatedX.y, startRotation.y, 0.25f), "X ring keeps authored Y");
        Expect(NearlyEqual(rotatedX.z, startRotation.z, 0.25f), "X ring keeps authored Z");
        Expect(std::isfinite(rotatedX.x), "X Rotate stays finite");

        editor::EndGizmoDrag(state);
        Expect(!state.dragging && state.active == EditorAxis::None, "mouse-up ends Rotate drag");
        Expect(
            editor::BeginRotateDrag(
                state, prop0, EditorAxis::Y, propOrigin, startRotation, RayThrough(propView, yRing)),
            "begin Y Rotate");
        const core::Vec3 ySwept{
            propOrigin.x + propLength * unique,
            propOrigin.y,
            propOrigin.z - propLength * unique};
        const core::Vec3 rotatedY = editor::GizmoRotateDegrees(state, RayThrough(propView, ySwept));
        Expect(NearlyEqual(rotatedY.x, startRotation.x, 0.25f), "Y ring keeps authored X");
        Expect(std::fabs(rotatedY.y - startRotation.y) > 1.0f, "Y ring changes authored Y");
        Expect(NearlyEqual(rotatedY.z, startRotation.z, 0.25f), "Y ring keeps authored Z");

        editor::EndGizmoDrag(state);
        Expect(
            editor::BeginRotateDrag(
                state, prop0, EditorAxis::Z, propOrigin, startRotation, RayThrough(propView, zRing)),
            "begin Z Rotate");
        const core::Vec3 zSwept{
            propOrigin.x - propLength * unique,
            propOrigin.y + propLength * unique,
            propOrigin.z};
        const core::Vec3 rotatedZ = editor::GizmoRotateDegrees(state, RayThrough(propView, zSwept));
        Expect(NearlyEqual(rotatedZ.x, startRotation.x, 0.25f), "Z ring keeps authored X");
        Expect(NearlyEqual(rotatedZ.y, startRotation.y, 0.25f), "Z ring keeps authored Y");
        Expect(std::fabs(rotatedZ.z - startRotation.z) > 1.0f, "Z ring changes authored Z");

        editor::EndGizmoDrag(state);
        const core::Vec3 startPos = working.staticProps[0].position;
        const core::Vec3 startScale = working.staticProps[0].scale;
        Expect(
            editor::UpdateRotateInteraction(
                state,
                prop0,
                working,
                propView,
                RayThrough(propView, xRing),
                false,
                false,
                true,
                true,
                false),
            "Rotate press consumes pointer so world pick behind the ring cannot run");
        Expect(state.dragging, "Rotate press starts drag");
        Expect(
            editor::UpdateRotateInteraction(
                state,
                prop0,
                working,
                propView,
                RayThrough(propView, xSwept),
                false,
                false,
                false,
                true,
                false),
            "Rotate drag consumes pointer");
        Expect(state.dragging, "active Rotate drag remains captured");
        Expect(
            std::fabs(working.staticProps[0].rotationDegrees.x - startRotation.x) > 1.0f,
            "Rotate writes workingCopy rotation.x");
        Expect(
            Vec3Near(working.staticProps[0].position, startPos),
            "Static Prop Rotate leaves Translate position");
        Expect(
            Vec3Near(working.staticProps[0].scale, startScale),
            "Static Prop Rotate leaves Scale");
        Expect(
            Vec3Near(active.staticProps[0].rotationDegrees, startRotation),
            "Rotate does not mutate active");
        Expect(
            editor::AuthoredGeometryDiffers(active, working, prop0),
            "semantic Rotate produces pending authored edit");
        Expect(
            !world::AuthoredLevelDataEqual(working, active),
            "semantic Rotate is Modified/Dirty vs active");

        const core::Vec3 localMin = editor::kStaticPropDefaultLocalMin;
        const core::Vec3 localMax = editor::kStaticPropDefaultLocalMax;
        core::Vec3 rotatedCenter{};
        core::Vec3 rotatedSize{};
        editor::StaticPropWorldAabb(
            working.staticProps[0], localMin, localMax, rotatedCenter, rotatedSize);
        Expect(
            rotatedSize.x > 0.0f && rotatedSize.y > 0.0f && rotatedSize.z > 0.0f
                && std::isfinite(rotatedCenter.x) && std::isfinite(rotatedSize.x),
            "transformed editor bounds remain useful after Rotate");

        Expect(
            editor::UpdateRotateInteraction(
                state,
                {EditorObjectKind::Ground, 0},
                working,
                propView,
                RayThrough(propView, xSwept),
                false,
                false,
                false,
                true,
                false),
            "selection change consumes the pointer while cancelling");
        Expect(!state.dragging, "selection change cancels Rotate drag");

        editor::GizmoInteractionState modeDrag{};
        Expect(
            editor::BeginRotateDrag(
                modeDrag, prop0, EditorAxis::X, propOrigin, startRotation, RayThrough(propView, xRing)),
            "mode-change fixture drag");
        Expect(
            editor::UpdateRotateInteraction(
                modeDrag,
                prop0,
                working,
                propView,
                RayThrough(propView, xRing),
                false,
                false,
                false,
                false,
                true),
            "mouse-up during Rotate ends drag");
        Expect(!modeDrag.dragging, "mode change is safe after mouse-up ends Rotate drag");

        working.staticProps[0].rotationDegrees = startRotation;
        Expect(
            world::AuthoredLevelDataEqual(working, active)
                || Vec3Near(working.staticProps[0].rotationDegrees, startRotation),
            "Revert path can restore Static Prop rotation");

        const core::Vec3 pickupStartRot = working.itemPickups[0].visualRotationDegrees;
        const core::Vec3 pickupPos = working.itemPickups[0].position;
        const core::Vec3 pickupOffset = working.itemPickups[0].visualOffset;
        const core::Vec3 pickupScale = working.itemPickups[0].visualScale;
        const core::Vec3 pickupXRing{
            pickupOrigin.x,
            pickupOrigin.y + pickupLength * unique,
            pickupOrigin.z + pickupLength * unique};
        const core::Vec3 pickupXSwept{
            pickupOrigin.x,
            pickupOrigin.y - pickupLength * unique,
            pickupOrigin.z + pickupLength * unique};
        editor::GizmoInteractionState pickupState{};
        Expect(
            editor::BeginRotateDrag(
                pickupState,
                pickup0,
                EditorAxis::X,
                pickupOrigin,
                pickupStartRot,
                RayThrough(pickupView, pickupXRing)),
            "begin Item Pickup X Rotate");
        const core::Vec3 pickupRotX =
            editor::GizmoRotateDegrees(pickupState, RayThrough(pickupView, pickupXSwept));
        Expect(std::fabs(pickupRotX.x - pickupStartRot.x) > 1.0f, "Item Pickup X ring changes visualRotation.x");
        Expect(NearlyEqual(pickupRotX.y, pickupStartRot.y, 0.25f), "Item Pickup X keeps visualRotation.y");
        Expect(NearlyEqual(pickupRotX.z, pickupStartRot.z, 0.25f), "Item Pickup X keeps visualRotation.z");
        editor::EndGizmoDrag(pickupState);

        const core::Vec3 pickupYRing{
            pickupOrigin.x + pickupLength * unique,
            pickupOrigin.y,
            pickupOrigin.z + pickupLength * unique};
        const core::Vec3 pickupYSwept{
            pickupOrigin.x + pickupLength * unique,
            pickupOrigin.y,
            pickupOrigin.z - pickupLength * unique};
        Expect(
            editor::BeginRotateDrag(
                pickupState,
                pickup0,
                EditorAxis::Y,
                pickupOrigin,
                pickupStartRot,
                RayThrough(pickupView, pickupYRing)),
            "begin Item Pickup Y Rotate");
        const core::Vec3 pickupRotY =
            editor::GizmoRotateDegrees(pickupState, RayThrough(pickupView, pickupYSwept));
        Expect(NearlyEqual(pickupRotY.x, pickupStartRot.x, 0.25f), "Item Pickup Y keeps visualRotation.x");
        Expect(std::fabs(pickupRotY.y - pickupStartRot.y) > 1.0f, "Item Pickup Y ring changes visualRotation.y");
        Expect(NearlyEqual(pickupRotY.z, pickupStartRot.z, 0.25f), "Item Pickup Y keeps visualRotation.z");
        editor::EndGizmoDrag(pickupState);

        const core::Vec3 pickupZRing{
            pickupOrigin.x + pickupLength * unique,
            pickupOrigin.y + pickupLength * unique,
            pickupOrigin.z};
        const core::Vec3 pickupZSwept{
            pickupOrigin.x - pickupLength * unique,
            pickupOrigin.y + pickupLength * unique,
            pickupOrigin.z};
        Expect(
            editor::BeginRotateDrag(
                pickupState,
                pickup0,
                EditorAxis::Z,
                pickupOrigin,
                pickupStartRot,
                RayThrough(pickupView, pickupZRing)),
            "begin Item Pickup Z Rotate");
        const core::Vec3 pickupRotZ =
            editor::GizmoRotateDegrees(pickupState, RayThrough(pickupView, pickupZSwept));
        Expect(NearlyEqual(pickupRotZ.x, pickupStartRot.x, 0.25f), "Item Pickup Z keeps visualRotation.x");
        Expect(NearlyEqual(pickupRotZ.y, pickupStartRot.y, 0.25f), "Item Pickup Z keeps visualRotation.y");
        Expect(std::fabs(pickupRotZ.z - pickupStartRot.z) > 1.0f, "Item Pickup Z ring changes visualRotation.z");

        working.itemPickups[0].visualRotationDegrees = pickupRotY;
        Expect(Vec3Near(working.itemPickups[0].position, pickupPos), "Item Pickup Rotate leaves position");
        Expect(
            Vec3Near(working.itemPickups[0].visualOffset, pickupOffset),
            "Item Pickup Rotate leaves visualOffset");
        Expect(
            Vec3Near(working.itemPickups[0].visualScale, pickupScale),
            "Item Pickup Rotate leaves visualScale");
        Expect(
            editor::GetEditablePosition(working, pickup0) == &working.itemPickups[0].position,
            "Translate still edits gameplay position");
        Expect(
            editor::GetEditableScale(working, pickup0) == &working.itemPickups[0].visualScale,
            "Scale still edits visualScale");
        Expect(
            editor::GetEditableRotation(working, pickup0)
                == &working.itemPickups[0].visualRotationDegrees,
            "Inspector Visual Rotation is the same workingCopy field");
        const world::StaticPropSpec visualProp = world::ItemPickupVisualProp(working.itemPickups[0]);
        Expect(
            Vec3Near(visualProp.rotationDegrees, working.itemPickups[0].visualRotationDegrees),
            "rendered staged GLB follows visualRotationDegrees");
        Expect(
            Vec3Near(visualProp.position, pickupOrigin),
            "rendered model origin stays position + visualOffset");
        Expect(
            NearlyEqual(working.itemPickups[0].position.x, pickupPos.x)
                && NearlyEqual(working.itemPickups[0].position.y, pickupPos.y)
                && NearlyEqual(working.itemPickups[0].position.z, pickupPos.z),
            "gameplay targeting position is unchanged by Rotate");

        editor::GizmoInteractionState inert{};
        const core::Vec3 platformCenter = working.elevatedPlatforms[0].center;
        Expect(
            !editor::UpdateRotateInteraction(
                inert,
                {EditorObjectKind::ElevatedPlatform, 0},
                working,
                propView,
                RayThrough(propView, xRing),
                false,
                false,
                true,
                true,
                false),
            "Rotate is inert for unsupported selection");
        Expect(Vec3Near(working.elevatedPlatforms[0].center, platformCenter), "inert Rotate does not mutate Platform");
        Expect(!inert.dragging, "inert Rotate does not start a drag");
    }

    // ---- M35 Phase A nudge ----
    {
        Expect(NearlyEqual(editor::NudgeStep(false), 0.10f, 0.0001f), "normal nudge is 0.10");
        Expect(NearlyEqual(editor::NudgeStep(true), 0.01f, 0.0001f), "precision nudge is 0.01");
        world::LevelDefinition working = MakeStubLevel();
        const world::LevelDefinition active = working;
        EditorSelection spawn{EditorObjectKind::Spawn, 0};
        Expect(
            editor::ApplyNudge(working, spawn, EditorAxis::X, 1.0f, false), "nudge spawn +X");
        Expect(NearlyEqual(working.initialSpawnVisualCenter.x, 0.10f, 0.0001f), "nudge +X step");
        Expect(NearlyEqual(working.initialSpawnVisualCenter.y, 0.8f), "nudge X keeps Y");
        Expect(NearlyEqual(working.initialSpawnVisualCenter.z, 0.0f), "nudge X keeps Z");
        Expect(
            editor::ApplyNudge(working, spawn, EditorAxis::X, -1.0f, false), "nudge spawn -X");
        Expect(NearlyEqual(working.initialSpawnVisualCenter.x, 0.0f, 0.0001f), "nudge -X restores");
        Expect(
            editor::ApplyNudge(working, spawn, EditorAxis::Y, 1.0f, true), "precision +Y");
        Expect(NearlyEqual(working.initialSpawnVisualCenter.y, 0.81f, 0.0001f), "precision Y step");
        Expect(
            editor::ApplyNudge(working, spawn, EditorAxis::Z, -1.0f, false), "nudge -Z");
        Expect(NearlyEqual(working.initialSpawnVisualCenter.z, -0.10f, 0.0001f), "nudge -Z step");
        Expect(NearlyEqual(active.initialSpawnVisualCenter.x, 0.0f), "nudge does not touch a copy of active");
        Expect(
            !editor::ApplyNudge(working, {EditorObjectKind::Camera, 0}, EditorAxis::X, 1.0f, false),
            "camera cannot nudge");
        Expect(
            editor::ApplyNudge(
                working, {EditorObjectKind::Ground, 0}, EditorAxis::X, 1.0f, false),
            "ground can nudge");
        Expect(std::isfinite(working.ground.center.x), "nudge result is finite");
        Expect(
            editor::NudgeAllowed(editor::EditorTransformMode::Translate, false, false),
            "nudge allowed in Translate");
        Expect(
            !editor::NudgeAllowed(editor::EditorTransformMode::Resize, false, false),
            "nudge blocked in Resize mode");
        Expect(
            !editor::NudgeAllowed(editor::EditorTransformMode::Scale, false, false),
            "nudge blocked in Scale mode");
        Expect(
            !editor::NudgeAllowed(editor::EditorTransformMode::Rotate, false, false),
            "nudge blocked in Rotate mode");
        Expect(
            !editor::NudgeAllowed(editor::EditorTransformMode::Translate, true, false),
            "nudge blocked when ImGui wants keyboard");
        Expect(
            !editor::NudgeAllowed(editor::EditorTransformMode::Translate, false, true),
            "nudge blocked during gizmo drag");
        const core::Vec3 spawnBefore = working.initialSpawnVisualCenter;
        Expect(
            !editor::ApplyNudge(
                working,
                spawn,
                EditorAxis::X,
                1.0f,
                false,
                editor::EditorTransformMode::Resize),
            "Resize mode ApplyNudge is a no-op");
        Expect(
            Vec3Near(working.initialSpawnVisualCenter, spawnBefore, 0.0001f),
            "Resize-mode nudge leaves working center");
        Expect(
            !editor::ApplyNudge(
                working,
                spawn,
                EditorAxis::X,
                1.0f,
                false,
                editor::EditorTransformMode::Rotate),
            "Rotate mode ApplyNudge is a no-op");
        Expect(
            Vec3Near(working.initialSpawnVisualCenter, spawnBefore, 0.0001f),
            "Rotate-mode nudge leaves working center");
    }

    // ---- resize handle pick identifies axis and sign ----
    {
        const core::Vec3 origin{};
        const float length = 2.0f;
        const float hitRadius = length * editor::kResizeHandleHitFraction;
        editor::Ray3 plusX{{length, 0.0f, -4.0f}, {0.0f, 0.0f, 1.0f}};
        const editor::ResizeHandlePick plus = editor::PickResizeHandle(plusX, origin, length, hitRadius);
        Expect(plus.axis == EditorAxis::X && plus.sign == 1, "+X cube pick");
        editor::Ray3 minusX{{-length, 0.0f, -4.0f}, {0.0f, 0.0f, 1.0f}};
        const editor::ResizeHandlePick minus =
            editor::PickResizeHandle(minusX, origin, length, hitRadius);
        Expect(minus.axis == EditorAxis::X && minus.sign == -1, "-X cube pick");
        editor::Ray3 miss{{8.0f, 8.0f, 8.0f}, {0.0f, 1.0f, 0.0f}};
        Expect(
            editor::PickResizeHandle(miss, origin, length, hitRadius).axis == EditorAxis::None,
            "missed resize handle");
    }

    // ---- Checkpoint gizmo Translate preserves trigger/respawn offset ----
    {
        world::LevelDefinition working{};
        const core::Vec3 center{16.5f, 1.8f, 0.0f};
        const core::Vec3 respawn{18.0f, 0.8f, 1.25f};
        const core::Vec3 delta{2.0f, -0.5f, 3.0f};
        working.checkpoints.push_back({center, {2.4f, 1.6f, 2.0f}, respawn});
        Expect(
            editor::SetCheckpointAssemblyCenter(
                working, 0, {center.x + delta.x, center.y + delta.y, center.z + delta.z}),
            "checkpoint assembly translate");
        const world::CheckpointSpec& moved = working.checkpoints[0];
        Expect(Vec3Near(moved.center, {center.x + delta.x, center.y + delta.y, center.z + delta.z}),
            "new center is C + D");
        Expect(
            Vec3Near(
                moved.respawnPosition,
                {respawn.x + delta.x, respawn.y + delta.y, respawn.z + delta.z}),
            "new respawn is R + D");
        Expect(
            Vec3Near(
                editor::Sub(moved.respawnPosition, moved.center),
                editor::Sub(respawn, center)),
            "gizmo translate preserves respawn-center offset");
    }

    {
        world::LevelDefinition working{};
        const core::Vec3 center{4.0f, 2.0f, -1.0f};
        const core::Vec3 respawn{4.5f, 1.0f, -1.0f};
        working.checkpoints.push_back({center, {2.4f, 1.6f, 2.0f}, respawn});
        Expect(
            editor::ApplyNudge(
                working,
                {EditorObjectKind::Checkpoint, 0},
                EditorAxis::X,
                1.0f,
                false),
            "nudge checkpoint +X");
        Expect(NearlyEqual(working.checkpoints[0].center.x, center.x + editor::kNudgeStep),
            "nudge moves trigger center");
        Expect(
            NearlyEqual(working.checkpoints[0].respawnPosition.x, respawn.x + editor::kNudgeStep),
            "nudge moves respawn by the same delta");
        Expect(
            NearlyEqual(
                working.checkpoints[0].respawnPosition.y - working.checkpoints[0].center.y,
                respawn.y - center.y),
            "nudge preserves Y offset");
    }

    // ---- Inspector-style center edit does not rewrite respawn ----
    {
        world::LevelDefinition working{};
        const core::Vec3 center{10.0f, 1.8f, 0.0f};
        const core::Vec3 respawn{12.0f, 0.8f, 2.0f};
        working.checkpoints.push_back({center, {2.4f, 1.6f, 2.0f}, respawn});
        core::Vec3* position =
            editor::GetEditablePosition(working, {EditorObjectKind::Checkpoint, 0});
        Expect(position != nullptr, "inspector can bind trigger center");
        if (position != nullptr)
        {
            *position = {11.0f, 2.5f, -0.5f};
        }
        Expect(Vec3Near(working.checkpoints[0].center, {11.0f, 2.5f, -0.5f}), "center write sticks");
        Expect(Vec3Near(working.checkpoints[0].respawnPosition, respawn),
            "direct center edit leaves respawn unchanged");
    }

    // ---- pending Checkpoint overlay reads workingCopy respawn, not active ----
    {
        world::LevelDefinition active{};
        world::LevelDefinition working{};
        active.checkpoints.push_back(
            {{16.5f, 1.8f, 0.0f}, {2.4f, 1.6f, 2.0f}, {16.5f, 1.8f, 0.0f}});
        working.checkpoints.push_back(
            {{20.0f, 2.2f, 1.0f}, {2.4f, 1.6f, 2.0f}, {21.5f, 0.9f, 2.0f}});
        const editor::CheckpointEditorOverlay overlay = editor::MakeCheckpointEditorOverlay(
            {EditorObjectKind::Checkpoint, 0}, working);
        Expect(overlay.visible, "selected checkpoint overlay is visible");
        Expect(Vec3Near(overlay.triggerCenter, working.checkpoints[0].center),
            "overlay trigger from workingCopy");
        Expect(Vec3Near(overlay.triggerSize, working.checkpoints[0].size),
            "overlay size from workingCopy");
        Expect(Vec3Near(overlay.respawnPosition, working.checkpoints[0].respawnPosition),
            "overlay respawn from workingCopy");
        Expect(
            !Vec3Near(overlay.respawnPosition, active.checkpoints[0].respawnPosition),
            "overlay respawn is not active");
        const editor::CheckpointEditorOverlay hidden = editor::MakeCheckpointEditorOverlay(
            {EditorObjectKind::Hazard, 0}, working);
        Expect(!hidden.visible, "non-checkpoint selection has no overlay");
    }

    // ---- pending object visuals: workingCopy authority, no Platform duplicate ----
    {
        Expect(
            !editor::HasDistinctPendingObjectVisual(EditorObjectKind::ElevatedPlatform),
            "platform visual == bounds; no second object ghost");
        Expect(
            !editor::HasDistinctPendingObjectVisual(EditorObjectKind::Ground),
            "ground has no distinct object visual");
        Expect(
            editor::HasDistinctPendingObjectVisual(EditorObjectKind::Checkpoint),
            "checkpoint has distinct object visual");
        Expect(
            editor::HasDistinctPendingObjectVisual(EditorObjectKind::Hazard),
            "hazard has distinct object visual");
        Expect(
            editor::HasDistinctPendingObjectVisual(EditorObjectKind::Collectible),
            "collectible has distinct object visual");
    }

    {
        world::LevelDefinition active = MakeStubLevel();
        world::LevelDefinition working = active;
        working.elevatedPlatforms[0].center.x = 9.0f;
        const EditorSelection platform0{EditorObjectKind::ElevatedPlatform, 0};
        const editor::EditorPendingTransformPreview bounds =
            editor::MakePendingTransformPreview(platform0, active, working);
        const editor::EditorPendingObjectVisual objectVisual =
            editor::MakePendingObjectVisual(platform0, active, working);
        Expect(bounds.visible, "platform pending bounds remain");
        Expect(!objectVisual.visible, "platform does not get a redundant object ghost");
        Expect(NearlyEqual(active.elevatedPlatforms[0].center.x, 5.0f), "platform active unchanged");
    }

    {
        world::LevelDefinition active{};
        world::LevelDefinition working{};
        active.checkpoints.push_back(
            {{16.5f, 1.8f, 0.0f}, {2.4f, 1.6f, 2.0f}, {16.5f, 1.8f, 0.0f}});
        working = active;
        working.checkpoints[0].center = {20.0f, 2.2f, 1.0f};
        working.checkpoints[0].respawnPosition = {21.5f, 0.9f, 2.0f};
        const EditorSelection checkpoint0{EditorObjectKind::Checkpoint, 0};
        const editor::EditorPendingObjectVisual visual =
            editor::MakePendingObjectVisual(checkpoint0, active, working);
        Expect(visual.visible, "moved checkpoint has object ghost");
        Expect(visual.kind == editor::PendingObjectVisualKind::Checkpoint, "checkpoint visual kind");
        Expect(Vec3Near(visual.checkpoint.center, working.checkpoints[0].center),
            "checkpoint visual center from workingCopy");
        Expect(
            Vec3Near(visual.checkpoint.respawnPosition, working.checkpoints[0].respawnPosition),
            "checkpoint visual respawn from workingCopy");
        Expect(
            !Vec3Near(visual.checkpoint.center, active.checkpoints[0].center),
            "checkpoint visual is not active center");
        Expect(Vec3Near(active.checkpoints[0].center, {16.5f, 1.8f, 0.0f}),
            "active checkpoint unchanged after workingCopy move");
        const world::CheckpointMarkerLayout pendingLayout =
            world::MakeCheckpointMarkerLayout(visual.checkpoint);
        const world::CheckpointMarkerLayout activeLayout =
            world::MakeCheckpointMarkerLayout(active.checkpoints[0]);
        Expect(!Vec3Near(pendingLayout.postCenter, activeLayout.postCenter),
            "pending checkpoint marker follows workingCopy, not active");
        Expect(
            NearlyEqual(pendingLayout.postCenter.x, working.checkpoints[0].center.x),
            "pending post X follows workingCopy trigger center");
    }

    {
        world::LevelDefinition active{};
        world::LevelDefinition working{};
        active.checkpoints.push_back(
            {{16.5f, 1.8f, 0.0f}, {2.4f, 1.6f, 2.0f}, {16.5f, 1.8f, 0.0f}});
        working = active;
        working.checkpoints[0].size = {3.0f, 2.0f, 2.5f};
        const EditorSelection checkpoint0{EditorObjectKind::Checkpoint, 0};
        Expect(
            editor::MakePendingTransformPreview(checkpoint0, active, working).visible,
            "checkpoint size edit still shows bounds");
        Expect(
            !editor::MakePendingObjectVisual(checkpoint0, active, working).visible,
            "checkpoint marker does not scale with trigger size");
    }

    {
        world::LevelDefinition active{};
        world::LevelDefinition working{};
        working.checkpoints.push_back(
            {{2.0f, 1.8f, 0.0f}, {2.4f, 1.6f, 2.0f}, {2.0f, 1.8f, 0.0f}});
        const EditorSelection added{EditorObjectKind::Checkpoint, 0};
        const editor::EditorPendingTransformPreview bounds =
            editor::MakePendingTransformPreview(added, active, working);
        const editor::EditorPendingObjectVisual visual =
            editor::MakePendingObjectVisual(added, active, working);
        Expect(bounds.visible, "added checkpoint has pending bounds");
        Expect(visual.visible, "added checkpoint has pending object visual");
        Expect(Vec3Near(visual.checkpoint.center, working.checkpoints[0].center),
            "add preview uses workingCopy checkpoint");
        Expect(active.checkpoints.empty(), "add preview has no active counterpart");
    }

    {
        world::LevelDefinition active{};
        world::LevelDefinition working{};
        active.checkpoints.push_back(
            {{16.5f, 1.8f, 0.0f}, {2.4f, 1.6f, 2.0f}, {18.0f, 0.8f, 1.0f}});
        working = active;
        world::CheckpointSpec copy = working.checkpoints[0];
        copy.center.x += 1.0f;
        copy.respawnPosition.x += 1.0f;
        working.checkpoints.push_back(copy);
        const EditorSelection duplicate{EditorObjectKind::Checkpoint, 1};
        const editor::EditorPendingObjectVisual visual =
            editor::MakePendingObjectVisual(duplicate, active, working);
        Expect(visual.visible, "duplicate checkpoint has object visual");
        Expect(Vec3Near(visual.checkpoint.center, working.checkpoints[1].center),
            "duplicate visual at workingCopy transform");
        Expect(Vec3Near(active.checkpoints[0].center, {16.5f, 1.8f, 0.0f}),
            "duplicate leaves original active unchanged");
        Expect(active.checkpoints.size() == 1, "duplicate has no active counterpart");
    }

    {
        world::LevelDefinition active{};
        world::LevelDefinition working{};
        active.hazards.push_back({{11.5f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
        working = active;
        working.hazards[0].center.x = 14.0f;
        const EditorSelection hazard0{EditorObjectKind::Hazard, 0};
        const editor::EditorPendingObjectVisual visual =
            editor::MakePendingObjectVisual(hazard0, active, working);
        Expect(visual.visible, "moved hazard has object ghost");
        Expect(visual.kind == editor::PendingObjectVisualKind::Hazard, "hazard visual kind");
        Expect(Vec3Near(visual.hazard.center, working.hazards[0].center),
            "hazard visual from workingCopy");
        Expect(NearlyEqual(active.hazards[0].center.x, 11.5f), "active hazard unchanged");
        world::LevelDefinition noHazards{};
        world::LevelDefinition addedOnly{};
        addedOnly.hazards.push_back({{3.0f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
        const editor::EditorPendingObjectVisual added =
            editor::MakePendingObjectVisual({EditorObjectKind::Hazard, 0}, noHazards, addedOnly);
        Expect(added.visible, "added hazard has pending object visual");
        Expect(Vec3Near(added.hazard.center, addedOnly.hazards[0].center),
            "added hazard visual from workingCopy");
        Expect(noHazards.hazards.empty(), "added hazard has no active counterpart");
    }

    {
        world::LevelDefinition active{};
        world::LevelDefinition working{};
        active.collectibles.push_back({{5.0f, 2.5f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        working = active;
        working.collectibles[0].center.x = 8.0f;
        const EditorSelection collectible0{EditorObjectKind::Collectible, 0};
        const editor::EditorPendingObjectVisual visual =
            editor::MakePendingObjectVisual(collectible0, active, working);
        Expect(visual.visible, "moved collectible has object ghost");
        Expect(visual.kind == editor::PendingObjectVisualKind::Collectible, "collectible visual kind");
        Expect(Vec3Near(visual.collectible.center, working.collectibles[0].center),
            "collectible visual from workingCopy");
        Expect(NearlyEqual(active.collectibles[0].center.x, 5.0f), "active collectible unchanged");
        working.collectibles[0].center = active.collectibles[0].center;
        working.collectibles[0].size.x = 2.0f;
        Expect(
            editor::MakePendingTransformPreview(collectible0, active, working).visible,
            "collectible size edit shows bounds");
        Expect(
            !editor::MakePendingObjectVisual(collectible0, active, working).visible,
            "collectible cube does not scale with authored size");
        world::LevelDefinition noCollectibles{};
        world::LevelDefinition addedOnly{};
        addedOnly.collectibles.push_back({{1.0f, 2.0f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        const editor::EditorPendingObjectVisual added = editor::MakePendingObjectVisual(
            {EditorObjectKind::Collectible, 0}, noCollectibles, addedOnly);
        Expect(added.visible, "added collectible has pending object visual");
        Expect(Vec3Near(added.collectible.center, addedOnly.collectibles[0].center),
            "added collectible visual from workingCopy");
        Expect(noCollectibles.collectibles.empty(), "added collectible has no active counterpart");
    }

    {
        world::LevelDefinition active{};
        active.collectibles.push_back({{5.0f, 2.5f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        active.collectibles.push_back({{8.0f, 2.5f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        std::uint8_t collected[2] = {1, 0};
        Expect(editor::ShouldDrawEditorAuthoredCollectible(collected[0]),
            "collected flag requests editor authored visual");
        Expect(!editor::ShouldDrawEditorAuthoredCollectible(collected[1]),
            "uncollected uses runtime visual only");
        core::Vec3 centers[8]{};
        const int drawn = editor::CollectEditorAuthoredCollectibleCenters(
            active, collected, 2, centers, 8);
        Expect(drawn == 1, "only collected authored collectibles get editor cubes");
        Expect(Vec3Near(centers[0], active.collectibles[0].center),
            "editor authored visual uses active center");
        Expect(collected[0] == 1 && collected[1] == 0, "visualization helper does not mutate collected flags");

        std::uint8_t allAvailable[2] = {0, 0};
        const int uncollectedDrawn = editor::CollectEditorAuthoredCollectibleCenters(
            active, allAvailable, 2, centers, 8);
        Expect(uncollectedDrawn == 0,
            "uncollected policy: no second opaque cube at the runtime visual");

        Expect(
            editor::IsValidSelection(active, {EditorObjectKind::Collectible, 0}),
            "collected authored collectible is a valid selection");
        Expect(
            editor::GetEditablePosition(active, {EditorObjectKind::Collectible, 0}) != nullptr,
            "Inspector can still read collected collectible center");
        const core::Vec3* inspectorCenter =
            editor::GetEditablePosition(active, {EditorObjectKind::Collectible, 0});
        Expect(
            inspectorCenter != nullptr && inspectorCenter->x == active.collectibles[0].center.x,
            "Inspector center is workingCopy/active authored center");
        Expect(active.collectibles[0].size.x > 0.0f, "Inspector size remains in authored spec");
    }

    {
        world::LevelDefinition active{};
        active.collectibles.push_back({{5.0f, 2.5f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        active.collectibles.push_back({{8.0f, 2.5f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        std::uint8_t collected[2] = {1, 1};
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        Expect(editor::DeleteSelected(working, {EditorObjectKind::Collectible, 0}).succeeded,
            "delete collected collectible");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::Collectible, true, 0);
        core::Vec3 centers[8]{};
        const int drawn = editor::CollectEditorAuthoredCollectibleCenters(
            active, collected, 2, centers, 8, &map);
        Expect(drawn == 1, "pending-delete collected collectible is not gold authored wire");
        Expect(Vec3Near(centers[0], active.collectibles[1].center),
            "surviving collected collectible keeps authored collected visual");
        Expect(
            editor::ResolveCollectibleEditorVisualMode(
                editor::IsPendingDeleteActiveIndex(map, EditorObjectKind::Collectible, 0),
                collected[0])
                == editor::CollectibleEditorVisualMode::PendingDelete,
            "collected+pending-delete precedence");
        Expect(
            editor::ResolveCollectibleEditorVisualMode(
                editor::IsPendingDeleteActiveIndex(map, EditorObjectKind::Collectible, 1),
                collected[1])
                == editor::CollectibleEditorVisualMode::CollectedAuthored,
            "surviving collected keeps collected authored style");
    }

    {
        world::LevelDefinition active{};
        active.collectibles.push_back({{5.0f, 2.5f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        Expect(editor::AddCollectible(working, {1.0f, 4.0f, 0.0f}).succeeded, "add ghost regression");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::Collectible, false, 0);
        const editor::EditorPendingObjectVisual added = editor::MakePendingObjectVisual(
            {EditorObjectKind::Collectible, 1}, active, working, map);
        Expect(added.visible && added.kind == editor::PendingObjectVisualKind::Collectible,
            "pending Add still uses cyan pending object visual");
        Expect(editor::MakePendingDeleteVisuals(active, map).collectibleCenters.empty(),
            "pending Add is not faded pending-delete");
        Expect(
            editor::ResolveAuthoredEditorVisualMode(
                editor::IsPendingDeleteActiveIndex(map, EditorObjectKind::Collectible, 0))
                == editor::AuthoredEditorVisualMode::Normal,
            "existing collectible stays normal after Add");
    }

    {
        world::LevelDefinition active{};
        active.checkpoints.push_back(
            {{16.5f, 1.8f, 0.0f}, {2.4f, 1.6f, 2.0f}, {18.0f, 0.8f, 1.0f}});
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        Expect(
            editor::DuplicateSelected(working, {EditorObjectKind::Checkpoint, 0}).succeeded,
            "duplicate ghost regression");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::Checkpoint, false, 0);
        const editor::EditorPendingObjectVisual visual = editor::MakePendingObjectVisual(
            {EditorObjectKind::Checkpoint, 1}, active, working, map);
        Expect(visual.visible && visual.kind == editor::PendingObjectVisualKind::Checkpoint,
            "pending Duplicate still uses cyan pending object visual");
        Expect(editor::MakePendingDeleteVisuals(active, map).checkpoints.empty(),
            "pending Duplicate is not faded pending-delete");
        Expect(
            editor::ResolveAuthoredEditorVisualMode(
                editor::IsPendingDeleteActiveIndex(map, EditorObjectKind::Checkpoint, 0))
                == editor::AuthoredEditorVisualMode::Normal,
            "original checkpoint stays normal after Duplicate");
    }

    {
        world::LevelDefinition active{};
        active.elevatedPlatforms.push_back({{0.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        active.collectibles.push_back({{5.0f, 2.5f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        active.checkpoints.push_back(
            {{16.5f, 1.8f, 0.0f}, {2.4f, 1.6f, 2.0f}, {18.0f, 0.8f, 1.0f}});
        active.hazards.push_back({{11.5f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        Expect(editor::AddPlatform(working, {2.0f, 4.0f, 0.0f}).succeeded, "persist add platform");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::ElevatedPlatform, false, 0);
        Expect(editor::AddCollectible(working, {1.0f, 4.0f, 0.0f}).succeeded, "persist add collectible");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::Collectible, false, 0);
        Expect(editor::AddCheckpoint(working, {3.0f, 4.0f, 0.0f}).succeeded, "persist add checkpoint");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::Checkpoint, false, 0);
        Expect(editor::AddHazard(working, {4.0f, 4.0f, 0.0f}).succeeded, "persist add hazard");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::Hazard, false, 0);

        const editor::EditorSelection other{EditorObjectKind::ElevatedPlatform, 0};
        const std::vector<editor::PendingAuthoringVisual> afterDeselect =
            editor::CollectPendingAuthoringVisuals(active, working, map, other);
        Expect(
            editor::PendingAuthoringContains(
                afterDeselect, EditorObjectKind::ElevatedPlatform, 1),
            "pending Add Platform remains after deselect");
        Expect(
            editor::PendingAuthoringContains(afterDeselect, EditorObjectKind::Collectible, 1),
            "pending Add Collectible remains after deselect");
        Expect(
            editor::PendingAuthoringContains(afterDeselect, EditorObjectKind::Checkpoint, 1),
            "pending Add Checkpoint remains after deselect");
        Expect(
            editor::PendingAuthoringContains(afterDeselect, EditorObjectKind::Hazard, 1),
            "pending Add Hazard remains after deselect");
        const editor::PendingAuthoringVisual* selected =
            editor::FindPendingAuthoringVisual(
                afterDeselect, EditorObjectKind::ElevatedPlatform, 0);
        Expect(selected == nullptr, "unmodified original platform is not pending");
        const editor::PendingAuthoringVisual* addedPlatform =
            editor::FindPendingAuthoringVisual(
                afterDeselect, EditorObjectKind::ElevatedPlatform, 1);
        Expect(
            addedPlatform != nullptr && !addedPlatform->selected,
            "unselected pending Add uses unselected emphasis");
        const std::vector<editor::PendingAuthoringVisual> selectedAdd =
            editor::CollectPendingAuthoringVisuals(
                active, working, map, {EditorObjectKind::Collectible, 1});
        const editor::PendingAuthoringVisual* selectedCollectible =
            editor::FindPendingAuthoringVisual(
                selectedAdd, EditorObjectKind::Collectible, 1);
        Expect(
            selectedCollectible != nullptr && selectedCollectible->selected,
            "selected pending Add uses selected emphasis");
    }

    {
        world::LevelDefinition active{};
        active.collectibles.push_back({{5.0f, 2.5f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        Expect(
            editor::DuplicateSelected(working, {EditorObjectKind::Collectible, 0}).succeeded,
            "persist duplicate");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::Collectible, false, 0);
        const std::vector<editor::PendingAuthoringVisual> visuals =
            editor::CollectPendingAuthoringVisuals(
                active, working, map, {EditorObjectKind::Collectible, 0});
        Expect(
            editor::PendingAuthoringContains(visuals, EditorObjectKind::Collectible, 1),
            "pending Duplicate remains after selecting original");
    }

    {
        world::LevelDefinition active{};
        active.hazards.push_back({{11.5f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        working.hazards[0].center.x = 14.0f;
        const std::vector<editor::PendingAuthoringVisual> visuals =
            editor::CollectPendingAuthoringVisuals(
                active, working, map, {EditorObjectKind::ElevatedPlatform, 0});
        Expect(
            editor::PendingAuthoringContains(visuals, EditorObjectKind::Hazard, 0),
            "pending Modify remains after deselect");
        const editor::PendingAuthoringVisual* modified =
            editor::FindPendingAuthoringVisual(visuals, EditorObjectKind::Hazard, 0);
        Expect(modified != nullptr && !modified->selected, "unselected pending Modify");
    }

    {
        world::LevelDefinition active{};
        active.collectibles.push_back({{0.0f, 2.0f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        active.collectibles.push_back({{1.0f, 2.0f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        Expect(editor::DeleteSelected(working, {EditorObjectKind::Collectible, 0}).succeeded,
            "delete for precedence");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::Collectible, true, 0);
        const std::vector<editor::PendingAuthoringVisual> visuals =
            editor::CollectPendingAuthoringVisuals(active, working, map, {});
        Expect(
            !editor::PendingAuthoringContains(visuals, EditorObjectKind::Collectible, 0),
            "pending-deleted object is not a cyan pending Add/Modify");
        Expect(
            editor::IsPendingDeleteActiveIndex(map, EditorObjectKind::Collectible, 0),
            "pending-delete identity remains");
    }

    {
        world::LevelDefinition active{};
        active.collectibles.push_back({{5.0f, 2.5f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        Expect(editor::AddCollectible(working, {1.0f, 4.0f, 0.0f}).succeeded, "add then apply");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::Collectible, false, 0);
        Expect(
            !editor::CollectPendingAuthoringVisuals(active, working, map, {}).empty(),
            "pending Add visible before Apply");
        active = working;
        editor::ResetStructuralIndexMap(map, active);
        Expect(
            editor::CollectPendingAuthoringVisuals(active, working, map, {}).empty(),
            "Apply clears pending authoring visuals");
        Expect(editor::MakePendingDeleteVisuals(active, map).collectibleCenters.empty(),
            "Apply clears pending-delete visuals");
    }

    {
        world::LevelDefinition active{};
        active.collectibles.push_back({{5.0f, 2.5f, 0.0f}, {1.0f, 1.2f, 1.0f}});
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        world::LevelDefinition working = active;
        Expect(editor::AddCollectible(working, {1.0f, 4.0f, 0.0f}).succeeded, "add then revert");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::Collectible, false, 0);
        working.collectibles[0].center.x = 9.0f;
        Expect(
            !editor::CollectPendingAuthoringVisuals(active, working, map, {}).empty(),
            "pending Add/Modify visible before Revert");
        working = active;
        editor::ResetStructuralIndexMap(map, active);
        Expect(
            editor::CollectPendingAuthoringVisuals(active, working, map, {}).empty(),
            "Revert clears pending Add/Modify visuals");
        Expect(editor::MakePendingDeleteVisuals(active, map).collectibleCenters.empty(),
            "Revert clears pending-delete visuals");
    }

    // ---- M76 snapping math (ImGui-free) ----
    {
        Expect(
            editor::EffectiveSnapEnabled(true, false) && !editor::EffectiveSnapEnabled(true, true),
            "Ctrl inverts enabled Snap");
        Expect(
            !editor::EffectiveSnapEnabled(false, false) && editor::EffectiveSnapEnabled(false, true),
            "Ctrl inverts disabled Snap");
        editor::EditorSnapPreferences persisted = editor::MakeDefaultEditorSnapPreferences();
        persisted.enabled = true;
        const bool invertHeld = true;
        Expect(editor::EffectiveSnapEnabled(persisted.enabled, invertHeld) == false,
            "invert disables snapping");
        Expect(persisted.enabled, "modifier does not mutate persisted Snap toggle");

        Expect(
            editor::EffectiveEditorViewportGridMinorSpacing(0.25f) == 0.25f
                && editor::EffectiveEditorViewportGridMinorSpacing(0.50f) == 0.50f,
            "M77 minor spacing follows Translate increment");
        editor::EditorViewportGridPreferences grid =
            editor::MakeDefaultEditorViewportGridPreferences();
        Expect(grid.visible, "M77 Grid defaults visible");
        Expect(
            grid.visible && !editor::EffectiveSnapEnabled(false, false),
            "Snap off does not hide Grid");
        Expect(
            editor::EditorSnapIsActive(&persisted, invertHeld) == false,
            "Ctrl inversion remains authoritative for Snap");

        Expect(
            editor::QuantizeToIncrement(1.10f, 0.25f) == 1.00f,
            "translate 0.25 snaps positive 1.10 to 1.00");
        Expect(
            editor::QuantizeToIncrement(1.13f, 0.25f) == 1.25f,
            "translate 0.25 snaps positive 1.13 to 1.25");
        Expect(
            editor::QuantizeToIncrement(-1.10f, 0.25f) == -1.00f,
            "translate 0.25 snaps negative -1.10 to -1.00");
        Expect(
            editor::QuantizeToIncrement(-1.13f, 0.25f) == -1.25f,
            "translate 0.25 snaps negative -1.13 to -1.25");
        Expect(editor::QuantizeToIncrement(1.00f, 0.25f) == 1.00f, "exact +boundary stays");
        Expect(editor::QuantizeToIncrement(-0.50f, 0.25f) == -0.50f, "exact -boundary stays");
        Expect(editor::QuantizeToIncrement(0.125f, 0.25f) == 0.25f,
            "half increment +0.125 rounds away from zero");
        Expect(editor::QuantizeToIncrement(-0.125f, 0.25f) == -0.25f,
            "half increment -0.125 rounds away from zero");
        Expect(editor::QuantizeToIncrement(0.124f, 0.25f) == 0.0f, "just below +half goes toward zero");
        Expect(editor::QuantizeToIncrement(0.126f, 0.25f) == 0.25f, "just above +half goes away from zero");

        float repeated = editor::QuantizeToIncrement(0.37f, 0.25f);
        Expect(repeated == 0.25f, "0.37 snaps to 0.25");
        for (int i = 0; i < 64; ++i)
        {
            repeated = editor::QuantizeToIncrement(repeated, 0.25f);
        }
        Expect(repeated == 0.25f, "repeated quantize does not drift");

        Expect(
            NearlyEqual(editor::QuantizeToIncrement(0.14f, 0.10f), 0.10f, 0.0001f),
            "scale 0.10 snaps 0.14 to 0.10");
        Expect(
            NearlyEqual(editor::QuantizeToIncrement(0.16f, 0.10f), 0.20f, 0.0001f),
            "scale 0.10 snaps 0.16 to 0.20");
        Expect(editor::QuantizeToIncrement(22.0f, 15.0f) == 15.0f, "rotate 15 snaps 22 to 15");
        Expect(editor::QuantizeToIncrement(23.0f, 15.0f) == 30.0f, "rotate 15 snaps 23 to 30");
        Expect(editor::QuantizeToIncrement(-7.0f, 15.0f) == 0.0f, "rotate 15 snaps -7 to 0");
        Expect(editor::QuantizeToIncrement(-8.0f, 15.0f) == -15.0f, "rotate 15 snaps -8 to -15");

        const core::Vec3 snappedX = editor::QuantizeVec3Axis({1.10f, 2.2f, 3.3f}, EditorAxis::X, 0.25f);
        Expect(snappedX.x == 1.00f && snappedX.y == 2.2f && snappedX.z == 3.3f,
            "quantization is per-axis");

        const core::Vec3 tinyResize = editor::ApplyAuthoredTransformSnap(
            {0.05f, 1.0f, 1.0f},
            editor::EditorTransformMode::Resize,
            EditorAxis::X,
            true,
            0.25f);
        Expect(tinyResize.x == editor::kMinAuthoredBoxExtent, "resize snap still clamps minimum");
        Expect(tinyResize.y == 1.0f && tinyResize.z == 1.0f, "resize snap leaves other axes");

        const core::Vec3 snappedResize = editor::ApplyAuthoredTransformSnap(
            {1.10f, 1.0f, 1.0f},
            editor::EditorTransformMode::Resize,
            EditorAxis::X,
            true,
            0.25f);
        Expect(snappedResize.x == 1.00f, "resize 0.25 snaps 1.10 to 1.00");

        const core::Vec3 unsnappedResize = editor::ApplyAuthoredTransformSnap(
            {1.10f, 1.0f, 1.0f},
            editor::EditorTransformMode::Resize,
            EditorAxis::X,
            false,
            0.25f);
        Expect(unsnappedResize.x == 1.10f, "snap disabled preserves unsnapped resize");

        const core::Vec3 tinyScale = editor::ApplyAuthoredTransformSnap(
            {0.02f, 1.0f, 1.0f},
            editor::EditorTransformMode::Scale,
            EditorAxis::X,
            true,
            0.10f);
        Expect(tinyScale.x == world::kMinStaticPropScale, "scale snap still clamps minimum");

        const core::Vec3 snappedScale = editor::ApplyAuthoredTransformSnap(
            {1.14f, 1.0f, 1.0f},
            editor::EditorTransformMode::Scale,
            EditorAxis::X,
            true,
            0.10f);
        Expect(NearlyEqual(snappedScale.x, 1.10f, 0.0001f), "scale 0.10 snaps 1.14 to 1.10");

        const core::Vec3 unsnappedTranslate = editor::ApplyAuthoredTransformSnap(
            {1.10f, 2.0f, 3.0f},
            editor::EditorTransformMode::Translate,
            EditorAxis::X,
            false,
            0.25f);
        Expect(unsnappedTranslate.x == 1.10f, "snap disabled preserves unsnapped translate");
    }

    // ---- M76 live gizmo snap uses drag-start authority ----
    {
        world::LevelDefinition working = MakeStubLevel();
        working.elevatedPlatforms[0].center = {0.0f, 1.0f, 0.0f};
        editor::GizmoInteractionState state{};
        const render::CameraView view = MakeView({0.0f, 1.0f, 10.0f}, {0.0f, 1.0f, 0.0f});
        const EditorSelection platform0{EditorObjectKind::ElevatedPlatform, 0};
        editor::EditorSnapPreferences snap = editor::MakeDefaultEditorSnapPreferences();
        snap.enabled = true;
        Expect(
            editor::UpdateGizmoInteraction(
                state,
                platform0,
                working,
                view,
                RayThrough(view, {0.5f, 1.0f, 0.0f}),
                false,
                false,
                true,
                true,
                false,
                &snap,
                false),
            "snapped translate can begin");
        const core::Vec3 dragStart = state.dragStartPosition;
        const editor::Ray3 holdRay = RayThrough(view, {1.10f, 1.0f, 0.0f});
        const core::Vec3 intended = editor::GizmoDragPosition(state, holdRay, view);
        const core::Vec3 expected = editor::ApplyAuthoredTransformSnap(
            intended,
            editor::EditorTransformMode::Translate,
            state.active,
            true,
            0.25f);
        editor::UpdateGizmoInteraction(
            state,
            platform0,
            working,
            view,
            holdRay,
            false,
            false,
            false,
            true,
            false,
            &snap,
            false);
        Expect(working.elevatedPlatforms[0].center.x == expected.x, "live translate snap writes quantized X");
        Expect(NearlyEqual(working.elevatedPlatforms[0].center.y, 1.0f), "live translate snap keeps Y");
        Expect(Vec3Near(state.dragStartPosition, dragStart, 0.0001f), "drag-start pose is stable");
        for (int i = 0; i < 32; ++i)
        {
            editor::UpdateGizmoInteraction(
                state,
                platform0,
                working,
                view,
                holdRay,
                false,
                false,
                false,
                true,
                false,
                &snap,
                false);
        }
        Expect(working.elevatedPlatforms[0].center.x == expected.x, "repeated snapped frames do not drift");

        snap.enabled = false;
        working.elevatedPlatforms[0].center = dragStart;
        editor::EndGizmoDrag(state);
        Expect(
            editor::UpdateGizmoInteraction(
                state,
                platform0,
                working,
                view,
                RayThrough(view, {0.5f, 1.0f, 0.0f}),
                false,
                false,
                true,
                true,
                false,
                &snap,
                false),
            "unsnapped translate can begin");
        const editor::Ray3 unsnappedRay = RayThrough(view, {1.10f, 1.0f, 0.0f});
        const core::Vec3 unsnappedIntended = editor::GizmoDragPosition(state, unsnappedRay, view);
        editor::UpdateGizmoInteraction(
            state,
            platform0,
            working,
            view,
            unsnappedRay,
            false,
            false,
            false,
            true,
            false,
            &snap,
            false);
        Expect(
            working.elevatedPlatforms[0].center.x == unsnappedIntended.x,
            "snap disabled keeps unsnapped translate");
        Expect(snap.enabled == false, "unsnapped drag does not enable persisted Snap");
        editor::EndGizmoDrag(state);

        snap.enabled = false;
        working.elevatedPlatforms[0].center = {0.0f, 1.0f, 0.0f};
        Expect(
            editor::UpdateGizmoInteraction(
                state,
                platform0,
                working,
                view,
                RayThrough(view, {0.5f, 1.0f, 0.0f}),
                false,
                false,
                true,
                true,
                false,
                &snap,
                true),
            "Ctrl can temporarily enable Snap");
        const editor::Ray3 invertRay = RayThrough(view, {1.10f, 1.0f, 0.0f});
        const core::Vec3 invertIntended = editor::GizmoDragPosition(state, invertRay, view);
        const core::Vec3 invertExpected = editor::ApplyAuthoredTransformSnap(
            invertIntended,
            editor::EditorTransformMode::Translate,
            state.active,
            true,
            0.25f);
        editor::UpdateGizmoInteraction(
            state,
            platform0,
            working,
            view,
            invertRay,
            false,
            false,
            false,
            true,
            false,
            &snap,
            true);
        Expect(
            working.elevatedPlatforms[0].center.x == invertExpected.x,
            "Ctrl temporarily enables translate snap");
        Expect(snap.enabled == false, "temporary enable does not persist Snap on");
        editor::EndGizmoDrag(state);
    }

    // ---- M76 Dynamic Box authored vs runtime ----
    {
        world::LevelDefinition working{};
        working.dynamicBoxes.push_back(
            {{1.10f, 2.0f, 0.0f}, world::kDefaultDynamicBoxSize, world::kDefaultDynamicBoxMassKg});
        const core::Vec3 runtimeCenter{9.0f, 8.0f, 7.0f};
        const core::Vec3 intended = editor::ApplyAuthoredTransformSnap(
            working.dynamicBoxes[0].center,
            editor::EditorTransformMode::Translate,
            EditorAxis::X,
            true,
            0.25f);
        working.dynamicBoxes[0].center = intended;
        Expect(working.dynamicBoxes[0].center.x == 1.00f, "Dynamic Box snap writes authored center");
        Expect(runtimeCenter.x == 9.0f && runtimeCenter.y == 8.0f && runtimeCenter.z == 7.0f,
            "Dynamic Box snap does not capture runtime Jolt pose");
        Expect(!editor::IsScaleSelection({EditorObjectKind::DynamicBox, 0}),
            "Dynamic Box still has no Scale");
        Expect(!editor::IsRotateSelection({EditorObjectKind::DynamicBox, 0}),
            "Dynamic Box still has no Rotate");
    }

    // ---- M76 Item Pickup Rotate still edits visualRotationDegrees ----
    {
        world::LevelDefinition working{};
        world::ItemPickupSpec pickup{};
        pickup.position = {3.0f, 1.0f, 0.0f};
        pickup.itemId = "key";
        pickup.quantity = 1;
        pickup.visualRotationDegrees = {7.0f, 0.0f, 0.0f};
        pickup.visualScale = {1.0f, 1.0f, 1.0f};
        working.itemPickups.push_back(pickup);
        const EditorSelection pickup0{EditorObjectKind::ItemPickup, 0};
        Expect(editor::IsRotateSelection(pickup0), "Item Pickup remains Rotate-supported");
        Expect(
            editor::GetEditableRotation(working, pickup0) == &working.itemPickups[0].visualRotationDegrees,
            "Rotate authority stays visualRotationDegrees");
        Expect(
            editor::GetEditablePosition(working, pickup0) == &working.itemPickups[0].position,
            "Translate authority stays logical position");
        *editor::GetEditableRotation(working, pickup0) = editor::ApplyAuthoredTransformSnap(
            working.itemPickups[0].visualRotationDegrees,
            editor::EditorTransformMode::Rotate,
            EditorAxis::X,
            true,
            15.0f);
        Expect(working.itemPickups[0].visualRotationDegrees.x == 0.0f, "Item Pickup Rotate snaps visual X");
        Expect(working.itemPickups[0].position.x == 3.0f, "Item Pickup Rotate does not move logical position");
    }

    // ---- M76 unsupported combinations stay unsupported ----
    {
        Expect(!editor::IsGizmoSelection({EditorObjectKind::Camera, 0}), "Camera still has no Translate");
        Expect(!editor::IsResizeSelection({EditorObjectKind::Spawn, 0}), "Spawn still has no Resize");
        Expect(!editor::IsResizeSelection({EditorObjectKind::Checkpoint, 0}), "Checkpoint still has no Resize");
        Expect(!editor::IsResizeSelection({EditorObjectKind::StaticProp, 0}), "Static Prop still has no Resize");
        Expect(!editor::IsScaleSelection({EditorObjectKind::ElevatedPlatform, 0}),
            "Platform still has no Scale");
        Expect(!editor::IsRotateSelection({EditorObjectKind::Door, 0}), "Door still has no Rotate");
        Expect(!editor::IsRotateSelection({EditorObjectKind::Ground, 0}), "Ground still has no Rotate");
        Expect(editor::IsScaleSelection({EditorObjectKind::StaticProp, 0}), "Static Prop keeps Scale");
        Expect(editor::IsRotateSelection({EditorObjectKind::StaticProp, 0}), "Static Prop keeps Rotate");
        Expect(editor::IsGizmoSelection({EditorObjectKind::Goal, 0}), "Goal keeps Translate");
        Expect(editor::IsResizeSelection({EditorObjectKind::Goal, 0}), "Goal keeps Resize");
    }

    // ---- M76 layout persistence / defaults / invalid values ----
    {
        const char* oldLayout =
            "[Window][Hierarchy]\n"
            "Pos=8,8\n"
            "Size=340,400\n";
        const editor::EditorSnapPreferences fromOld =
            editor::ParseEditorSnapPreferencesFromLayoutText(oldLayout);
        Expect(fromOld.enabled, "old layout defaults Snap enabled");
        Expect(fromOld.translateIncrement == editor::kDefaultTranslateSnapIncrement,
            "old layout defaults Translate increment");
        Expect(fromOld.resizeIncrement == editor::kDefaultResizeSnapIncrement,
            "old layout defaults Resize increment");
        Expect(fromOld.scaleIncrement == editor::kDefaultScaleSnapIncrement,
            "old layout defaults Scale increment");
        Expect(fromOld.rotateIncrementDegrees == editor::kDefaultRotateSnapIncrementDegrees,
            "old layout defaults Rotate increment");

        editor::EditorSnapPreferences custom = editor::MakeDefaultEditorSnapPreferences();
        custom.enabled = false;
        custom.translateIncrement = 0.5f;
        custom.resizeIncrement = 1.0f;
        custom.scaleIncrement = 0.25f;
        custom.rotateIncrementDegrees = 45.0f;
        const std::string merged =
            editor::MergeEditorSnapPreferencesIntoLayoutText(oldLayout, custom);
        Expect(merged.find("[Window][Hierarchy]") != std::string::npos,
            "merge keeps existing ImGui windows");
        const editor::EditorSnapPreferences roundTrip =
            editor::ParseEditorSnapPreferencesFromLayoutText(merged);
        Expect(!roundTrip.enabled, "round-trip preserves Snap off");
        Expect(roundTrip.translateIncrement == 0.5f, "round-trip Translate increment");
        Expect(roundTrip.resizeIncrement == 1.0f, "round-trip Resize increment");
        Expect(NearlyEqual(roundTrip.scaleIncrement, 0.25f, 0.0001f), "round-trip Scale increment");
        Expect(roundTrip.rotateIncrementDegrees == 45.0f, "round-trip Rotate increment");

        const char* invalidLayout =
            "[Platformer3D.Snap][Settings]\n"
            "Enabled=1\n"
            "Translate=-4\n"
            "Resize=nan\n"
            "Scale=0\n"
            "Rotate=5000\n";
        const editor::EditorSnapPreferences sanitized =
            editor::ParseEditorSnapPreferencesFromLayoutText(invalidLayout);
        Expect(sanitized.translateIncrement == editor::kDefaultTranslateSnapIncrement,
            "negative increment falls back");
        Expect(sanitized.resizeIncrement == editor::kDefaultResizeSnapIncrement,
            "NaN increment falls back");
        Expect(sanitized.scaleIncrement == editor::kDefaultScaleSnapIncrement,
            "zero increment falls back");
        Expect(sanitized.rotateIncrementDegrees == editor::kMaxSnapIncrement,
            "huge increment clamps to max");

        const std::filesystem::path temp =
            std::filesystem::temp_directory_path() / "platformer3d_m76_snap";
        std::filesystem::remove_all(temp);
        std::filesystem::create_directories(temp);
        const std::filesystem::path layoutPath = temp / "editor_layout.ini";
        {
            std::ofstream out(layoutPath, std::ios::binary);
            out << oldLayout;
        }
        Expect(
            editor::LoadEditorSnapPreferencesFromLayoutPath(layoutPath).enabled,
            "missing M76 fields load defaults from disk");
        Expect(
            editor::SaveEditorSnapPreferencesToLayoutPath(layoutPath, custom),
            "layout save writes M76 section");
        const editor::EditorSnapPreferences loaded =
            editor::LoadEditorSnapPreferencesFromLayoutPath(layoutPath);
        Expect(!loaded.enabled && loaded.translateIncrement == 0.5f, "disk round-trip");
        std::string onDisk;
        {
            std::ifstream in(layoutPath);
            std::string line;
            while (std::getline(in, line))
            {
                onDisk += line;
                onDisk += '\n';
            }
        }
        Expect(onDisk.find("[Window][Hierarchy]") != std::string::npos,
            "disk merge keeps Hierarchy window");
        std::filesystem::remove_all(temp);

        const editor::EditorSnapPreferences missingFile =
            editor::LoadEditorSnapPreferencesFromLayoutPath({});
        Expect(missingFile.translateIncrement == editor::kDefaultTranslateSnapIncrement,
            "empty path uses M76 defaults");
    }

    // ---- M78 Group Translate shared snapped delta ----
    {
        world::LevelDefinition working{};
        working.elevatedPlatforms.push_back({{1.10f, 1.0f, 0.0f}, {2.0f, 0.5f, 2.0f}});
        working.elevatedPlatforms.push_back({{3.37f, 1.0f, 0.0f}, {2.0f, 0.5f, 2.0f}});
        const EditorSelection primary{EditorObjectKind::ElevatedPlatform, 0};
        const EditorSelection secondary{EditorObjectKind::ElevatedPlatform, 1};
        const std::vector<EditorSelection> members{primary, secondary};
        std::vector<core::Vec3> starts;
        Expect(editor::CaptureGroupTranslateStarts(working, members, starts), "capture starts");
        Expect(starts[0].x == 1.10f && starts[1].x == 3.37f, "starts are drag-start poses");
        const core::Vec3 snappedPrimary = editor::SnappedPrimaryTranslateResult(
            {1.20f, 1.0f, 0.0f}, EditorAxis::X, true, 0.25f);
        Expect(snappedPrimary.x == 1.25f, "M76 snaps PRIMARY result only");
        const core::Vec3 delta = editor::SharedTranslationDelta(starts[0], snappedPrimary);
        Expect(NearlyEqual(delta.x, 0.15f, 0.0001f), "shared delta is primary snapped minus start");
        Expect(
            editor::ApplySharedTranslationDelta(working, members, starts, delta),
            "group translate applies shared delta");
        Expect(working.elevatedPlatforms[0].center.x == 1.25f, "primary is snapped result");
        Expect(NearlyEqual(working.elevatedPlatforms[1].center.x, 3.52f, 0.0001f),
            "secondary is not independently snapped");
        Expect(
            NearlyEqual(
                working.elevatedPlatforms[1].center.x - working.elevatedPlatforms[0].center.x,
                2.27f,
                0.0001f),
            "relative offset is preserved");

        working.elevatedPlatforms[0].center = {1.25f, 1.0f, 0.0f};
        working.elevatedPlatforms[1].center = {3.52f, 1.0f, 0.0f};
        starts[0] = {1.10f, 1.0f, 0.0f};
        starts[1] = {3.37f, 1.0f, 0.0f};
        for (int i = 0; i < 24; ++i)
        {
            Expect(
                editor::ApplyGroupTranslateFromPrimaryResult(
                    working, members, starts, {1.20f, 1.0f, 0.0f}, EditorAxis::X, true, 0.25f),
                "repeated group frames stay on drag-start authority");
        }
        Expect(working.elevatedPlatforms[0].center.x == 1.25f, "repeated group frames do not drift primary");
        Expect(NearlyEqual(working.elevatedPlatforms[1].center.x, 3.52f, 0.0001f),
            "repeated group frames do not drift secondary");
    }

    // ---- M78 Ctrl snap inversion and object-specific authority ----
    {
        world::LevelDefinition working{};
        world::ItemPickupSpec pickup{};
        pickup.position = {1.10f, 0.5f, 0.0f};
        pickup.itemId = "key";
        pickup.quantity = 1;
        pickup.visualOffset = {0.3f, 0.0f, 0.0f};
        pickup.visualScale = {1.0f, 1.0f, 1.0f};
        working.itemPickups.push_back(pickup);
        world::DynamicBoxSpec box{};
        box.center = {3.37f, 0.5f, 0.0f};
        box.size = {1.0f, 1.0f, 1.0f};
        box.massKg = 30.0f;
        working.dynamicBoxes.push_back(box);
        const core::Vec3 runtimeJoltPose{9.0f, 8.0f, 7.0f};
        const EditorSelection pickupSel{EditorObjectKind::ItemPickup, 0};
        const EditorSelection boxSel{EditorObjectKind::DynamicBox, 0};
        Expect(editor::GetEditablePosition(working, pickupSel) == &working.itemPickups[0].position,
            "Item Pickup Translate edits logical position");
        Expect(editor::GetEditablePosition(working, boxSel) == &working.dynamicBoxes[0].center,
            "Dynamic Box Translate edits authored center");
        const std::vector<EditorSelection> members{pickupSel, boxSel};
        std::vector<core::Vec3> starts;
        Expect(editor::CaptureGroupTranslateStarts(working, members, starts), "mixed translatable starts");
        editor::EditorSnapPreferences snap = editor::MakeDefaultEditorSnapPreferences();
        snap.enabled = true;
        const bool inverted = editor::EditorSnapIsActive(&snap, true);
        Expect(!inverted, "Ctrl inverts enabled Snap during Group Translate");
        Expect(
            editor::ApplyGroupTranslateFromPrimaryResult(
                working,
                members,
                starts,
                {1.20f, 0.5f, 0.0f},
                EditorAxis::X,
                inverted,
                0.25f),
            "Ctrl inversion keeps primary unsnapped");
        Expect(NearlyEqual(working.itemPickups[0].position.x, 1.20f, 0.0001f),
            "inverted group translate does not snap primary");
        Expect(NearlyEqual(working.dynamicBoxes[0].center.x, 3.47f, 0.0001f),
            "inverted shared delta applies to Dynamic Box center");
        Expect(working.itemPickups[0].visualOffset.x == 0.3f, "Item Pickup visualOffset is untouched");
        Expect(runtimeJoltPose.x == 9.0f, "Dynamic Box runtime Jolt pose is not captured");
    }

    // ---- M78 unsupported member refuses partial movement ----
    {
        world::LevelDefinition working{};
        working.elevatedPlatforms.push_back({{1.0f, 1.0f, 0.0f}, {2.0f, 0.5f, 2.0f}});
        const EditorSelection platform{EditorObjectKind::ElevatedPlatform, 0};
        const EditorSelection camera{EditorObjectKind::Camera, 0};
        std::vector<EditorSelection> additional{camera};
        Expect(
            !editor::EditorSelectionSetSupportsGroupTranslate(working, platform, additional),
            "camera in the set blocks Group Translate");
        Expect(
            editor::GroupTranslateDisableReason(working, platform, additional) != nullptr,
            "unsupported member reports compact feedback");
        const core::Vec3 platformStart = working.elevatedPlatforms[0].center;
        const std::vector<EditorSelection> members{platform, camera};
        std::vector<core::Vec3> starts;
        Expect(!editor::CaptureGroupTranslateStarts(working, members, starts),
            "unsupported member is not captured for partial movement");
        Expect(working.elevatedPlatforms[0].center.x == platformStart.x,
            "supported subset is not moved");
        Expect(!editor::EditorGroupAllowsTransformMode(editor::EditorTransformMode::Resize, true),
            "Resize is not a group operation");
        Expect(!editor::EditorGroupAllowsTransformMode(editor::EditorTransformMode::Scale, true),
            "Scale is not a group operation");
        Expect(editor::EditorGroupAllowsTransformMode(editor::EditorTransformMode::Rotate, true),
            "Rotate is a group operation for compatible members");
        Expect(editor::EditorGroupAllowsTransformMode(editor::EditorTransformMode::Translate, true),
            "Translate remains the group mode");
        Expect(editor::EditorGroupAllowsTransformMode(editor::EditorTransformMode::Resize, false),
            "single-selection Resize remains available");
        Expect(
            editor::MultiSelectionTransformDisableReason(editor::EditorTransformMode::Resize)
                != nullptr,
            "Resize still reports that it is not a group operation");
        Expect(
            editor::MultiSelectionTransformDisableReason(editor::EditorTransformMode::Scale)
                != nullptr,
            "Scale still reports that it is not a group operation");
        Expect(
            editor::MultiSelectionTransformDisableReason(editor::EditorTransformMode::Rotate)
                == nullptr,
            "Rotate no longer uses the unsupported-group-mode message");
    }

    // ---- M78 live Group Translate drag uses shared snapped delta ----
    {
        world::LevelDefinition working = MakeStubLevel();
        working.elevatedPlatforms[0].center = {1.10f, 1.0f, 0.0f};
        working.elevatedPlatforms[1].center = {3.37f, 1.0f, 0.0f};
        editor::GizmoInteractionState state{};
        const render::CameraView view = MakeView({1.10f, 1.0f, 10.0f}, {1.10f, 1.0f, 0.0f});
        const EditorSelection platform0{EditorObjectKind::ElevatedPlatform, 0};
        const EditorSelection platform1{EditorObjectKind::ElevatedPlatform, 1};
        std::vector<EditorSelection> additional{platform1};
        editor::EditorSnapPreferences snap = editor::MakeDefaultEditorSnapPreferences();
        snap.enabled = true;
        Expect(
            editor::UpdateGizmoInteraction(
                state,
                platform0,
                working,
                view,
                RayThrough(view, {1.60f, 1.0f, 0.0f}),
                false,
                false,
                true,
                true,
                false,
                &snap,
                false,
                &additional),
            "group translate can begin on primary gizmo");
        Expect(state.dragMembers.size() == 2, "drag captures both members");
        const editor::Ray3 holdRay = RayThrough(view, {1.20f, 1.0f, 0.0f});
        const core::Vec3 intended = editor::GizmoDragPosition(state, holdRay, view);
        const core::Vec3 expectedPrimary = editor::ApplyAuthoredTransformSnap(
            intended,
            editor::EditorTransformMode::Translate,
            state.active,
            true,
            0.25f);
        editor::UpdateGizmoInteraction(
            state,
            platform0,
            working,
            view,
            holdRay,
            false,
            false,
            false,
            true,
            false,
            &snap,
            false,
            &additional);
        Expect(NearlyEqual(working.elevatedPlatforms[0].center.x, expectedPrimary.x, 0.0001f),
            "live group translate snaps primary");
        const float sharedDx = expectedPrimary.x - 1.10f;
        Expect(NearlyEqual(working.elevatedPlatforms[1].center.x, 3.37f + sharedDx, 0.0001f),
            "live secondary uses the shared delta");
        const float independentlySnapped = editor::QuantizeToIncrement(3.37f + sharedDx, 0.25f);
        if (!NearlyEqual(3.37f + sharedDx, independentlySnapped, 0.0001f))
        {
            Expect(
                !NearlyEqual(working.elevatedPlatforms[1].center.x, independentlySnapped, 0.0001f),
                "secondary is not independently quantized onto the grid");
        }
        editor::EndGizmoDrag(state);
    }

    // ---- M79 duplicated selection can Group Translate immediately ----
    {
        world::LevelDefinition working{};
        working.elevatedPlatforms.push_back({{2.0f, 1.0f, 0.0f}, {2.0f, 0.5f, 2.0f}});
        working.elevatedPlatforms.push_back({{4.5f, 1.0f, 0.0f}, {2.0f, 0.5f, 2.0f}});
        working.checkpoint1PlatformIndex = 0;
        working.checkpoint2PlatformIndex = 0;
        working.goalPlatformIndex = 0;
        const EditorSelection primary{EditorObjectKind::ElevatedPlatform, 0};
        const std::vector<EditorSelection> additional{{EditorObjectKind::ElevatedPlatform, 1}};
        const editor::LifecycleEditResult duplicated =
            editor::DuplicateSelectionSet(working, primary, additional);
        Expect(duplicated.succeeded, "Duplicate Selected before Group Translate");
        Expect(
            editor::EditorSelectionSetSupportsGroupTranslate(
                working, duplicated.selection, duplicated.additionalSelections),
            "duplicated copies are immediately Group-Translate compatible");
        const std::vector<EditorSelection> copies =
            editor::EditorSelectionSetMembers(
                duplicated.selection, duplicated.additionalSelections);
        std::vector<core::Vec3> starts;
        Expect(editor::CaptureGroupTranslateStarts(working, copies, starts),
            "capture starts from duplicated copies");
        const float relativeBefore = starts[1].x - starts[0].x;
        Expect(NearlyEqual(relativeBefore, 2.5f, 0.0001f),
            "duplicated copies keep composition spacing");
        Expect(
            editor::ApplyGroupTranslateFromPrimaryResult(
                working,
                copies,
                starts,
                {starts[0].x + 1.0f, starts[0].y, starts[0].z},
                EditorAxis::X,
                false,
                0.25f),
            "Group Translate moves the duplicated composition");
        Expect(
            NearlyEqual(
                working.elevatedPlatforms[copies[1].index].center.x
                    - working.elevatedPlatforms[copies[0].index].center.x,
                relativeBefore,
                0.0001f),
            "Group Translate preserves duplicated relative spacing");
        Expect(
            NearlyEqual(working.elevatedPlatforms[0].center.x, 2.0f, 0.0001f)
                && NearlyEqual(working.elevatedPlatforms[1].center.x, 4.5f, 0.0001f),
            "originals are not translated with the copies");
    }

    // ---- M80 Group Rotate: eligibility, PRIMARY pivot, shared delta ----
    {
        world::LevelDefinition working{};
        world::StaticPropSpec primary{};
        primary.modelIdentity = "models/test_static.glb";
        primary.position = {0.0f, 1.0f, 0.0f};
        primary.rotationDegrees = {7.0f, 0.0f, 0.0f};
        primary.scale = {1.0f, 1.0f, 1.0f};
        working.staticProps.push_back(primary);
        world::StaticPropSpec secondary{};
        secondary.modelIdentity = "models/test_static.glb";
        secondary.position = {2.0f, 1.0f, 0.0f};
        secondary.rotationDegrees = {31.0f, 0.0f, 0.0f};
        secondary.scale = {1.0f, 1.0f, 1.0f};
        working.staticProps.push_back(secondary);
        const EditorSelection primarySel{EditorObjectKind::StaticProp, 0};
        const EditorSelection secondarySel{EditorObjectKind::StaticProp, 1};
        std::vector<EditorSelection> additional{secondarySel};
        Expect(
            editor::EditorSelectionSetSupportsGroupRotate(working, primarySel, additional),
            "two Static Props support Group Rotate");
        Expect(
            editor::GroupRotateDisableReason(working, primarySel, additional) == nullptr,
            "compatible Group Rotate has no disable reason");
        Expect(
            !editor::EditorSelectionSetSupportsGroupRotate(working, primarySel, {}),
            "single selection is not Group Rotate");

        const std::vector<EditorSelection> members{primarySel, secondarySel};
        std::vector<core::Vec3> startPositions;
        std::vector<core::Vec3> startRotations;
        Expect(
            editor::CaptureGroupRotateStarts(working, members, startPositions, startRotations),
            "capture Group Rotate drag-start transforms");
        core::Vec3 pivot{};
        Expect(editor::TryPrimaryRotatePivot(working, primarySel, pivot), "PRIMARY pivot exists");
        Expect(Vec3Near(pivot, working.staticProps[0].position, 0.0001f),
            "Static Prop Rotate pivot is authored position");
        Expect(
            editor::ApplySharedGroupRotation(
                working,
                members,
                startPositions,
                startRotations,
                pivot,
                EditorAxis::Y,
                90.0f),
            "90-degree Group Rotate applies");
        Expect(Vec3Near(working.staticProps[0].position, {0.0f, 1.0f, 0.0f}, 0.0001f),
            "PRIMARY world position stays fixed");
        Expect(Vec3Near(working.staticProps[1].position, {0.0f, 1.0f, -2.0f}, 0.0001f),
            "secondary orbits PRIMARY 90 degrees around world Y");
        Expect(NearlyEqual(working.staticProps[0].rotationDegrees.y, 90.0f, 0.0001f),
            "PRIMARY orientation receives the shared +90 Y delta");
        Expect(NearlyEqual(working.staticProps[1].rotationDegrees.y, 90.0f, 0.0001f),
            "secondary orientation receives the same +90 Y delta");
        Expect(NearlyEqual(working.staticProps[0].rotationDegrees.x, 7.0f, 0.0001f)
                && NearlyEqual(working.staticProps[1].rotationDegrees.x, 31.0f, 0.0001f),
            "inactive Euler axes stay at drag-start");
        Expect(
            NearlyEqual(
                editor::Length(editor::Sub(working.staticProps[1].position, pivot)),
                2.0f,
                0.0001f),
            "secondary distance to PRIMARY pivot is preserved");

        working.staticProps[0].position = startPositions[0];
        working.staticProps[1].position = startPositions[1];
        working.staticProps[0].rotationDegrees = startRotations[0];
        working.staticProps[1].rotationDegrees = startRotations[1];
        Expect(
            editor::ApplySharedGroupRotation(
                working,
                members,
                startPositions,
                startRotations,
                pivot,
                EditorAxis::Y,
                -90.0f),
            "negative Group Rotate applies");
        Expect(Vec3Near(working.staticProps[0].position, startPositions[0], 0.0001f),
            "negative rotate keeps PRIMARY fixed");
        Expect(Vec3Near(working.staticProps[1].position, {0.0f, 1.0f, 2.0f}, 0.0001f),
            "negative Y rotation orbits the opposite direction");
        Expect(NearlyEqual(working.staticProps[0].rotationDegrees.y, -90.0f, 0.0001f),
            "PRIMARY receives the shared -90 Y delta");
        Expect(NearlyEqual(working.staticProps[1].rotationDegrees.y, -90.0f, 0.0001f),
            "secondary receives the shared -90 Y delta");

        working.staticProps[0].position = startPositions[0];
        working.staticProps[1].position = startPositions[1];
        working.staticProps[0].rotationDegrees = startRotations[0];
        working.staticProps[1].rotationDegrees = startRotations[1];
        Expect(
            editor::ApplySharedGroupRotation(
                working,
                members,
                startPositions,
                startRotations,
                pivot,
                EditorAxis::X,
                90.0f),
            "world X Group Rotate applies");
        Expect(Vec3Near(working.staticProps[1].position, {2.0f, 1.0f, 0.0f}, 0.0001f),
            "world X rotation of an X-offset secondary does not move it");
        Expect(NearlyEqual(working.staticProps[0].rotationDegrees.x, 97.0f, 0.0001f),
            "world X adds the shared delta to PRIMARY X");
        Expect(NearlyEqual(working.staticProps[1].rotationDegrees.x, 121.0f, 0.0001f),
            "world X adds the same delta to secondary X");

        working.staticProps[1].position = {0.0f, 1.0f, 2.0f};
        startPositions[1] = working.staticProps[1].position;
        Expect(
            editor::ApplySharedGroupRotation(
                working,
                members,
                startPositions,
                startRotations,
                pivot,
                EditorAxis::X,
                90.0f),
            "world X Group Rotate of a Z-offset secondary");
        Expect(Vec3Near(working.staticProps[1].position, {0.0f, -1.0f, 0.0f}, 0.0001f),
            "world X 90-degree orbit matches existing RotateX");
        Expect(Vec3Near(working.staticProps[0].position, startPositions[0], 0.0001f),
            "PRIMARY stays fixed on world X Group Rotate");
    }

    // ---- M80 shared Rotate Snap, Snap OFF, no per-member snap, no drift ----
    {
        world::LevelDefinition working{};
        world::StaticPropSpec primary{};
        primary.modelIdentity = "models/test_static.glb";
        primary.position = {0.0f, 1.0f, 0.0f};
        primary.rotationDegrees = {0.0f, 7.0f, 0.0f};
        primary.scale = {1.0f, 1.0f, 1.0f};
        working.staticProps.push_back(primary);
        world::StaticPropSpec secondary{};
        secondary.modelIdentity = "models/test_static.glb";
        secondary.position = {2.0f, 1.0f, 0.25f};
        secondary.rotationDegrees = {0.0f, 31.0f, 0.0f};
        secondary.scale = {1.0f, 1.0f, 1.0f};
        working.staticProps.push_back(secondary);
        const std::vector<EditorSelection> members{
            {EditorObjectKind::StaticProp, 0}, {EditorObjectKind::StaticProp, 1}};
        std::vector<core::Vec3> startPositions;
        std::vector<core::Vec3> startRotations;
        Expect(
            editor::CaptureGroupRotateStarts(working, members, startPositions, startRotations),
            "capture starts for snap tests");
        core::Vec3 pivot{};
        Expect(editor::TryPrimaryRotatePivot(working, members[0], pivot), "snap-test pivot");

        Expect(
            editor::ApplySharedGroupRotation(
                working,
                members,
                startPositions,
                startRotations,
                pivot,
                EditorAxis::Y,
                15.0f),
            "explicit shared +15 delta");
        Expect(NearlyEqual(working.staticProps[0].rotationDegrees.y, 22.0f, 0.0001f),
            "PRIMARY 7 + shared 15 = 22");
        Expect(NearlyEqual(working.staticProps[1].rotationDegrees.y, 46.0f, 0.0001f),
            "secondary 31 + shared 15 = 46");

        working.staticProps[0].position = startPositions[0];
        working.staticProps[1].position = startPositions[1];
        working.staticProps[0].rotationDegrees = startRotations[0];
        working.staticProps[1].rotationDegrees = startRotations[1];
        const core::Vec3 intended{0.0f, 22.0f, 0.0f};
        Expect(
            editor::ApplyGroupRotateFromPrimaryResult(
                working,
                members,
                startPositions,
                startRotations,
                pivot,
                intended,
                EditorAxis::Y,
                true,
                15.0f),
            "M76 snaps PRIMARY rotation result once");
        const core::Vec3 snappedPrimary = editor::SnappedPrimaryRotateResult(
            intended, EditorAxis::Y, true, 15.0f);
        Expect(NearlyEqual(snappedPrimary.y, 15.0f, 0.0001f),
            "M76 15-degree snap sends 22 to 15");
        const float sharedDelta =
            editor::SharedRotationDelta(startRotations[0], snappedPrimary, EditorAxis::Y);
        Expect(NearlyEqual(sharedDelta, 8.0f, 0.0001f),
            "shared delta is snapped PRIMARY minus drag-start");
        Expect(NearlyEqual(working.staticProps[0].rotationDegrees.y, 15.0f, 0.0001f),
            "PRIMARY receives the snapped result");
        Expect(NearlyEqual(working.staticProps[1].rotationDegrees.y, 39.0f, 0.0001f),
            "secondary uses the shared delta, not an independent snap");
        Expect(
            !NearlyEqual(
                working.staticProps[1].rotationDegrees.y,
                editor::QuantizeToIncrement(39.0f, 15.0f),
                0.0001f),
            "secondary result 39 is not independently snapped to 45");
        Expect(
            !NearlyEqual(
                working.staticProps[1].rotationDegrees.y,
                editor::QuantizeToIncrement(31.0f, 15.0f),
                0.0001f),
            "secondary start 31 is not independently snapped to 30");
        Expect(
            !NearlyEqual(
                working.staticProps[1].position.x,
                editor::QuantizeToIncrement(working.staticProps[1].position.x, 0.25f),
                0.0001f),
            "secondary orbit position is not independently Translate-snapped");
        Expect(
            NearlyEqual(
                editor::Length(editor::Sub(working.staticProps[1].position, pivot)),
                editor::Length(editor::Sub(startPositions[1], pivot)),
                0.0001f),
            "snapped Group Rotate still preserves pivot distance");

        for (int i = 0; i < 24; ++i)
        {
            Expect(
                editor::ApplyGroupRotateFromPrimaryResult(
                    working,
                    members,
                    startPositions,
                    startRotations,
                    pivot,
                    intended,
                    EditorAxis::Y,
                    true,
                    15.0f),
                "repeated Group Rotate frames stay on drag-start authority");
        }
        Expect(NearlyEqual(working.staticProps[0].rotationDegrees.y, 15.0f, 0.0001f),
            "repeated frames do not drift PRIMARY rotation");
        Expect(NearlyEqual(working.staticProps[1].rotationDegrees.y, 39.0f, 0.0001f),
            "repeated frames do not drift secondary rotation");
        Expect(Vec3Near(working.staticProps[0].position, startPositions[0], 0.0001f),
            "repeated frames do not drift PRIMARY position");

        working.staticProps[0].position = startPositions[0];
        working.staticProps[1].position = startPositions[1];
        working.staticProps[0].rotationDegrees = startRotations[0];
        working.staticProps[1].rotationDegrees = startRotations[1];
        Expect(
            editor::ApplyGroupRotateFromPrimaryResult(
                working,
                members,
                startPositions,
                startRotations,
                pivot,
                intended,
                EditorAxis::Y,
                false,
                15.0f),
            "Snap OFF Group Rotate");
        Expect(NearlyEqual(working.staticProps[0].rotationDegrees.y, 22.0f, 0.0001f),
            "Snap OFF keeps unsnapped PRIMARY 22");
        Expect(NearlyEqual(working.staticProps[1].rotationDegrees.y, 46.0f, 0.0001f),
            "Snap OFF applies the same unsnapped +15 delta");

        editor::EditorSnapPreferences snap = editor::MakeDefaultEditorSnapPreferences();
        snap.enabled = true;
        const bool inverted = editor::EditorSnapIsActive(&snap, true);
        Expect(!inverted, "Ctrl inverts enabled Rotate Snap during Group Rotate");
        working.staticProps[0].position = startPositions[0];
        working.staticProps[1].position = startPositions[1];
        working.staticProps[0].rotationDegrees = startRotations[0];
        working.staticProps[1].rotationDegrees = startRotations[1];
        Expect(
            editor::ApplyGroupRotateFromPrimaryResult(
                working,
                members,
                startPositions,
                startRotations,
                pivot,
                intended,
                EditorAxis::Y,
                inverted,
                15.0f),
            "Ctrl inversion keeps the group unsnapped");
        Expect(NearlyEqual(working.staticProps[0].rotationDegrees.y, 22.0f, 0.0001f),
            "Ctrl-inverted Group Rotate does not snap PRIMARY");
        Expect(NearlyEqual(working.staticProps[1].rotationDegrees.y, 46.0f, 0.0001f),
            "Ctrl-inverted Group Rotate uses the same unsnapped delta");

        working.staticProps[0].rotationDegrees = {0.0f, 350.0f, 0.0f};
        working.staticProps[1].rotationDegrees = {0.0f, 31.0f, 0.0f};
        startRotations[0] = working.staticProps[0].rotationDegrees;
        startRotations[1] = working.staticProps[1].rotationDegrees;
        const core::Vec3 beyondIntended{0.0f, 370.0f, 0.0f};
        Expect(
            editor::ApplyGroupRotateFromPrimaryResult(
                working,
                members,
                startPositions,
                startRotations,
                pivot,
                beyondIntended,
                EditorAxis::Y,
                true,
                15.0f),
            "M76 >360 authored rotation remains valid for the group");
        const core::Vec3 beyondSnapped = editor::SnappedPrimaryRotateResult(
            beyondIntended, EditorAxis::Y, true, 15.0f);
        Expect(beyondSnapped.y > 360.0f, "snapped result may exceed 360");
        Expect(NearlyEqual(working.staticProps[0].rotationDegrees.y, beyondSnapped.y, 0.0001f),
            "PRIMARY keeps accumulated authored rotation beyond 360");
        const float beyondDelta =
            editor::SharedRotationDelta(startRotations[0], beyondSnapped, EditorAxis::Y);
        Expect(
            NearlyEqual(working.staticProps[1].rotationDegrees.y, 31.0f + beyondDelta, 0.0001f),
            "secondary receives the same >360 shared delta");
    }

    // ---- M80 unsupported member refuses the complete Group Rotate ----
    {
        world::LevelDefinition working{};
        world::StaticPropSpec prop{};
        prop.modelIdentity = "models/test_static.glb";
        prop.position = {1.0f, 1.0f, 0.0f};
        prop.rotationDegrees = {10.0f, 20.0f, 30.0f};
        prop.scale = {1.0f, 1.0f, 1.0f};
        working.staticProps.push_back(prop);
        world::DynamicBoxSpec box{};
        box.center = {3.0f, 1.0f, 0.0f};
        box.size = {1.0f, 1.0f, 1.0f};
        box.massKg = 30.0f;
        working.dynamicBoxes.push_back(box);
        working.elevatedPlatforms.push_back({{5.0f, 1.0f, 0.0f}, {2.0f, 0.5f, 2.0f}});
        const EditorSelection propSel{EditorObjectKind::StaticProp, 0};
        const EditorSelection boxSel{EditorObjectKind::DynamicBox, 0};
        const EditorSelection platformSel{EditorObjectKind::ElevatedPlatform, 0};
        const core::Vec3 runtimeJoltPose{9.0f, 8.0f, 7.0f};
        const core::Vec3 runtimeJoltRotation{12.0f, 24.0f, 36.0f};
        std::vector<EditorSelection> withBox{boxSel};
        Expect(
            !editor::EditorSelectionSetSupportsGroupRotate(working, propSel, withBox),
            "Dynamic Box blocks Group Rotate");
        Expect(
            editor::GroupRotateDisableReason(working, propSel, withBox) != nullptr,
            "unsupported member reports compact Group Rotate feedback");
        Expect(!editor::IsRotateSelection(boxSel), "Dynamic Box remains Rotate-unsupported");
        Expect(editor::GetEditableRotation(working, boxSel) == nullptr,
            "Dynamic Box has no authored rotation field");
        const world::LevelDefinition before = working;
        const std::vector<EditorSelection> mixedMembers{propSel, boxSel};
        std::vector<core::Vec3> startPositions;
        std::vector<core::Vec3> startRotations;
        Expect(
            !editor::CaptureGroupRotateStarts(working, mixedMembers, startPositions, startRotations),
            "unsupported member is not captured for a partial rotate");
        Expect(
            !editor::ApplySharedGroupRotation(
                working,
                mixedMembers,
                {working.staticProps[0].position, working.dynamicBoxes[0].center},
                {working.staticProps[0].rotationDegrees, {}},
                working.staticProps[0].position,
                EditorAxis::Y,
                90.0f),
            "Apply refuses the complete mixed Group Rotate");
        Expect(world::AuthoredLevelDataEqual(working, before),
            "unsupported refusal does not mutate a supported subset");
        Expect(runtimeJoltPose.x == 9.0f && runtimeJoltRotation.y == 24.0f,
            "Dynamic Box runtime Jolt pose/orientation is never captured");
        std::vector<EditorSelection> withPlatform{platformSel};
        Expect(
            !editor::EditorSelectionSetSupportsGroupRotate(working, propSel, withPlatform),
            "Platform blocks Group Rotate");
        Expect(!editor::IsRotateSelection(platformSel), "Platform remains Rotate-unsupported");
        Expect(!editor::IsRotateSelection({EditorObjectKind::Checkpoint, 0}),
            "Checkpoint remains Rotate-unsupported");
        Expect(!editor::IsRotateSelection({EditorObjectKind::Door, 0}),
            "Door remains Rotate-unsupported");
        Expect(!editor::IsRotateSelection({EditorObjectKind::Ground, 0}),
            "Ground remains Rotate-unsupported");
    }

    // ---- M80 Item Pickup logical position / visual rotation authority ----
    {
        world::LevelDefinition working{};
        world::StaticPropSpec prop{};
        prop.modelIdentity = "models/test_static.glb";
        prop.position = {0.0f, 1.0f, 0.0f};
        prop.rotationDegrees = {0.0f, 0.0f, 0.0f};
        prop.scale = {1.0f, 1.0f, 1.0f};
        working.staticProps.push_back(prop);
        world::ItemPickupSpec pickup{};
        pickup.position = {2.0f, 1.0f, 0.0f};
        pickup.itemId = "key";
        pickup.quantity = 1;
        pickup.visualOffset = {0.3f, 0.5f, 0.0f};
        pickup.visualRotationDegrees = {10.0f, 20.0f, 30.0f};
        pickup.visualScale = {1.0f, 1.0f, 1.0f};
        pickup.idleAnimationEnabled = true;
        pickup.idleBobAmplitude = 0.15f;
        pickup.idleBobSpeed = 1.0f;
        pickup.idleSpinSpeedDegrees = 90.0f;
        working.itemPickups.push_back(pickup);
        const EditorSelection propSel{EditorObjectKind::StaticProp, 0};
        const EditorSelection pickupSel{EditorObjectKind::ItemPickup, 0};
        Expect(editor::GetEditablePosition(working, pickupSel) == &working.itemPickups[0].position,
            "Item Pickup Group orbit edits logical position");
        Expect(
            editor::GetEditableRotation(working, pickupSel)
                == &working.itemPickups[0].visualRotationDegrees,
            "Item Pickup Group Rotate edits visualRotationDegrees");
        const std::vector<EditorSelection> members{propSel, pickupSel};
        std::vector<core::Vec3> startPositions;
        std::vector<core::Vec3> startRotations;
        Expect(
            editor::CaptureGroupRotateStarts(working, members, startPositions, startRotations),
            "mixed Static Prop / Item Pickup capture");
        core::Vec3 pivot{};
        Expect(editor::TryPrimaryRotatePivot(working, propSel, pivot), "prop PRIMARY pivot");
        const core::Vec3 presentedBefore =
            world::ItemPickupPresentedVisualRotationDegrees(working.itemPickups[0], 1.25);
        Expect(
            editor::ApplySharedGroupRotation(
                working,
                members,
                startPositions,
                startRotations,
                pivot,
                EditorAxis::Y,
                90.0f),
            "mixed compatible Group Rotate");
        Expect(Vec3Near(working.staticProps[0].position, {0.0f, 1.0f, 0.0f}, 0.0001f),
            "prop PRIMARY stays fixed");
        Expect(Vec3Near(working.itemPickups[0].position, {0.0f, 1.0f, -2.0f}, 0.0001f),
            "Item Pickup logical position orbits PRIMARY");
        Expect(Vec3Near(working.itemPickups[0].visualOffset, {0.3f, 0.5f, 0.0f}, 0.0001f),
            "Item Pickup visualOffset is untouched");
        Expect(NearlyEqual(working.itemPickups[0].visualRotationDegrees.y, 110.0f, 0.0001f),
            "Item Pickup visualRotationDegrees receives the shared delta");
        Expect(working.itemPickups[0].idleAnimationEnabled, "idle flag is not captured into authored");
        Expect(NearlyEqual(working.itemPickups[0].idleBobAmplitude, 0.15f, 0.0001f),
            "idle bob amplitude stays authored, not runtime");
        Expect(NearlyEqual(working.itemPickups[0].idleSpinSpeedDegrees, 90.0f, 0.0001f),
            "idle spin speed stays authored, not runtime");
        Expect(
            !NearlyEqual(working.itemPickups[0].visualRotationDegrees.y, presentedBefore.y, 0.0001f),
            "presented bob/spin rotation is not written back");
        Expect(
            editor::MakeRotateGizmoDrawRequest(
                pickupSel,
                working,
                MakeView({20.0f, 8.0f, 20.0f}, world::ItemPickupVisualPosition(working.itemPickups[0])),
                {})
                .visible,
            "Item Pickup still has a single-object Rotate gizmo");

        world::LevelDefinition pickupPrimary = working;
        pickupPrimary.itemPickups[0].position = {2.0f, 1.0f, 0.0f};
        pickupPrimary.itemPickups[0].visualRotationDegrees = {10.0f, 20.0f, 30.0f};
        pickupPrimary.staticProps[0].position = {0.0f, 1.0f, 0.0f};
        pickupPrimary.staticProps[0].rotationDegrees = {};
        const std::vector<EditorSelection> pickupFirst{pickupSel, propSel};
        std::vector<core::Vec3> pickupStarts;
        std::vector<core::Vec3> pickupRots;
        Expect(
            editor::CaptureGroupRotateStarts(pickupPrimary, pickupFirst, pickupStarts, pickupRots),
            "Item Pickup PRIMARY capture");
        core::Vec3 pickupPivot{};
        Expect(
            editor::TryPrimaryRotatePivot(pickupPrimary, pickupSel, pickupPivot),
            "Item Pickup PRIMARY uses Rotate gizmo origin");
        Expect(
            Vec3Near(pickupPivot, world::ItemPickupVisualPosition(pickupPrimary.itemPickups[0]), 0.0001f),
            "Item Pickup pivot is position + visualOffset, not bob");
        Expect(
            editor::ApplySharedGroupRotation(
                pickupPrimary,
                pickupFirst,
                pickupStarts,
                pickupRots,
                pickupPivot,
                EditorAxis::Y,
                90.0f),
            "Item Pickup PRIMARY Group Rotate");
        Expect(Vec3Near(pickupPrimary.itemPickups[0].position, {2.0f, 1.0f, 0.0f}, 0.0001f),
            "Item Pickup PRIMARY logical position stays fixed");
        Expect(NearlyEqual(pickupPrimary.itemPickups[0].visualRotationDegrees.y, 110.0f, 0.0001f),
            "Item Pickup PRIMARY visual rotation still receives the delta");
    }

    // ---- M80 workingCopy / selection / Duplicate Selected -> Group Rotate ----
    {
        world::LevelDefinition original{};
        world::StaticPropSpec a{};
        a.modelIdentity = "models/test_static.glb";
        a.position = {0.0f, 1.0f, 0.0f};
        a.rotationDegrees = {0.0f, 5.0f, 0.0f};
        a.scale = {1.0f, 1.0f, 1.0f};
        original.staticProps.push_back(a);
        world::StaticPropSpec b{};
        b.modelIdentity = "models/test_static.glb";
        b.position = {2.0f, 1.0f, 0.0f};
        b.rotationDegrees = {0.0f, 11.0f, 0.0f};
        b.scale = {1.0f, 1.0f, 1.0f};
        original.staticProps.push_back(b);
        original.checkpoint1PlatformIndex = 0;
        original.checkpoint2PlatformIndex = 0;
        original.goalPlatformIndex = 0;
        world::LevelDefinition working = original;
        const EditorSelection primary{EditorObjectKind::StaticProp, 0};
        const std::vector<EditorSelection> additional{{EditorObjectKind::StaticProp, 1}};
        const std::vector<EditorSelection> members =
            editor::EditorSelectionSetMembers(primary, additional);
        std::vector<core::Vec3> startPositions;
        std::vector<core::Vec3> startRotations;
        Expect(
            editor::CaptureGroupRotateStarts(working, members, startPositions, startRotations),
            "capture workingCopy starts");
        core::Vec3 pivot{};
        Expect(editor::TryPrimaryRotatePivot(working, primary, pivot), "workingCopy pivot");
        Expect(
            editor::ApplySharedGroupRotation(
                working, members, startPositions, startRotations, pivot, EditorAxis::Y, 90.0f),
            "Group Rotate mutates workingCopy");
        Expect(!world::AuthoredLevelDataEqual(working, original),
            "Group Rotate makes workingCopy Modified versus active");
        Expect(world::AuthoredLevelDataEqual(original, original),
            "active/original is unchanged until Apply");
        const world::LevelDefinition applied = working;
        Expect(world::AuthoredLevelDataEqual(applied, working),
            "Apply promotion preserves Group Rotate authored results");
        Expect(applied.staticProps[0].rotationDegrees.y == 95.0f
                && applied.staticProps[1].rotationDegrees.y == 101.0f,
            "applied orientations keep the shared delta");
        Expect(primary.kind == EditorObjectKind::StaticProp && primary.index == 0,
            "PRIMARY identity is unchanged by Group Rotate");
        Expect(additional.size() == 1 && additional[0].index == 1,
            "secondary selection is unchanged by Group Rotate");

        const editor::LifecycleEditResult duplicated =
            editor::DuplicateSelectionSet(original, primary, additional);
        Expect(duplicated.succeeded, "Duplicate Selected before Group Rotate");
        Expect(
            editor::EditorSelectionSetSupportsGroupRotate(
                original, duplicated.selection, duplicated.additionalSelections),
            "duplicated copies are immediately Group-Rotate compatible");
        const std::vector<EditorSelection> copies =
            editor::EditorSelectionSetMembers(
                duplicated.selection, duplicated.additionalSelections);
        std::vector<core::Vec3> copyPositions;
        std::vector<core::Vec3> copyRotations;
        Expect(
            editor::CaptureGroupRotateStarts(original, copies, copyPositions, copyRotations),
            "capture starts from duplicated copies");
        core::Vec3 copyPivot{};
        Expect(editor::TryPrimaryRotatePivot(original, duplicated.selection, copyPivot),
            "copy PRIMARY pivot");
        const float copyDistance =
            editor::Length(editor::Sub(copyPositions[1], copyPivot));
        Expect(
            editor::ApplySharedGroupRotation(
                original,
                copies,
                copyPositions,
                copyRotations,
                copyPivot,
                EditorAxis::Y,
                90.0f),
            "Group Rotate the duplicated composition immediately");
        Expect(Vec3Near(original.staticProps[copies[0].index].position, copyPositions[0], 0.0001f),
            "duplicated PRIMARY stays fixed");
        Expect(
            NearlyEqual(
                editor::Length(
                    editor::Sub(original.staticProps[copies[1].index].position, copyPivot)),
                copyDistance,
                0.0001f),
            "duplicated secondary orbit preserves composition distance");
        Expect(Vec3Near(original.staticProps[0].position, a.position, 0.0001f)
                && Vec3Near(original.staticProps[1].position, b.position, 0.0001f),
            "originals are not rotated with the copies");
    }

    // ---- M80 live Group Rotate drag uses PRIMARY gizmo and shared delta ----
    {
        world::LevelDefinition working = MakeStubLevel();
        world::StaticPropSpec primary{};
        primary.modelIdentity = "models/test_static.glb";
        primary.position = {3.0f, 1.0f, 0.0f};
        primary.rotationDegrees = {5.0f, 15.0f, 25.0f};
        primary.scale = {1.0f, 1.0f, 1.0f};
        working.staticProps.push_back(primary);
        world::StaticPropSpec secondary{};
        secondary.modelIdentity = "models/test_static.glb";
        secondary.position = {5.0f, 1.0f, 0.0f};
        secondary.rotationDegrees = {1.0f, 2.0f, 3.0f};
        secondary.scale = {1.0f, 1.0f, 1.0f};
        working.staticProps.push_back(secondary);
        const EditorSelection prop0{EditorObjectKind::StaticProp, 0};
        const EditorSelection prop1{EditorObjectKind::StaticProp, 1};
        std::vector<EditorSelection> additional{prop1};
        const core::Vec3 propOrigin = working.staticProps[0].position;
        const render::CameraView propView = MakeView({20.0f, 8.0f, 20.0f}, propOrigin);
        const float propLength = editor::GizmoWorldLength(propView, propOrigin);
        const float unique = 0.70710678f;
        const core::Vec3 xRing{
            propOrigin.x,
            propOrigin.y + propLength * unique,
            propOrigin.z + propLength * unique};
        const core::Vec3 xSwept{
            propOrigin.x,
            propOrigin.y - propLength * unique,
            propOrigin.z + propLength * unique};
        editor::GizmoInteractionState state{};
        editor::EditorSnapPreferences snap = editor::MakeDefaultEditorSnapPreferences();
        snap.enabled = false;
        Expect(
            editor::UpdateRotateInteraction(
                state,
                prop0,
                working,
                propView,
                RayThrough(propView, xRing),
                false,
                false,
                true,
                true,
                false,
                &snap,
                false,
                &additional),
            "group rotate can begin on PRIMARY gizmo");
        Expect(state.dragging, "Group Rotate press starts drag");
        Expect(state.dragMembers.size() == 2, "drag captures both rotate members");
        Expect(state.dragMemberStartRotations.size() == 2, "drag captures start rotations");
        const core::Vec3 startPrimaryPos = state.dragMemberStartPositions[0];
        const core::Vec3 startSecondaryPos = state.dragMemberStartPositions[1];
        const core::Vec3 startPrimaryRot = state.dragMemberStartRotations[0];
        const core::Vec3 startSecondaryRot = state.dragMemberStartRotations[1];
        const editor::Ray3 holdRay = RayThrough(propView, xSwept);
        const core::Vec3 intended = editor::GizmoRotateDegrees(state, holdRay);
        Expect(
            editor::UpdateRotateInteraction(
                state,
                prop0,
                working,
                propView,
                holdRay,
                false,
                false,
                false,
                true,
                false,
                &snap,
                false,
                &additional),
            "Group Rotate drag consumes pointer");
        const float liveDelta = intended.x - startPrimaryRot.x;
        Expect(Vec3Near(working.staticProps[0].position, startPrimaryPos, 0.0001f),
            "live Group Rotate keeps PRIMARY position fixed");
        Expect(NearlyEqual(working.staticProps[0].rotationDegrees.x, intended.x, 0.05f),
            "live Group Rotate writes PRIMARY orientation from drag-start");
        Expect(
            NearlyEqual(
                working.staticProps[1].rotationDegrees.x, startSecondaryRot.x + liveDelta, 0.05f),
            "live secondary orientation uses the shared delta");
        Expect(
            NearlyEqual(
                editor::Length(editor::Sub(working.staticProps[1].position, state.dragStartPosition)),
                editor::Length(editor::Sub(startSecondaryPos, state.dragStartPosition)),
                0.05f),
            "live secondary keeps pivot distance");
        Expect(
            editor::UpdateRotateInteraction(
                state,
                prop0,
                working,
                propView,
                holdRay,
                false,
                false,
                false,
                false,
                true,
                &snap,
                false,
                &additional),
            "Group Rotate mouse-up ends the drag");
        Expect(!state.dragging, "Group Rotate drag ends cleanly");
        Expect(working.staticProps[0].scale.x == 1.0f && working.staticProps[1].scale.x == 1.0f,
            "Group Rotate does not mutate scale");
    }

    // ---- M85.2 Directional Light visualization Translate/Scale ----
    {
        world::LevelDefinition working = MakeStubLevel();
        const core::Vec3 startRay = working.environment.directionalRayDirection;
        const float startIntensity = working.environment.directionalIntensity;
        const bool startEnabled = working.environment.directionalEnabled;
        const bool startShadows = working.environment.directionalShadowsEnabled;
        editor::DirectionalLightVisualization visualization{};
        Expect(Vec3Near(visualization.anchor, editor::kDirectionalLightAuthoringAnchor),
            "default visualization anchor preserves M85.1");
        Expect(visualization.scale == editor::kDirectionalLightAuthoringDefaultScale,
            "default visualization scale preserves M85.1 size");

        const EditorSelection light{EditorObjectKind::DirectionalLight, 0};
        editor::GizmoInteractionState state{};
        const core::Vec3 origin = visualization.anchor;
        const render::CameraView view = MakeView({20.0f, 8.0f, 20.0f}, origin);
        const float length = editor::GizmoWorldLength(view, origin);
        const core::Vec3 xHandle{origin.x + length, origin.y, origin.z};
        const core::Vec3 xMoved{origin.x + length + 3.0f, origin.y, origin.z};
        Expect(
            editor::UpdateGizmoInteraction(
                state,
                light,
                working,
                view,
                RayThrough(view, xHandle),
                false,
                false,
                true,
                true,
                false,
                nullptr,
                false,
                nullptr,
                &visualization),
            "Directional Light Translate drag starts");
        Expect(state.dragging && state.active == EditorAxis::X,
            "Translate drag captures the X handle");
        const core::Vec3 intendedTranslate =
            editor::GizmoDragPosition(state, RayThrough(view, xMoved), view);
        Expect(
            editor::UpdateGizmoInteraction(
                state,
                light,
                working,
                view,
                RayThrough(view, xMoved),
                false,
                false,
                false,
                true,
                false,
                nullptr,
                false,
                nullptr,
                &visualization),
            "Directional Light Translate drag updates");
        Expect(NearlyEqual(visualization.anchor.x, intendedTranslate.x, 0.15f)
                && visualization.anchor.x > origin.x + 0.5f,
            "Translate moves the visualization anchor");
        Expect(NearlyEqual(visualization.anchor.y, origin.y, 0.001f)
                && NearlyEqual(visualization.anchor.z, origin.z, 0.001f),
            "Translate X keeps visualization Y/Z");
        Expect(Vec3Near(working.environment.directionalRayDirection, startRay, 0.0001f),
            "Translate does not change ray direction");
        Expect(working.environment.directionalIntensity == startIntensity,
            "Translate does not change intensity");
        Expect(working.environment.directionalEnabled == startEnabled,
            "Translate does not change authored enabled");
        Expect(working.environment.directionalShadowsEnabled == startShadows,
            "Translate does not change shadows");
        Expect(
            editor::GetEditablePosition(working, light) == nullptr,
            "visualization is not authored Level position");

        editor::EditorSnapPreferences snap = editor::MakeDefaultEditorSnapPreferences();
        snap.enabled = true;
        snap.translateIncrement = 0.25f;
        visualization.anchor = editor::kDirectionalLightAuthoringAnchor;
        editor::EndGizmoDrag(state);
        const float snapLength = editor::GizmoWorldLength(view, visualization.anchor);
        const core::Vec3 snapHandle{
            visualization.anchor.x + snapLength, visualization.anchor.y, visualization.anchor.z};
        const core::Vec3 snapMoved{
            visualization.anchor.x + snapLength + 1.10f,
            visualization.anchor.y,
            visualization.anchor.z};
        Expect(
            editor::UpdateGizmoInteraction(
                state,
                light,
                working,
                view,
                RayThrough(view, snapHandle),
                false,
                false,
                true,
                true,
                false,
                &snap,
                false,
                nullptr,
                &visualization),
            "snapped Translate drag starts");
        const core::Vec3 intendedSnap = editor::ApplyAuthoredTransformSnap(
            editor::GizmoDragPosition(state, RayThrough(view, snapMoved), view),
            editor::EditorTransformMode::Translate,
            state.active,
            true,
            0.25f);
        Expect(
            editor::UpdateGizmoInteraction(
                state,
                light,
                working,
                view,
                RayThrough(view, snapMoved),
                false,
                false,
                false,
                true,
                false,
                &snap,
                false,
                nullptr,
                &visualization),
            "snapped Translate drag updates");
        Expect(NearlyEqual(visualization.anchor.x, intendedSnap.x, 0.001f),
            "Translate uses M76 snapping");
        editor::EndGizmoDrag(state);

        visualization.anchor = origin;
        visualization.scale = 1.0f;
        const float scaleLength = editor::GizmoWorldLength(view, visualization.anchor);
        const core::Vec3 scaleHandle{
            visualization.anchor.x + scaleLength, visualization.anchor.y, visualization.anchor.z};
        const core::Vec3 scaleMoved{
            visualization.anchor.x + scaleLength + 1.0f,
            visualization.anchor.y,
            visualization.anchor.z};
        Expect(
            editor::UpdateScaleInteraction(
                state,
                light,
                working,
                view,
                RayThrough(view, scaleHandle),
                false,
                false,
                true,
                true,
                false,
                nullptr,
                false,
                &visualization),
            "Directional Light Scale drag starts");
        Expect(
            editor::UpdateScaleInteraction(
                state,
                light,
                working,
                view,
                RayThrough(view, scaleMoved),
                false,
                false,
                false,
                true,
                false,
                nullptr,
                false,
                &visualization),
            "Directional Light Scale drag updates");
        Expect(visualization.scale > 1.0f, "Scale increases visualization size");
        Expect(Vec3Near(working.environment.directionalRayDirection, startRay, 0.0001f),
            "Scale does not change ray direction");
        Expect(working.environment.directionalIntensity == startIntensity,
            "Scale does not change intensity");
        Expect(working.environment.directionalShadowsEnabled == startShadows,
            "Scale does not change shadow coverage");
        editor::EndGizmoDrag(state);

        visualization.scale = -4.0f;
        editor::CanonicalizeDirectionalLightVisualization(visualization);
        Expect(visualization.scale == editor::kMinDirectionalLightVisualizationScale,
            "Scale stays positive and at the floor");
        visualization.scale = 99.0f;
        editor::CanonicalizeDirectionalLightVisualization(visualization);
        Expect(visualization.scale == editor::kMaxDirectionalLightVisualizationScale,
            "Scale stays bounded");

        visualization.scale = 1.14f;
        snap.scaleIncrement = 0.10f;
        const core::Vec3 snappedScale = editor::ApplyAuthoredTransformSnap(
            {visualization.scale, visualization.scale, visualization.scale},
            editor::EditorTransformMode::Scale,
            EditorAxis::X,
            true,
            0.10f);
        Expect(NearlyEqual(snappedScale.x, 1.10f, 0.0001f), "Scale snapping uses M76 increments");

        visualization.anchor = {4.0f, 6.0f, -2.0f};
        visualization.scale = 2.0f;
        core::Vec3 boxCenter{};
        core::Vec3 boxSize{};
        Expect(
            editor::GetGizmoPreviewBox(working, light, boxCenter, boxSize, &visualization)
                && Vec3Near(boxCenter, visualization.anchor)
                && Vec3Near(boxSize, editor::ScaledDirectionalLightPickSize(2.0f)),
            "picking/preview box follows translated and scaled visualization");

        const core::Vec3 rotated = editor::RotateAuthoredDirectionalRay(
            startRay, {0.0f, 1.0f, 0.0f}, 40.0f);
        Expect(NearlyEqual(
                std::sqrt(rotated.x * rotated.x + rotated.y * rotated.y + rotated.z * rotated.z),
                1.0f),
            "Rotate result stays normalized");
        Expect(!Vec3Near(rotated, startRay, 0.01f), "Rotate still edits real ray direction");
        Expect(Vec3Near(visualization.anchor, {4.0f, 6.0f, -2.0f}),
            "Rotate does not move visualization anchor");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d editor gizmo/layout test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Editor gizmo tests passed.\n");
    return 0;
}
