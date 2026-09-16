## Milestone 46 — Dynamic Box Runtime Recovery

### Status

**Implemented on `milestone/46-dynamic-box-runtime-recovery`. Awaiting mandatory manual user acceptance. Not CLOSED.**

This milestone remains OPEN until all of the following are complete:

1. implementation on the milestone branch;
2. automated validation;
3. manual user acceptance;
4. Git pre-closure inspection;
5. one meaningful milestone commit;
6. milestone branch push;
7. normal merge into `main`;
8. `main` push;
9. final confirmation that `main` is clean and synchronized.

A Cursor implementation report alone does **not** close M46.

---

### Official title

**M46 — Dynamic Box Runtime Recovery**

### Exact Git branch

`milestone/46-dynamic-box-runtime-recovery`

---

### Baseline / prerequisites

M46 starts from the post-M45 repository state.

Required baseline:

- M00–M45 are CLOSED.
- Canonical branch is `main`.
- `main` is clean and synchronized with its remote.
- M45 — Authored Dynamic Physics Objects is fully integrated.
- M45 Correction 1 is included.
- The current repository, current tests, and current documentation are the source of truth.
- `PROJECT_STATE_M45.md` is the architectural checkpoint.
- `DEVELOPMENT_WORKFLOW.md` defines the required milestone workflow.

Before changing code, inspect the actual current M45 Dynamic Box/Jolt implementation and identify the smallest existing runtime/physics authority where recovery can be implemented safely. Do not presume that any particular class, method, or API exists merely because this specification describes responsibilities.

---

### Post-M45 context

After M45, Dynamic Boxes are repeatable authored rigid bodies in Level Format v1.

Each Dynamic Box has authored properties including:

- center;
- size;
- mass.

Applied Dynamic Boxes create Jolt dynamic box bodies.

The key existing authority invariant is:

> Authored state and simulated runtime state are distinct.

The active authored Dynamic Box definition describes the authoritative applied authoring state. Jolt owns transient runtime pose and velocity during simulation.

Rendering and active picking use the current runtime Jolt pose. Saving continues to serialize authored state rather than silently capturing simulated physics state.

Current reset semantics established by M45:

- full Restart Run restores Dynamic Boxes from authored transforms and clears runtime velocities;
- checkpoint respawn does **not** reset Dynamic Boxes;
- Apply rebuilds from applied authored definitions;
- staged runtime reload rebuilds from staged authored definitions.

M46 extends this runtime behavior with **individual out-of-world recovery** while preserving all of those distinctions.

---

### Problem

A Dynamic Box can be pushed or fall out of the playable area.

Without a defined recovery rule, an authored physics object may become permanently unavailable during the current run even though its authored definition remains valid.

A naïve implementation could introduce regressions by:

- rebuilding the complete PhysicsWorld;
- resetting every Dynamic Box when only one fell;
- mutating editor authoring state from runtime simulation;
- changing Modified/Dirty semantics;
- persisting simulated positions through Save;
- conflating checkpoint respawn, Restart Run, Apply, reload, and runtime recovery.

M46 must solve this narrowly and deterministically.

---

### Goal

When an **active Dynamic Box runtime body** falls below the currently applied authored level kill plane, recover **only that Dynamic Box runtime instance** to its currently applied authored transform and clear its runtime motion.

The recovery must be:

- individual;
- runtime-only;
- deterministic;
- based on `active.killPlane`;
- based on the Dynamic Box's currently applied authored definition;
- independent from editor pending state;
- independent from other Dynamic Boxes;
- performed without rebuilding the entire PhysicsWorld.

---

### Core architectural decisions

#### 1. Reuse the existing authored kill plane

Recovery threshold is exactly the currently applied authored level kill plane:

`active.killPlane`

No additional margin is introduced.

No Dynamic-Box-specific recovery threshold is introduced.

No new Level Format field is introduced.

Level Format remains **v1**.

#### 2. Recovery is runtime-only

Crossing below `active.killPlane` is a runtime simulation event.

It must not perform an authoring operation.

It must not modify:

- `workingCopy`;
- active authored Dynamic Box definitions;
- `savedSourceBaseline`;
- Modified state;
- Dirty state;
- serialized Save output.

#### 3. Recover only the fallen Dynamic Box

If one Dynamic Box is below the kill plane, only its corresponding runtime body is reset.

