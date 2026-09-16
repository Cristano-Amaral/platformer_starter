## Milestone 41 — Authored Object Add / Delete / Duplicate

### Status
**CLOSED.** Phase A, Phase B, and Phase C accepted. Milestone 41 is complete and merged.

M40 is closed. M41 is closed. Milestone 42 is Object Palette & Placement Workflow.

### Branch
`milestone/41-authored-object-lifecycle`

### Recommended Cursor model
**Grok 4.6 High — Fast OFF**

### Goal
Extend the Development Level Editor so repeatable Level Format v1 objects can be **added, duplicated, and deleted** without hand-editing `level_01.level`.

Supported lifecycle categories:
- Platforms
- Checkpoints
- Hazards
- Collectibles

Fixed/out of scope lifecycle categories:
- Spawn, Ground, Camera, Goal
- slopes, moving platform, dynamic cyan box
- technical/demo renderer probes

This is not a generic scene graph, prefab system, ECS conversion, undo/redo system, or Level Format v2.

### Authoring semantics
Lifecycle operations modify `workingCopy` only.

- **Add:** inserts a supported object into `workingCopy` using editor-camera X/Y and `workingCopy` spawn.z as the current gameplay-lane Z; new object becomes selected.
- **Duplicate:** copies the selected supported object, applies +1 world X, preserves source Z (no lane snap), and selects the copy.
- **Delete:** removes the selected supported object from `workingCopy` and safely clears/reconciles selection. Platforms require count > 1 and must not be referenced by checkpoint/goal support metadata.

Collected runtime visibility is separate from authored editor visibility. Opening F2 does not mutate `CollectibleRunState`. Collected authored Collectibles remain selectable/editable.

Pending-deleted objects remain in `active` until Apply, with a Development editor wireframe mark. A session-local active↔working index map keeps surviving same-category viewport picks valid. Delete key (Development) matches Edit > Delete Selected and is ignored while ImGui captures the keyboard.

Immediately after any lifecycle edit:
- `workingCopy` changes;
- `active` does not;
- `Modified = true`;
- runtime physics/gameplay does not change;
- no automatic Apply or Save occurs.

`Apply Preview` remains the transactional commit boundary. On success, rebuild/validate using the existing path and commit to `active`. On failure, preserve the active runtime world and keep `workingCopy` available for correction.

`Revert Working Copy` restores `workingCopy = active`, cancels pending additions/deletions/duplicates, clears unsafe transient gizmo state, and returns `Modified = false`.

`Save Level Source` remains Development-only and saves `active`, never unapplied `workingCopy`.

### Level Format v1
Level Format v1 remains unchanged. Phase A must audit whether repeatable categories currently use fixed-size arrays/assumptions and make the smallest safe change needed for variable counts.

Requirements:
- existing v1 files still load;
- parser/writer remain deterministic;
- existing v1 count grammar is preserved;
- cooker accepts authored variable counts;
- runtime state sizes safely follow authored counts;
- M39 Reload and M40 Cook, Stage & Reload continue to work;
- no silent migration to v2.

Prefer standard variable-length containers only for genuinely repeatable categories. Do not convert the whole level to a generic entity list.

### Selection and ordering
Current type+index selection may remain if safe invalidation rules are explicit and tested. Do not add GUIDs unless Phase A proves they are necessary.

Required:
- Add selects the new object;
- Duplicate selects the copy;
- Delete cannot leave a dangling index;
- Apply/Revert/Reload reconcile selection safely;
- deleting during a gizmo interaction clears transient manipulation state.

Insertion/order must be deterministic. Prefer Add-at-end. Phase A must choose and document whether Duplicate inserts after the selected object or appends. Do not reorder unrelated objects.

Checkpoint container/writer order remains gameplay checkpoint order unless the existing format already defines otherwise.

### Defaults
Phase A must define deterministic valid Add defaults for each category and a small deterministic Duplicate offset (preferably world +X if valid).

No drag-to-place, snapping, placement mode, surface raycast placement, or configurable duplicate offset in M41.

### UI
Preferred Development menu:

```text
Edit
├ Add
│  ├ Platform
│  ├ Checkpoint
│  ├ Hazard
│  └ Collectible
├ Duplicate Selected
└ Delete Selected
```

Existing View / Transform / Level / Build menus remain.

