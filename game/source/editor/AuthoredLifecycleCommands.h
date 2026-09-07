#pragma once

// Owner-side M41 lifecycle intents. ImGui emits LevelEditorRequest only;
// this handler mutates workingCopy. No Apply, Save, physics, or reload.

#include "editor/LevelEditor.h"

#include "core/Vec3.h"

namespace editor
{
bool IsAuthoredLifecycleRequest(LevelEditorRequest request);

// Development Edit-menu enable. Dirty and Modified are not parameters.
bool CanIssueAuthoredLifecycleRequest(
    bool authoringAvailable,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection,
    bool gizmoDragging,
    LevelEditorRequest request);

// Mutates workingCopy, selection, structuralPending, structuralMap, gizmo,
// derived Modified/Dirty. placementAnchor is used only by Add: camera-region
// X/Y, with authored Z from workingCopy spawn.z. Duplicate/Delete ignore it.
// Returns true when the request was a lifecycle intent (handled, whether it
// succeeded or was rejected).
bool HandleAuthoredLifecycleRequest(
    LevelEditorState& state,
    const world::LevelDefinition& activeLevel,
    LevelEditorRequest request,
    bool authoringAvailable,
    core::Vec3 placementAnchor = {});
}