Other Dynamic Boxes must preserve their own current runtime:

- position;
- orientation;
- linear velocity;
- angular velocity.

No global Dynamic Box reset and no full PhysicsWorld rebuild are allowed for individual recovery.

#### 4. Recovery transform comes from applied authored authority

The recovery transform must come from the Dynamic Box definition in the **currently applied authored state**.

Do not use:

- current `workingCopy` pending edits;
- the last saved source baseline unless it is also the active applied state;
- the current simulated transform;
- a hard-coded spawn position.

#### 5. Orientation returns to authored/initial state

The current Level Format v1 Dynamic Box record authors center, size, and mass and does not introduce a new orientation field in M46.

The implementation must inspect the actual current M45 body creation representation and restore the runtime orientation to the same authored/initial orientation used when that active Dynamic Box body was constructed.

M46 must not invent a new authored orientation representation or Level Format field.

#### 6. Runtime velocities are cleared

After individual recovery:

- linear velocity = zero;
- angular velocity = zero.

The body must not retain pre-fall momentum after being restored.

#### 7. No generic respawn framework

Implement the smallest coherent Dynamic Box runtime recovery path supported by the current architecture.

Do not introduce a generalized "respawnable physics entity" abstraction or unrelated framework.

---

### Authored authority vs runtime authority

The existing editor state model remains unchanged:

- `workingCopy`: pending authored edits;
- `active`: applied authored world;
- `savedSourceBaseline`: saved-source baseline;
- Modified = `workingCopy != active`;
- Dirty = `active != savedSourceBaseline`.

M46 adds no fourth authored authority.

For Dynamic Boxes:

- authored transform/data comes from `active`;
- simulated pose/velocity comes from Jolt;
- recovery uses the active authored transform as the reset target;
- after recovery, Jolt again owns subsequent transient motion.

Runtime recovery must not cause any authored-state comparison to change.

---

### Exact Dynamic Box Runtime Recovery behavior

For each active Dynamic Box runtime instance:

1. observe its actual current runtime Jolt pose using the established runtime authority;
2. read the current runtime Jolt body center/position Y;
3. if runtimeBodyCenterY < active.killPlane, recover that instance;
4. if the Dynamic Box is below `active.killPlane`, recover that instance;
5. restore its runtime position to the currently applied authored center/transform;
6. restore its runtime orientation to the authored/initial orientation represented by the current implementation;
7. set linear velocity to zero;
8. set angular velocity to zero;
9. preserve all other Dynamic Box runtime instances unchanged;
10. continue normal Jolt simulation after recovery.

The implementation must use the actual current project conventions for the precise body transform update and activation/wake semantics required by Jolt.

Do not invent an API in advance; inspect the repository and use the smallest existing authority boundary.

---

### Multiple Dynamic Boxes

M46 must work correctly with zero, one, or multiple Dynamic Boxes.

With multiple boxes:

- each runtime instance remains independent;
- recovery is determined per instance;
- one fallen box must not reset another valid box;
- another box's runtime pose and velocity must survive unchanged;
- capacity accounting remains shared with Platforms exactly as before.

Multiple boxes may recover independently if multiple bodies independently satisfy the recovery condition.

---

### PhysicsWorld / Jolt integration

The implementation must inspect the current M45 architecture and place:

- recovery detection; and
- individual runtime body reset

at the smallest existing runtime/physics authority that preserves ownership and avoids duplicated state.

Required constraints:

- no full PhysicsWorld rebuild for individual recovery;
- no alteration to transactional `PhysicsWorld::TryRebuild` semantics;
- no new arbitrary body quota;
- no authored-state mutation from physics;
- no opportunistic physics architecture rewrite.

The implementation should use existing body ownership/mapping structures where suitable rather than introducing speculative identity systems.

---

### Shared physics body budget

The current physics capacity invariant remains unchanged.

Current fixed body accounting after M45 is 5:

- Ground;
- two slopes;
- moving platform kinematic body;
- CharacterVirtual inner body.

Platforms and Dynamic Boxes share:

`fixedBodies + platformCount + dynamicBoxCount <= 64`

Therefore the current authored constraint remains conceptually:

`platformCount + dynamicBoxCount <= 59`

M46 must not:

- increase `kPhysicsMaxBodies`;
- create a second independent Dynamic Box quota;
- add extra persistent recovery bodies;
- weaken validation;
- alter transactional rebuild guarantees.

