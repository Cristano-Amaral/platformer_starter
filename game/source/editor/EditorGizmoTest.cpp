#include "editor/EditorGizmo.h"
#include "editor/EditorLayout.h"
#include "editor/EditorMath.h"
#include "editor/EditorSelection.h"
#include "world/LevelDefinition.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
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
    for (world::Box& platform : level.elevatedPlatforms)
    {
        platform = {{0.0f, 1.0f, 0.0f}, {1.0f, 0.5f, 1.0f}};
    }
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
            !editor::IsGizmoSelection({EditorObjectKind::ElevatedPlatform, 6}),
            "platform 6 is not gizmo");
        Expect(!editor::IsGizmoSelection({EditorObjectKind::Camera, 0}), "camera has no gizmo");
        Expect(!editor::IsGizmoSelection({EditorObjectKind::Slope, 0}), "slope has no gizmo");
        Expect(
            !editor::IsGizmoSelection({EditorObjectKind::MovingPlatform, 0}),
            "moving platform has no gizmo");
        Expect(!editor::IsGizmoSelection({EditorObjectKind::Checkpoint, 0}), "checkpoint no gizmo");
        Expect(!editor::IsGizmoSelection({EditorObjectKind::Hazard, 0}), "hazard no gizmo");
        Expect(!editor::IsGizmoSelection({EditorObjectKind::Collectible, 0}), "collectible no gizmo");
        Expect(!editor::IsGizmoSelection({EditorObjectKind::Goal, 0}), "goal no gizmo");
        Expect(!editor::IsGizmoSelection({EditorObjectKind::DynamicBox, 0}), "cyan box no gizmo");
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
        Expect(defaults.inspector.x > defaults.metrics.x, "inspector is on the right");
        Expect(defaults.hierarchy.y > defaults.metrics.y, "hierarchy sits below metrics");

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

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d editor gizmo/layout test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Editor gizmo tests passed.\n");
    return 0;
}
