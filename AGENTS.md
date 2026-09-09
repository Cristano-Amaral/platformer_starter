# Project Instructions — 3D Platformer

## Project goal
Build a C++ platform game with 3D visuals and a platformer-style camera. Windows is the first shipping target. Architecture must preserve a practical path to Linux/Raspberry Pi, Android, and iOS.

## Non-negotiable engineering principles
1. Work in small, testable, compilable milestones.
2. Do not implement future milestones while completing the current one.
3. Keep gameplay code platform-agnostic.
4. Platform/library-specific APIs must stay behind adapters/interfaces.
5. CMake is the canonical build definition. IDE project files are generated artifacts and must not be hand-maintained.
6. Every meaningful change must leave the repository in a buildable state.
7. Prefer simple code over speculative abstractions, except at platform boundaries where abstraction is intentional.
8. Before adding a subsystem or dependency, check whether an equivalent already exists.

## Initial technology choices
- Language: C++20 for game code.
- Build: CMake 3.25+.
- Windows compiler: Visual Studio 2022 / MSVC.
- Linux/Raspberry Pi compiler: GCC or Clang.
- Graphics/window/input backend for initial Windows build: raylib.
- Math: GLM when needed.
- Physics: Jolt Physics when the physics milestone begins.
- Debug/editor UI: Dear ImGui, Development/Debug builds only.
- Tools/asset pipeline: Python 3.

## Architecture boundaries
Game code must not include platform OS headers or call OS APIs directly.

Use these layers:
- `game/source/core/`: application loop, timing, config, logging, shared infrastructure.
- `game/source/platform/`: platform and backend adapters (window, input, filesystem, timing, lifecycle).
- `game/source/gameplay/`: player, movement, camera behavior, levels, game rules.
- `game/source/render/`: renderer-facing abstractions and rendering systems.
- `game/source/physics/`: physics integration/adapters.
- `game/source/ui/`: HUD, menus, debug UI.

Avoid calling raylib directly from gameplay code. If a feature requires raylib, expose the smallest useful wrapper in the appropriate platform/render layer.

## Portability rules
- Paths must use `std::filesystem`.
- Do not assume drive letters, backslashes, current working directory, keyboard availability, mouse availability, or a desktop window.
- Input gameplay actions must be semantic (`MoveLeft`, `Jump`, `Pause`) rather than hardwired keys outside the input backend.
- Do not put Win32-specific code outside a clearly named Windows implementation file.
- File names and asset paths are case-sensitive by convention, even on Windows.
- Avoid undefined behavior and compiler-specific extensions unless isolated behind a portability layer.
- Do not allocate per-frame unless justified and measured.

## Code style
- Classes/structs/functions/methods/files: PascalCase.
- Local variables and data members: camelCase.
- Constants: kPascalCase.
- Macros: ALL_UPPER_CASE and only when necessary.
- Namespaces: short lowercase names.
- Prefer `#pragma once` for new headers.
- RAII is required for owning resources.
- Prefer value semantics and `std::unique_ptr` where ownership must be dynamic. Avoid shared ownership unless there is a concrete reason.
- Keep headers lean; use forward declarations where useful.
- Comments explain why, constraints, or non-obvious behavior — not obvious syntax.

## Build configurations
- Debug: diagnostics, assertions, debug tools, low optimization.
- Development: symbols + debug tools + representative optimization; default for gameplay/performance testing.
- Release: shipping configuration; no editor/debug UI, assertions may be reduced, optimized.

Performance conclusions must not be drawn from Debug builds.

## Milestone workflow
For each milestone:
1. Read `docs/MILESTONES.md` and identify the single active milestone.
2. State a short implementation plan.
3. Implement only that milestone.
4. Configure/build the project.
5. Run available tests/smoke checks.
6. Summarize changed files, architectural decisions, how to test, and remaining known limitations.
7. Do not mark a milestone complete if the build is broken.

## Current milestone
Milestone 51 — Dynamic Box Grab / Carry
(Gameplay: target, grab, carry, and drop authored Dynamic Boxes only;
runtime-only carry state; awaiting manual acceptance).
Milestone 50 is complete. Milestone 52 has not started.
See `docs/MILESTONES.md` and `docs/ARCHITECTURE.md`.

Milestone 33 is complete and merged. F2 still pauses simulation, edits a
working copy, and uses Apply Preview / Revert / Save Level Source. Viewport
pick/highlight use the active/applied world; Inspector and the translation
gizmo edit `workingCopy`. Debug compiles the visual editor but cannot author.
Release has no editor.

