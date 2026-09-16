## Milestone 44 --- Legacy Prototype Scene Cleanup

**Branch:** `milestone/44-legacy-prototype-scene-cleanup`\
**Status:** CLOSED. Merged to main. Milestone 45 is Authored Dynamic Physics Objects.\
**Prerequisite:** Milestone 43 --- Editor Quick Toolbar --- CLOSED\
**Scope:** Audit and remove obsolete prototype/test visuals from the
playable/editor scene while preserving useful automated asset-pipeline
coverage and intentional gameplay geometry.

### Goal

Clean the Level 01 runtime/editor scene of legacy visual probes and
prototype objects introduced during early rendering, asset-pipeline,
physics, and authoring milestones.

The cleanup must distinguish between obsolete scene instances, useful
automated test assets, intentional gameplay/editor geometry, and physics
probes. Do not delete an object merely because it looks temporary.

### Phase A --- provenance audit

Before changing runtime behavior, identify every suspicious prototype
object. For each candidate report its visible description, code/data
symbol, source file, runtime creation path, render path, physics path if
any, LevelDefinition/Level Format relationship, Hierarchy relationship,
automated-test dependencies, asset-pipeline dependencies, and proposed
disposition.

At minimum audit:

-   plain white/grey test cube;
-   pink/black checker-textured cube;
-   larger textured test cube;
-   orange wedge/ramp-like object;
-   `textures/test_checker.png`;
-   `models/test_static.glb`;
-   `models/test_authored.glb`;
-   `models/test_textured.glb`;
-   both canonical slopes;
-   moving platform;
-   dynamic cyan box;
-   the four historical cooker probes intentionally excluded from
    Hierarchy.

Allowed dispositions:

-   Keep in canonical scene
-   Remove scene instance, keep test asset
-   Remove scene instance and obsolete code
-   Defer because purpose remains intentional/uncertain

Do not perform broad deletion before this audit.

### Core policy

Separate **test coverage** from **canonical scene clutter**.

If an asset exists only to prove cooking/loading/texturing, prefer
keeping the asset and automated test while removing its always-visible
runtime/editor scene instance.

If an object is intentional gameplay geometry or required by current
semantics, keep it.

### Slopes

The canonical world historically has exactly two slopes, and M42
placement treats slopes as eligible placement surfaces.

Do not remove a slope merely because the orange wedge looks like a
prototype. First prove whether the marked wedge is a canonical slope. If
it is, keep it. If it is only an obsolete visual proxy, remove only that
proxy.

M44 must not silently change slope gameplay.

### Moving platform

Audit the moving platform and determine whether it remains an
intentional gameplay mechanic/regression fixture. Do not remove it
automatically.

### Dynamic cyan box

Audit the dynamic body introduced during physics validation. If it is
only a legacy runtime probe, prefer removing it from the canonical scene
while preserving focused physics coverage where useful.

Do not remove Jolt/dynamic-body infrastructure merely because a probe is
removed.

### Asset-pipeline probes

Trace:

-   `textures/test_checker.png`
-   `models/test_static.glb`
-   `models/test_authored.glb`
-   `models/test_textured.glb`

Determine which are visibly instantiated in Level 01.

Preferred policy:

-   automated cooker/staging/model/texture coverage remains;
-   test assets remain if tests require them;
-   visible canonical probe instances are removed when they have no
    gameplay/editor purpose.

The four historical cooker probes were intentionally excluded from
Hierarchy. Audit whether these correspond to the user-visible legacy
objects.

### Phase B --- minimal cleanup

After the audit, perform only justified cleanup:

-   remove obsolete always-visible probe instances;
-   keep useful test assets/tests;
-   preserve intentional slopes;
-   preserve/remove moving and dynamic probes only according to
    evidence;
-   remove dead scene-specific render/setup code made unreachable.

Do not turn M44 into a scene architecture rewrite.

### Level Format and lifecycle

Preferred outcome: **no Level Format v1 schema change**.

Do not add fields just to hide prototype probes. Do not introduce Level
Format v2.

