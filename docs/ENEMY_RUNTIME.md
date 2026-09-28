# Enemy runtime foundation (M113)

A valid, resolved Enemy Character placement creates one `EnemyRuntimeActor` and
borrows the exact M110 CharacterInstance owned for that placement by
`render::LevelCharacters`. NPCs use their separate `NpcRuntimeActor` vector.
Player and Animal receive no patrol actor; `characters/player` stays visual-only.
Malformed/missing references cannot create either actor. Presentation may be
Exact, Retargeted, Static or Unavailable without changing actor eligibility.

## Authored authority and shared patrol

CharacterDefinition owns shared metadata and presentation references. Placement
owns authored TRS and patrol enabled/distance/speed. Actors own session handles,
source indices, copied origin/identity, current position/rotation, endpoints,
phase/direction, Idle/Move and diagnostics; their instance reference is borrowed.
None of the runtime state is serialized.

M112 storage and Level v1 `npc_patrol <0|1> <distance> <speed>` are reused unchanged.
The historical token means the bounded Character-placement patrol payload for
both NPC and Enemy. Defaults, bounds, parse/write order and omission rules are
unchanged; no migration or version bump. `CharacterPatrolState` shares only the
existing bounded calculation between two independently typed actors, without a
generalized entity or behavior system.

Origin is authored model position. Local X under Rz*Ry*Rx is projected onto XZ
and normalized; a vertical projection falls back deterministically to authored
yaw. Scale does not change distance. Endpoints are origin +/- distance on that
axis. Double phase starts at distance, moving toward the positive endpoint;
modulo folding reverses deterministically, including multiple endpoints per step.
Invalid/nonpositive delta does not move. Disabled stays at exact authored TRS in
Idle. Enabled selects Move and pure horizontal yaw aligning model +Z to travel.
Endpoint reversal changes transient facing only. No root motion, collision,
grounding, gravity, perception, targeting or combat is implemented.

## Lifecycle, editor and production respawn

Clear destroys both actor sets before releasing instances. Every established
active-owner rebuild (Apply, restart, load/Open, transition, staged reload and
catalog promotion) replaces state from authored data. Deletion and identity/type
changes take effect at promotion. Handles are fresh, counts never accumulate.
Shutdown clears while the graphics context is alive.

Resolved Enemy Inspector shows Enemy Runtime, NPC shows NPC Runtime. Controls
edit workingCopy only; Apply promotes and rebuilds; Save writes active authored
state. Preview disables both actor types and draws authored transforms. Leaving
F2 permits active simulation; reloading rebuilds origins and initial playback.

The existing real Gameplay R -> `PerformRespawn(Manual)` rebuild now naturally
resets both vectors. `AdvanceLevelCharacters` retains the M112 reset-frame guard:
origin and initial facing/phase/playback are displayed before independent patrol
resumes. Player/checkpoint/inventory/health/timer and moving-platform behavior are
unchanged. Fall/death retain their previous narrower authority.

## Presentation, diagnostics and verification

Advance drives transform and Idle/Move into the associated CharacterInstance,
then advances its playback. Existing compatibility, materials, skinning and
shared world/directional-shadow draws are unchanged; there is no Enemy renderer.
Development F1 adds M113 Enemy Runtime alongside M112 NPC Runtime, exposing
handle, source placement, identity/type, patrol flag, origin/current position,
direction/endpoints, locomotion, instance handle, presentation and diagnostics.

CharacterPlacementTest runs the same production parser/writer/Save/load, Apply
candidate, owner, renderer and shadow contract for NPC and Enemy. Test-only
registry definitions reuse existing Player/retarget/static assets; permanent
canonical definitions are unnecessary. The existing `characters/guard` is Enemy
metadata without a World Model, so it is useful for unavailable-presentation
acceptance; visible animated acceptance can use a separately authored Enemy
definition referencing the existing Player model and humanoid Idle/Move assets. Mixed NPC/Enemy Application fixtures call
real Manual Respawn and full RestartRun, inspect the production reset frame,
verify authored data and stable counts, then verify independent deterministic
resume. Player and moving-platform preservation remain asserted.

Manual acceptance remains user-owned: use a separate test Level and an Enemy
Character Database definition referencing existing assets. Verify controls,
working-copy isolation, Apply, reversal/facing, disable/Idle, duplicate/delete,
Save and reopen/cook/stage/reload, mixed NPC/Enemy Gameplay R, and unaffected
Player animation/equipment/shadows. Do not leave test placements in canonical
levels. Character Ghost, Pause Restart, physics/grounding and moving-platform
respawn tuning remain deferred. No M114 work or Git closure is included.

## Changed files

