#include "editor/EditorGizmo.h"

#include "editor/AuthoredObjectLifecycle.h"
#include "editor/EditorMath.h"
#include "editor/StaticPropTransform.h"
#include "world/RespawnWorld.h"
#include "world/ItemPickup.h"
#include "world/StaticProp.h"

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

bool Vec3Differs(core::Vec3 a, core::Vec3 b)
{
    return a.x != b.x || a.y != b.y || a.z != b.z;
}

bool BoxDiffers(const world::Box& a, const world::Box& b)
{
    return Vec3Differs(a.center, b.center) || Vec3Differs(a.size, b.size);
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
    case EditorObjectKind::Checkpoint:
    case EditorObjectKind::Hazard:
    case EditorObjectKind::Collectible:
    case EditorObjectKind::DynamicBox:
    case EditorObjectKind::PressurePlate:
    case EditorObjectKind::Door:
    case EditorObjectKind::ItemPickup:
    case EditorObjectKind::StaticProp:
        return true;
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
    case EditorObjectKind::DynamicBox:
    case EditorObjectKind::PressurePlate:
    case EditorObjectKind::Door:
        return true;
    default:
        return false;
    }
}

bool IsScaleSelection(EditorSelection selection)
{
    return selection.kind == EditorObjectKind::StaticProp
        || selection.kind == EditorObjectKind::ItemPickup;
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
    case EditorObjectKind::Checkpoint:
        if (selection.index < level.checkpoints.size())
        {
            return &level.checkpoints[selection.index].center;
        }
        break;
    case EditorObjectKind::Hazard:
        if (selection.index < level.hazards.size())
        {
            return &level.hazards[selection.index].center;
        }
        break;
    case EditorObjectKind::Collectible:
        if (selection.index < level.collectibles.size())
        {
            return &level.collectibles[selection.index].center;
        }
        break;
    case EditorObjectKind::DynamicBox:
        if (selection.index < level.dynamicBoxes.size())
        {
            return &level.dynamicBoxes[selection.index].center;
        }
        break;
    case EditorObjectKind::PressurePlate:
        if (selection.index < level.pressurePlates.size())
        {
            return &level.pressurePlates[selection.index].center;
        }
        break;
    case EditorObjectKind::Door:
        if (selection.index < level.doors.size())
        {
            return &level.doors[selection.index].center;
        }
        break;
    case EditorObjectKind::ItemPickup:
        if (selection.index < level.itemPickups.size())
        {
            return &level.itemPickups[selection.index].position;
        }
        break;
    case EditorObjectKind::StaticProp:
        if (selection.index < level.staticProps.size())
        {
            return &level.staticProps[selection.index].position;
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
    case EditorObjectKind::Checkpoint:
        if (selection.index < level.checkpoints.size())
        {
            return &level.checkpoints[selection.index].center;
        }
        break;
    case EditorObjectKind::Hazard:
        if (selection.index < level.hazards.size())
        {
            return &level.hazards[selection.index].center;
        }
        break;
    case EditorObjectKind::Collectible:
        if (selection.index < level.collectibles.size())
        {
            return &level.collectibles[selection.index].center;
        }
        break;
    case EditorObjectKind::DynamicBox:
        if (selection.index < level.dynamicBoxes.size())
        {
            return &level.dynamicBoxes[selection.index].center;
        }
        break;
    case EditorObjectKind::PressurePlate:
        if (selection.index < level.pressurePlates.size())
        {
            return &level.pressurePlates[selection.index].center;
        }
        break;
    case EditorObjectKind::Door:
        if (selection.index < level.doors.size())
        {
            return &level.doors[selection.index].center;
        }
        break;
    case EditorObjectKind::ItemPickup:
        if (selection.index < level.itemPickups.size())
        {
            return &level.itemPickups[selection.index].position;
        }
        break;
    case EditorObjectKind::StaticProp:
        if (selection.index < level.staticProps.size())
        {
            return &level.staticProps[selection.index].position;
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
    case EditorObjectKind::DynamicBox:
        if (selection.index < level.dynamicBoxes.size())
        {
            return &level.dynamicBoxes[selection.index].size;
        }
        break;
    case EditorObjectKind::PressurePlate:
        if (selection.index < level.pressurePlates.size())
        {
            return &level.pressurePlates[selection.index].size;
        }
        break;
    case EditorObjectKind::Door:
        if (selection.index < level.doors.size())
        {
            return &level.doors[selection.index].size;
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
    case EditorObjectKind::DynamicBox:
        if (selection.index < level.dynamicBoxes.size())
        {
            return &level.dynamicBoxes[selection.index].size;
        }
        break;
    case EditorObjectKind::PressurePlate:
        if (selection.index < level.pressurePlates.size())
        {
            return &level.pressurePlates[selection.index].size;
        }
        break;
    case EditorObjectKind::Door:
        if (selection.index < level.doors.size())
        {
            return &level.doors[selection.index].size;
        }
        break;
    default:
        break;
    }
    return nullptr;
}

core::Vec3* GetEditableScale(world::LevelDefinition& level, EditorSelection selection)
{
    if (selection.kind == EditorObjectKind::StaticProp
        && selection.index < level.staticProps.size())
    {
        return &level.staticProps[selection.index].scale;
    }
    if (selection.kind == EditorObjectKind::ItemPickup
        && selection.index < level.itemPickups.size())
    {
        return &level.itemPickups[selection.index].visualScale;
    }
    return nullptr;
}

const core::Vec3* GetEditableScale(
    const world::LevelDefinition& level,
    EditorSelection selection)
{
    if (selection.kind == EditorObjectKind::StaticProp
        && selection.index < level.staticProps.size())
    {
        return &level.staticProps[selection.index].scale;
    }
    if (selection.kind == EditorObjectKind::ItemPickup
        && selection.index < level.itemPickups.size())
    {
        return &level.itemPickups[selection.index].visualScale;
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
    case EditorObjectKind::Checkpoint:
        if (selection.index < workingCopy.checkpoints.size())
        {
            const world::CheckpointSpec& checkpoint = workingCopy.checkpoints[selection.index];
            center = checkpoint.center;
            size = checkpoint.size;
            return true;
        }
        break;
    case EditorObjectKind::Hazard:
        if (selection.index < workingCopy.hazards.size())
        {
            const world::HazardSpec& hazard = workingCopy.hazards[selection.index];
            center = hazard.center;
            size = hazard.size;
            return true;
        }
        break;
    case EditorObjectKind::Collectible:
        if (selection.index < workingCopy.collectibles.size())
        {
            const world::CollectibleSpec& collectible = workingCopy.collectibles[selection.index];
            center = collectible.center;
            size = collectible.size;
            return true;
        }
        break;
    case EditorObjectKind::DynamicBox:
        if (selection.index < workingCopy.dynamicBoxes.size())
        {
            const world::DynamicBoxSpec& box = workingCopy.dynamicBoxes[selection.index];
            center = box.center;
            size = box.size;
            return true;
        }
        break;
    case EditorObjectKind::PressurePlate:
        if (selection.index < workingCopy.pressurePlates.size())
        {
            const world::PressurePlateSpec& plate = workingCopy.pressurePlates[selection.index];
            center = plate.center;
            size = plate.size;
            return true;
        }
        break;
    case EditorObjectKind::Door:
        if (selection.index < workingCopy.doors.size())
        {
            const world::DoorSpec& door = workingCopy.doors[selection.index];
            center = door.center;
            size = door.size;
            return true;
        }
        break;
    case EditorObjectKind::ItemPickup:
        if (selection.index < workingCopy.itemPickups.size())
        {
            ItemPickupEditorBounds(
                workingCopy.itemPickups[selection.index], center, size);
            return true;
        }
        break;
    case EditorObjectKind::StaticProp:
        if (selection.index < workingCopy.staticProps.size())
        {
            StaticPropWorldAabb(
                workingCopy.staticProps[selection.index],
                kStaticPropDefaultLocalMin,
                kStaticPropDefaultLocalMax,
                center,
                size);
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
    StructuralIndexMap identity{};
    ResetStructuralIndexMap(identity, active);
    return AuthoredGeometryDiffers(active, workingCopy, selection, identity);
}

bool AuthoredGeometryDiffers(
    const world::LevelDefinition& active,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection,
    const StructuralIndexMap& map)
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
    {
        if (selection.index >= workingCopy.elevatedPlatforms.size())
        {
            return false;
        }
        const int activeIndex =
            MappedActiveIndex(map, EditorObjectKind::ElevatedPlatform, selection.index);
        if (activeIndex < 0
            || static_cast<std::size_t>(activeIndex) >= active.elevatedPlatforms.size())
        {
            return true;
        }
        return BoxDiffers(
            active.elevatedPlatforms[static_cast<std::size_t>(activeIndex)],
            workingCopy.elevatedPlatforms[selection.index]);
    }
    case EditorObjectKind::Checkpoint:
    {
        if (selection.index >= workingCopy.checkpoints.size())
        {
            return false;
        }
        const int activeIndex =
            MappedActiveIndex(map, EditorObjectKind::Checkpoint, selection.index);
        if (activeIndex < 0
            || static_cast<std::size_t>(activeIndex) >= active.checkpoints.size())
        {
            return true;
        }
        const world::CheckpointSpec& activeCheckpoint =
            active.checkpoints[static_cast<std::size_t>(activeIndex)];
        const world::CheckpointSpec& workingCheckpoint = workingCopy.checkpoints[selection.index];
        return Vec3Differs(activeCheckpoint.center, workingCheckpoint.center)
            || Vec3Differs(activeCheckpoint.size, workingCheckpoint.size)
            || Vec3Differs(activeCheckpoint.respawnPosition, workingCheckpoint.respawnPosition);
    }
    case EditorObjectKind::Hazard:
    {
        if (selection.index >= workingCopy.hazards.size())
        {
            return false;
        }
        const int activeIndex = MappedActiveIndex(map, EditorObjectKind::Hazard, selection.index);
        if (activeIndex < 0
            || static_cast<std::size_t>(activeIndex) >= active.hazards.size())
        {
            return true;
        }
        const world::HazardSpec& activeHazard =
            active.hazards[static_cast<std::size_t>(activeIndex)];
        const world::HazardSpec& workingHazard = workingCopy.hazards[selection.index];
        return Vec3Differs(activeHazard.center, workingHazard.center)
            || Vec3Differs(activeHazard.size, workingHazard.size);
    }
    case EditorObjectKind::Collectible:
    {
        if (selection.index >= workingCopy.collectibles.size())
        {
            return false;
        }
        const int activeIndex =
            MappedActiveIndex(map, EditorObjectKind::Collectible, selection.index);
        if (activeIndex < 0
            || static_cast<std::size_t>(activeIndex) >= active.collectibles.size())
        {
            return true;
        }
        const world::CollectibleSpec& activeCollectible =
            active.collectibles[static_cast<std::size_t>(activeIndex)];
        const world::CollectibleSpec& workingCollectible =
            workingCopy.collectibles[selection.index];
        return Vec3Differs(activeCollectible.center, workingCollectible.center)
            || Vec3Differs(activeCollectible.size, workingCollectible.size);
    }
    case EditorObjectKind::DynamicBox:
    {
        if (selection.index >= workingCopy.dynamicBoxes.size())
        {
            return false;
        }
        const int activeIndex =
            MappedActiveIndex(map, EditorObjectKind::DynamicBox, selection.index);
        if (activeIndex < 0
            || static_cast<std::size_t>(activeIndex) >= active.dynamicBoxes.size())
        {
            return true;
        }
        const world::DynamicBoxSpec& activeBox =
            active.dynamicBoxes[static_cast<std::size_t>(activeIndex)];
        const world::DynamicBoxSpec& workingBox = workingCopy.dynamicBoxes[selection.index];
        return Vec3Differs(activeBox.center, workingBox.center)
            || Vec3Differs(activeBox.size, workingBox.size)
            || activeBox.massKg != workingBox.massKg;
    }
    case EditorObjectKind::PressurePlate:
    {
        if (selection.index >= workingCopy.pressurePlates.size())
        {
            return false;
        }
        const int activeIndex =
            MappedActiveIndex(map, EditorObjectKind::PressurePlate, selection.index);
        if (activeIndex < 0
            || static_cast<std::size_t>(activeIndex) >= active.pressurePlates.size())
        {
            return true;
        }
        const world::PressurePlateSpec& activePlate =
            active.pressurePlates[static_cast<std::size_t>(activeIndex)];
        const world::PressurePlateSpec& workingPlate = workingCopy.pressurePlates[selection.index];
        return Vec3Differs(activePlate.center, workingPlate.center)
            || Vec3Differs(activePlate.size, workingPlate.size)
            || activePlate.linkedDoorIndex != workingPlate.linkedDoorIndex
            || activePlate.activateByDynamicBox != workingPlate.activateByDynamicBox
            || activePlate.activateByPlayer != workingPlate.activateByPlayer
            || activePlate.visibleInGameplay != workingPlate.visibleInGameplay;
    }
    case EditorObjectKind::Door:
    {
        if (selection.index >= workingCopy.doors.size())
        {
            return false;
        }
        const int activeIndex =
            MappedActiveIndex(map, EditorObjectKind::Door, selection.index);
        if (activeIndex < 0
            || static_cast<std::size_t>(activeIndex) >= active.doors.size())
        {
            return true;
        }
        const world::DoorSpec& activeDoor = active.doors[static_cast<std::size_t>(activeIndex)];
        const world::DoorSpec& workingDoor = workingCopy.doors[selection.index];
        return Vec3Differs(activeDoor.center, workingDoor.center)
            || Vec3Differs(activeDoor.size, workingDoor.size)
            || activeDoor.openDistance != workingDoor.openDistance
            || activeDoor.requiredItemId != workingDoor.requiredItemId;
    }
    case EditorObjectKind::ItemPickup:
    {
        if (selection.index >= workingCopy.itemPickups.size())
        {
            return false;
        }
        const int activeIndex =
            MappedActiveIndex(map, EditorObjectKind::ItemPickup, selection.index);
        if (activeIndex < 0
            || static_cast<std::size_t>(activeIndex) >= active.itemPickups.size())
        {
            return true;
        }
        const world::ItemPickupSpec& activePickup =
            active.itemPickups[static_cast<std::size_t>(activeIndex)];
        const world::ItemPickupSpec& workingPickup = workingCopy.itemPickups[selection.index];
        return Vec3Differs(activePickup.position, workingPickup.position)
            || activePickup.itemId != workingPickup.itemId
            || activePickup.quantity != workingPickup.quantity
            || activePickup.modelIdentity != workingPickup.modelIdentity
            || Vec3Differs(activePickup.visualOffset, workingPickup.visualOffset)
            || Vec3Differs(
                activePickup.visualRotationDegrees, workingPickup.visualRotationDegrees)
            || Vec3Differs(activePickup.visualScale, workingPickup.visualScale);
    }
    case EditorObjectKind::StaticProp:
    {
        if (selection.index >= workingCopy.staticProps.size())
        {
            return false;
        }
        const int activeIndex =
            MappedActiveIndex(map, EditorObjectKind::StaticProp, selection.index);
        if (activeIndex < 0
            || static_cast<std::size_t>(activeIndex) >= active.staticProps.size())
        {
            return true;
        }
        const world::StaticPropSpec& activeProp =
            active.staticProps[static_cast<std::size_t>(activeIndex)];
        const world::StaticPropSpec& workingProp = workingCopy.staticProps[selection.index];
        return activeProp.modelIdentity != workingProp.modelIdentity
            || Vec3Differs(activeProp.position, workingProp.position)
            || Vec3Differs(activeProp.rotationDegrees, workingProp.rotationDegrees)
            || Vec3Differs(activeProp.scale, workingProp.scale);
    }
    default:
        return false;
    }
}

EditorPendingTransformPreview MakePendingTransformPreview(
    EditorSelection selection,
    const world::LevelDefinition& active,
    const world::LevelDefinition& workingCopy)
{
    StructuralIndexMap identity{};
    ResetStructuralIndexMap(identity, active);
    return MakePendingTransformPreview(selection, active, workingCopy, identity);
}

EditorPendingTransformPreview MakePendingTransformPreview(
    EditorSelection selection,
    const world::LevelDefinition& active,
    const world::LevelDefinition& workingCopy,
    const StructuralIndexMap& map)
{
    EditorPendingTransformPreview preview{};
    if (!AuthoredGeometryDiffers(active, workingCopy, selection, map))
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

bool SetCheckpointAssemblyCenter(
    world::LevelDefinition& workingCopy,
    std::size_t index,
    core::Vec3 newCenter)
{
    if (index >= workingCopy.checkpoints.size() || !IsFiniteVec3(newCenter))
    {
        return false;
    }

    world::CheckpointSpec& checkpoint = workingCopy.checkpoints[index];
    const core::Vec3 delta = Sub(newCenter, checkpoint.center);
    const core::Vec3 newRespawn = Add(checkpoint.respawnPosition, delta);
    if (!IsFiniteVec3(newRespawn))
    {
        return false;
    }

    checkpoint.center = newCenter;
    checkpoint.respawnPosition = newRespawn;
    return true;
}

CheckpointEditorOverlay MakeCheckpointEditorOverlay(
    EditorSelection selection,
    const world::LevelDefinition& workingCopy)
{
    CheckpointEditorOverlay overlay{};
    if (selection.kind != EditorObjectKind::Checkpoint
        || selection.index >= workingCopy.checkpoints.size())
    {
        return overlay;
    }

    const world::CheckpointSpec& checkpoint = workingCopy.checkpoints[selection.index];
    overlay.visible = true;
    overlay.triggerCenter = checkpoint.center;
    overlay.triggerSize = checkpoint.size;
    overlay.respawnPosition = checkpoint.respawnPosition;
    return overlay;
}

EditorPendingObjectVisual MakePendingObjectVisual(
    EditorSelection selection,
    const world::LevelDefinition& active,
    const world::LevelDefinition& workingCopy)
{
    StructuralIndexMap identity{};
    ResetStructuralIndexMap(identity, active);
    return MakePendingObjectVisual(selection, active, workingCopy, identity);
}

EditorPendingObjectVisual MakePendingObjectVisual(
    EditorSelection selection,
    const world::LevelDefinition& active,
    const world::LevelDefinition& workingCopy,
    const StructuralIndexMap& map)
{
    EditorPendingObjectVisual visual{};
    if (!HasDistinctPendingObjectVisual(selection.kind))
    {
        return visual;
    }

    switch (selection.kind)
    {
    case EditorObjectKind::Checkpoint:
        if (selection.index >= workingCopy.checkpoints.size())
        {
            return visual;
        }
        {
            const int activeIndex =
                MappedActiveIndex(map, EditorObjectKind::Checkpoint, selection.index);
            if (activeIndex >= 0
                && static_cast<std::size_t>(activeIndex) < active.checkpoints.size())
            {
                const world::CheckpointSpec& activeCheckpoint =
                    active.checkpoints[static_cast<std::size_t>(activeIndex)];
                const world::CheckpointSpec& workingCheckpoint =
                    workingCopy.checkpoints[selection.index];
                if (!Vec3Differs(activeCheckpoint.center, workingCheckpoint.center)
                    && !Vec3Differs(
                        activeCheckpoint.respawnPosition, workingCheckpoint.respawnPosition))
                {
                    return visual;
                }
            }
        }
        visual.visible = true;
        visual.kind = PendingObjectVisualKind::Checkpoint;
        visual.checkpoint = workingCopy.checkpoints[selection.index];
        return visual;
    case EditorObjectKind::Hazard:
        if (selection.index >= workingCopy.hazards.size())
        {
            return visual;
        }
        {
            const int activeIndex =
                MappedActiveIndex(map, EditorObjectKind::Hazard, selection.index);
            if (activeIndex >= 0
                && static_cast<std::size_t>(activeIndex) < active.hazards.size())
            {
                const world::HazardSpec& activeHazard =
                    active.hazards[static_cast<std::size_t>(activeIndex)];
                const world::HazardSpec& workingHazard = workingCopy.hazards[selection.index];
                if (!Vec3Differs(activeHazard.center, workingHazard.center)
                    && !Vec3Differs(activeHazard.size, workingHazard.size))
                {
                    return visual;
                }
            }
        }
        visual.visible = true;
        visual.kind = PendingObjectVisualKind::Hazard;
        visual.hazard = workingCopy.hazards[selection.index];
        return visual;
    case EditorObjectKind::Collectible:
        if (selection.index >= workingCopy.collectibles.size())
        {
            return visual;
        }
        {
            const int activeIndex =
                MappedActiveIndex(map, EditorObjectKind::Collectible, selection.index);
            if (activeIndex >= 0
                && static_cast<std::size_t>(activeIndex) < active.collectibles.size())
            {
                const world::CollectibleSpec& activeCollectible =
                    active.collectibles[static_cast<std::size_t>(activeIndex)];
                const world::CollectibleSpec& workingCollectible =
                    workingCopy.collectibles[selection.index];
                if (!Vec3Differs(activeCollectible.center, workingCollectible.center))
                {
                    return visual;
                }
            }
        }
        visual.visible = true;
        visual.kind = PendingObjectVisualKind::Collectible;
        visual.collectible = workingCopy.collectibles[selection.index];
        return visual;
    default:
        break;
    }
    return visual;
}

std::vector<PendingAuthoringVisual> CollectPendingAuthoringVisuals(
    const world::LevelDefinition& active,
    const world::LevelDefinition& workingCopy,
    const StructuralIndexMap& map,
    EditorSelection selection)
{
    std::vector<PendingAuthoringVisual> visuals;
    const EditorObjectKind kinds[] = {
        EditorObjectKind::ElevatedPlatform,
        EditorObjectKind::Checkpoint,
        EditorObjectKind::Hazard,
        EditorObjectKind::Collectible,
        EditorObjectKind::DynamicBox,
        EditorObjectKind::PressurePlate,
        EditorObjectKind::Door,
        EditorObjectKind::ItemPickup,
        EditorObjectKind::StaticProp};
    for (const EditorObjectKind kind : kinds)
    {
        const std::size_t count = [&]() -> std::size_t {
            switch (kind)
            {
            case EditorObjectKind::ElevatedPlatform:
                return workingCopy.elevatedPlatforms.size();
            case EditorObjectKind::Checkpoint:
                return workingCopy.checkpoints.size();
            case EditorObjectKind::Hazard:
                return workingCopy.hazards.size();
            case EditorObjectKind::Collectible:
                return workingCopy.collectibles.size();
            case EditorObjectKind::DynamicBox:
                return workingCopy.dynamicBoxes.size();
            case EditorObjectKind::PressurePlate:
                return workingCopy.pressurePlates.size();
            case EditorObjectKind::Door:
                return workingCopy.doors.size();
            case EditorObjectKind::ItemPickup:
                return workingCopy.itemPickups.size();
            case EditorObjectKind::StaticProp:
                return workingCopy.staticProps.size();
            default:
                return 0;
            }
        }();
        for (std::size_t index = 0; index < count; ++index)
        {
            const EditorSelection item{kind, index};
            if (!AuthoredGeometryDiffers(active, workingCopy, item, map))
            {
                continue;
            }
            PendingAuthoringVisual visual{};
            visual.kind = kind;
            visual.workingIndex = index;
            visual.selected = selection.kind == kind && selection.index == index;
            if (!GetGizmoPreviewBox(
                    workingCopy, item, visual.boundsCenter, visual.boundsSize))
            {
                continue;
            }
            const EditorPendingObjectVisual objectVisual =
                MakePendingObjectVisual(item, active, workingCopy, map);
            visual.hasObjectVisual = objectVisual.visible;
            if (kind == EditorObjectKind::Checkpoint)
            {
                visual.checkpoint = workingCopy.checkpoints[index];
            }
            else
            {
                visual.checkpoint = objectVisual.checkpoint;
            }
            visual.hazard = objectVisual.hazard;
            visual.collectible = objectVisual.collectible;
            if (kind == EditorObjectKind::Hazard)
            {
                visual.hazard = workingCopy.hazards[index];
            }
            if (kind == EditorObjectKind::Collectible)
            {
                visual.collectible = workingCopy.collectibles[index];
            }
            visuals.push_back(visual);
        }
    }
    return visuals;
}

int CollectEditorAuthoredCollectibleCenters(
    const world::LevelDefinition& active,
    const std::uint8_t* collected,
    std::size_t collectedCount,
    core::Vec3* outCenters,
    int maxCenters,
    const StructuralIndexMap* pendingDeleteMap)
{
    // Read-only vs CollectibleRunState: never writes collected flags.
    if (outCenters == nullptr || maxCenters <= 0)
    {
        return 0;
    }

    int count = 0;
    const std::size_t limit = active.collectibles.size() < collectedCount
        ? active.collectibles.size()
        : collectedCount;
    for (std::size_t index = 0; index < limit && count < maxCenters; ++index)
    {
        const std::uint8_t flag = collected == nullptr ? 0 : collected[index];
        const bool pendingDelete = pendingDeleteMap != nullptr
            && IsPendingDeleteActiveIndex(
                *pendingDeleteMap, EditorObjectKind::Collectible, index);
        if (ResolveCollectibleEditorVisualMode(pendingDelete, flag)
            != CollectibleEditorVisualMode::CollectedAuthored)
        {
            continue;
        }
        outCenters[count] = active.collectibles[index].center;
        ++count;
    }
    return count;
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
    state.dragStartScale = {};
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

bool TryScaleGizmoOrigin(
    const world::LevelDefinition& workingCopy,
    EditorSelection selection,
    core::Vec3& origin)
{
    if (selection.kind == EditorObjectKind::ItemPickup
        && selection.index < workingCopy.itemPickups.size())
    {
        origin = world::ItemPickupVisualPosition(workingCopy.itemPickups[selection.index]);
        return true;
    }
    const core::Vec3* position = GetEditablePosition(workingCopy, selection);
    if (position == nullptr)
    {
        return false;
    }
    origin = *position;
    return true;
}

GizmoDrawRequest MakeScaleGizmoDrawRequest(
    EditorSelection selection,
    const world::LevelDefinition& workingCopy,
    const render::CameraView& view,
    const GizmoInteractionState& interaction)
{
    GizmoDrawRequest request{};
    core::Vec3 origin{};
    if (!TryScaleGizmoOrigin(workingCopy, selection, origin) || !IsScaleSelection(selection)
        || GetEditableScale(workingCopy, selection) == nullptr)
    {
        return request;
    }

    request.visible = true;
    request.origin = origin;
    request.axisLength = GizmoWorldLength(view, request.origin);
    request.hovered = interaction.hovered;
    request.hoveredSign = interaction.hoveredSign;
    request.active = interaction.dragging ? interaction.active : EditorAxis::None;
    request.activeSign = interaction.dragging ? interaction.dragHandleSign : 1;
    return request;
}

bool BeginScaleDrag(
    GizmoInteractionState& state,
    EditorSelection selection,
    EditorAxis axis,
    int handleSign,
    core::Vec3 workingPosition,
    core::Vec3 workingScale,
    Ray3 mouseRay,
    const render::CameraView& view)
{
    if (!IsScaleSelection(selection) || axis == EditorAxis::None || handleSign == 0
        || !IsFiniteVec3(workingPosition) || !world::StaticPropScaleIsValid(workingScale))
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
    state.dragStartScale = workingScale;
    state.dragHandleSign = handleSign > 0 ? 1 : -1;
    state.hoveredSign = state.dragHandleSign;
    return true;
}

core::Vec3 GizmoScaleSize(
    const GizmoInteractionState& state,
    Ray3 mouseRay,
    const render::CameraView& view)
{
    if (!state.dragging || state.active == EditorAxis::None
        || !world::StaticPropScaleIsValid(state.dragStartScale))
    {
        return state.dragStartScale;
    }

    float parameter = 0.0f;
    if (!AxisParameterFromRay(
            mouseRay,
            state.dragStartPosition,
            EditorAxisDirection(state.active),
            view,
            parameter))
    {
        return state.dragStartScale;
    }

    const float delta = parameter - state.dragStartAxisParameter;
    if (!std::isfinite(delta))
    {
        return state.dragStartScale;
    }

    const float signedDelta = static_cast<float>(state.dragHandleSign) * delta;
    const float change = kScaleFromAxisDelta * signedDelta;
    core::Vec3 scale = state.dragStartScale;
    switch (state.active)
    {
    case EditorAxis::X:
        scale.x = world::ClampStaticPropScaleAxis(state.dragStartScale.x + change);
        break;
    case EditorAxis::Y:
        scale.y = world::ClampStaticPropScaleAxis(state.dragStartScale.y + change);
        break;
    case EditorAxis::Z:
        scale.z = world::ClampStaticPropScaleAxis(state.dragStartScale.z + change);
        break;
    case EditorAxis::None:
        break;
    }
    if (!world::StaticPropScaleIsValid(scale) || !IsFiniteVec3(scale))
    {
        return state.dragStartScale;
    }
    return scale;
}

bool UpdateScaleInteraction(
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

        core::Vec3* scale = GetEditableScale(workingCopy, state.dragTarget);
        if (scale == nullptr)
        {
            ClearGizmoInteraction(state);
            return true;
        }

        *scale = GizmoScaleSize(state, mouseRay, view);
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

    if (!IsScaleSelection(currentSelection))
    {
        state.hovered = EditorAxis::None;
        state.hoveredSign = 1;
        return false;
    }

    const core::Vec3* scale = GetEditableScale(workingCopy, currentSelection);
    core::Vec3 origin{};
    if (!TryScaleGizmoOrigin(workingCopy, currentSelection, origin) || scale == nullptr)
    {
        state.hovered = EditorAxis::None;
        state.hoveredSign = 1;
        return false;
    }

    const float axisLength = GizmoWorldLength(view, origin);
    const ResizeHandlePick pick =
        PickResizeHandle(mouseRay, origin, axisLength, axisLength * kResizeHandleHitFraction);
    state.hovered = pick.axis;
    state.hoveredSign = pick.sign;
    if (lookHeld || !selectPressed || pick.axis == EditorAxis::None)
    {
        return false;
    }

    return BeginScaleDrag(
        state,
        currentSelection,
        pick.axis,
        pick.sign,
        origin,
        *scale,
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

        const core::Vec3 newCenter = GizmoDragPosition(state, mouseRay, view);
        if (state.dragTarget.kind == EditorObjectKind::Checkpoint)
        {
            if (!SetCheckpointAssemblyCenter(workingCopy, state.dragTarget.index, newCenter))
            {
                ClearGizmoInteraction(state);
                return true;
            }
        }
        else
        {
            *position = newCenter;
        }
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