---

### Interaction with Apply

Apply semantics remain unchanged.

Apply validates and promotes `workingCopy` to `active`, then reconstructs the active/runtime world through the established transactional path.

After a successful Apply:

- Dynamic Box runtime bodies are rebuilt from the newly applied authored definitions;
- future runtime recovery targets use those newly active authored transforms;
- pending `workingCopy` state that has not been applied must never be used as a recovery target.

M46 must not make runtime recovery invoke Apply.

---

### Interaction with Revert

Revert semantics remain unchanged:

`workingCopy = active`

Revert affects pending authored edits only.

Runtime recovery must not:

- trigger Revert;
- change whether Revert is available;
- use Revert as a recovery mechanism;
- reset runtime bodies simply because Revert occurred unless the current established editor flow already rebuilds runtime state for another valid reason.

Do not change existing Revert behavior outside what is strictly necessary for M46.

---

### Interaction with Save

Save remains authored-only.

Runtime recovery must not:

- write files;
- dirty authored state;
- change active authored transforms;
- serialize recovered runtime transforms;
- serialize the simulated pre-recovery transform.

If a box is authored at X=2, simulated to X=8, then falls and recovers, Save must still serialize its authored X=2 definition.

---

### Interaction with Restart Run

Existing M45 Restart Run semantics must be preserved.

Restart Run resets **all** Dynamic Boxes from the current authoritative authored run state according to the existing implementation and clears their runtime linear/angular velocity.

This is deliberately distinct from M46 runtime recovery:

- runtime recovery resets only each box that individually falls below the kill plane;
- Restart Run resets all Dynamic Boxes as part of the established full-run reset semantics.

Do not replace Restart Run with the individual recovery path if that would change existing run-reset behavior.

---

### Interaction with checkpoint respawn

Existing checkpoint respawn semantics must be preserved.

Respawning the player at a checkpoint does **not** reset valid Dynamic Boxes.

Example:

- box A is pushed to a new valid runtime position;
- player dies and respawns at a checkpoint;
- box A remains at its runtime position;
- if box A later falls below `active.killPlane`, only then does M46 recovery restore it.

Do not conflate player respawn with Dynamic Box recovery.

---

### Interaction with staged runtime reload

Existing Development staged runtime reload remains authoritative.

A successful staged reload:

- reloads staged authored level data through the established path;
- transactionally rebuilds runtime state;
- reconstructs Dynamic Boxes from that staged authored definition.

M46 runtime recovery must not change staged reload authority or introduce source/cooked fallback.

After reload, recovery targets must correspond to the newly active staged authored state.

---

### Rendering and picking

Existing M45 behavior remains unchanged:

- active Dynamic Box rendering follows current Jolt pose;
- active Dynamic Box picking follows current Jolt pose;
- pending editor picking follows existing workingCopy proxy rules.

After recovery, rendering and picking naturally observe the recovered Jolt runtime pose.

Do not add a separate visual-only recovery position.

---

### Development editor

Development retains full authoring/editor functionality.

M46 does not introduce a new editor command or UI.

Required Development behavior:

- runtime recovery can occur while exercising the Development runtime;
- it does not alter Modified/Dirty state;
- it does not change selection/lifecycle authority unnecessarily;
- pending authored edits remain governed by `workingCopy`;
- active runtime bodies remain governed by applied state and Jolt;
- Add/Duplicate/Delete/Translate/Resize, Apply/Revert/Save, picking and gizmos remain regressions to protect.

No "Reset Dynamic Box" button or menu item is added.

---

### Debug

Debug must compile and preserve the runtime behavior available in that configuration under the current project architecture.

M46 must not introduce Development-editor dependencies into Debug runtime code.

No new Debug-only authoring feature is required.

---

### Release

Release must compile without Development editor dependencies and preserve runtime Dynamic Box recovery where Dynamic Boxes exist in authored runtime data.

M46 must not expose editor-only UI or authoring systems in Release.

---

### Level Format

Level Format remains **v1**.

Existing Dynamic Box syntax remains unchanged:

`dynamic_box <cx> <cy> <cz> <sx> <sy> <sz> <massKg>`

No new:

- recovery threshold;
- respawn flag;
- orientation field;
- recovery position;
- recovery count;
- format version

is introduced.

---

### Canonical Level 01 safety

