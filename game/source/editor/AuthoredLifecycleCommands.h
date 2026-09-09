#pragma once

// Owner-side M41 lifecycle intents. ImGui emits LevelEditorRequest only;
// this handler mutates workingCopy. No Apply, Save, physics, or reload.

#include "editor/LevelEditor.h"

#include "core/Vec3.h"

#include <string>
#include <string_view>

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
    LevelEditorRequest request,
    std::string_view staticPropIdentity = {});

// Right-hand menu-row copy for Edit > Add > Static Prop. Static Prop is the
// only Add entry whose enablement comes from another window, so the row names
// the Content Browser asset it would consume instead of leaving the dependency
// invisible. Returns the placeholder when nothing usable is selected.
std::string AddStaticPropMenuHint(std::string_view staticPropIdentity);

// Hover copy for Edit > Add > Static Prop and Content Browser Add Static Prop.
// nullptr when the shared CanIssueAuthoredLifecycleRequest path is enabled.
const char* AddStaticPropDisableReason(
    bool authoringAvailable,
    const world::LevelDefinition& workingCopy,
    bool gizmoDragging,
    std::string_view staticPropIdentity);

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
