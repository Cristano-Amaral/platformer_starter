# NPC runtime foundation (M112)

`CharacterDefinition` owns shared metadata and model/animation references. Level
Character placement owns authored TRS and optional bounded patrol settings.
`gameplay::NpcRuntimeActor` is session-only simulation state; `render::LevelCharacters`
creates actors only for valid, resolved NPC placements, referencing exactly the
same M110 instance already owned by that placement. Player/Animal remain
generic visuals. M113 adds an independent Enemy actor set; the bounded patrol
calculation is shared through CharacterPatrolState. `characters/player` never acquires Player gameplay authority.

## Level Format v1

```text
character <px> <py> <pz> <rx> <ry> <rz> <sx> <sy> <sz> characters/<name> [npc_patrol <enabled> <distance> <speed>]
```

The suffix is all-or-nothing, in this exact order. Enabled is exactly `0` or `1`.
Distance is finite in `(0, 100]` world units; speed is finite in `(0, 20]` world
units/second. Values must be valid even when disabled or when the definition is
Player/Animal/missing. Omission means disabled, distance 2, speed 1. The writer omits
the suffix only for those exact defaults; disabled non-default values persist.
No version change: existing 256-record, 512-byte-line, 65536-byte-file guards
remain authoritative. No handles, positions from simulation, phase, direction,
pose, or animation clock serialize. Missing textual identities remain preserved.

## Simulation and lifetime

Authored position is the origin, never a surface-adjusted center. The patrol axis
is local +X under M110's `Rz * Ry * Rx`, projected onto world XZ and normalized.
If projection length is at most 1e-6 (vertical X), use the authored yaw's horizontal
X axis deterministically. Scale does not change patrol distance. Endpoints are
origin minus/plus distance times this axis; Y stays at authored origin.

The double-precision triangular phase starts at origin moving toward +distance.
At +distance direction becomes negative; at -distance it becomes positive.
Modulo folding handles multiple endpoints in a single finite positive delta.
Zero, negative, or non-finite deltas do not move actors. Disabled patrol remains
at exact authored TRS/Idle. Enabled patrol selects Move and pure horizontal yaw
aligning model +Z to travel; endpoint reversal changes only transient facing.
Authored pitch/roll/rotation and scale are preserved in placement data; disabled
patrol restores the authored rotation. No root motion, collision, gravity,
navigation, or Player interaction is added.

Active `LevelCharacters` clears NPC references before releasing owning instances
at every established M111 rebuild boundary: Apply, Restart/New Run, Open,
transition, staged Reload, and catalog promotion. Actors and instances get fresh
session handles and reset to origin/initial animation clock. Delete/rebuild drops
stale actors and recreates surviving authored occurrences. Ordinary gameplay R
(Manual Respawn) also rebuilds the placed presentation/NPC owner, while its
Player/checkpoint/inventory/health/timer and moving-platform rules remain unchanged.
Fall and health-death respawns retain placed runtime state. Manual R and full
RestartRun render their reset frame at origin with initial playback before patrol
resumes. Shutdown clears before graphics teardown.

`Advance` supplies NPC runtime transform and Idle/Move to the existing
CharacterInstance before advancing animation. Exact, M109 Retargeted, Static,
and Unavailable use unchanged M110 compatibility, materials, skinning, and shared
world/directional-shadow rendering. No NPC renderer or second spawning system.

## Editor and diagnostics

The resolved NPC Inspector exposes Patrol Enabled, Distance, Speed, and the
local horizontal X explanation. Invalid values block Apply. M113 Enemy definitions show their own Enemy Runtime section. Player/Animal show
that NPC/Enemy behavior is inactive; their stored settings persist safely but are ignored.
All controls edit workingCopy. Apply promotes authored data and rebuilds active
runtime at the existing boundary. Save writes active authored data; pending edits
need Apply. Reload reads staged assets; use Cook, Stage & Reload after source Save.
The working preview owner explicitly disables NPC creation and remains authored-only.
M111 Character Ghost tuning is deferred; transform preview behavior is unchanged.
Existing editor/menu/pause simulation authority is preserved: leave F2 to observe
active patrol advancing; the editor continues to show its authored working preview.

Development F1 adds `M112 NPC Runtime`: active actor handle, source index, identity,
NPC type, patrol flag, origin/current position, direction/endpoints, Idle/Move,
CharacterInstance handle, presentation mode, and concise diagnostics. This reads
active transient state and never saves it.

## Validation and manual acceptance

`CharacterPlacementTest` extends the real M111 parser/writer/file-save, Apply
candidate, owner, production renderer, and shadow boundaries with test-only NPC
definitions. No canonical demo data is needed. Coverage includes legacy grammar,
invalid patrol values, deterministic persistence, type gating, independent state,
endpoint folding/reversal, facing, preview separation, Apply/Restart/reload resets,
delete survivors, and Exact/Retargeted/Static/Unavailable safety.

User acceptance remains required: create an NPC definition in the Character
Database and a separate test Level; edit patrol then Apply; observe travel/facing,
disable/Apply, Save/Cook/Stage/Reload, Restart, duplicate, delete, and repeated
Apply. Check Player gameplay, animation, equipment, and shadows remain unchanged.
No canonical levels/catalog are modified by implementation. M112 is not closed;
Character Ghost tuning and all M113+ work remain deferred.