M34 added world-space X/Y/Z translation for Spawn, Ground, and Elevated
Platforms, a pending ghost, persistent Dear ImGui layout, and a
depth-independent gizmo overlay. M35 is complete (resize, orientation widget,
Translate-only nudge, Alt+wheel dolly). M36 is complete (F2 Dear ImGui menu
bar: View / Transform / Level). M37 is complete: Development-only Build menu
and Tool Output. `Build > Cook Assets` remains only `python tools/cook_assets.py`.
M38 is complete: Development `Build > Stage Runtime Assets` and
`Build > Cook & Stage` use `cmake -P cmake/StageRuntimeAssets.cmake`.
Cook Assets remains cook-only. M39 is complete: Development
`Level > Reload Runtime Level` reloads staged Level Format v1 in-process.
It does not Save, Cook, Stage, or restart. M40 is complete: Development
`Build > Cook, Stage & Reload`
(canonical Cook & Stage, then one in-process M39 reload).
M41 is complete: Development Edit menu Add / Duplicate / Delete for Platform,
Checkpoint, Hazard, and Collectible mutates `workingCopy` only. Apply remains
transactional. StructuralIndexMap is the session active↔working pick map.
Pending Add/Modify uses the cyan family (selected stronger, unselected softer).
Pending Delete is faded/desaturated with a delete outline and is not pickable.
Visible pending workingCopy objects are viewport-pickable. Platform Add follows
the Jolt leftover body budget (59 after M44 removed the unused dynamic-probe
body from the canonical scene). Checkpoint/Hazard/Collectible share the v1
256-line / 64 KiB parser guard. Edit > Add uses camera-region X/Y and spawn.z
lane. Duplicate is +1 X and does not snap to the lane.
M42 is complete: Development-only Object Palette and explicit viewport
placement mode for those four categories. Palette entries are tool toggles:
the same category again, or Esc, exits placement. Active category is highlighted with
status text. Placement mode itself does not mutate workingCopy. Confirm reuses
M41 Add at the resolved world center. Repeated clicks stay in the same mode.
Gizmo/widget/ImGui/RMB gestures consume the pointer for the whole LMB hold so
they never confirm placement. Surface-hit previews stay bright; camera-fallback
previews use a quieter wire style. F2 close / successful Apply / Revert /
successful Reload cancel placement. Debug has no Object Palette. Release has
no editor.
M43 is complete: Development-only Quick Toolbar fixed below the menu bar:
Translate/Resize/Scale (canonical TransformMode), Apply/Revert/Save (canonical
LevelEditorRequest), and a persisted build selector (Debug/Development/Release/All,
default Development) that Run maps through EditorToolRunner. View > Quick Toolbar
shares workspace visibility. Reset Editor Layout restores toolbar visibility and
does not reset the last build selection. The 3D viewport starts below menu+toolbar
when the toolbar is visible.
M44 removed the M15–M19 cooker-probe instances and the M23 cyan dynamic crate
from the canonical runtime/editor scene. Source/cooked/staged test assets and
their cooker/staging tests remain. Both authored slopes and the moving platform
remain. M45 turns `dynamic_box` into a repeatable authored Dynamic Box
(`center`, `size`, `massKg`) with a real Jolt dynamic body. Canonical Level 01
has 0 Dynamic Boxes. Shared leftover is 59 bodies for platforms plus Dynamic
Boxes (`kPhysicsMaxBodies` 64 minus 5 fixed bodies).
M46 recovers each active Dynamic Box whose runtime Jolt body center Y is
strictly below `active.killPlane` back to its currently applied authored
transform, with identity orientation and zero linear/angular velocity. Recovery
is per body and does not rebuild PhysicsWorld or mutate authoring state.
Do not add Undo/Redo, a probe framework, Level Format v2,
or Milestone 52.