Optional shortcuts:
- `Ctrl+D` — Duplicate Selected
- `Delete` — Delete Selected

Only add shortcuts if they cleanly honor current-frame ImGui keyboard capture and do not conflict with navigation/gizmo/text input. Menu-only operation is acceptable.

### Hierarchy / Inspector / pending state
Hierarchy is an authoring view and should reflect `workingCopy` immediately.

Therefore before Apply:
- pending additions/duplicates may appear in Hierarchy/Inspector;
- pending deletions may disappear from Hierarchy;
- active runtime world remains unchanged.

World picking keeps the existing rule: **the active visible world is the authority**. Phase A must explicitly prevent a pending-deleted active object from producing an invalid working-copy selection.

Reuse the smallest existing pending/ghost visualization where practical for newly added/duplicated objects. Do not build a second full preview renderer or a tombstone system.

Inspector edits selected `workingCopy` objects. Deleted objects cannot leave stale Inspector references.

Existing gizmo capabilities remain category-dependent. New Platforms must support existing Translate/Resize. No rotation gizmo.

### Validation and limits
Audit existing validation before choosing count limits. Protect against:
- unsafe/pathological counts;
- parser/writer count mismatch;
- unsafe allocations/overflow;
- invalid dimensions;
- non-finite values.

A conservative shared per-category maximum is acceptable if justified by current runtime assumptions.

### Runtime audit
Phase A must inspect all fixed-count assumptions in at least:
- `LevelDefinition`;
- Level Format v1 parser/writer;
- cooker;
- hierarchy/inspector;
- selection/picking;
- gizmos/pending ghost;
- `PhysicsWorld::TryRebuild`;
- static platform creation;
- checkpoint progression/state;
- hazard state;
- collectible state/count;
- run reset/respawn;
- M39 runtime reload;
- M40 workflow;
- tests/docs.

Variable checkpoint, hazard, collectible, and platform counts must resize/rebuild runtime state safely.

### Configuration boundaries
**Development:** lifecycle authoring UI enabled.

**Debug:** preserve existing editor/debug behavior, but no source-authoring lifecycle operation that violates the established Development-only authoring boundary.

**Release:** no editor/lifecycle authoring UI.

### Phase A — architecture and fixed-count audit
1. Audit all fixed-count assumptions.
2. Map v1 parser/writer/count grammar and cooker assumptions.
3. Map runtime state sizing.
4. Map hierarchy/selection/picking/gizmo assumptions.
5. Choose smallest safe variable-count representation.
6. Define insertion ordering.
7. Define Add defaults.
8. Define Duplicate offset.
9. Define Delete selection behavior.
10. Resolve active-vs-working picking edge cases.
11. Define validation/count limits.
12. Define configuration gating.
13. Implement/refactor pure testable data operations where useful.
14. Add focused non-live tests.
15. Preserve M39/M40 behavior.
16. Update architecture docs.
17. Prepare exact Phase B plan.

**Do not add the live Edit menu in Phase A.**
No Phase C. No commit/push/merge.

Phase A implemented (this branch): `std::vector` for the four repeatable categories; singletons unchanged; append-only Add/Duplicate (existing platform-index references do not shift); Platform delete remaps `support_index_*` with `R > D`, rejects `R == D` and the last platform; delete success clears selection; duplicate offset +1 X; per-category `CategoryStructuralPending` so active picks are not remapped across index shifts; min 1 platform; no GUIDs. The original Phase A 16/8/8/16 caps were defensive policy only; Correction 7 replaces them with physics leftover for Platforms (58) and the shared 256-line / 64 KiB parser guard for Checkpoint / Hazard / Collectible.

