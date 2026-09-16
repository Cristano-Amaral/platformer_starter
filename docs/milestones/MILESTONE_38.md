## Milestone 38 --- Runtime Asset Staging & Cook-and-Stage Workflow

**Branch:** `milestone/38-runtime-asset-staging`\
**Recommended Cursor model:** Grok 4.6 High --- Fast OFF\
**Status:** Phase B implemented (live Development Build menu). Awaiting Phase C. Milestone 38 is not complete. Milestone 39 has not started.

### Goal

Make the Development editor's asset workflow explicit and convenient by
separating **cooking** from **runtime staging**, while preserving the
canonical CLI/CMake pipeline established through M37.

M38 addresses a UX finding from M37 Phase C:

-   `Save Level Source` writes authored data to `game/assets/source/`.
-   `Cook Assets` updates `game/assets/cooked/`.
-   the running/built game consumes staged assets from
    `build/windows-vs2022/bin/<Config>/assets/`.
-   today, refreshing those staged assets is coupled to the existing
    CMake build/POST_BUILD path.
-   therefore a pure asset edit can require invoking `Build Development`
    even when no C++ rebuild is needed.

M38 will introduce an explicit, deterministic staging operation and a
convenience `Cook & Stage` workflow for Development without pretending
that asset staging is a C++ build.

### Core principles

1.  Source, cooked, and staged assets remain distinct.
2.  The existing cooker remains canonical.
3.  CMake build presets remain canonical for C++ builds.
4.  Staging must not reimplement asset cooking.
5.  Staging must not compile or link C++.
6.  `Cook & Stage` is an explicit user-requested sequence, not an
    automatic reaction to Save/Apply.
7.  No hot reload or automatic game restart.
8.  No arbitrary shell/terminal UI.
9.  Repository-root and unrelated-CWD safety remain mandatory.
10. Tool execution remains Development-only.
11. One active external tool job at a time.
12. Existing M37 Tool Output remains the common status/log surface.
13. No gameplay or LevelDefinition state may be modified by staging.

### Intended Development menu

``` text
Build
├ Cook Assets
├ Stage Runtime Assets
├ Cook & Stage
├ ----------------
├ Build Debug
├ Build Development
├ Build Release
├ Build All
├ ----------------
└ Tool Output
```

Debug must continue to have no external-tool Build menu. Release must
continue to have no editor/tool UI.

### Phase A --- Audit and architecture

Before implementing live UI behavior:

-   Audit the existing CMake POST_BUILD asset staging commands exactly.
-   Identify every cooked runtime file/directory staged for
    `Platformer3D`.
-   Identify destination directories for Debug, Development, and
    Release.
-   Determine whether staging should reuse a CMake script/helper or
    introduce a small project-owned staging tool.
-   Prefer one canonical staging implementation that both the editor and
    CMake can invoke, if this can be achieved without unnecessary
    build-system complexity.
-   Avoid maintaining two independent lists of runtime assets.
-   Define Development staging semantics precisely.
-   Define `Cook & Stage` as a sequential runner job:
    1.  Cook Assets.
    2.  If cook succeeds, Stage Runtime Assets.
    3.  If cook fails, do not stage.
-   Define success/failure and exit-code behavior.
-   Confirm staging uses `copy_if_different` or equivalent incremental
    behavior.
-   Confirm stale/removed cooked runtime files are handled deliberately;
    do not silently invent deletion semantics.
-   Preserve repository-root safety and structured process invocation.
-   Add focused tests for command/job construction and sequence
    behavior.
-   Do not add the live menu items during Phase A.

### Phase B --- Live Development integration

Add:

-   `Build > Stage Runtime Assets`
-   `Build > Cook & Stage`

Use the existing M37 `EditorToolRunner` and Tool Output.

#### Stage Runtime Assets

The action must:

-   stage the canonical cooked runtime assets into the Development
    runtime asset directory;
-   not invoke a C++ compile/link;
-   not invoke the cooker;
-   not Apply, Revert, or Save;
-   not restart the game;
-   work from unrelated process CWD;
-   report useful output and exit status through Tool Output;
-   be disabled while another job is running.

The expected Development destination is based on the audited build
layout, currently:

`build/windows-vs2022/bin/Development/assets/`

but Phase A must verify rather than blindly hard-code assumptions.

#### Cook & Stage

The action must:

1.  run the canonical cooker;
2.  stop immediately if cooking fails;
3.  stage runtime assets if cooking succeeds;
4.  report both steps clearly in Tool Output;
5.  finish Succeeded only if both steps succeed.

Suggested log markers:

