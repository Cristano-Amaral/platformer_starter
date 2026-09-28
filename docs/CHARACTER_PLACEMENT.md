# Character placement and spawning (M111)

`CharacterDefinition` is shared authored data in the existing gameplay catalog.
A Level Character placement is a persistent request for one occurrence.
`render::CharacterInstance` is its transient runtime realization. The runtime
handle is session-local, changes on rebuild, and is never serialized.

Level Format v1 uses this repeatable record:

```text
character <px> <py> <pz> <rx> <ry> <rz> <sx> <sy> <sz> characters/<name>
```

Position is the World Model origin in world space. Rotation is Euler XYZ degrees,
using the existing M110 model TRS, and each scale axis must be finite and positive.
Scale is included because the existing editor and M110 both use model TRS.
There is no bounds/support adjustment or hidden Y offset. Changing definition
never rewrites position. Preview and runtime use identical authored TRS.

Character records consume the existing 256-line budget; the 512-byte line and
65536-byte file guards are unchanged. The canonical writer emits Characters in
vector order before Static Props. It stores no pose, playback, locomotion,
runtime handle, controller, or CharacterDefinition payload.
Identity uses the registry's case-sensitive `characters/[a-z][a-z0-9_]{0,31}`
grammar. Malformed records are rejected. Syntactically valid missing identities
are preserved exactly, remain writable, and never spawn as resolved definitions.

In Development, F2 → Edit → Add → Character creates a None placement. The normal
Hierarchy selects it; Inspector chooses from the active CharacterDefinition
registry, showing Display Name and stable identity, plus None/Resolved/Missing.
Definitions are authored separately in the Character Database. Existing catalog
reload authority still applies when leaving the editor. Missing references can
be repaired with the picker or cleared. A new None placement is discarded by
Apply only when its structural map proves it has no active counterpart, matching
the existing Item Pickup rule. Clearing an existing placement blocks Apply until
it is repaired or deleted. Invalid transforms cannot promote to active state.

Translate/Rotate/Scale, Duplicate (+1 world X), Delete, multi-selection, and
authoring groups use the existing category/index infrastructure. Picking and
ghost bounds use the existing default model box transformed by authored TRS;
the working preview draws the actual World Model through M110, including skins.
Unavailable visuals use the established magenta fallback box. The preview is a
separate transient owner and never promotes its handles or state. Active runtime
instances remain untouched before Apply. The Inspector correlates working index
through the structural map to active placement index, session handle, and
Exact/Retargeted/Static/Unavailable diagnostics. Diagnostics do not log per frame.

Application owns the active `LevelCharacters` set. Initial load realizes it before
the first world draw. Successful Apply, staged Reload, authored level Open, level
transition, New Run, and Restart replace the set; previous instances release their
resources before replacement. Checkpoint/manual/fall/death respawns preserve the
set, matching those paths' existing narrower Player reset authority. Leaving F2
clears instances before catalog reload and realizes them from the new registry.
Shutdown clears borrowed renderer spans and owning instances before the graphics
context closes. Repeated realization cannot accumulate instances. Survivors may
receive new handles after delete/rebuild; occurrence semantics are the authored
fields, never handle equality. Save writes the active authored level, following
existing editor authority; pending working edits require Apply first. Reload
Runtime Level reads staged data: use Cook, Stage & Reload after saving source.

Every resolved placement creates one M110 instance with deterministic Idle and
its own model, pose, bone matrices, transform, clock, and handle. Shared definition
identity does not share mutable state. M105 Exact remains preferred, M109
Retargeted remains its validated fallback, and static/unavailable models remain
safe. Application supplies one borrowed draw span containing placed instances and
the unchanged opt-in M110 harness. Renderer draws this span in its existing shared
world/shadow geometry path, with existing materials and bone matrices.