The final canonical source Level 01 must remain semantically:

- Platforms = 6
- Checkpoints = 2
- Hazards = 2
- Collectibles = 3
- Dynamic Boxes = 0
- FOV = 40

The intentional M45 deletion of:

`dynamic_box 0 5 0 1 1 1 30`

must remain deleted.

Manual testing may use temporary Dynamic Boxes, but those objects must **not** be left saved in canonical Level 01.

Before reporting completion, explicitly inspect the semantic diff of:

`game/assets/source/levels/level_01.level`

Do not mechanically run a worktree restore that could accidentally restore the legacy Dynamic Box line. Only restore proven unintended worktree/EOL noise, and only after confirming that doing so cannot undo intentional semantic state.

---

### Required automated tests

Tests must exercise the real runtime/physics authority boundary rather than only isolated helper logic.

The final test layout may extend existing tests or add a focused test according to current repository conventions. Do not invent a test architecture before inspecting the repository.

Automated coverage must prove at least:

1. one active Dynamic Box whose runtime position falls below the active kill plane recovers to its active authored transform;
2. recovered linear velocity is zero;
3. recovered angular velocity is zero;
4. recovered runtime orientation is restored to the correct authored/initial orientation;
5. with multiple Dynamic Boxes, another valid box preserves its independent runtime pose and velocity;
6. recovery does not modify authored `LevelDefinition` state;
7. recovery does not modify `workingCopy` / `savedSourceBaseline` / Modified / Dirty semantics where those authorities are available at the tested integration boundary;
8. recovery works correctly after Apply/rebuild so the recovery target is the newly active authored transform;
9. Restart Run still resets all Dynamic Boxes according to M45 semantics;
10. checkpoint respawn still does not reset valid Dynamic Boxes;
11. staged runtime reload still reconstructs Dynamic Boxes from staged authored state and subsequent recovery uses that new active state;
12. zero Dynamic Boxes remains valid;
13. shared Platform + Dynamic Box capacity validation remains unchanged;
14. transactional PhysicsWorld rebuild behavior remains unchanged.
15. recovery threshold semantics must be tested using runtime body center Y, not box bottom/AABB extent or authored center.

Where editor authority cannot reasonably be asserted inside a low-level physics test, cover it at the appropriate integration boundary rather than creating artificial coupling.

---

### Required integration validation

Run the most relevant current integration suites, including those that cover:

- authored object lifecycle;
- lifecycle owner dispatch/integration;
- Dynamic Box physics/runtime behavior;
- editor picking;
- level parsing/serialization;
- physics rebuild/capacity/transactionality;
- canonical scene cleanup;
- Apply/Revert/Save authority;
- staged Cook/Stage/Reload workflow;
- editor workspace/tooling paths affected by the changes.

Current known relevant suites from the post-M45 checkpoint include:

- `AuthoredObjectLifecycleTest`
- `AuthoredLifecycleIntegrationTest`
- `CanonicalSceneCleanupTest`
- `EditorWorkspaceTest`
- `EditorGizmoTest`
- `EditorOrientationTest`
- `EditorPickingTest`
- `EditorPlacementTest`
- `EditorQuickToolbarTest`
- `LevelFileTest`
- `PhysicsRebuildTest`
- `CookStageReloadWorkflowTest`
- `EditorToolRunnerTest`

Inspect the actual current test targets and run those that exist and are relevant. If names or organization have changed in the repository, follow the current repository rather than this historical list.

---

### Required regression protection

M46 must preserve at minimum:

- M41 authored lifecycle behavior;
- M42 Object Palette placement behavior;
- M43 Quick Toolbar behavior;
- M44 canonical legacy-scene cleanup;
- M45 repeatable Dynamic Boxes;
- M45 Correction 1 lifecycle request dispatch protection;
- Dynamic Box Add/Duplicate/Delete;
- Dynamic Box Translate/Resize;
- Apply/Revert/Save;
- runtime render and picking from Jolt pose;
- pending editor proxy/picking rules;
- restart semantics;
- checkpoint respawn semantics;
- staged reload semantics;
- body capacity validation;
- transactional rebuild;
- Level Format v1 round-trip behavior;
- zero/one/multiple Dynamic Box validity.

---

### Build validation

Perform canonical Windows build validation:

