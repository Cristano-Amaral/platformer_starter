## Milestone 32 --- Development Level Editor v1

### Status

Phase A complete: design resolved, Level Format v1 writer implemented and
tested, Development authoring boundary in place. Phase B complete: the
Development editor is live — F2 toggle, whole-simulation pause, editable
working copy, validated Apply Preview with PhysicsWorld rebuild, Modified and
Dirty tracking, and explicit source Save. M32 is complete and merged.

### Branch

`milestone/32-development-level-editor-v1`

### Goal

Create the first Development-only in-game level editor on top of M31's
external `PLATFORMER_LEVEL 1` data contract.

M32 proves that the Development build can enter an editor mode, inspect
the active `world::LevelDefinition`, edit a deliberately small set of
authored properties, preview changes consistently in rendering/physics,
and explicitly save a deterministic Level Format v1 file back to the
canonical project source.

This is the architectural foundation for the larger map/game editor
discussed for later milestones; it is not yet a complete editor.

### Development contract

-   M31 gameplay remains unchanged while the editor is closed.
-   `F2` is the preferred semantic toggle unless inspection finds a
    conflict.
-   Editor mode pauses gameplay simulation.
-   Dear ImGui exposes a Level Editor panel.
-   First editable set: initial spawn, camera offset/FOV, and
    static/elevated platform center/size.
-   A platform can be selected from the UI.
-   Preview must keep rendered geometry and Jolt collision consistent.
-   Save is explicit; no autosave.
-   Save targets the canonical source
    `game/assets/source/levels/level_01.level`, never the staged runtime
    copy.
-   Normal cooker/build remains responsible for source → cooked → staged
    propagation.
-   Release exposes no editor UI or authoring write path.

### Architecture

`Application` remains owner of the active `LevelDefinition`. A focused
`editor/LevelEditor` module may inspect/edit it but must not become
gameplay ownership.

Add a focused deterministic Level Format v1 serializer/writer as the
inverse companion to the M31 parser. Do not add generic serialization,
reflection, a scene manager, entity manager, ECS, or a general property
framework.

The source-authoring path must be resolved explicitly for Development
tooling and must not depend on CWD or heuristic parent-directory
searching. Phase A must inspect CMake/build configuration and design
this boundary before live writing is enabled.

### Simulation pause

While editor mode is active: - no gameplay timer advancement; - no
Player simulation; - no moving-platform progression; - no dynamic-box
simulation; - no checkpoint/hazard/collectible/goal processing; - no
deaths or BEST changes.

Editor UI remains responsive.

### Preview rules

-   Camera offset/FOV may preview immediately.
-   Spawn edits change authored future fresh-run spawn; they must not
    cause arbitrary teleports on every field edit.
-   Static/elevated platform edits must update rendering and collision
    together. Visual-only movement with stale Jolt collision is
    forbidden.
-   A controlled physics/world rebuild while paused is acceptable if it
    is the smallest safe solution.

### Writer/save rules

-   Canonical header remains `PLATFORMER_LEVEL 1`.
-   Deterministic canonical record ordering.
-   Locale-independent numeric formatting with enough precision for
    round-trip.
-   Existing semantic validation must pass before save.
-   Invalid/non-finite/zero-or-negative required values must not be
    saved.
-   Prefer temp-file + safe replacement rather than truncate-in-place.
-   Successful save clears dirty state.
-   Save failure is reported and does not pretend success.

### Dirty state

Expose at least Clean/Dirty and last save result. Merely opening the
editor must not mark the level dirty.

### Explicitly out of scope

Level 02, transitions, campaign, enemies, bosses, combat, full object
creation/deletion, 3D gizmos, world mouse picking, undo/redo,
copy/paste, hierarchy/scene graph, generic inspector/reflection, asset
browser, material/audio/animation/terrain editors, automatic
Python/CMake spawning from the game, asset packs, Web/mobile/Pi editor,
save v2, per-level BEST, LevelManager, SceneManager, ECS, event bus.

### Phases

#### Phase A --- Inspection + writer/editor scaffolding

Inspect configuration boundaries, frame loop, Dear ImGui,
LevelDefinition/parser/validation, physics rebuild constraints,
runtime/source paths and CMake. Design Development authoring-root
resolution and editor ownership. Implement deterministic Level Format v1
serialization/writer tests and non-live scaffolding as appropriate. Do
not enable live editing/saving yet.

**Phase A outcome (complete):**

