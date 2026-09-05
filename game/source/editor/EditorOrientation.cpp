#include "editor/EditorOrientation.h"

#include "editor/EditorMath.h"

#include <algorithm>
#include <cmath>

namespace editor
{
namespace
{
core::Vec3 CameraUp(const EditorCamera& camera)
{
    return NormalizeOr(
        Cross(EditorCameraRight(camera), EditorCameraForward(camera)), {0.0f, 1.0f, 0.0f});
}

core::Vec3 ProjectWorldAxis(const EditorCamera& camera, core::Vec3 worldAxis)
{
    const core::Vec3 right = EditorCameraRight(camera);
    const core::Vec3 up = CameraUp(camera);
    const core::Vec3 forward = EditorCameraForward(camera);
    return {Dot(worldAxis, right), Dot(worldAxis, up), Dot(worldAxis, forward)};
}

float DistanceSquared(float ax, float ay, float bx, float by)
{
    const float dx = ax - bx;
    const float dy = ay - by;
    return dx * dx + dy * dy;
}

float DefaultEditorPanelWidth(float viewportWidth)
{
    return std::min(
        kOrientationWidgetDefaultPanelWidth,
        std::max(kOrientationWidgetMinPanelWidth, viewportWidth * kOrientationWidgetPanelWidthFraction));
}
}

OrientationWidgetLayout MakeOrientationWidgetLayout(
    float viewportWidth,
    float viewportHeight,
    float extraTopInset)
{
    OrientationWidgetLayout layout{};
    layout.radius = kOrientationWidgetRadius;
    layout.hitRadius = kOrientationWidgetHitRadius;

    const float width = viewportWidth > 1.0f ? viewportWidth : 1280.0f;
    const float height = viewportHeight > 1.0f ? viewportHeight : 720.0f;
    const float panelWidth = DefaultEditorPanelWidth(width);
    const float reserved = panelWidth + kOrientationWidgetEdgeMargin;
    const float topInset = extraTopInset > 0.0f ? extraTopInset : 0.0f;

    layout.originX =
        width - reserved - layout.radius - kOrientationWidgetInspectorGap;
    layout.originY = kOrientationWidgetTopMargin + topInset + layout.radius;

    const float minX = layout.radius + kOrientationWidgetEdgeMargin;
    const float maxX = width - layout.radius - kOrientationWidgetEdgeMargin;
    const float minY = layout.radius + kOrientationWidgetEdgeMargin;
    const float maxY = height - layout.radius - kOrientationWidgetEdgeMargin;
    const float corridorLeft = reserved + layout.radius;
    const float corridorRight = width - reserved - layout.radius;

    if (corridorRight < corridorLeft + 1.0f)
    {
        layout.originX = width * 0.5f;
        layout.originY = std::fmax(layout.originY, kOrientationWidgetNarrowShiftY);
    }
    else if (layout.originX < corridorLeft)
    {
        layout.originX = corridorLeft;
        layout.originY = std::fmax(layout.originY, kOrientationWidgetNarrowShiftY);
    }

    layout.originX = std::clamp(layout.originX, minX, std::fmax(minX, maxX));
    layout.originY = std::clamp(layout.originY, minY, std::fmax(minY, maxY));
    return layout;
}

OrientationWidgetAxes ProjectOrientationWidgetAxes(const EditorCamera& camera)
{
    OrientationWidgetAxes axes{};
    axes.x = ProjectWorldAxis(camera, {1.0f, 0.0f, 0.0f});
    axes.y = ProjectWorldAxis(camera, {0.0f, 1.0f, 0.0f});
    axes.z = ProjectWorldAxis(camera, {0.0f, 0.0f, 1.0f});
    return axes;
}

void ApplyCanonicalEditorView(EditorCamera& camera, CanonicalEditorView view)
{
    switch (view)
    {
    case CanonicalEditorView::Right:
        camera.yawDegrees = -90.0f;
        camera.pitchDegrees = 0.0f;
        break;
    case CanonicalEditorView::Left:
        camera.yawDegrees = 90.0f;
        camera.pitchDegrees = 0.0f;
        break;
    case CanonicalEditorView::Top:
        camera.pitchDegrees = kEditorCameraMinPitchDegrees;
        break;
    case CanonicalEditorView::Front:
        camera.yawDegrees = 0.0f;
        camera.pitchDegrees = 0.0f;
        break;
    case CanonicalEditorView::Back:
        camera.yawDegrees = 180.0f;
        camera.pitchDegrees = 0.0f;
        break;
    case CanonicalEditorView::None:
        return;
    }
    ClampEditorCamera(camera);
}

CanonicalEditorView PickOrientationWidget(
    float mouseX,
    float mouseY,
    const OrientationWidgetLayout& layout,
    const OrientationWidgetAxes& axes)
{
    if (!(layout.radius > 0.0f) || !(layout.hitRadius > 0.0f))
    {
        return CanonicalEditorView::None;
    }

    struct Tip
    {
        CanonicalEditorView view;
        float x;
        float y;
    };

    // Screen Y is down; widget Y is up.
    const Tip tips[] = {
        {CanonicalEditorView::Right, layout.originX + axes.x.x * layout.radius,
         layout.originY - axes.x.y * layout.radius},
        {CanonicalEditorView::Left, layout.originX - axes.x.x * layout.radius,
         layout.originY + axes.x.y * layout.radius},
        {CanonicalEditorView::Top, layout.originX + axes.y.x * layout.radius,
         layout.originY - axes.y.y * layout.radius},
        {CanonicalEditorView::Front, layout.originX + axes.z.x * layout.radius,
         layout.originY - axes.z.y * layout.radius},
        {CanonicalEditorView::Back, layout.originX - axes.z.x * layout.radius,
         layout.originY + axes.z.y * layout.radius},
    };

    CanonicalEditorView best = CanonicalEditorView::None;
    float bestDistance = layout.hitRadius * layout.hitRadius;
    for (const Tip& tip : tips)
    {
        const float distance = DistanceSquared(mouseX, mouseY, tip.x, tip.y);
        if (std::isfinite(distance) && distance <= bestDistance)
        {
            bestDistance = distance;
            best = tip.view;
        }
    }
    return best;
}

const char* CanonicalEditorViewName(CanonicalEditorView view)
{
    switch (view)
    {
    case CanonicalEditorView::Right:
        return "Right";
    case CanonicalEditorView::Left:
        return "Left";
    case CanonicalEditorView::Front:
        return "Front";
    case CanonicalEditorView::Back:
        return "Back";
    case CanonicalEditorView::Top:
        return "Top";
    case CanonicalEditorView::None:
        break;
    }
    return "None";
}
}
