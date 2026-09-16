## Milestone 40 --- Cook, Stage & Reload Workflow

**Branch:** `milestone/40-cook-stage-reload`\
**Recommended Cursor model:** Grok 4.6 High --- Fast OFF\
**Status:** Phase B implemented (live Development `Build > Cook, Stage & Reload`). Awaiting Phase C. Milestone 40 is not complete. Milestone 41 has not started.

### Goal

Add a Development-only convenience workflow composing the proven
M37--M39 systems:

`Cook Assets → Stage Runtime Assets → Reload Runtime Level`

It reduces repetitive authoring steps without replacing the individual
canonical commands or weakening their ownership boundaries.

### Intended loop

Current:
`Edit → Apply Preview → Save Level Source → Cook & Stage → Reload Runtime Level → Test`

M40:
`Edit → Apply Preview → Save Level Source → Cook, Stage & Reload → Test`

No automatic Apply or Save.

### Preferred Development Build menu

``` text
Build
├ Cook Assets
├ Stage Runtime Assets
├ Cook & Stage
├ Cook, Stage & Reload
├ ----------------
├ Build Debug
├ Build Development
├ Build Release
├ Build All
├ ----------------
└ Tool Output
```

`Level > Reload Runtime Level` remains available and unchanged.

### Architecture

This is cross-boundary orchestration, not a monolithic external tool
job.

1.  UI requests the convenience workflow.
2.  Existing EditorToolRunner performs canonical Cook & Stage.
3.  Application/editor orchestration observes successful terminal
    completion.
4.  Only then it invokes the existing M39 in-process staged-level
    reload.
5.  Any Cook/Stage failure stops the chain.

Do not put reload in HostProcess, Python, CMake, or the cooker. Do not
duplicate cooking, staging, parsing, physics rebuild, or reconciliation.

### Preconditions

Unavailable/rejected when: - authoring unavailable; - Modified ==
true; - EditorToolRunner already running; - another convenience sequence
is pending.

Dirty alone does not block.

### Sequence

Conceptually:

``` text
Cook, Stage & Reload
  Step 1/3 — Cook Assets
  Step 2/3 — Stage Runtime Assets
  Step 3/3 — Reload Runtime Level
```

If EditorToolRunner only models external-process steps, do not force the
in-process reload into that abstraction. Phase A must design the
smallest clean orchestration state bridging asynchronous Cook & Stage to
synchronous semantic Reload.

### Failures

Cook failure: no Stage, no Reload.\
Stage failure: no Reload.\
Reload failure after successful Cook & Stage: cooked/staged files
remain; active world remains intact per M39; report failure; no asset
rollback.\
No automatic retry.

### Shutdown

Preserve M37/M38 process cleanup. If shutdown begins during Cook/Stage,
no later Reload may execute.

### Status

Cook/Stage output remains in Tool Output. Reload must not create a fake
external-process log. The UI should clearly expose running step and
final success/failure without a new logging framework.

### Existing commands remain canonical

Unchanged: - Cook Assets - Stage Runtime Assets - Cook & Stage - Build
Debug - Build Development - Build Release - Build All - Level \> Reload
Runtime Level

### Phase A

Audit before live UI: - EditorToolRunner sequence lifecycle and terminal
result polling; - LevelEditor/DebugUi/Application tool-request
ownership; - M39 reload request/guards; - clean owner for pending
Cook→Stage→Reload state; - exact state machine and terminal states; -
F2/editor visibility interaction; - shutdown behavior; - status
presentation; - testable orchestration helpers; - all M37--M39
regressions.

Do not add the live menu item in Phase A.

**Phase A outcome:** `CookStageReloadWorkflow` is the testable state machine (`Idle` / `WaitingForCookAndStage` / `ReloadPending`). Application owns it, observes the single runner Poll, and invokes M39 `ReloadRuntimeLevelFromStaged` exactly once after canonical `CookAndStage` Succeeded. `CanStartCookStageReload` / pending-workflow Reload guard are in `EditorWorkspace`. No live Build menu item. F2/window visibility does not own the workflow. Shutdown Cancels before runner cleanup.

### Phase B

After approval: - add Development-only menu action; - start canonical
Cook & Stage; - trigger M39 Reload only after successful completion; -
stop on failures; - preserve guards/boundaries; - integrate status/Tool
Output; - add regressions.

**Phase B outcome:** Development Build menu includes `Cook, Stage & Reload` after `Cook & Stage`. The item emits `LevelEditorRequest::CookStageAndReload` only. Application starts canonical `CookAndStage`, observes Poll snapshots, and invokes M39 reload exactly once. Debug has no Build menu. Awaiting Phase C. Milestone 40 is **not** complete. Milestone 41 has not started.

### Phase C

Primary acceptance: 1. baseline FOV 40; 2. change 40 → 55; 3. Apply
Preview; 4. Save Level Source; 5. choose Cook, Stage & Reload; 6. no
separate Cook & Stage; 7. no separate Reload; 8. no Build Development;
9. no restart.

Expected: Cook succeeds → Stage succeeds → Reload succeeds → loaded FOV
55 in the same Development process.

Restore 55 → 40 through the same workflow.

Also validate Modified guard, Dirty allowed, running-job guard, failure
stopping, individual M38/M39 commands, coherent status, F2 visibility,
and safe shutdown.

### Documentation

Update README, ARCHITECTURE, MILESTONES, and AGENTS only where
appropriate. Document this as convenience orchestration over canonical
commands.

### Out of scope

No automatic Apply, automatic Save, file watching, source-change
automation, general hot reload, texture/model/shader reload, restart,
C++/DLL hot reload, stale-file mirroring/deletion, parallel jobs,
arbitrary shell commands, undo/redo, Add/Delete/Duplicate, rotation
gizmo, Level Format v2, multiple levels/scenes, mobile/Web deployment,
packaging, or M41 work.

### Completion

M40 completes only after Phase A, Phase B, Phase C, final FOV 40 across
authored/cooked/staged/loaded state, M37--M39 regression validation,
documentation, and normal Git closure: test → approve → commit → push
branch → normal merge to main → push main → confirm clean/synchronized
main. M41 must not start before closure.