Preserve all M41--M43 contracts:

-   active/workingCopy;
-   StructuralIndexMap;
-   Platforms/Checkpoints/Hazards/Collectibles;
-   Spawn/Ground/Camera/Goal;
-   support-index metadata;
-   Object Palette and placement;
-   pending visuals/picking;
-   Quick Toolbar.

Do not force legacy probes into the authored lifecycle system merely to
remove them.

### Runtime/resource cleanup

For obsolete instances:

-   remove always-visible draw calls;
-   remove scene-only transforms/constants no longer needed;
-   remove scene-only model handles if no remaining runtime/test path
    needs them;
-   preserve RAII/resource lifetime;
-   avoid loading an asset at runtime solely for a removed visual unless
    a current runtime test requires it.

### Physics/body-budget safety

If a removed probe has a physics body, audit whether the body is still
required.

M41 Platform capacity is tied to the Jolt body budget. If canonical
non-Platform body count changes:

-   recompute actual capacity;
-   keep accounting centralized/static-asserted;
-   update parser/lifecycle capacity consistently;
-   add regression coverage.

Do not change Platform capacity unless the actual body accounting
changes.

### Editor and placement safety

Removed probes must no longer clutter the viewport, intercept picking,
create hidden proxies, or affect placement surfaces.

Intentional Ground/Platform/Slope placement remains correct.

Verify M42 candidate ray, fallback, pending picking, and gizmo behavior.

Verify M43 Quick Toolbar remains unaffected, visible and hidden.

### Visual acceptance target

The canonical scene should read as an intentional platformer level
rather than an engine test room.

The user-marked obsolete cubes should disappear if the audit confirms
they are probes. Any retained unusual object must have a clear current
purpose documented in the report.

## Non-goals

Do not add new art, terrain, lighting overhaul, gameplay mechanics,
Level Format v2, prefabs, scene graph, asset browser, singleton deletion
UI, broad Jolt refactor, or asset-pack redesign.

### Automated validation

Add focused M44 coverage where appropriate for:

1.  removed probe instances no longer contributing to canonical
    rendering;
2.  retained slope count/behavior;
3.  moving-platform disposition;
4.  dynamic-body disposition;
5.  useful asset tests remaining covered;
6.  picking excluding removed probes;
7.  placement surfaces remaining Ground/Platform/Slope;
8.  Level Format v1 round-trip unchanged;
9.  body-budget/Platform capacity correctness if physics accounting
    changes;
10. M41 lifecycle regression;
11. M42 placement regression;
12. M43 toolbar regression.

Run all applicable configurations for:

    AuthoredObjectLifecycleTest
    AuthoredLifecycleIntegrationTest
    EditorWorkspaceTest
    EditorGizmoTest
    EditorOrientationTest
    EditorPickingTest
    EditorPlacementTest
    EditorQuickToolbarTest
    LevelFileTest
    PhysicsRebuildTest
    CookStageReloadWorkflowTest
    EditorToolRunnerTest

Also run existing asset/model/texture tests found during the audit.

Run:

    python tools/test_stage_runtime_assets.py
    python tools/test_cook_level_v1.py
    python tools/test_cook_runtime_png.py

and relevant existing GLB/asset cooker tests.

Then:

    cmake --preset windows-vs2022
    cmake --build --preset windows-debug
    cmake --build --preset windows-development
    cmake --build --preset windows-release

### Manual acceptance

#### Visual cleanup

Launch canonical Level 01 and inspect the region where old test objects
were visible. Confirm every removed object is gone and every retained
unusual object matches the audit.

#### Slopes

Locate both canonical slopes, traverse them, verify collision, and in F2
verify Object Palette surface placement over a slope.

#### Moving platform

If retained, verify motion and CharacterVirtual interaction. If removed,
confirm the audit justification and absence of stale render/physics
artifacts.

#### Dynamic body

If retained, document why. If removed, confirm no legacy dynamic probe
remains and physics regressions pass.

#### Asset probes

Confirm obsolete white/checker/textured test cubes no longer clutter the
scene if classified as probes. Verify cooker, staging and runtime
startup still work.

