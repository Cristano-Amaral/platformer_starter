#pragma once

// Owner-side M41 lifecycle intents. ImGui emits LevelEditorRequest only;
// this handler mutates workingCopy. No Apply, Save, physics, or reload.

#include "editor/LevelEditor.h"

#include "core/Vec3.h"

namespace editor
{
bool IsAuthoredLifecycleRequest(LevelEditorRequest request);

// Canonical Edit > Add menu mapping. Direct-add lifecycle request, not Object
// Palette placement mode. ImGui MenuItem must use this instead of constructing
// a parallel request set inside Draw().
LevelEditorRequest EditAddMenuRequest(EditorObjectKind kind);

// Development Edit-menu enable. Dirty and Modified are not parameters.
bool CanIssueAuthoredLifecycleRequest(
    bool authoringAvailable,
    const world::LevelDefinition& workingCopy,
    EditorSelection selection,
    bool gizmoDragging,
    LevelEditorRequest request);

// Mutates workingCopy, selection, structuralPending, structuralMap, gizmo,
// derived Modified/Dirty. Duplicate/Delete ignore placementAnchor.
// When worldCenterPlacement is false (Edit > Add), Add uses camera-region X/Y
// and authored Z from workingCopy spawn.z. When true (Object Palette confirm),
// placementAnchor is the resolved world center and is not snapped to spawn.z.
// Returns true when the request was a lifecycle intent (handled, whether it
// succeeded or was rejected).
bool HandleAuthoredLifecycleRequest(
    LevelEditorState& state,
    const world::LevelDefinition& activeLevel,
    LevelEditorRequest request,
    bool authoringAvailable,
    core::Vec3 placementAnchor = {},
    bool worldCenterPlacement = false);
}
