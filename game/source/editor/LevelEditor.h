#pragma once

// Development/Debug level editor (Milestone 32 + Milestone 33 Phase B).
//
// Application stays the owner of the active world::LevelDefinition and of all
// gameplay state. The editor owns only its working copy of authored data and
// asks Application to perform Apply/Revert/Save through a returned request, so
// physics rebuilds and gameplay resets stay where ownership already lives.
//
// M33 adds session camera, one selection, Hierarchy, and Inspector routing.
// No property/value model, no reflection, no undo stack, no manager types.

#include "world/LevelDefinition.h"

#include "core/Vec3.h"
#include "editor/EditorCamera.h"
#include "editor/EditorGizmo.h"
#include "editor/EditorSelection.h"

#include <string>

namespace editor
{
// Authored values M32 exposes. Nothing else is editable.
inline constexpr int kEditableSpawnCount = 1;
inline constexpr int kEditableCameraCount = 2;
inline constexpr int kEditableGroundCount = 1;
inline constexpr int kEditableElevatedPlatformCount = 6;

enum class LevelEditorApplyStatus
{
    NotAttempted,
    Applied,
    Invalid,
    Error,
};

enum class LevelEditorSaveStatus
{
    NotAttempted,
    Saved,
    Invalid,
    Error,
};

// What the panel asks Application to do this frame. Application executes it
// after the frame is presented, so no frame ever draws mismatched geometry.
enum class LevelEditorRequest
{
    None,
    ApplyPreview,
    RevertWorkingCopy,
    SaveLevelSource,
};

const char* LevelEditorApplyStatusName(LevelEditorApplyStatus status);
const char* LevelEditorSaveStatusName(LevelEditorSaveStatus status);

struct LevelEditorState
{
    // Simulation is paused while this is true.
    bool active = false;

    // Authored data the panel edits. Reset from the active definition every
    // time an editor session begins. Contains no runtime gameplay state.
    world::LevelDefinition workingCopy{};

    // Authored data as of the last successful source save in this session,
    // seeded at Initialize from the loaded staged level so Dirty starts false.
    world::LevelDefinition savedSourceBaseline{};

    // Derived each draw by comparing authored data, never inferred from widget
    // return values: working copy differs from the active/applied definition.
    bool modified = false;
    // Derived each draw: active/applied definition differs from the last
    // successfully saved source state.
    bool dirty = false;

    // Single selection shared by Hierarchy, world picking, Inspector, and
    // highlight. Not persisted. Survives F2 close/reopen in this process.
    EditorSelection selection{};
    EditorCamera editorCamera{};
    GizmoInteractionState gizmo{};
    // Set by Reset Editor Layout; DebugUi consumes it over the following
    // frames so Metrics (drawn before the button) also snaps to defaults.
    int forceDefaultLayoutFrames = 0;
    LevelEditorApplyStatus lastApplyStatus = LevelEditorApplyStatus::NotAttempted;
    LevelEditorSaveStatus lastSaveStatus = LevelEditorSaveStatus::NotAttempted;
    // Short human-readable context for the last Apply or Save. Not a log.
    std::string lastMessage;
};

struct LevelEditorSaveResult
{
    LevelEditorSaveStatus status = LevelEditorSaveStatus::Error;
    std::string message;
};

// Frozen runtime poses and path text the Inspector may display read-only.
// Not authored data and never written back into workingCopy.
struct LevelEditorViewContext
{
    const char* runtimeLevelPath = "";
    core::Vec3 movingPlatformRuntimeCenter{};
    core::Vec3 dynamicBoxRuntimeCenter{};
    float viewportWidth = 1280.0f;
    float viewportHeight = 720.0f;
    bool forceDefaultLayout = false;
    bool recoverOffscreenLayout = false;
};

// Serialize the supplied authored definition to the canonical project source
// through the Development authoring root. Returns Error without touching any
// file where source authoring is not compiled in (Debug and Release).
LevelEditorSaveResult SaveLevelSource(const world::LevelDefinition& level);

// Draws Hierarchy, Inspector, and Level/Save controls. Must be called inside
// an active Dear ImGui frame. Updates state.modified / state.dirty and edits
// state.workingCopy only. No-op returning None where Dear ImGui is not
// compiled in, so Release links no editor UI.
LevelEditorRequest DrawLevelEditor(
    LevelEditorState& state,
    const world::LevelDefinition& activeLevel,
    const LevelEditorViewContext& view);
}
