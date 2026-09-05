#include "editor/EditorCamera.h"
#include "editor/EditorOrientation.h"

#include <cmath>
#include <cstdio>
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

bool FiniteVec(core::Vec3 value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}
}

int main()
{
    using editor::CanonicalEditorView;
    using editor::EditorCamera;

    {
        EditorCamera camera{};
        camera.yawDegrees = 0.0f;
        camera.pitchDegrees = 0.0f;
        const core::Vec3 forward = editor::EditorCameraForward(camera);
        Expect(NearlyEqual(forward.x, 0.0f), "yaw0 pitch0 forward X");
        Expect(NearlyEqual(forward.y, 0.0f), "yaw0 pitch0 forward Y");
        Expect(NearlyEqual(forward.z, -1.0f), "yaw0 pitch0 looks along -Z");
        const editor::OrientationWidgetAxes axes = editor::ProjectOrientationWidgetAxes(camera);
        Expect(FiniteVec(axes.x) && FiniteVec(axes.y) && FiniteVec(axes.z), "yaw0 axes finite");
        Expect(NearlyEqual(axes.z.x, 0.0f) && NearlyEqual(axes.z.y, 0.0f), "+Z is along view at yaw 0");
    }

    {
        EditorCamera camera{};
        camera.yawDegrees = 90.0f;
        camera.pitchDegrees = 0.0f;
        const core::Vec3 forward = editor::EditorCameraForward(camera);
        Expect(NearlyEqual(forward.x, 1.0f), "yaw 90 looks +X");
        Expect(NearlyEqual(forward.z, 0.0f), "yaw 90 forward Z ~ 0");
        const editor::OrientationWidgetAxes axes = editor::ProjectOrientationWidgetAxes(camera);
        Expect(FiniteVec(axes.x), "yaw90 X finite");
    }

    {
        EditorCamera camera{};
        camera.yawDegrees = 35.0f;
        camera.pitchDegrees = -20.0f;
        const editor::OrientationWidgetAxes axes = editor::ProjectOrientationWidgetAxes(camera);
        Expect(FiniteVec(axes.x) && FiniteVec(axes.y) && FiniteVec(axes.z), "oblique axes finite");
        Expect(std::fabs(axes.x.x) + std::fabs(axes.x.y) > 0.2f, "oblique X has screen length");
    }

    {
        EditorCamera camera{};
        camera.position = {3.0f, 4.0f, 12.0f};
        camera.movementSpeed = 8.0f;
        camera.fieldOfViewY = 40.0f;
        editor::ApplyCanonicalEditorView(camera, CanonicalEditorView::Front);
        Expect(NearlyEqual(camera.yawDegrees, 0.0f), "Front yaw 0");
        Expect(NearlyEqual(camera.pitchDegrees, 0.0f), "Front pitch 0");
        Expect(NearlyEqual(camera.position.z, 12.0f), "canonical view keeps position");
        Expect(NearlyEqual(camera.fieldOfViewY, 40.0f), "canonical view keeps FOV");
        editor::ApplyCanonicalEditorView(camera, CanonicalEditorView::Back);
        Expect(NearlyEqual(camera.yawDegrees, 180.0f), "Back yaw 180");
        editor::ApplyCanonicalEditorView(camera, CanonicalEditorView::Right);
        Expect(NearlyEqual(camera.yawDegrees, -90.0f), "Right yaw -90");
        const core::Vec3 rightForward = editor::EditorCameraForward(camera);
        Expect(NearlyEqual(rightForward.x, -1.0f), "Right looks along -X");
        editor::ApplyCanonicalEditorView(camera, CanonicalEditorView::Left);
        Expect(NearlyEqual(camera.yawDegrees, 90.0f), "Left yaw 90");
        editor::ApplyCanonicalEditorView(camera, CanonicalEditorView::Top);
        Expect(NearlyEqual(camera.pitchDegrees, editor::kEditorCameraMinPitchDegrees), "Top pitch min");
        Expect(NearlyEqual(camera.yawDegrees, 90.0f), "Top preserves yaw");
    }

    {
        EditorCamera camera{};
        camera.yawDegrees = 35.0f;
        camera.pitchDegrees = -25.0f;
        const editor::OrientationWidgetLayout layout = editor::MakeOrientationWidgetLayout(1280.0f, 720.0f);
        Expect(layout.originX > 800.0f, "widget sits on the right of a 1280 viewport");
        Expect(layout.originX < 932.0f, "widget stays left of default Inspector");
        Expect(
            layout.originY >= 80.0f && layout.originY <= 120.0f,
            "widget center is 80-120 from the top");
        Expect(NearlyEqual(layout.radius, 36.0f), "widget radius unchanged");
        Expect(NearlyEqual(layout.hitRadius, 12.0f), "widget hit radius unchanged");
        const editor::OrientationWidgetLayout wide =
            editor::MakeOrientationWidgetLayout(1600.0f, 720.0f);
        Expect(wide.originX > layout.originX, "wider viewport moves widget further right");
        const editor::OrientationWidgetLayout narrow =
            editor::MakeOrientationWidgetLayout(400.0f, 720.0f);
        Expect(narrow.originX - narrow.radius >= 0.0f, "narrow widget stays on-screen left");
        Expect(
            narrow.originX + narrow.radius <= 400.0f, "narrow widget stays on-screen right");
        const editor::OrientationWidgetLayout withMenu =
            editor::MakeOrientationWidgetLayout(1280.0f, 720.0f, 32.0f);
        Expect(
            NearlyEqual(withMenu.originY, layout.originY + 32.0f),
            "menu-bar inset lowers widget without changing X");
        Expect(NearlyEqual(withMenu.originX, layout.originX), "menu-bar inset keeps right placement");
        const editor::OrientationWidgetAxes axes = editor::ProjectOrientationWidgetAxes(camera);
        const float xTipX = layout.originX + axes.x.x * layout.radius;
        const float xTipY = layout.originY - axes.x.y * layout.radius;
        Expect(
            editor::PickOrientationWidget(xTipX, xTipY, layout, axes) == CanonicalEditorView::Right,
            "click +X tip is Right");
        const float yTipX = layout.originX + axes.y.x * layout.radius;
        const float yTipY = layout.originY - axes.y.y * layout.radius;
        Expect(
            editor::PickOrientationWidget(yTipX, yTipY, layout, axes) == CanonicalEditorView::Top,
            "click +Y tip is Top");
        Expect(
            editor::PickOrientationWidget(0.0f, 0.0f, layout, axes) == CanonicalEditorView::None,
            "far click misses widget");
    }

    {
        EditorCamera camera{};
        camera.position = {0.0f, 3.0f, 12.0f};
        camera.yawDegrees = 0.0f;
        camera.pitchDegrees = 0.0f;
        camera.movementSpeed = 8.0f;
        camera.fieldOfViewY = 40.0f;
        const float speed = camera.movementSpeed;
        Expect(editor::ApplyEditorCameraDolly(camera, 1.0f), "forward dolly");
        Expect(camera.position.z < 12.0f, "positive wheel moves along look -Z");
        Expect(NearlyEqual(camera.movementSpeed, speed), "dolly does not change nav speed");
        Expect(NearlyEqual(camera.fieldOfViewY, 40.0f), "dolly does not change FOV");
        const float zAfterForward = camera.position.z;
        Expect(editor::ApplyEditorCameraDolly(camera, -1.0f), "backward dolly");
        Expect(camera.position.z > zAfterForward, "negative wheel moves backward");
        Expect(FiniteVec(camera.position), "dolly position finite");
        Expect(!editor::ApplyEditorCameraDolly(camera, 0.0f), "zero wheel is a no-op");
        camera.yawDegrees = 90.0f;
        camera.position = {0.0f, 3.0f, 12.0f};
        Expect(editor::ApplyEditorCameraDolly(camera, 1.0f), "dolly at yaw 90");
        Expect(camera.position.x > 0.0f, "yaw90 dolly moves +X");
        Expect(editor::ApplyEditorCameraDolly(camera, 1000.0f), "huge wheel is clamped");
        Expect(FiniteVec(camera.position), "clamped dolly stays finite");
    }

    {
        using editor::EditorWheelIntent;
        Expect(
            editor::ResolveEditorWheel(false, false, false, 1.0f) == EditorWheelIntent::NavigationSpeed,
            "ordinary wheel is nav speed");
        Expect(
            editor::ResolveEditorWheel(false, false, true, 1.0f) == EditorWheelIntent::Dolly,
            "Alt+wheel is dolly");
        Expect(
            editor::ResolveEditorWheel(true, false, true, 1.0f) == EditorWheelIntent::None,
            "ImGui capture blocks Alt+wheel");
        Expect(
            editor::ResolveEditorWheel(true, false, false, 1.0f) == EditorWheelIntent::None,
            "ImGui capture blocks speed wheel");
        Expect(
            editor::ResolveEditorWheel(false, true, true, 1.0f) == EditorWheelIntent::None,
            "active drag blocks dolly");
        Expect(
            editor::ResolveEditorWheel(false, false, true, 0.0f) == EditorWheelIntent::None,
            "zero wheel is none");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d editor orientation test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Editor orientation tests passed.\n");
    return 0;
}
