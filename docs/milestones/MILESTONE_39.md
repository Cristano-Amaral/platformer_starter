## Milestone 39 --- Development Runtime Level Reload

**Branch:** `milestone/39-runtime-level-reload`\
**Recommended Cursor model:** Grok 4.6 High --- Fast OFF\
**Status:** Phase B implemented (live Development `Level > Reload Runtime Level`). Awaiting Phase C. Milestone 39 is not complete. Milestone 40 has not started.

### Goal

Add an explicit Development-only runtime level reload so staged Level
Format v1 data can be tested without closing and reopening the game.

M38 established `source → cooked → staged runtime assets`. M39 adds the
next explicit step:

`Apply Preview → Save Level Source → Cook & Stage → Reload Runtime Level`

Reload reads the staged Development runtime level through the normal
runtime loading path. It must not silently Save, Cook, Stage, Build,
restart the executable, or introduce file watching.

### Core principles

-   Explicit, user-triggered reload.
-   Staged runtime data is the reload authority.
-   Reuse the normal runtime parser/loading and world-rebuild paths.
-   Never load reload data directly from authored source or
    `game/assets/cooked/`.
-   No automatic Save/Cook/Stage/Build.
-   No file watching, executable restart, or general-purpose hot reload.
-   Failed reload must not leave a partially replaced world.
-   Preserve Development/Debug/Release boundaries.
-   Preserve persistent BEST unless the audited architecture gives a
    compelling reason otherwise.

### Intended Development workflow

``` text
Edit level
→ Apply Preview
→ Save Level Source
→ Cook & Stage
→ Reload Runtime Level
→ test immediately
```

Reload is independent: if the user does not cook/stage first, it reloads
the currently staged level.

### Preferred menu

``` text
Level
├ Apply Preview
├ Revert Working Copy
├ Save Level Source
├ ----------------
├ Reload Runtime Level
├ ----------------
└ Reset Editor Layout
```

Phase A must verify that Level is the correct semantic home. Reload is
an in-process editor/runtime action, not an external Build tool.

### Phase A

Audit before live UI: - startup level loading end-to-end; - exact staged
Development level path; - LevelDefinition ownership and active-world
rebuild; - Apply Preview rebuild path and reusable pieces; -
physics/static world reconstruction; - player/run/session reset; -
checkpoints, hazards, collectibles, goal and timer; - moving/dynamic
runtime objects; - camera authored state; - editor
workingCopy/active/savedSourceBaseline, Modified/Dirty; - selection and
editor camera; - persistent BEST; - transactional reload/failure
behavior.

Define the smallest safe reload architecture and focused tests. Do not
add the live menu item in Phase A.

**Phase A outcome:** Reload core is `PrepareRuntimeLevelReload` (absolute
staged path only) plus `PhysicsWorld::TryRebuild` (replacement world,
swap on success; Jolt Factory refcount so two worlds can exist briefly).
Policy A rejects reload while Modified. Development-only
(`CanReloadRuntimeLevel` / authoring flag). Apply Preview now uses
TryRebuild so physics failure is no longer fatal. No live Level menu
item. No EditorToolKind.

### Phase B --- Live editor integration

**Phase B outcome:** Development `Level > Reload Runtime Level` sits after Save Level Source and before Reset Editor Layout. The item emits `LevelEditorRequest::ReloadRuntimeLevel` only; Application owns `ReloadRuntimeLevelFromStaged`. Enable is `CanReloadRuntimeLevel(authoringAvailable, modified, toolRunner.IsRunning())` (Policy A: also disabled while any Build tool is Running). Debug omits the item. Release has no editor. One Level-action status/message in the Level Editor panel. Dirty does not block. No Cook/Stage/Build/restart/hot-reload chain. Awaiting Phase C. Milestone 39 is **not** complete. Milestone 40 has not started.

### Runtime source of truth

Reload must read the staged runtime level, conceptually:

`build/windows-vs2022/bin/Development/assets/levels/level_01.level`

The exact path must come from existing runtime-path mechanisms and be
verified.

Do not reload from: - `game/assets/source/levels/level_01.level` -
`game/assets/cooked/levels/level_01.level`

### Transactional safety

Preferred design: 1. Parse/validate staged level into a temporary
candidate. 2. On parse/validation failure, preserve the current active
world. 3. Commit/rebuild only after validation succeeds. 4. If later
reconstruction can fail, define the smallest safe rollback/commit
boundary supported by the current architecture.

### Successful reload policy

Phase A must audit and explicitly choose semantics. Preferred
direction: - active LevelDefinition becomes staged definition; -
physics/world rebuilds; - player/run state resets deterministically; -
checkpoints/collectibles/hazards/goal/timer reset coherently; -
moving/dynamic objects reset coherently; - authored camera values
update; - workingCopy reconciles to active; - Modified becomes false; -
Dirty remains coherent with saved authored-source baseline; - unsafe
selection is cleared; - editor navigation camera remains editor state
where possible; - Tool Output remains unrelated; - persistent BEST is
preserved.

Do not implement these assumptions blindly; report the exact policy.

### Failure policy

Missing/invalid staged level must: - show a clear Development
error/status; - not crash; - not partially replace the current world; -
not write source/cooked/staged files; - not invoke external tools; - not
erase persistent BEST.

Prefer an existing editor status/error surface rather than a new large
framework.

### Scope

M39 reloads Level Format v1 level data only. It does not add general
GLB, texture, shader, executable, DLL, or C++ hot reload.

M38 commands and semantics remain unchanged. There is no automatic
`Cook & Stage → Reload` chain.

### Tests

Where practical cover: - staged path resolution; - successful candidate
load; - invalid/missing staged level; - active world preserved on
pre-commit failure; - world/physics rebuild; - session reset policy; -
checkpoint/collectible/hazard/goal reset; - working-copy
reconciliation; - Modified/Dirty semantics; - selection policy; - BEST
preservation; - no source/cooked writes; - no external tool
invocation; - M37/M38 runner regressions; - existing
workspace/gizmo/orientation/picking/level/physics tests.

### Manual Phase C

Primary acceptance:

``` text
Camera FOV 40 → 55
Apply Preview
Save Level Source
Cook & Stage
Reload Runtime Level
```

Expected: loaded runtime FOV becomes 55 without Build Development and
without closing/reopening Development.

Then restore final intended FOV 40 through the same explicit workflow.

Also validate reload of old staged data when Cook & Stage is skipped,
safe invalid-level failure, approved session reset behavior,
editor-state reconciliation, BEST preservation, and M35--M38
regressions.

### Documentation

Update README, ARCHITECTURE, MILESTONES, and AGENTS only where
appropriate. Document source vs cooked vs staged vs currently loaded
in-memory state.

### Out of scope

No automatic Save→Cook, Stage→Reload, file watching, background hot
reload, automatic restart, launcher, C++/DLL hot reload, general
texture/model/shader reload, stale-file deletion, arbitrary shell UI,
parallel jobs, Add/Delete/Duplicate, Level Format v2, rotation gizmo,
undo/redo, multiple levels/scenes, mobile/Web deployment, or packaging.

### Completion

M39 completes only after Phase A approval, Phase B approval, Phase C
manual pass, final authored FOV 40, accurate docs, and normal Git
closure: test → approve → commit → push milestone branch → normal merge
to main → push main → confirm clean/synchronized main. M40 must not
start before closure.