Phase B implemented (this branch): Development Edit menu (Add/Duplicate/Delete, menu-only plus Delete key); `LevelEditorRequest` intents; `HandleAuthoredLifecycleRequest` mutates workingCopy only; Inspector edits for Checkpoint/Hazard/Collectible authored fields; Translate gizmo for those three categories (Resize remains Ground/Platform); Checkpoint world Translate moves `center` and `respawnPosition` by the same delta so the authored relative offset is preserved (Inspector Trigger Center / Respawn Position stay independent); category-local pick guard; cyan **wireframe** pending bounds plus Development pending **object** ghosts (checkpoint post/beacon, full hazard, collectible visual cube) from `workingCopy` so bounds do not hide the object; pending Add/Modify ghosts persist after deselection (`CollectPendingAuthoringVisuals`); selected pending uses stronger cyan, unselected pending uses softer cyan; Platform uses one cyan wire AABB (visual == bounds); Add uses editor-camera placement anchor (look-forward * 10), not Spawn; Duplicate remains +1 X; editor-only Checkpoint respawn marker from `workingCopy`; pending ghosts create no physics/gameplay; visible pending Add/Duplicate/Modify ghosts are viewport-pickable (`PendingPickProxy`, working index, priority over mapped active hits); pending deletes keep the full active object visible until Apply, drawn faded/desaturated with world depth and a subtle dusty-red outline (not a second entity); pending-deleted objects stay non-pickable; pending-delete visual precedence beats cyan pending Add/Modify and collected authored gold wire; Platform Add follows the Jolt body leftover (58) rather than a 16-object design cap; Checkpoint/Hazard/Collectible Add disable only at the v1 256-line file guard; M39/M40 unchanged. No live Edit menu in Debug. No editor in Release. Object Palette is Milestone 42, not part of M41.

### Phase B — live integration
After Phase A approval:
- add Development Edit menu;
- wire Add Platform/Checkpoint/Hazard/Collectible;
- wire Duplicate/Delete;
- integrate Hierarchy/Inspector;
- integrate pending visualization where appropriate;
- safely reconcile selection and gizmo state;
- preserve active-world picking authority;
- preserve Apply/Revert/Save semantics;
- verify M39/M40 compatibility;
- update tests/docs.

No commit/push/merge until Phase C approval.

### Phase C — manual acceptance
Primary scenario:
1. Start Development.
2. Add Platform.
3. Verify pending authoring state.
4. Edit position/size.
5. Apply Preview.
6. Verify active platform/collision.
7. Save Level Source.
8. Cook, Stage & Reload.
9. Verify it survives staged reload without build/restart.
10. Duplicate it and verify deterministic offset/selection.
11. Apply/Save/Cook-Stage-Reload.
12. Delete the duplicate.
13. Apply/Save/Cook-Stage-Reload.
14. Verify final level.

Also exercise lifecycle operations for Checkpoint, Hazard, and Collectible.

Validate Revert, Modified/Dirty semantics, selection safety, no stale gizmo, checkpoint ordering/state, collectible count/reset, hazard respawn, M39 manual Reload, M40 workflow, F2/editor visibility, unrelated-CWD behavior, and no CWD `imgui.ini`.

### Automated tests
Add focused coverage for:
- Add/Duplicate/Delete supported categories;
- rejection of unsupported singleton lifecycle;
- deterministic insertion/order and duplicate offset;
- selection after lifecycle operations;
- Revert after Add/Delete;
- variable-count parser round trip;
- deterministic writer;
- invalid/excessive counts;
- physics rebuild after platform count changes;
- checkpoint/hazard/collectible state resizing;
- reload with changed counts;
- M40 workflow regression.

Retain existing editor, physics, cooker, staging, reload, and tool-runner regressions.

### Documentation
Update as appropriate:
- `README.md`
- `docs/ARCHITECTURE.md`
- `docs/MILESTONES.md`
- `AGENTS.md`

Document supported categories, workingCopy-only lifecycle edits, Apply/Revert behavior, selection invalidation, ordering, v1 compatibility, runtime resizing, M39/M40 compatibility, and configuration gating.

### Out of scope
- undo/redo;
- generic scene graph/ECS/reflection;
- prefabs/GUID system unless strictly proven necessary;
- drag placement/surface or grid snapping;
- explicit placement plane / multi-lane snapping (spawn.z is the current v1 proxy);
- rotation gizmo;
- multi-select;
- clipboard copy/paste;
- grouping/parenting/rename;
- arbitrary object types;
- lifecycle for slopes/moving platform/dynamic body/Spawn/Ground/Camera/Goal;
- Level Format v2;
- multiple levels/scenes;
- general hot reload/file watching;
- mobile/Web deployment;
- packaging;
- M42;
- Object Palette / Object Browser (future editor UX candidate; not required for M41).

### Completion
M41 completes only after Phases A/B/C are approved, variable-count supported objects survive Apply/Save/Cook/Stage/Reload safely, regressions pass, `git diff --check` is clean, the branch is committed/pushed/merged through the normal workflow, `main` is clean/synchronized, and M42 has not started.