Writer. `world/LevelWriter.h/.cpp` beside the M31 parser.
`SerializeLevelText` emits one canonical record order (documented in
`docs/LEVEL_FORMAT_V1.md`) using `std::to_chars` shortest round-trip form:
locale-independent, exactly reparsable, never NaN/Inf.
`IsWritableLevelDefinition` = `IsValidLevelIdToken` +
`LevelDefinitionHasRequiredAuthoredContent` gates serialization and saving, so
an invalid definition yields no text and touches no file. `SaveLevelFile`
requires an absolute path, writes `<target>.tmp`, flush/closes, then promotes
via `platform::ReplaceFileWithTemporary`. `WriteLevelFileStatus` is
`Saved`/`Invalid`/`Error`. `ReplaceFileWithTemporary` moved into its own
`FileReplace{Windows,Posix}.cpp` so the test target links the boundary without
ole32/shell32. `IsValidLevelIdToken` was lifted out of the parser's anonymous
namespace into `LevelFile.h` and is now shared; parser validation was not
weakened.

Authoring boundary. `editor/AuthoringPaths.h/.cpp` resolves
`game/assets/source/levels/level_01.level` from the CMake compile definition
`PLATFORMER_AUTHORING_SOURCE_ROOT`, injected for the **Development**
configuration only together with `PLATFORMER_ENABLE_LEVEL_AUTHORING`. No CWD,
no parent-directory searching. Release compiles neither, so the path literal
does not exist in the Release binary (verified by string search: present in
Development, absent in Debug and Release).

Configuration policy. `PLATFORMER_ENABLE_DEBUG_UI` (Debug + Development) means
Dear ImGui is present. `PLATFORMER_ENABLE_LEVEL_AUTHORING` (Development only)
means the build may write project source. Debug therefore compiles the editor
but cannot author.

Editor ownership. `editor/LevelEditor.h/.cpp` holds `LevelEditorState`
(`active`, `dirty`, `selectedPlatformIndex`, `lastSaveStatus`) and a read-only
scaffolding panel. Application still owns the active `LevelDefinition` and all
gameplay state, holds the editor state beside `DebugUi`, and passes both
read-only into `DebugUi::Draw`. No manager/ECS/event-bus/reflection.

Verification. Configure plus Debug/Development/Release builds pass with no new
warnings. `LevelFileTest` passes in all three configurations and covers
round-trip semantic equality across every authored field, byte-level
determinism, canonical header/record counts, a keyword whitelist proving no
runtime state is serialized, invalid-write rejection (FOV 0, negative size,
NaN, Inf, empty and non-grammar id), and safe-save behavior against a
disposable temp directory (new save, replacement save, invalid save leaves the
existing file, relative path rejected, replacement failure returns `Error`, no
leftover temp). Cooker tests pass. Development and Release both launch from an
unrelated CWD with no stderr. `game/assets/source/levels/level_01.level` is
byte-identical before and after
(`SHA256 a494c3b29a7db84661fc5ca32a079f13a9f73c3cf446fbca3edf06d9dc6a0791`).

Not in Phase A: F2 toggle, simulation pause, physics rebuild, field editing,
Apply Preview, and source Save from the running game.

#### Phase B --- Live Development editor

Implement semantic F2 editor mode, simulation pause, Level Editor UI,
supported property editing, safe preview/rebuild, dirty state, explicit
source Save and diagnostics. Preserve Release and M31 gameplay.

**Phase B outcome (complete):**

Toggle. `input::InputState::toggleLevelEditorPressed` is the semantic action,
mapped to the F2 edge press in `input/Input.cpp`; Application consumes only the
flag. `DebugUiBackend::WantsKeyboardCapture()` exposes
`ImGui::GetIO().WantCaptureKeyboard` through `DebugUi::WantsKeyboardCapture()`,
and Application ignores the toggle while it is true, so typing into a field
cannot close the editor. ImGui knowledge stays in the debug-UI backend.

Pause. One guard in the frame loop. Input polling, the toggle, rendering, the
debug UI, the editor panel and window close stay outside it; the whole M31
simulation block — run timer, moving platform, Player, gravity/CharacterVirtual,
hazard detection, fall/hazard/manual respawn, checkpoint activation, goal
completion, BEST comparison and save, collectible pickup, Enter restart,
`PhysicsWorld::Update`, camera follow — sits inside `if (!simulationPaused)`
with its M31 order and content unchanged. No time-scale system.

Ownership. Application still owns the active `LevelDefinition` and all gameplay
state. `editor::LevelEditorState` owns only authored data: `workingCopy` and
`savedSourceBaseline`. `Modified` (working vs active) and `Dirty` (active vs
last saved source) are derived every draw from `world::AuthoredLevelDataEqual`,
lifted out of the writer test into `world/LevelDefinition.h` so the editor and
the round-trip tests share one explicit field list. No reflection. The panel
returns an `editor::LevelEditorRequest` that Application executes after
`renderer.EndFrame()`, so the definition/physics swap happens between frames.

