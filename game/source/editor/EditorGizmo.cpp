#include "editor/EditorGizmo.h"

#include "editor/EditorMath.h"
#include "world/RespawnWorld.h"

#include <cmath>
#include <limits>

namespace editor
{
namespace
{
constexpr float kMinRayLengthSquared = 1.0e-12f;

core::Vec3 Add(core::Vec3 a, core::Vec3 b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

bool BoxDiffers(const world::Box& a, const world::Box& b)
{
    return a.center.x != b.center.x || a.center.y != b.center.y || a.center.z != b.center.z
        || a.size.x != b.size.x || a.size.y != b.size.y || a.size.z != b.size.z;
}

core::Vec3 ViewForward(const render::CameraView& view)
{
    return NormalizeOr(Sub(view.target, view.position), {0.0f, 0.0f, -1.0f});
}

bool IntersectRayPlane(
    Ray3 ray,
    core::Vec3 planePoint,
    core::Vec3 planeNormal,
    core::Vec3& hit)
{
    const float denom = Dot(ray.direction, planeNormal);
    if (std::fabs(denom) <= kGizmoParallelEpsilon * kGizmoParallelEpsilon)
    {
        return false;
    }

    const float t = Dot(Sub(planePoint, ray.origin), planeNormal) / denom;
    if (!(t >= 0.0f))
    {
        return false;
    }

    hit = Add(ray.origin, Scale(ray.direction, t));
    return IsFiniteVec3(hit);
}

bool ClosestAxisParameter(Ray3 ray, core::Vec3 origin, core::Vec3 axisDir, float& parameter)
{
    const core::Vec3 w0 = Sub(origin, ray.origin);
    const float a = Dot(axisDir, axisDir);
    const float b = Dot(axisDir, ray.direction);
    const float c = Dot(ray.direction, ray.direction);
    const float d = Dot(axisDir, w0);
    const float e = Dot(ray.direction, w0);
    const float denom = a * c - b * b;
    if (std::fabs(denom) <= kGizmoParallelEpsilon * kGizmoParallelEpsilon)
    {
        return false;
    }

    parameter = (b * e - c * d) / denom;
    return std::isfinite(parameter);
}

bool AxisParameterFromRay(
    Ray3 ray,
    core::Vec3 origin,
    core::Vec3 axisDir,
    const render::CameraView& view,
    float& parameter)
{
    if (LengthSquared(ray.direction) < kMinRayLengthSquared)
    {
        return false;
    }

    const core::Vec3 viewForward = ViewForward(view);
    const core::Vec3 side = Cross(viewForward, axisDir);
    if (Length(side) >= kGizmoParallelEpsilon)
    {
        const core::Vec3 planeNormal = NormalizeOr(Cross(axisDir, side), viewForward);
        core::Vec3 hit{};
        if (IntersectRayPlane(ray, origin, planeNormal, hit))
        {
            parameter = Dot(Sub(hit, origin), axisDir);
            return std::isfinite(parameter);
        }
    }

    return ClosestAxisParameter(ray, origin, axisDir, parameter);
}

float RaySegmentDistance(Ray3 ray, core::Vec3 a, core::Vec3 b, float& rayDistance)
{
    const core::Vec3 ab = Sub(b, a);
    const float abLengthSq = LengthSquared(ab);
    if (abLengthSq <= kMinRayLengthSquared || LengthSquared(ray.direction) < kMinRayLengthSquared)
    {
        rayDistance = 0.0f;
        return std::numeric_limits<float>::infinity();
    }

    const core::Vec3 ao = Sub(a, ray.origin);
    const float d1 = Dot(ray.direction, ab);
    const float d2 = LengthSquared(ray.direction);
    const float d3 = Dot(ray.direction, ao);
    const float d4 = Dot(ab, ao);
    const float denom = d2 * abLengthSq - d1 * d1;

    float rayT = 0.0f;
    float segT = 0.0f;
    if (std::fabs(denom) > kMinRayLengthSquared)
    {
        rayT = (abLengthSq * d3 - d1 * d4) / denom;
        segT = (d1 * d3 - d2 * d4) / denom;
    }
    else
    {
        segT = Dot(Sub(ray.origin, a), ab) / abLengthSq;
    }

    if (segT < 0.0f)
    {
        segT = 0.0f;
    }
    else if (segT > 1.0f)
    {
        segT = 1.0f;
    }

    const core::Vec3 onSeg = Add(a, Scale(ab, segT));
    rayT = Dot(Sub(onSeg, ray.origin), ray.direction) / d2;
    if (rayT < 0.0f)
    {
        rayT = 0.0f;
    }

    const core::Vec3 onRay = Add(ray.origin, Scale(ray.direction, rayT));
    rayDistance = rayT;
    return Length(Sub(onRay, onSeg));
}

float RayPointDistance(Ray3 ray, core::Vec3 point, float& rayDistance)
{
    const float directionLengthSq = LengthSquared(ray.direction);
    if (directionLengthSq < kMinRayLengthSquared)
    {
        rayDistance = 0.0f;
        return std::numeric_limits<float>::infinity();
    }

    const core::Vec3 toPoint = Sub(point, ray.origin);
    float rayT = Dot(toPoint, ray.direction) / directionLengthSq;
    if (rayT < 0.0f)
    {
        rayT = 0.0f;
    }
    rayDistance = rayT;
    const core::Vec3 onRay = Add(ray.origin, Scale(ray.direction, rayT));
    return Length(Sub(onRay, point));
}
}

const char* EditorAxisName(EditorAxis axis)
{
    switch (axis)
    {
    case EditorAxis::X:
        return "X";
    case EditorAxis::Y:
        return "Y";
    case EditorAxis::Z:
        return "Z";
    case EditorAxis::None:
        break;
    }
    return "None";
}

core::Vec3 EditorAxisDirection(EditorAxis axis)
{
    switch (axis)
    {
    case EditorAxis::X:
        return {1.0f, 0.0f, 0.0f};
    case EditorAxis::Y:
        return {0.0f, 1.0f, 0.0f};
    case EditorAxis::Z:
        return {0.0f, 0.0f, 1.0f};
    case EditorAxis::None:
        break;
    }
    return {};
}

void ClearGizmoInteraction(GizmoInteractionState& state)
{
    state = {};
}

bool IsGizmoSelection(EditorSelection selection)
{
    switch (selection.kind)
    {
    case EditorObjectKind::Spawn:
    case EditorObjectKind::Ground:
        return selection.index == 0;
    case EditorObjectKind::ElevatedPlatform:
        return selection.index < world::kLevel01ElevatedPlatformCount;
    default:
        return false;
    }
}

bool IsResizeSelection(EditorSelection selection)
{
    switch (selection.kind)
    {
    case EditorObjectKind::Ground:
        return selection.index == 0;
    case EditorObjectKind::ElevatedPlatform:
        return selection.index < world::kLevel01ElevatedPlatformCount;
    default:
        return false;
    }
}

core::Vec3* GetEditablePosition(world::LevelDefinition& level, EditorSelection selection)
{
    switch (selection.kind)
    {
    case EditorObjectKind::Spawn:
        if (selection.index == 0)
        {
            return &level.initialSpawnVisualCenter;
        }
        break;
    case EditorObjectKind::Ground:
        if (selection.index == 0)
        {
            return &level.ground.center;
        }
        break;
    case EditorObjectKind::ElevatedPlatform:
        if (selection.index < level.elevatedPlatforms.size())
        {
            return &level.elevatedPlatforms[selection.index].center;
        }
        break;
    default:
        break;
    }
    return nullptr;
}

const core::Vec3* GetEditablePosition(
    const world::LevelDefinition& level,
    EditorSelection selection)
{
    switch (selection.kind)
    {
    case EditorObjectKind::Spawn:
        if (selection.index == 0)
        {
            return &level.initialSpawnVisualCenter;
        }
        break;
    case EditorObjectKind::Ground:
        if (selection.index == 0)
        {
            return &level.ground.center;
        }
        break;
    case EditorObjectKind::ElevatedPlatform:
        if (selection.index < level.elevatedPlatforms.size())
        {
            return &level.elevatedPlatforms[selection.index].center;
        }
        break;
    default:
        break;
    }
    return nullptr;
}

core::Vec3* GetEditableSize(world::LevelDefinition& level, EditorSelection selection)
{
    switch (selection.kind)
    {
    case EditorObjectKind::Ground:
        if (selection.index == 0)
        {
            return &level.ground.size;
        }
        break;
    case EditorObjectKind::ElevatedPlatform:
        if (selection.index < level.elevatedPlatforms.size())
        {
            return &level.elevatedPlatforms[selection.index].size;
        }
        break;
    default:
        break;
    }
    return nullptr;
}

const core::Vec3* GetEditableSize(
    const world::LevelDefinition& level,
    EditorSelection selection)
{
    switch (selection.kind)
    {
    case EditorObjectKind::Ground:
        if (selection.index == 0)
        {
            return &level.ground.size;
        }
        break;
    case EditorObjectKind::ElevatedPlatform:
        if (selection.index < level.elevatedPlatforms.size())
        {
            return &level.elevatedPlatforms[selection.index].size;
        }
        break;
    default:
        break;
    }
    return nullptr;
}

float ClampAuthoredBoxExtent(float value)
{
    if (!std::isfinite(value) || value < kMinAuthoredBoxExtent)
    {
        return kMinAuthoredBoxExtent;
    }
    return value;
}

core::Vec3 ClampAuthoredBoxSize(core::Vec3 size)
{
    return {
        ClampAuthoredBoxExtent(size.x),
        ClampAuthoredBoxExtent(size.y),
        ClampAuthoredBoxExtent(size.z)};
}

bool GetGizmoPreviewBox(
    const world::LevelDefinition& workingCopy,
    EditorSelection selection,
    core::Vec3& center,
    core::Vec3& size)
{
    switch (selection.kind)
    {
    case EditorObjectKind::Spawn:
        if (selection.index == 0)
        {
            center = workingCopy.initialSpawnVisualCenter;
            size = world::kPlayerVisualSize;
            return true;
        }
        break;
    case EditorObjectKind::Ground:
        if (selection.index == 0)
        {
            center = workingCopy.ground.center;
            size = workingCopy.ground.size;
            return true;
        }
        break;
    case EditorObjectKind::ElevatedPlatform:
        if (selection.index < workingCopy.elevatedPlatforms.size())
        {
            const world::Box& platform = workingCopy.elevatedPlatforms[selection.index];
            center = platform.center;
            size = platform.size;
            return true;
        }
        break;
    default:
        break;
    }
    return false;
}

float GizmoWorldLength(const render::CameraView& view, core::Vec3 origin)
{
    float distance = Length(Sub(origin, view.position));
    if (distance < 0.5f)
    {
        distance = 0.5f;
    }

    float fov = view.fieldOfViewY;
    if (!(fov > 1.0f) || !(fov < 179.0f))
    {
        fov = 40.0f;
    }

    const float length =
        distance * std::tan(0.5f * fov * kDegreesToRadians) * kGizmoViewHeightFraction;
    if (length < kGizmoMinWorldLength)
    {
        return kGizmoMinWorldLength;
    }
    if (length > kGizmoMaxWorldLength)
    {
        return kGizmoMaxWorldLength;
    }
    return length;
}

float GizmoVisualRadius(float axisLength)
{
    return axisLength * kGizmoVisualRadiusFraction;
}

float GizmoHitRadius(float axisLength)
{
    return axisLength * kGizmoHitRadiusFraction;
}

GizmoDrawRequest MakeGizmoDrawRequest(
    EditorSelection selection,
    const world::LevelDefinition& workingCopy,
    const render::CameraView& view,
    const GizmoInteractionState& interaction)
{
    GizmoDrawRequest request{};
    const core::Vec3* origin = GetEditablePosition(workingCopy, selection);
    if (origin == nullptr || !IsGizmoSelection(selection))
    {
        return request;
    }

    request.visible = true;
    request.origin = *origin;
    request.axisLength = GizmoWorldLength(view, request.origin);
    request.hovered = interaction.hovered;
    request.active = interaction.dragging ? interaction.active : EditorAxis::None;
    return request;
}

bool AuthoredGeometryDiffers(
    const world::LevelDefinition& active,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection)
{
    switch (selection.kind)
    {
    case EditorObjectKind::Spawn:
        return selection.index == 0
            && (active.initialSpawnVisualCenter.x != workingCopy.initialSpawnVisualCenter.x
                || active.initialSpawnVisualCenter.y != workingCopy.initialSpawnVisualCenter.y
                || active.initialSpawnVisualCenter.z != workingCopy.initialSpawnVisualCenter.z);
    case EditorObjectKind::Ground:
        return selection.index == 0 && BoxDiffers(active.ground, workingCopy.ground);
    case EditorObjectKind::ElevatedPlatform:
        return selection.index < active.elevatedPlatforms.size()
            && BoxDiffers(
                   active.elevatedPlatforms[selection.index],
                   workingCopy.elevatedPlatforms[selection.index]);
    default:
        return false;
    }
}

EditorPendingTransformPreview MakePendingTransformPreview(
    EditorSelection selection,
    const world::LevelDefinition& active,
    const world::LevelDefinition& workingCopy)
{
    EditorPendingTransformPreview preview{};
    if (!AuthoredGeometryDiffers(active, workingCopy, selection))
    {
        return preview;
    }
    if (!GetGizmoPreviewBox(workingCopy, selection, preview.center, preview.size))
    {
        return preview;
    }
    preview.visible = true;
    return preview;
}

EditorAxis PickGizmoHandle(Ray3 ray, core::Vec3 origin, float axisLength, float hitRadius)
{
    if (!(axisLength > 0.0f) || !(hitRadius > 0.0f))
    {
        return EditorAxis::None;
    }

    EditorAxis best = EditorAxis::None;
    float bestDistance = std::numeric_limits<float>::infinity();
    const EditorAxis axes[] = {EditorAxis::X, EditorAxis::Y, EditorAxis::Z};
    for (EditorAxis axis : axes)
    {
        const core::Vec3 direction = EditorAxisDirection(axis);
        const core::Vec3 shaftStart =
            Add(origin, Scale(direction, axisLength * kGizmoPickHubSkipFraction));
        const core::Vec3 tip = Add(origin, Scale(direction, axisLength));
        float rayDistance = 0.0f;
        const float distance = RaySegmentDistance(ray, shaftStart, tip, rayDistance);
        if (distance <= hitRadius && rayDistance >= 0.0f && distance < bestDistance)
        {
            bestDistance = distance;
            best = axis;
        }
    }
    return best;
}

bool BeginGizmoDrag(
    GizmoInteractionState& state,
    EditorSelection selection,
    EditorAxis axis,
    core::Vec3 workingPosition,
    Ray3 mouseRay,
    const render::CameraView& view)
{
    if (!IsGizmoSelection(selection) || axis == EditorAxis::None || !IsFiniteVec3(workingPosition))
    {
        return false;
    }

    float parameter = 0.0f;
    if (!AxisParameterFromRay(
            mouseRay, workingPosition, EditorAxisDirection(axis), view, parameter))
    {
        return false;
    }

    state.dragging = true;
    state.active = axis;
    state.hovered = axis;
    state.dragTarget = selection;
    state.dragStartPosition = workingPosition;
    state.dragStartAxisParameter = parameter;
    return true;
}

core::Vec3 GizmoDragPosition(
    const GizmoInteractionState& state,
    Ray3 mouseRay,
    const render::CameraView& view)
{
    if (!state.dragging || state.active == EditorAxis::None)
    {
        return state.dragStartPosition;
    }

    float parameter = 0.0f;
    if (!AxisParameterFromRay(
            mouseRay,
            state.dragStartPosition,
            EditorAxisDirection(state.active),
            view,
            parameter))
    {
        return state.dragStartPosition;
    }

    const float delta = parameter - state.dragStartAxisParameter;
    if (!std::isfinite(delta))
    {
        return state.dragStartPosition;
    }

    core::Vec3 position =
        Add(state.dragStartPosition, Scale(EditorAxisDirection(state.active), delta));
    if (!IsFiniteVec3(position))
    {
        return state.dragStartPosition;
    }
    return position;
}

void EndGizmoDrag(GizmoInteractionState& state)
{
    state.dragging = false;
    state.active = EditorAxis::None;
    state.dragTarget = {};
    state.dragStartPosition = {};
    state.dragStartAxisParameter = 0.0f;
    state.dragStartSize = {};
    state.dragHandleSign = 1;
}

GizmoDrawRequest MakeResizeGizmoDrawRequest(
    EditorSelection selection,
    const world::LevelDefinition& workingCopy,
    const render::CameraView& view,
    const GizmoInteractionState& interaction)
{
    GizmoDrawRequest request{};
    const core::Vec3* origin = GetEditablePosition(workingCopy, selection);
    if (origin == nullptr || !IsResizeSelection(selection))
    {
        return request;
    }

    request.visible = true;
    request.origin = *origin;
    request.axisLength = GizmoWorldLength(view, request.origin);
    request.hovered = interaction.hovered;
    request.hoveredSign = interaction.hoveredSign;
    request.active = interaction.dragging ? interaction.active : EditorAxis::None;
    request.activeSign = interaction.dragging ? interaction.dragHandleSign : 1;
    return request;
}

ResizeHandlePick PickResizeHandle(
    Ray3 ray,
    core::Vec3 origin,
    float axisLength,
    float hitRadius)
{
    ResizeHandlePick pick{};
    if (!(axisLength > 0.0f) || !(hitRadius > 0.0f) || LengthSquared(ray.direction) < kMinRayLengthSquared)
    {
        return pick;
    }

    float bestDistance = std::numeric_limits<float>::infinity();
    const EditorAxis axes[] = {EditorAxis::X, EditorAxis::Y, EditorAxis::Z};
    const int signs[] = {1, -1};
    for (EditorAxis axis : axes)
    {
        const core::Vec3 direction = EditorAxisDirection(axis);
        for (int sign : signs)
        {
            const core::Vec3 handle =
                Add(origin, Scale(direction, axisLength * static_cast<float>(sign)));
            float rayDistance = 0.0f;
            const float distance = RayPointDistance(ray, handle, rayDistance);
            if (distance <= hitRadius && rayDistance >= 0.0f && distance < bestDistance)
            {
                bestDistance = distance;
                pick.axis = axis;
                pick.sign = sign;
            }
        }
    }
    return pick;
}

bool BeginResizeDrag(
    GizmoInteractionState& state,
    EditorSelection selection,
    EditorAxis axis,
    int handleSign,
    core::Vec3 workingPosition,
    core::Vec3 workingSize,
    Ray3 mouseRay,
    const render::CameraView& view)
{
    if (!IsResizeSelection(selection) || axis == EditorAxis::None || handleSign == 0
        || !IsFiniteVec3(workingPosition) || !IsFiniteVec3(workingSize))
    {
        return false;
    }

    float parameter = 0.0f;
    if (!AxisParameterFromRay(
            mouseRay, workingPosition, EditorAxisDirection(axis), view, parameter))
    {
        return false;
    }

    state.dragging = true;
    state.active = axis;
    state.hovered = axis;
    state.dragTarget = selection;
    state.dragStartPosition = workingPosition;
    state.dragStartAxisParameter = parameter;
    state.dragStartSize = workingSize;
    state.dragHandleSign = handleSign > 0 ? 1 : -1;
    state.hoveredSign = state.dragHandleSign;
    return true;
}

core::Vec3 GizmoResizeSize(
    const GizmoInteractionState& state,
    Ray3 mouseRay,
    const render::CameraView& view)
{
    if (!state.dragging || state.active == EditorAxis::None)
    {
        return ClampAuthoredBoxSize(state.dragStartSize);
    }

    float parameter = 0.0f;
    if (!AxisParameterFromRay(
            mouseRay,
            state.dragStartPosition,
            EditorAxisDirection(state.active),
            view,
            parameter))
    {
        return ClampAuthoredBoxSize(state.dragStartSize);
    }

    const float delta = parameter - state.dragStartAxisParameter;
    if (!std::isfinite(delta))
    {
        return ClampAuthoredBoxSize(state.dragStartSize);
    }

    const float signedDelta = static_cast<float>(state.dragHandleSign) * delta;
    core::Vec3 size = state.dragStartSize;
    const float change = kResizeSizeFromAxisDelta * signedDelta;
    switch (state.active)
    {
    case EditorAxis::X:
        size.x = state.dragStartSize.x + change;
        break;
    case EditorAxis::Y:
        size.y = state.dragStartSize.y + change;
        break;
    case EditorAxis::Z:
        size.z = state.dragStartSize.z + change;
        break;
    case EditorAxis::None:
        break;
    }
    return ClampAuthoredBoxSize(size);
}

bool UpdateResizeInteraction(
    GizmoInteractionState& state,
    EditorSelection currentSelection,
    world::LevelDefinition& workingCopy,
    const render::CameraView& view,
    Ray3 mouseRay,
    bool imguiWantsMouse,
    bool lookHeld,
    bool selectPressed,
    bool selectHeld,
    bool selectReleased)
{
    if (state.dragging)
    {
        if (selectReleased || !selectHeld)
        {
            EndGizmoDrag(state);
            return true;
        }

        core::Vec3* size = GetEditableSize(workingCopy, state.dragTarget);
        if (size == nullptr)
        {
            ClearGizmoInteraction(state);
            return true;
        }

        *size = GizmoResizeSize(state, mouseRay, view);
        state.hovered = state.active;
        state.hoveredSign = state.dragHandleSign;
        return true;
    }

    if (imguiWantsMouse)
    {
        state.hovered = EditorAxis::None;
        state.hoveredSign = 1;
        return false;
    }

    if (!IsResizeSelection(currentSelection))
    {
        state.hovered = EditorAxis::None;
        state.hoveredSign = 1;
        return false;
    }

    const core::Vec3* origin = GetEditablePosition(workingCopy, currentSelection);
    const core::Vec3* size = GetEditableSize(workingCopy, currentSelection);
    if (origin == nullptr || size == nullptr)
    {
        state.hovered = EditorAxis::None;
        state.hoveredSign = 1;
        return false;
    }

    const float axisLength = GizmoWorldLength(view, *origin);
    const ResizeHandlePick pick =
        PickResizeHandle(mouseRay, *origin, axisLength, axisLength * kResizeHandleHitFraction);
    state.hovered = pick.axis;
    state.hoveredSign = pick.sign;
    if (lookHeld || !selectPressed || pick.axis == EditorAxis::None)
    {
        return false;
    }

    return BeginResizeDrag(
        state,
        currentSelection,
        pick.axis,
        pick.sign,
        *origin,
        *size,
        mouseRay,
        view);
}

bool UpdateGizmoInteraction(
    GizmoInteractionState& state,
    EditorSelection currentSelection,
    world::LevelDefinition& workingCopy,
    const render::CameraView& view,
    Ray3 mouseRay,
    bool imguiWantsMouse,
    bool lookHeld,
    bool selectPressed,
    bool selectHeld,
    bool selectReleased)
{
    if (state.dragging)
    {
        if (selectReleased || !selectHeld)
        {
            EndGizmoDrag(state);
            return true;
        }

        core::Vec3* position = GetEditablePosition(workingCopy, state.dragTarget);
        if (position == nullptr)
        {
            ClearGizmoInteraction(state);
            return true;
        }

        *position = GizmoDragPosition(state, mouseRay, view);
        state.hovered = state.active;
        return true;
    }

    if (imguiWantsMouse)
    {
        state.hovered = EditorAxis::None;
        return false;
    }

    if (!IsGizmoSelection(currentSelection))
    {
        state.hovered = EditorAxis::None;
        return false;
    }

    const core::Vec3* origin = GetEditablePosition(workingCopy, currentSelection);
    if (origin == nullptr)
    {
        state.hovered = EditorAxis::None;
        return false;
    }

    const float axisLength = GizmoWorldLength(view, *origin);
    state.hovered = PickGizmoHandle(mouseRay, *origin, axisLength, GizmoHitRadius(axisLength));
    if (lookHeld || !selectPressed || state.hovered == EditorAxis::None)
    {
        return false;
    }

    return BeginGizmoDrag(
        state, currentSelection, state.hovered, *origin, mouseRay, view);
}

bool IsFiniteVec3(core::Vec3 value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}
}