`characters/player` is allowed **only as a generic visual occurrence**. M110 has
no Player input, controller, physics, stats, inventory, or equipment ownership;
the actual gameplay Player remains on its existing authoritative path. Player,
Enemy, NPC, and Animal Character Types activate no behavior. M111 provides
placement and spawning; it provides no NPC, Enemy, Animal, AI, or combat behavior.

`CharacterPlacementTest` uses real parse/write/load, editor Apply preparation,
active placement realization, and production `Renderer::DrawWorld`. Pixel
comparisons cover two same-definition animated instances, independent animation,
simultaneous Exact/Retargeted, rebuild replacement, and deletion with surviving
pixels unchanged. Canonical authored levels/catalog are read only.

Manual acceptance remains user-owned: add, assign, transform, Apply, duplicate,
Apply, inspect distinct handles, delete, Apply, use Retargeted, Save, Cook/Stage/
Reload, exercise Missing, Restart, then verify normal Player Idle/Move/Jump,
equipment, shadows, and gameplay. No demonstration placement is added to either
canonical level. Use a separate authored test level to avoid canonical test residue.

## Implementation validation

Implementation is ready for Development manual acceptance; M111 is not closed.

| Check | Result |
| --- | --- |
| `cmake --preset windows-vs2022` | Passed |
| `cmake --build --preset windows-debug` | Passed |
| `cmake --build --preset windows-development` | Passed |
| `cmake --build --preset windows-release` | Passed |
| Development C++ test executables | 73 passed, including targeted reruns |
| Required Python suites | All seven passed (98 tests) |
| Real Development executable startup | Alive after five seconds; graphics/audio and staged assets loaded; stderr empty |
| Canonical `level_01.level`, `level_02.level`, `definitions.gameplay` | No diff; semantic text equals HEAD |
| Historical M45 Dynamic Box | Remains absent from both canonical levels |

The C++ sweep includes CharacterPlacementTest, CharacterInstanceTest,
GameplayDefinitionTest, CharacterAssetValidatorTest, CharacterPreviewTest,
AnimationLibraryTest, SkeletalAnimationTest, PlayerPresentationTest,
PlayerAnimationAssetTest, EquipmentAttachmentTest, PlayerCharacterStatsTest,
LevelFileTest, authored lifecycle/selection/group/gizmo/picking suites, level
transition/rebuild tests, and static/material/lighting/terrain regressions.
M109 and M110 production rendering regressions remain green. The M111 test proves
two same-definition occurrences own distinct handles/poses/matrices; authored TRS
and Save/Reload semantics persist; actual production pixels change independently;
mixed Exact/Retargeted animation reaches production rendering; deletion removes
the intended occurrence with surviving pixels preserved; and repeated rebuilds
do not accumulate instances. Interactive acceptance was not performed.

Python validation ran `test_milestone_docs.py`, `test_agent_instructions.py`,
`test_stage_runtime_assets.py`, `test_cook_level_v1.py`, `test_cook_runtime_png.py`,
`test_import_static_glb.py`, and `test_stage_world_shaders.py`. Cooker regression
now also covers repeatable Character records with resolved and missing identities.

Two existing test fixtures were corrected to current production authority:
AuthoredLifecycleIntegrationTest explicitly assigns its secondary Item Pickup's
expected key after proving Add starts None; ItemPickupTargetHighlightTest uses the
existing support-anchored primitive helper in its fallback expectations. Neither
correction changes production Item Pickup behavior. EditorToolRunnerTest passes
outside the sandbox; its initial sandbox run could not poll child processes.
MSBuild used process-local PATH deduplication and disabled node reuse to avoid
the host environment's duplicate `PATH`/`Path` and parallel-node restrictions.

Known acceptance limits: Character selection/ghost bounds use the existing default
model box, while the actual World Model preview uses M110 TRS. Normal-editor
interaction, Player gameplay/Idle/Move/Jump/equipment, and directional shadows still
require the user's step-by-step Development observation. No commit, push, merge,
branch switch, milestone closure, or M112 work was performed.