Apply Preview. Validate `IsWritableLevelDefinition` and the `level_01` identity
while the live world is intact, then `Shutdown` / `Initialize(candidate)` /
`InitializePlayer`, and commit `levelDefinition = candidate` only afterwards. A
rebuild failure after `Shutdown` is fatal by policy: stderr diagnostic, Apply
status `Error`, `Application::fatalError` exits the loop through the normal
`Shutdown()` and `Run()` returns 1. No rollback transaction. Success starts a
fresh preview run (Player at authored spawn, movement/respawn/completion/
collectible/timer reset, moving platform and cyan box reset via the rebuild,
`ApplyLevelFraming` + `camera.Initialize`). `SessionBestTimeState`, persisted
BEST, persistence statuses, loaded level id and level-loading diagnostics are
untouched, so the editor can never write BEST.

Editable set. Spawn, camera offset, camera FOV, and ground plus
`elevatedPlatforms[0..5]` center/size, with a plain combo selector and `%.6f`
field precision so authored values such as `1.6732` cannot be silently rounded.
Nothing else is editable; invalid values are reported, never clamped.

Saving. `editor::SaveLevelSource` compiles only under
`PLATFORMER_ENABLE_LEVEL_AUTHORING`; Debug and Release link a stub with no path
and no write call. It resolves `AuthoringLevel01SourcePath()` and delegates to
`world::SaveLevelFile`. Save always serializes the **active** definition and the
button is disabled while `Modified` is true. Success updates the baseline,
clearing `Dirty`; failure leaves both alone and the source intact. No cooking,
rebuilding, restarting, or process spawning.

Session semantics. Activation seeds the working copy and clears `Modified`
without touching gameplay or files; closing never auto-applies or auto-saves;
reopening re-seeds, discarding unapplied edits. `Dirty` is derived, so it
survives the toggle. The baseline is seeded in `Initialize` from the staged
level, so `Dirty` starts false and tracks only this session.

Verification. Configure plus Debug/Development/Release builds pass with no new
warnings. `LevelFileTest` (now covering authored-equality sensitivity for every
editable field) and the new `PhysicsRebuildTest` harness pass in all three
configurations; the harness proves three repeated
`Shutdown`/`Initialize`/`InitializePlayer` cycles with edited data, idempotent
double shutdown and reuse, correct static body count, moving platform and cyan
box reset to authored values, and that collision follows a platform edit (the
old position no longer supports the character, the new one does). Cooker tests
pass. Development and Debug were driven interactively: F2 opens the editor and
freezes the world (0.003% / 0.008% sampled pixel change over three seconds
against 23–24% while running, with `TIME` identical), editing a field sets
`Modified` with `Dirty` false and Save disabled behind "Apply Preview first",
`Revert Working Copy` restores the field, and Apply Preview of both a camera FOV
change and a ground-size change reported `Applied` with `Modified` false,
`Dirty` true, a reset timer, a preserved BEST and a visibly rebuilt world.
Debug reports `Authoring: Unavailable` with Save disabled and no source path.
Release shows no editor and ignores F2. The authoring root literal is present in
the Development binary and absent from Debug and Release. Canonical
`game/assets/source/levels/level_01.level` is byte-identical throughout
(`SHA256 a494c3b29a7db84661fc5ca32a079f13a9f73c3cf446fbca3edf06d9dc6a0791`).

Not in Phase B: real source writing to the canonical file (Phase C), interactive
proof of walking onto a moved platform, `Reload Runtime Level`, filesystem
watching, and any widening of the editable set.

#### Phase C --- Manual validation

Validate editor toggle/absence in Release, true simulation pause,
camera/spawn/platform edits, render/collision agreement, validation
failures, dirty/save behavior, source-file modification, parse
compatibility, cook/build propagation, relaunch, M31 gameplay, BEST
persistence, CWD independence, and restoration of canonical Level 01
values.

### Acceptance criteria

M32 completes only when Development has the first functional Level
Editor; Release has none; simulation pauses in editor mode; supported
fields can be edited and previewed safely; platform rendering/collision
never diverge; v1 serialization is deterministic and reparses; Save
writes the canonical source rather than staged data; cooker/build
propagates it; invalid data cannot be saved; normal M31 gameplay and M29
BEST persistence remain intact; no generic editor/serialization
framework is introduced; all canonical Level 01 values are restored
after validation; all configurations build; Phase C passes; and no
commit/push/merge occurs before approval.