```powershell
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

If the repository's current canonical commands have changed, use the current documented equivalents and report that explicitly.

All required configurations must build successfully before Cursor stops.

---

### Python / tooling regressions

Run the relevant existing Python regressions, including when applicable:

```powershell
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
```

Because Level Format must remain v1 and staged reload semantics are a required regression, the level cooker/staging paths should be verified unless current repository organization demonstrates a different canonical command.

Report each executed Python regression and its result.

---

### `git diff --check`

Before the implementation report:

```powershell
git diff --check
```

must pass.

Also inspect:

```powershell
git diff -- game/assets/source/levels/level_01.level
```

and verify canonical semantics explicitly.

---

### Detailed manual acceptance checklist

Manual acceptance is mandatory after Cursor completes automated validation.

Use Development for authoring/runtime observation unless a specific check targets another build configuration.

#### A. Basic individual recovery

1. Start from canonical Level 01 without saving test objects.
2. Add one temporary Dynamic Box through the established editor workflow.
3. Apply it.
4. Push or otherwise cause the box to fall below the active level kill plane.
5. Confirm the box returns to its applied authored position.
6. Confirm it does not continue with obvious pre-fall linear momentum.
7. Confirm it does not continue spinning with pre-fall angular momentum.
8. Confirm its orientation returns to the expected initial/authored orientation.

#### B. Multi-box independence

1. Create and Apply at least two temporary Dynamic Boxes at distinguishable positions.
2. Move/push both into different runtime states.
3. Cause only box A to fall below the kill plane.
4. Confirm box A recovers.
5. Confirm box B remains at its independent runtime pose.
6. Confirm box B does not lose its independent runtime motion merely because box A recovered.

#### C. Checkpoint respawn distinction

1. Put a Dynamic Box in a changed but valid runtime position above the kill plane.
2. Trigger player death/checkpoint respawn.
3. Confirm the Dynamic Box remains in its runtime position.
4. Then cause that box to fall below the kill plane.
5. Confirm only the kill-plane event recovers it.

#### D. Restart Run distinction

1. Move multiple Dynamic Boxes away from authored positions.
2. Use Restart Run.
3. Confirm all Dynamic Boxes reset according to established M45 run-reset behavior.
4. Confirm this remains distinct from individual kill-plane recovery.

#### E. Apply semantics

1. Make a pending authored transform change to a Dynamic Box.
2. Before Apply, confirm runtime recovery does not start using the pending `workingCopy` transform.
3. Apply the change.
4. Cause the box to fall below the kill plane.
5. Confirm recovery now targets the newly applied authored transform.

#### F. Revert semantics

1. Create a pending Dynamic Box edit without applying it.
2. Revert.
3. Confirm the pending edit is discarded according to existing behavior.
4. Confirm runtime recovery has not introduced unexpected authored or runtime side effects.

#### G. Save authority

1. Apply a Dynamic Box at a known authored position.
2. Move it through runtime physics.
3. Optionally cause it to recover.
4. Save using the established workflow only if testing on a safe noncanonical level or with a plan to avoid persisting temporary canonical data.
5. Confirm Save represents the authored state, not the simulated pre-recovery or post-recovery transient state.
6. Ensure no temporary box remains in canonical Level 01.

#### H. Staged reload

1. Exercise the existing Cook/Stage/Reload path using safe test data/workflow.
2. Confirm reload reconstructs Dynamic Boxes from staged authored definitions.
3. Move a reloaded box below the active kill plane.
4. Confirm recovery targets the newly loaded active authored transform.

#### I. Picking/rendering

1. Push a Dynamic Box to a runtime position different from its authored center.
2. Confirm rendering follows that Jolt pose.
3. Confirm active picking selects it at that runtime pose.
4. Cause recovery.
5. Confirm rendering/picking immediately follow the recovered runtime Jolt pose.
6. Confirm there is no separate stale visual or picking proxy for active physics bodies.

#### J. Zero-box canonical scene

1. Remove/avoid all temporary test boxes.
2. Return to canonical Level 01 state.
3. Confirm the level runs correctly with zero Dynamic Boxes.
4. Confirm editor Hierarchy/placement/lifecycle behavior remains stable with zero boxes.

#### K. Canonical-data final verification

Before approving M46:

- Platforms = 6
- Checkpoints = 2
- Hazards = 2
- Collectibles = 3
- Dynamic Boxes = 0
- FOV = 40
- legacy `dynamic_box 0 5 0 1 1 1 30` remains absent.

No temporary authored test object may remain saved.

---

### Documentation to update

Update only documentation whose description of current runtime behavior becomes incomplete after M46.

Likely candidates, subject to inspection of the current repository:

- `README.md`
- `AGENTS.md`
- `docs/ARCHITECTURE.md`
- `docs/MILESTONES.md`
- physics/editor/runtime behavior documentation that currently defines Dynamic Box semantics.

`docs/LEVEL_FORMAT_V1.md` should not gain a new syntax field because the format is unchanged. It should only be edited if current documentation needs a behavior clarification that genuinely belongs there.

Prefer describing the resulting current system instead of adding stale chronological history.

---

### Explicit non-goals

M46 must **not** implement:

- Level Format v2;
- any new Dynamic Box syntax field;
- per-box recovery threshold;
- kill-plane recovery margin;
- separate Dynamic Box kill plane;
- authored recovery position;
- authored orientation support;
- new physics shapes;
- spheres;
- capsules;
- cylinders;
- arbitrary convex/dynamic GLB collision;
- friction UI;
- restitution UI;
- density-based mass;
- physics material editor;
- joints;
- constraints;
- ropes;
- ragdolls;
- grab/carry;
- pressure plates;
- switches;
- doors;
- generic gameplay interaction systems;
- generic "respawnable physics entity" framework;
- physics-layer editor;
- prefabs;
- GUID identity;
- ECS migration;
- undo/redo;
- multi-select;
- generic hot-reload watcher;
- unrelated editor UX;
- unrelated refactors;
- M47 work.

---

### Completion criteria

M46 implementation is ready for manual acceptance only when all of the following are true:

- runtime recovery uses exactly the active authored kill plane;
- an individual fallen Dynamic Box is restored to its active authored transform;
- its runtime orientation is reset correctly using the current representation;
- linear velocity is zeroed;
- angular velocity is zeroed;
- other Dynamic Boxes preserve independent runtime state;
- no full PhysicsWorld rebuild is used for individual recovery;
- `workingCopy` is unchanged by recovery;
- active authored definitions are unchanged by recovery;
- `savedSourceBaseline` is unchanged by recovery;
- Modified/Dirty state is unchanged by recovery;
- Save remains authored-only;
- Restart Run semantics remain intact;
- checkpoint respawn semantics remain intact;
- Apply/Revert semantics remain intact;
- staged reload semantics remain intact;
- active rendering and picking still follow Jolt runtime pose;
- zero Dynamic Boxes remains valid;
- body budget/capacity validation is unchanged;
- transactional rebuild remains unchanged;
- Level Format remains v1;
- focused automated tests pass;
- relevant M41–M45 regressions pass;
- required Python/tooling regressions pass;
- Debug build passes;
- Development build passes;
- Release build passes;
- `git diff --check` passes;
- canonical Level 01 is semantically unchanged from the approved post-M45 state;
- documentation affected by the behavior is updated;
- Cursor provides the required detailed report.

After that, the user performs mandatory manual acceptance.

M46 is **not CLOSED** until manual approval and the complete Git closure procedure are performed and `main` is clean/synchronized.

---

### Cursor implementation report requirements

At the end of implementation and automated validation, Cursor must report:

- root implementation approach;
- exact files changed;
- where recovery detection lives;
- where individual body reset lives;
- how active authored state is used as the recovery source;
- how runtime/editor authority separation is preserved;
- how multi-box independence is guaranteed;
- how orientation reset is represented;
- how linear/angular velocity reset is performed;
- how Restart Run remains distinct;
- how checkpoint respawn remains distinct;
- how Apply/Revert remain correct;
- how staged reload remains authoritative;
- tests added or changed;
- focused test results;
- relevant regression results;
- Python/tooling results;
- Debug build result;
- Development build result;
- Release build result;
- canonical Level 01 verification;
- `git diff --check` result;
- any remaining risks or limitations.

---

### STOP condition

After implementation, automated validation, build validation, canonical-data verification, documentation updates, and the detailed report:

- Do **NOT** commit.
- Do **NOT** push.
- Do **NOT** merge.
- Do **NOT** checkout or start M47.
- Do **NOT** declare M46 CLOSED.
- **STOP.**

M46 then waits for mandatory manual user acceptance and subsequent Git closure.