#### Editor

Verify Hierarchy, picking, Object Palette, pending Add/Modify/Delete
visuals, Quick Toolbar and orientation widget.

#### Gameplay

Perform a short run covering movement, jump, slopes, moving platform if
retained, checkpoint, hazard, collectible, goal and respawn/restart as
practical.

### Canonical safety

Unless the audit proves an authored record itself is obsolete, preserve:

-   Platforms = 6
-   Checkpoints = 2
-   Hazards = 2
-   Collectibles = 3
-   FOV = 40

Run:

    git diff -- game/assets/source/levels/level_01.level
    git diff --check

If there is no intended semantic level change and only the known EOL
artifact appears:

    git restore --worktree --source=HEAD -- game/assets/source/levels/level_01.level

Do not normalize EOL.

If the audit requires an intentional authored level change, STOP and
report it before treating it as accepted.

### Documentation

Update as appropriate:

-   `README.md`
-   `AGENTS.md`
-   `docs/ARCHITECTURE.md`
-   `docs/MILESTONES.md`

Document identified probes, removed scene instances, retained test
assets, slopes/moving-platform/dynamic-body disposition, body-budget
changes if any, and Level Format impact.

### Required report

Return:

1.  branch and baseline
2.  provenance audit
3.  white/grey cube identity/disposition
4.  checker cube identity/disposition
5.  larger textured cube identity/disposition
6.  orange wedge identity/disposition
7.  `test_checker.png` disposition
8.  `test_static.glb` disposition
9.  `test_authored.glb` disposition
10. `test_textured.glb` disposition
11. slope 0 disposition
12. slope 1 disposition
13. moving platform disposition
14. dynamic cyan box disposition
15. cooker-probe/Hierarchy relationship
16. removed render paths
17. removed runtime loads/resources
18. removed physics bodies, if any
19. retained test-only assets
20. staging inventory changes, if any
21. cooker-test changes, if any
22. body-budget before/after
23. Platform capacity before/after
24. Level Format v1 impact
25. LevelDefinition impact
26. editor picking impact
27. placement-surface impact
28. M41 lifecycle regression
29. M42 placement regression
30. M43 toolbar regression
31. focused M44 tests
32. regression-suite results
33. asset/model/texture regression results
34. Python staging
35. Python level cooker
36. Python PNG cooker
37. CMake configure
38. Debug build
39. Development build
40. Release build
41. manual visual cleanup status
42. manual slopes status
43. manual moving-platform status
44. manual dynamic-body status
45. manual editor status
46. manual gameplay status
47. canonical counts
48. FOV 40
49. canonical level diff
50. git diff --check
51. docs
52. no commit
53. no push
54. no merge
55. M44 incomplete pending manual approval
56. M45 not started
57. STOP

### Implemented (this branch)

Provenance audit classified the user-marked spawn-area cubes as the
M15–M19 cooker probes (hard-coded renderer instances, never Hierarchy,
never Level Format). They are removed from canonical load/draw. Source,
cooked, and staged files plus Python cooker/staging tests remain.

The spawn-area orange pyramid is `models/test_static.glb`, not a
gameplay slope. Slope 0 (30° walkable) and slope 1 (60° steep) stay.
The moving platform stays. Authored `dynamic_box` stays in Level Format
v1 for parser/writer compatibility; it is not a Hierarchy/Inspector
scene object, not drawn, not picked, and has no Jolt body. Platform leftover is 59. No Level Format schema change and
no semantic `level_01.level` edit.

Correction 1: remove Dynamic Cyan Box from Development Hierarchy and
the obsolete Inspector path. `IsValidSelection(DynamicBox)` is false.

Focused coverage: `CanonicalSceneCleanupTest`. M44 is not complete until
manual acceptance.

### Stop condition

After audit, minimal implementation, automated validation, builds and
report:

**STOP.**

Do not commit.\
Do not push.\
Do not merge.\
Do not start M46 from this closed milestone.

M44 is CLOSED and merged.
