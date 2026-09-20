#pragma once

// Milestone 80: shared world-axis Group Rotate around the PRIMARY Rotate
// gizmo pivot. Snap the primary rotation result, then apply one signed angle
// delta to every member from drag-start transforms. Not a group-transform
// framework, scene graph, or persistent Authoring Group.

#include "core/Vec3.h"
#include "editor/EditorGizmo.h"
#include "editor/EditorGroupTranslate.h"
#include "editor/EditorMath.h"
#include "editor/EditorSelection.h"
#include "editor/EditorSelectionSet.h"
#include "editor/EditorSnap.h"
#include "editor/LocalLightAuthoring.h"
#include "world/ItemPickup.h"
#include "world/LevelDefinition.h"
#include "world/LocalLight.h"

#include <cmath>
#include <vector>

namespace editor
{
inline bool SelectionSupportsRotate(
    const world::LevelDefinition& workingCopy,
    EditorSelection selection)
{
    return IsRotateSelection(selection)
        && GetEditableRotation(workingCopy, selection) != nullptr
        && GetEditablePosition(workingCopy, selection) != nullptr;
}

inline bool SelectionIsGroupRotatePointOrbit(EditorSelection selection)
{
    return selection.kind == EditorObjectKind::PointLight;
}

inline bool SelectionIsGroupRotateSpot(EditorSelection selection)
{
    return selection.kind == EditorObjectKind::SpotLight;
}

inline bool CaptureGroupRotateStartRotation(
    const world::LevelDefinition& workingCopy,
    EditorSelection selection,
    core::Vec3& outRotation)
{
    if (SelectionIsGroupRotatePointOrbit(selection)
        && selection.index < workingCopy.pointLights.size())
    {
        // Point Lights have no authored orientation. Capture a dummy so the
        // existing start-rotation array stays aligned with members.
        outRotation = {};
        return true;
    }
    if (SelectionIsGroupRotateSpot(selection)
        && selection.index < workingCopy.spotLights.size())
    {
        const core::Vec3 direction = workingCopy.spotLights[selection.index].direction;
        if (!world::SpotLightDirectionIsValid(direction) || !IsFiniteVec3(direction))
        {
            return false;
        }
        outRotation = world::CanonicalSpotLightDirection(direction);
        return true;
    }
    const core::Vec3* rotation = GetEditableRotation(workingCopy, selection);
    if (rotation == nullptr || !IsFiniteVec3(*rotation))
    {
        return false;
    }
    outRotation = *rotation;
    return true;
}

inline bool SelectionSupportsGroupRotateMember(
    const world::LevelDefinition& workingCopy,
    EditorSelection selection)
{
    if (GetEditablePosition(workingCopy, selection) == nullptr)
    {
        return false;
    }
    core::Vec3 ignored{};
    return CaptureGroupRotateStartRotation(workingCopy, selection, ignored);
}

inline bool SelectionDrivesGroupRotate(
    const world::LevelDefinition& workingCopy,
    EditorSelection selection)
{
    return SelectionHasRotateOrientation(workingCopy, selection)
        && GetEditablePosition(workingCopy, selection) != nullptr;
}

inline bool EditorSelectionSetSupportsGroupRotate(
    const world::LevelDefinition& workingCopy,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional)
{
    const std::vector<EditorSelection> members =
        EditorSelectionSetMembers(primary, additional);
    if (members.size() < 2 || !SelectionDrivesGroupRotate(workingCopy, primary))
    {
        return false;
    }
    for (const EditorSelection& member : members)
    {
        if (!SelectionSupportsGroupRotateMember(workingCopy, member))
        {
            return false;
        }
    }
    return true;
}

inline const char* GroupRotateDisableReason(
    const world::LevelDefinition& workingCopy,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional)
{
    if (!EditorSelectionSetIsMulti(primary, additional))
    {
        return nullptr;
    }
    if (EditorSelectionSetSupportsGroupRotate(workingCopy, primary, additional))
    {
        return nullptr;
    }
    return "Group Rotate blocked: selection contains a non-rotatable object.";
}

// PRIMARY Rotate gizmo origin: Item Pickup visual authored origin, otherwise
// the same authored position/center used by Translate.
inline bool TryPrimaryRotatePivot(
    const world::LevelDefinition& workingCopy,
    EditorSelection primary,
    core::Vec3& outPivot)
{
    if (primary.kind == EditorObjectKind::ItemPickup
        && primary.index < workingCopy.itemPickups.size())
    {
        outPivot = world::ItemPickupVisualPosition(workingCopy.itemPickups[primary.index]);
        return IsFiniteVec3(outPivot);
    }
    const core::Vec3* position = GetEditablePosition(workingCopy, primary);
    if (position == nullptr || !IsFiniteVec3(*position))
    {
        return false;
    }
    outPivot = *position;
    return true;
}

inline float Vec3AxisComponent(core::Vec3 value, EditorAxis axis)
{
    switch (axis)
    {
    case EditorAxis::X:
        return value.x;
    case EditorAxis::Y:
        return value.y;
    case EditorAxis::Z:
        return value.z;
    case EditorAxis::None:
        break;
    }
    return 0.0f;
}

inline core::Vec3 AddVec3AxisComponent(core::Vec3 value, EditorAxis axis, float delta)
{
    switch (axis)
    {
    case EditorAxis::X:
        value.x += delta;
        break;
    case EditorAxis::Y:
        value.y += delta;
        break;
    case EditorAxis::Z:
        value.z += delta;
        break;
    case EditorAxis::None:
        break;
    }
    return value;
}

inline core::Vec3 ApplyAuthoredRotationDelta(
    core::Vec3 startRotation,
    EditorAxis axis,
    float deltaDegrees)
{
    return AddVec3AxisComponent(startRotation, axis, deltaDegrees);
}

inline float SharedRotationDelta(
    core::Vec3 primaryStartRotation,
    core::Vec3 primaryResultRotation,
    EditorAxis axis)
{
    return Vec3AxisComponent(primaryResultRotation, axis)
        - Vec3AxisComponent(primaryStartRotation, axis);
}

inline core::Vec3 SnappedPrimaryRotateResult(
    core::Vec3 intended,
    EditorAxis axis,
    bool snapEnabled,
    float increment)
{
    return ApplyAuthoredTransformSnap(
        intended, EditorTransformMode::Rotate, axis, snapEnabled, increment);
}

inline core::Vec3 RotateAroundWorldAxis(
    core::Vec3 relative,
    EditorAxis axis,
    float deltaDegrees)
{
    switch (axis)
    {
    case EditorAxis::X:
        return RotateX(relative, deltaDegrees);
    case EditorAxis::Y:
        return RotateY(relative, deltaDegrees);
    case EditorAxis::Z:
        return RotateZ(relative, deltaDegrees);
    case EditorAxis::None:
        break;
    }
    return relative;
}

inline core::Vec3 OrbitAuthoredPositionAroundPivot(
    core::Vec3 startPosition,
    core::Vec3 pivot,
    EditorAxis axis,
    float deltaDegrees)
{
    const core::Vec3 relative = Sub(startPosition, pivot);
    return pivot + RotateAroundWorldAxis(relative, axis, deltaDegrees);
}

inline bool ApplyAuthoredRotationTo(
    world::LevelDefinition& workingCopy,
    EditorSelection selection,
    core::Vec3 newRotation)
{
    if (!IsFiniteVec3(newRotation) || !SelectionSupportsRotate(workingCopy, selection))
    {
        return false;
    }
    core::Vec3* rotation = GetEditableRotation(workingCopy, selection);
    if (rotation == nullptr)
    {
        return false;
    }
    *rotation = newRotation;
    return true;
}

inline bool CaptureGroupRotateStarts(
    const world::LevelDefinition& workingCopy,
    const std::vector<EditorSelection>& members,
    std::vector<core::Vec3>& outPositions,
    std::vector<core::Vec3>& outRotations)
{
    outPositions.clear();
    outRotations.clear();
    outPositions.reserve(members.size());
    outRotations.reserve(members.size());
    for (const EditorSelection& member : members)
    {
        const core::Vec3* position = GetEditablePosition(workingCopy, member);
        core::Vec3 startRotation{};
        if (position == nullptr || !IsFiniteVec3(*position)
            || !CaptureGroupRotateStartRotation(workingCopy, member, startRotation))
        {
            outPositions.clear();
            outRotations.clear();
            return false;
        }
        outPositions.push_back(*position);
        outRotations.push_back(startRotation);
    }
    return !members.empty();
}

inline bool ApplyGroupRotateOrientationTo(
    world::LevelDefinition& workingCopy,
    EditorSelection selection,
    core::Vec3 nextOrientation)
{
    if (SelectionIsGroupRotatePointOrbit(selection))
    {
        return selection.index < workingCopy.pointLights.size();
    }
    if (SelectionIsGroupRotateSpot(selection))
    {
        if (selection.index >= workingCopy.spotLights.size())
        {
            return false;
        }
        return TryCommitAuthoredSpotDirection(
            nextOrientation, workingCopy.spotLights[selection.index].direction);
    }
    return ApplyAuthoredRotationTo(workingCopy, selection, nextOrientation);
}

inline core::Vec3 NextGroupRotateOrientation(
    EditorSelection selection,
    core::Vec3 startOrientation,
    EditorAxis axis,
    float sharedDeltaDegrees)
{
    if (SelectionIsGroupRotatePointOrbit(selection))
    {
        return startOrientation;
    }
    if (SelectionIsGroupRotateSpot(selection))
    {
        return RotateAuthoredSpotDirection(
            startOrientation, EditorAxisDirection(axis), sharedDeltaDegrees);
    }
    return ApplyAuthoredRotationDelta(startOrientation, axis, sharedDeltaDegrees);
}

// members[0] is PRIMARY. PRIMARY authored position stays at its drag-start
// value. Secondaries orbit primaryPivot. Every member orientation receives
// the same signed world-axis delta. Results are computed from drag-start
// snapshots, never from the previous frame.
inline bool ApplySharedGroupRotation(
    world::LevelDefinition& workingCopy,
    const std::vector<EditorSelection>& members,
    const std::vector<core::Vec3>& startPositions,
    const std::vector<core::Vec3>& startRotations,
    core::Vec3 primaryPivot,
    EditorAxis axis,
    float sharedDeltaDegrees)
{
    if (members.size() < 2 || members.size() != startPositions.size()
        || members.size() != startRotations.size() || axis == EditorAxis::None)
    {
        return false;
    }
    if (!IsFiniteVec3(primaryPivot) || !std::isfinite(sharedDeltaDegrees))
    {
        return false;
    }

    std::vector<core::Vec3> nextPositions;
    std::vector<core::Vec3> nextRotations;
    nextPositions.reserve(members.size());
    nextRotations.reserve(members.size());
    for (std::size_t index = 0; index < members.size(); ++index)
    {
        if (!SelectionSupportsGroupRotateMember(workingCopy, members[index])
            || !IsFiniteVec3(startPositions[index]) || !IsFiniteVec3(startRotations[index]))
        {
            return false;
        }

        core::Vec3 nextPosition = startPositions[index];
        if (index != 0)
        {
            nextPosition = OrbitAuthoredPositionAroundPivot(
                startPositions[index], primaryPivot, axis, sharedDeltaDegrees);
        }
        const core::Vec3 nextRotation = NextGroupRotateOrientation(
            members[index], startRotations[index], axis, sharedDeltaDegrees);
        if (!IsFiniteVec3(nextPosition) || !IsFiniteVec3(nextRotation))
        {
            return false;
        }
        if (SelectionIsGroupRotateSpot(members[index])
            && !world::SpotLightDirectionIsValid(nextRotation))
        {
            return false;
        }
        nextPositions.push_back(nextPosition);
        nextRotations.push_back(nextRotation);
    }

    for (std::size_t index = 0; index < members.size(); ++index)
    {
        if (!ApplyAuthoredTranslationTo(workingCopy, members[index], nextPositions[index])
            || !ApplyGroupRotateOrientationTo(
                   workingCopy, members[index], nextRotations[index]))
        {
            return false;
        }
    }
    return true;
}

// Snap only the primary intended rotation result, then apply that exact
// signed axis delta to every captured member from drag-start transforms.
// Does not independently snap member rotations or secondary orbit positions.
inline bool ApplyGroupRotateFromPrimaryResult(
    world::LevelDefinition& workingCopy,
    const std::vector<EditorSelection>& members,
    const std::vector<core::Vec3>& startPositions,
    const std::vector<core::Vec3>& startRotations,
    core::Vec3 primaryPivot,
    core::Vec3 primaryIntendedRotation,
    EditorAxis axis,
    bool snapEnabled,
    float increment)
{
    if (members.size() < 2 || members.size() != startPositions.size()
        || members.size() != startRotations.size())
    {
        return false;
    }
    const core::Vec3 snappedPrimary = SnappedPrimaryRotateResult(
        primaryIntendedRotation, axis, snapEnabled, increment);
    const float sharedDelta =
        SharedRotationDelta(startRotations[0], snappedPrimary, axis);
    return ApplySharedGroupRotation(
        workingCopy,
        members,
        startPositions,
        startRotations,
        primaryPivot,
        axis,
        sharedDelta);
}
}