| File | Change |
| --- | --- |
| [CharacterPatrolState.h](../game/source/gameplay/CharacterPatrolState.h) (new) | Existing M112 bounded calculation shared without changing semantics |
| [EnemyRuntime.h](../game/source/gameplay/EnemyRuntime.h) (new) | Separately typed transient Enemy actor and handle |
| [NpcRuntime.h](../game/source/gameplay/NpcRuntime.h) | NPC actor retains its API over shared calculation |
| [LevelCharacters.h](../game/source/render/LevelCharacters.h) | Independent actor sets, type gating, clear/rebuild/advance and presentation |
| [CharacterPlacement.h](../game/source/world/CharacterPlacement.h) | Clarify historical patrol names are shared; no schema change |
| [LevelEditor.cpp](../game/source/editor/LevelEditor.cpp) | Type-specific Enemy/NPC Inspector sections |
| [DebugUi.cpp](../game/source/ui/debug/DebugUi.cpp) | M113 diagnostics alongside M112 |
| [CharacterPlacementTest.cpp](../game/source/render/CharacterPlacementTest.cpp) | Production contract for both types and mixed real Manual Respawn fixtures |
| [AGENTS.md](../AGENTS.md), [MILESTONES.md](MILESTONES.md) | Active M113 pointer; acceptance/closure remain separate |
| [README.md](../README.md), [ARCHITECTURE.md](ARCHITECTURE.md) | Current Enemy architecture and validation entry point |
| [CHARACTER_PLACEMENT.md](CHARACTER_PLACEMENT.md), [NPC_RUNTIME.md](NPC_RUNTIME.md), [LEVEL_FORMAT_V1.md](LEVEL_FORMAT_V1.md) | Shared patrol grammar, authority and independent Enemy behavior |
| [ENEMY_RUNTIME.md](ENEMY_RUNTIME.md) (new) | Focused architecture, lifecycle, verification and manual acceptance |
| [test_agent_instructions.py](../tools/test_agent_instructions.py), [test_milestone_docs.py](../tools/test_milestone_docs.py) | Recognize active M113 and its supplied canonical contract |

The supplied, initially untracked `milestones/MILESTONE_113.md` frozen contract
is preserved unchanged. Neither canonical Level nor the gameplay catalog changes.

## Implementation validation — 2026-09-28

Implementation is validated on `milestone/113-enemy-runtime-foundation`.
Manual acceptance and Git closure remain pending; this is not milestone closure.

| Command/check | Result |
| --- | --- |
| `cmake --preset windows-vs2022` | Passed |
| `cmake --build --preset windows-debug` | Full preset passed |
| `cmake --build --preset windows-development` | Full preset passed |
| `cmake --build --preset windows-release` | Full preset passed |
| `build/windows-vs2022/Debug/CharacterPlacementTest.exe` | Passed |
| `build/windows-vs2022/Development/CharacterPlacementTest.exe` | Passed |
| `build/windows-vs2022/Release/CharacterPlacementTest.exe` | Passed |
| All 73 `build/windows-vs2022/Development/*Test.exe` executables | Passed, with the ToolRunner sandbox retry described below |
| `python tools/test_milestone_docs.py` | 7 tests passed |
| `python tools/test_agent_instructions.py` | 9 tests passed |
| `python tools/test_stage_runtime_assets.py` | 13 tests passed |
| `python tools/test_cook_level_v1.py` | 29 tests passed |
| `python tools/test_cook_runtime_png.py` | 23 tests passed |
| `python tools/test_import_static_glb.py` | 13 tests passed |
| `python tools/test_stage_world_shaders.py` | 4 tests passed |
| Development startup (`bin/Development/Platformer3D.exe`) | Alive after five seconds, staged graphics loaded, stderr empty, no warnings/errors |
| `git diff --check` | Passed |
| Both canonical levels and `definitions.gameplay` | No diff; existing line endings retained; no added placements/catalog data; M45 record absent |

Builds used process-local PATH/Path deduplication, disabled MSBuild node reuse,
and `/m:1 /nr:false`, following the validated Windows environment workaround.
No compiler/build-definition changes were needed. Initial expanded-test fixture
failures were corrected by isolating the single-placement type-gating fixture
and using a fresh production Renderer for each NPC/Enemy contract invocation.

EditorToolRunnerTest timed out under the existing sandbox child-process polling
restriction. Its outside-sandbox rerun passed. All other Development executables
passed normally. This is recorded separately in the validation logs rather than
masking the initial restricted result. The task-owned hidden startup process was
stopped after the smoke check; no user process was stopped.

Logs are under ignored `build/m113-*` paths, including per-test output,
`m113-cpp-results.json`, full preset logs, startup output and canonical audit.
The real Application Manual Respawn regression passes in all three configurations;
the broader sweep covers Player, equipment, physics, authored lifecycle, editor,
materials, terrain and lighting. Interactive acceptance has not been performed.
No commit, push, merge, Git closure, branch switch or M114 work was performed.