``` text
=== Cook & Stage: Step 1/2 - Cook Assets ===
=== Cook & Stage: Step 2/2 - Stage Runtime Assets ===
```

It must not build C++.

**Phase B outcome:** Development Build menu order is Cook Assets, Stage Runtime Assets, Cook & Stage, separator, C++ builds, separator, Tool Output. Both new items call `TryStart` (sequence owned by the runner). Tool Output uses generic `sequenceStep*` fields and `{displayLabel}: n/N label` so Cook & Stage is not shown as Build All. Debug still has no Build menu. No hot reload, restart, Save/Apply, or stale-file deletion.

### Tool Output

Reuse the M37 window and state.

For staging/sequences, display:

-   job label;
-   Running / Succeeded / Failed;
-   elapsed time;
-   exit code where applicable;
-   current sequence step;
-   bounded incremental log.

Do not add a second output system.

### Runtime behavior

M38 does **not** add hot reload.

If Development is already running and its runtime asset files can safely
be replaced, staging may update files on disk, but the currently loaded
level is not required to change in memory.

The validated workflow for authored level changes becomes:

``` text
Apply Preview
→ Save Level Source
→ Cook & Stage
→ Restart Development
```

or explicitly:

``` text
Apply Preview
→ Save Level Source
→ Cook Assets
→ Stage Runtime Assets
→ Restart Development
```

No C++ build should be necessary solely to propagate level-data changes.

### CMake integration

A key architectural goal is to avoid divergence between:

-   editor-triggered staging; and
-   build-triggered POST_BUILD staging.

Phase A must determine the smallest maintainable design.

Preferred outcome: one reusable staging definition/helper invoked by
both paths.

Do not make CMake builds dependent on the editor.

Existing Debug/Development/Release builds must continue to stage assets
as they do today unless the shared implementation deliberately replaces
the internals with equivalent behavior.

### Tests

Automated coverage should include, where practical:

-   staging command/path construction;
-   repository-root validation;
-   Development destination resolution;
-   unrelated-CWD behavior;
-   paths containing spaces;
-   successful staging;
-   staging failure;
-   incremental `copy_if_different` behavior;
-   `Cook & Stage` success sequence;
-   cook failure prevents staging;
-   staging failure makes `Cook & Stage` fail;
-   one-active-job guard;
-   bounded log remains intact;
-   existing M37 Build All behavior remains unchanged;
-   existing editor/workspace/gizmo/orientation/picking/level/physics
    tests remain green;
-   cooker regression tests remain green.

Avoid tests that repeatedly rebuild the whole project when a focused
temporary-directory staging test is sufficient.

### Manual Phase C acceptance

Manually validate at minimum:

-   Development shows the two new menu actions.
-   Debug still has no Build menu.
-   Release still has no tool UI.
-   `Stage Runtime Assets` does not build C++.
-   `Cook & Stage` clearly executes two sequential steps.
-   Tool Output remains responsive.
-   one-job-at-a-time disabling works.
-   close/reopen Tool Output does not affect the job.
-   unrelated-CWD launch still works.
-   a controlled FOV authoring test propagates with:
    `Apply Preview → Save → Cook & Stage → Restart`.
-   the Development executable timestamp does not need to change for
    asset-only edits.
-   no automatic Save/Apply/restart occurs.
-   M35/M36/M37 regressions remain healthy.

### Documentation

Update as appropriate:

-   `README.md`
-   `docs/ARCHITECTURE.md`
-   `docs/MILESTONES.md`
-   `AGENTS.md` only if a durable agent rule is needed.

Document the final canonical developer workflow and the distinction
between source, cooked, staged, and loaded runtime state.

### Explicitly out of scope

M38 must not add:

-   automatic Save → Cook;
-   automatic Apply → Cook;
-   automatic cook on file change;
-   file watching;
-   hot reload;
-   automatic game restart;
-   launcher/self-restart architecture;
-   arbitrary shell input;
-   terminal emulator;
-   interactive stdin;
-   parallel tool jobs;
-   remote builds;
-   Linux staging backend beyond portability-friendly structure;
-   Android/iOS/Web deployment;
-   packaging;
-   Add/Delete/Duplicate;
-   Level Format v2;
-   rotation gizmo;
-   undo/redo;
-   multiple levels/scenes.

### Completion condition

M38 is complete only after:

1.  Phase A audit/architecture is approved.
2.  Phase B implementation and automated validation are approved.
3.  Phase C manual validation passes.
4.  Documentation is accurate.
5.  No unintended authored-level changes remain.
6.  Git closure is performed through the normal project flow: test →
    approve → commit → push milestone branch → merge to main → push main
    → confirm clean/synchronized main.
7.  No M39 work begins before M38 is formally closed.