Milestone 31 is complete and merged. One playable level (`level_01`). The sole
live authored source is `game/assets/source/levels/level_01.level` → cooker →
cooked → staged `<exe>/assets/levels/level_01.level` → `LoadLevelFile` →
`LevelDefinition`. Application loads that staged file once in `Initialize` and
owns it. There is no compiled Level 01 fallback. Missing/invalid/unsupported
Level 01 is a fatal init error. M29 BEST save remains nonfatal. The M32 writer,
authoring boundary, F2 editor, Apply Preview and source Save remain the live
authoring path. Save Level Source is not Cook Assets; Cook Assets is not
runtime staging. M38 Development `Build > Stage Runtime Assets` stages
cooked files into `build/windows-vs2022/bin/Development/assets/` without a
C++ build. `Build > Cook & Stage` cooks then stages. Those jobs do not
change M37 Cook Assets / Build Development meanings. M39 Development
`Level > Reload Runtime Level` reloads the staged runtime level in-process.
It does not Save, Cook, Stage, or restart. M40 Development
`Build > Cook, Stage & Reload` runs canonical Cook & Stage then one
in-process M39 reload. M41 lifecycle edits mutate `workingCopy` only and
still require Apply Preview, then Save, then Cook, Stage & Reload.
M42 placement still requires that same Apply / Save / Cook, Stage & Reload
path. M43 placement and picking use the content viewport below the Quick Toolbar.
M44 is complete and merged. M45 adds repeatable authored Dynamic Boxes
(Jolt dynamic bodies, mass in kg, shared platform+box leftover). Canonical
Level 01 has 0 Dynamic Boxes. M46 adds individual runtime kill-plane recovery
for those boxes without mutating authored state. M47 adds Development
`Assets > Import Static GLB`: a compatible external `.glb` is copied into
`game/assets/source/models/<filename>.glb` with project-relative identity
`models/<filename>.glb`. Collision never overwrites. Import does not edit
the level, cook, or stage; the next step is the existing Cook / Stage
workflow. M48 adds a Development Content Browser over that derived
catalog (search/filter, refresh, the same import request, and confirmed
Delete Asset that removes canonical source plus matching cooked/staged
copies). Browser selection is not scene selection and does not instantiate
a level object. M48.1 adds Development Content Browser thumbnails: default
Thumbnails grid, optional List mode, local derived cache under
`%LOCALAPPDATA%\Platformer3D\thumbnails\`, and Reset Editor Layout restoring
Thumbnails. M48.2 adds a Development Model Preview window that renders the
real selected static GLB (orbit/zoom/Reset View) without instantiating it.
M49 adds repeatable authored Static Props (`world::StaticPropSpec`: canonical
identity plus position, Euler XYZ degrees, and visual scale). The primary
direct-add UI is Development Content Browser **Add Static Prop**;
`Edit > Add > Static Prop` is retained and routes the same
`LevelEditorRequest::AddStaticProp`. Both consume the Content Browser selected
identity, and the Edit row states that asset (or `select asset`) so the
cross-window dependency is visible. Milestone 50 adds Development Content
Browser **Place Static Prop**, a distinct interactive viewport placement
mode: transient real-model preview on Ground/Platform/Slope, confirmation
creates the existing M49 Static Prop in `workingCopy` only, and Direct Add
remains immediate. Props are visual only (no Jolt body). Scene
rendering resolves staged runtime assets only; a missing staged model draws a
placeholder cube and the editor says to run Cook & Stage (no source fallback,
no automatic cook/stage). Authored `Scale (1,1,1)` keeps the model's raw GLB
size after the runtime loader bakes glTF node transforms; nothing normalizes
imported geometry. The Inspector reports that loaded size and warns if the
gameplay camera sits inside the prop AABB. Inspector edits position, rotation,
and scale. Translate gizmo edits position; Scale gizmo edits visual model
scale; rotation remains Inspector-only. Primitive Resize still edits authored
box size and is not Static Prop Scale. `DrawProp` restores default shader
`colDiffuse`/texture0 after each model for later greybox draws in the same
3D pass. Model Preview and thumbnails may set tight `rlSetClipPlanes` for a
model frame; they restore `RL_CULL_DISTANCE_NEAR/FAR` after the offscreen pass,
and `DrawWorld` establishes those same planes again before Gameplay
`BeginMode3D`. A leaked Chest-like far plane (~6) clips the gameplay camera.
Delete
Asset refuses identities referenced by `workingCopy`, `active`, or
`savedSourceBaseline`. A transient M50 placement preview is not an authored
reference; Delete Asset cancels placement when the identity matches the
pending preview asset. Canonical Level 01 still has 0 Static Props. Debug has
the visual editor but cannot author. Release has no editor. Temporary Correction 5
Gameplay framebuffer capture was removed after the clip-plane fix. M50 is
complete. Milestone 51 adds gameplay Grab / Carry for authored Dynamic Boxes
only (`E` toggles Grab/Drop). Carry is runtime-only: it does not mutate
`workingCopy`, active authored definitions, Modified/Dirty, or Level Format.
Static Props stay visual and non-grabbable. Do not start Milestone 52.