## Implementation validation — 2026-09-28

| Check | Result |
| --- | --- |
| `cmake --preset windows-vs2022` | Passed |
| `cmake --build --preset windows-debug` | Passed |
| `cmake --build --preset windows-development` | Passed |
| `cmake --build --preset windows-release` | Passed |
| All 73 Development C++ test executables | Passed |
| Seven workflow Python suites (98 tests) | Passed |
| Development startup | Alive after five seconds; staged graphics loaded; stderr empty; no stdout warnings/errors |
| `git diff --check` | Passed |
| Canonical levels and gameplay definitions | No diff; text equals HEAD; M45 record absent |

The C++ sweep includes the extended `CharacterPlacementTest` with real NPC actors,
Exact/Retargeted animation pixels, unavailable-model moving fallback, static safety,
and production directional-shadow receiver pixels. It also covers existing Player,
equipment, editor, authored lifecycle, physics rebuild, terrain, and lighting suites.
Python suites: `test_milestone_docs.py`, `test_agent_instructions.py`,
`test_stage_runtime_assets.py`, `test_cook_level_v1.py`, `test_cook_runtime_png.py`,
`test_import_static_glb.py`, and `test_stage_world_shaders.py`.

MSBuild needed a child-process environment with duplicate PATH/Path removed,
`/m:1 /nr:false`, and disabled node reuse. Initial unnormalized/multi-node attempts
failed; all final preset builds passed with those process-local settings.
`EditorToolRunnerTest` timed out under sandbox child-process polling restrictions;
its rerun outside that restriction passed. The task-owned startup process was
stopped after validation when normal hidden-window closure did not exit it.
Validation logs are under the ignored `build/m112-*` paths.

Manual editor/gameplay acceptance is still pending. No commit, push, merge,
milestone closure, branch switch, canonical demonstration edits, or M113 work.

## M112 Correction 1 — real ordinary-gameplay R boundary

Manual acceptance used R during ordinary gameplay. `input::Poll` maps R to
`respawnPressed`; `Application::Run` dispatches it to `PerformRespawn(Manual)`.
That narrower Player respawn intentionally did not call `RestartRun`, so it kept
NPC patrol state (and existing moving-platform state). Enter on an eligible
completed-level path and R on RUN COMPLETE call `RestartRun`, which already
rebuilt placed Characters. The prior automated "Restart" assertion called
`LevelCharacters::Rebuild` directly; CMake excluded Application.cpp from its
production-boundary target. It proved replacement, not the user's command path.
The previous report overstated that coverage.

`PerformRespawn(Manual)` now rebuilds placed Character presentation/NPC state
from the active authored level, with no Save or authored mutation. It does not
call full RestartRun and does not reset moving-platform physics, Player Health,
checkpoint progress, Inventory, Equipment, collectibles, or timer. Fall/death
remain on their previous narrower authority. A transient pending-reset flag is
consumed by `AdvanceLevelCharacters`, the shared production frame-update method,
so Manual R and full RestartRun display origin/initial animation on their reset
frame instead of immediately consuming its delta. Enabled patrol starts Move,
outbound at its authored distance phase; disabled patrol remains Idle.

CharacterPlacementTest now links actual Application.cpp (only Main.cpp is
excluded). A test-only friend arranges fixture state and calls the real private
Manual Respawn, full RestartRun, and Character frame-update boundaries without
reimplementing any reset logic. Two same-definition enabled NPCs with different
TRS/distance/speed are advanced past reversal, reset three times, inspected at
the render-frame boundary, and advanced again. Checks cover authored origin,
phase, direction, facing, playback/playing/Exact mode, unchanged authored data,
fresh handles, stable counts, and deterministic independent resume. Player
checkpoint/health/inventory/timer/death-count and moving-platform preservation
are asserted; fall retains NPC state. A controlled removal of the new Manual R
reset block made this regression fail; restoring production code made it pass.
Fixtures clear their in-memory baseline Character vector so user manual-test
placements do not change fixture counts. Authored source files are never cleared.

User manual-test authored data is preserved for renewed acceptance. Character
Ghost tuning and moving-platform Restart behavior remain outside this correction.

Correction validation: configure and full Windows Debug/Development/Release
builds passed; the real Application NPC lifecycle regression passed in all
three configurations. Nineteen related Development C++ suites passed, including
LevelFileTest after its authored-keyword whitelist was extended to recognize
the existing Character record exposed by user manual-test data. All seven
required Python suites passed (98 tests). Development startup remained alive
for five seconds with empty stderr and no logged errors/warnings. Diff checking
passed. SHA-256 audits confirm level_01.level, level_02.level, and
definitions.gameplay are byte-identical to the start of this correction; the
existing user Character/environment/light edits in level_01 are preserved.
The historical M45 dynamic_box record remains absent. Renewed manual R
acceptance remains with the user. No Git closure was performed.

M113 reuses this exact payload, simulation and lifetime contract for separately typed Enemy actors; see [ENEMY_RUNTIME.md](ENEMY_RUNTIME.md). NPC behavior remains independent.
