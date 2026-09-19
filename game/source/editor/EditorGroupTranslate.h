#pragma once

// Milestone 78: shared world-space Group Translate. Snap the primary result,
// then apply one delta to every member. Not a group-transform framework.

#include "core/Vec3.h"
#include "editor/EditorGizmo.h"
#include "editor/EditorMath.h"
#include "editor/EditorSelection.h"
#include "editor/EditorSelectionSet.h"
#include "editor/EditorSnap.h"
#include "world/LevelDefinition.h"

#include <vector>

namespace editor
{
inline bool SelectionSupportsTranslate(
    const world::LevelDefinition& workingCopy,
    EditorSelection selection)
{
    return IsGizmoSelection(selection)
        && GetEditablePosition(workingCopy, selection) != nullptr;
}

inline bool EditorSelectionSetSupportsGroupTranslate(
    const world::LevelDefinition& workingCopy,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional)
{
    const std::vector<EditorSelection> members =
        EditorSelectionSetMembers(primary, additional);
    if (members.empty())
    {
        return false;
    }
    for (const EditorSelection& member : members)
    {
        if (!SelectionSupportsTranslate(workingCopy, member))
        {
            return false;
        }
    }
    return true;
}

inline const char* GroupTranslateDisableReason(
    const world::LevelDefinition& workingCopy,
    EditorSelection primary,
    const std::vector<EditorSelection>& additional)
{
    if (!EditorSelectionSetIsMulti(primary, additional))
    {
        return nullptr;
    }
    if (EditorSelectionSetSupportsGroupTranslate(workingCopy, primary, additional))
    {
        return nullptr;
    }
    return "Group Translate blocked: selection contains a non-translatable object.";
}

inline bool EditorGroupAllowsTransformMode(EditorTransformMode mode, bool multiSelected)
{
    if (!multiSelected)
    {
        return true;
    }
    return mode == EditorTransformMode::Translate || mode == EditorTransformMode::Rotate;
}

inline const char* MultiSelectionTransformDisableReason(EditorTransformMode mode)
{
    switch (mode)
    {
    case EditorTransformMode::Resize:
        return "Resize is not a group operation.";
    case EditorTransformMode::Scale:
        return "Scale is not a group operation.";
    case EditorTransformMode::Translate:
    case EditorTransformMode::Rotate:
        break;
    }
    return nullptr;
}

inline core::Vec3 SharedTranslationDelta(core::Vec3 primaryStart, core::Vec3 primaryResult)
{
    return Sub(primaryResult, primaryStart);
}

inline core::Vec3 SnappedPrimaryTranslateResult(
    core::Vec3 intended,
    EditorAxis axis,
    bool snapEnabled,
    float increment)
{
    return ApplyAuthoredTransformSnap(
        intended, EditorTransformMode::Translate, axis, snapEnabled, increment);
}

inline bool ApplyAuthoredTranslationTo(
    world::LevelDefinition& workingCopy,
    EditorSelection selection,
    core::Vec3 newPosition)
{
    if (!IsFiniteVec3(newPosition) || !SelectionSupportsTranslate(workingCopy, selection))
    {
        return false;
    }
    if (selection.kind == EditorObjectKind::Checkpoint)
    {
        return SetCheckpointAssemblyCenter(workingCopy, selection.index, newPosition);
    }
    core::Vec3* position = GetEditablePosition(workingCopy, selection);
    if (position == nullptr)
    {
        return false;
    }
    *position = newPosition;
    return true;
}

inline bool CaptureGroupTranslateStarts(
    const world::LevelDefinition& workingCopy,
    const std::vector<EditorSelection>& members,
    std::vector<core::Vec3>& outStarts)
{
    outStarts.clear();
    outStarts.reserve(members.size());
    for (const EditorSelection& member : members)
    {
        const core::Vec3* position = GetEditablePosition(workingCopy, member);
        if (position == nullptr || !IsFiniteVec3(*position))
        {
            outStarts.clear();
            return false;
        }
        outStarts.push_back(*position);
    }
    return true;
}

inline bool ApplySharedTranslationDelta(
    world::LevelDefinition& workingCopy,
    const std::vector<EditorSelection>& members,
    const std::vector<core::Vec3>& starts,
    core::Vec3 sharedDelta)
{
    if (members.size() != starts.size() || members.empty())
    {
        return false;
    }
    if (!IsFiniteVec3(sharedDelta))
    {
        return false;
    }

    std::vector<core::Vec3> nextPositions;
    nextPositions.reserve(members.size());
    for (std::size_t index = 0; index < members.size(); ++index)
    {
        const core::Vec3 next = starts[index] + sharedDelta;
        if (!IsFiniteVec3(next) || !SelectionSupportsTranslate(workingCopy, members[index]))
        {
            return false;
        }
        nextPositions.push_back(next);
    }

    for (std::size_t index = 0; index < members.size(); ++index)
    {
        if (!ApplyAuthoredTranslationTo(workingCopy, members[index], nextPositions[index]))
        {
            return false;
        }
    }
    return true;
}

// Snap only the primary intended result, then apply that exact delta to every
// captured member from drag-start poses. Does not independently snap members.
inline bool ApplyGroupTranslateFromPrimaryResult(
    world::LevelDefinition& workingCopy,
    const std::vector<EditorSelection>& members,
    const std::vector<core::Vec3>& starts,
    core::Vec3 primaryIntended,
    EditorAxis axis,
    bool snapEnabled,
    float increment)
{
    if (members.empty() || members.size() != starts.size())
    {
        return false;
    }
    const core::Vec3 snappedPrimary =
        SnappedPrimaryTranslateResult(primaryIntended, axis, snapEnabled, increment);
    const core::Vec3 sharedDelta = SharedTranslationDelta(starts[0], snappedPrimary);
    return ApplySharedTranslationDelta(workingCopy, members, starts, sharedDelta);
}
}
