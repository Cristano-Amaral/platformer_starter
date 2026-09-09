#include "editor/EditorGizmo.h"
#include "editor/AuthoredObjectLifecycle.h"
#include "editor/EditorLayout.h"
#include "editor/EditorMath.h"
#include "editor/EditorNudge.h"
#include "editor/EditorSelection.h"
#include "world/LevelDefinition.h"

#include <cstdint>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
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
        Expect(!editor::IsGizmoSelection({EditorObjectKind::Slope, 0}), "slope has no gizmo");
        Expect(
            !editor::IsGizmoSelection({EditorObjectKind::MovingPlatform, 0}),
            "moving platform has no gizmo");
        Expect(editor::IsGizmoSelection({EditorObjectKind::Checkpoint, 0}), "checkpoint translate gizmo");
        Expect(editor::IsGizmoSelection({EditorObjectKind::Hazard, 0}), "hazard translate gizmo");
        Expect(editor::IsGizmoSelection({EditorObjectKind::Collectible, 0}), "collectible translate gizmo");
        Expect(!editor::IsGizmoSelection({EditorObjectKind::Goal, 0}), "goal has no gizmo");
        Expect(editor::IsGizmoSelection({EditorObjectKind::DynamicBox, 0}), "Dynamic Box has gizmo");
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
        Expect(
            !editor::IsScaleSelection({EditorObjectKind::Ground, 0}),
            "Ground is not Static Prop Scale");
        Expect(
            !editor::IsScaleSelection({EditorObjectKind::DynamicBox, 0}),
            "Dynamic Box is not Static Prop Scale");
        core::Vec3* propScale = editor::GetEditableScale(level, {EditorObjectKind::StaticProp, 0});
        Expect(propScale != nullptr, "Static Prop has editable scale");
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
        Expect(editor::FindDefaultPlacement(defaults, editor::kObjectPaletteWindowName) != nullptr,
            "object palette has a default placement");
        Expect(editor::FindDefaultPlacement(defaults, editor::kContentBrowserWindowName) != nullptr,
            "content browser has a default placement");
        Expect(editor::FindDefaultPlacement(defaults, editor::kModelPreviewWindowName) != nullptr,
            "model preview has a default placement");
        Expect(defaults.contentBrowser.y > defaults.hierarchy.y, "content browser sits below hierarchy");
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
        Expect(editor::IsResizeSelection({EditorObjectKind::DynamicBox, 0}), "Dynamic Box is resize");
        Expect(!editor::IsResizeSelection({EditorObjectKind::StaticProp, 0}), "Static Prop is not primitive Resize");
        Expect(editor::IsScaleSelection({EditorObjectKind::StaticProp, 0}), "Static Prop is Scale selection");
        Expect(!editor::IsScaleSelection({EditorObjectKind::Spawn, 0}), "spawn is not Scale");
        Expect(!editor::IsScaleSelection({EditorObjectKind::ElevatedPlatform, 0}), "platform is not Scale");
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

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d editor gizmo/layout test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Editor gizmo tests passed.\n");
    return 0;
}
