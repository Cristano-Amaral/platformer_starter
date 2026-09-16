## Milestone 37 --- Editor Tool Runner: Asset Cooker & Build Integration

### Status

Implementation, Phase C, and Git closure are complete. Milestone 37 is
merged. Milestone 38 has started on `milestone/38-runtime-asset-staging`.

### Branch

`milestone/37-editor-tool-runner`

### Recommended Cursor model

**Grok 4.6 High --- Fast OFF**

------------------------------------------------------------------------

### 1. Goal

Add a Development-only editor tool runner that can launch the project's
existing asset cooker and CMake build commands as **external child
processes**, while keeping all build/cooker logic outside gameplay and
preserving the existing command-line workflow as canonical.

M37 adds the infrastructure and UI needed to run:

-   `Cook Assets`
-   `Build Debug`
-   `Build Development`
-   `Build Release`
-   `Build All`

from the editor.

The editor does **not** reimplement CMake, Python, the cooker, or build
logic. It only launches the already-established project commands,
captures their output, reports status, and prevents unsafe concurrent
execution.

------------------------------------------------------------------------

### 2. Why M37 exists

M36 established a stable editor menu/workspace shell. M37 is the next
tooling layer: frequently used project commands can be invoked from the
Development editor without replacing the command-line workflow.

This milestone deliberately separates:

1.  editor UI;
2.  process execution;
3.  project command definitions;
4.  gameplay/runtime systems.

The resulting process-runner boundary should remain portable enough for
a future Linux development host without introducing platform-specific
process APIs into gameplay or generic editor logic.

------------------------------------------------------------------------

### 3. Core principles

#### 3.1 CLI remains canonical

The existing Python/CMake commands remain the source of truth.

The editor must launch them; it must not reproduce their behavior in
C++.

#### 3.2 External process boundary

Tool execution must live behind an editor/tooling process abstraction.

Windows-specific process APIs, if required, must be isolated in a
platform implementation.

#### 3.3 Development-only execution

Actual Cooker/build launching from the editor is **Development-only**
for M37.

Debug may keep the M36 editor/menu but must not launch project tool
processes.

Release remains editor-free.

#### 3.4 One active tool job

M37 supports at most one editor tool process/job at a time.

While a job is running, commands that would start another job are
disabled.

No parallel builds/cooks in M37.

#### 3.5 No self-rebuild assumption

A running Development executable may be locked on Windows.

M37 must not assume that `Build Development` can always replace/relink
the executable currently running the editor.

This condition must be detected/reported cleanly as a build failure or
prevented if the architecture can determine it safely.

Do not invent dangerous self-restart/hot-reload behavior.

------------------------------------------------------------------------

### 4. In scope

#### 4.1 New Build menu

Extend the M36 menu bar with:

`Build`

Required items:

-   `Cook Assets`
-   separator
-   `Build Debug`
-   `Build Development`
-   `Build Release`
-   `Build All`
-   separator
-   `Tool Output`

Exact wording may be adjusted slightly if required by existing UI
conventions, but semantics must remain explicit.

#### 4.2 Tool Output window

Add a Development editor window that displays:

-   current/last command label;
-   state: Idle / Running / Succeeded / Failed;
-   exit code when available;
-   elapsed time;
-   captured stdout/stderr text;
-   auto-scroll behavior suitable for build logs;
-   a clear-log action when not destructive to a running process.

The output window must be recoverable from the editor UI.

#### 4.3 Process execution abstraction

Introduce a small editor/tooling abstraction responsible for:

-   launching a child process;
-   setting a working directory;
-   capturing stdout;
-   capturing stderr;
-   non-blocking status polling from the game/editor loop;
-   obtaining exit code;
-   safe cleanup;
-   preventing leaked handles/process objects.

Do not block the render/game loop waiting for a build.

#### 4.4 Repository-root safety

Tool commands must execute relative to the resolved project/repository
root, not the user's current working directory.

Launching the game from `%TEMP%` must not cause cooker/build commands to
target `%TEMP%`.

Reuse existing authoring/runtime path knowledge where appropriate, but
do not hard-code a developer-specific absolute path.

#### 4.5 Cook Assets

`Cook Assets` must launch the project's existing cooker entry
point/command.

Phase A must inspect the repository and identify the canonical command
before implementation.

The editor must not embed cooker logic.

#### 4.6 Build commands

Use the existing CMake presets:

-   Debug → `windows-debug`
-   Development → `windows-development`
-   Release → `windows-release`

Use the existing configure preset if required by the current workflow:

-   `windows-vs2022`

Do not silently redesign the CMake preset structure.

#### 4.7 Build All semantics

`Build All` means:

1.  Build Debug
2.  Build Development
3.  Build Release

in a deterministic sequence.

It does **not** mean that `Build Release` implicitly builds all
configurations.

If one step fails, preferred M37 behavior is to stop the sequence and
report which step failed.

#### 4.8 Command status

Required conceptual states:

-   Idle
-   Running
-   Succeeded
-   Failed

Optional internal states such as Starting may be used if genuinely
useful.

#### 4.9 UI responsiveness

While a process runs:

-   rendering continues;
-   editor camera remains responsive;
-   panels remain responsive;
-   output updates incrementally;
-   the application must not freeze waiting for process completion.

#### 4.10 Output bounds

Build output can become large.

Implement a reasonable bounded log strategy so a long tool run cannot
grow editor memory without limit.

The exact cap should be documented and deterministic.

------------------------------------------------------------------------

### 5. Configuration policy

#### Development

Full M37 functionality:

-   Build menu visible;
-   Cooker execution enabled;
-   Build execution enabled;
-   Tool Output available.

#### Debug

M36 editor remains available, but external project tool execution is
unavailable in M37.

Preferred UX:

-   Build menu may be absent, or commands clearly disabled.

Choose one consistent policy in Phase A and document it.

Debug must not accidentally become an authoring/build host.

#### Release

No editor, no menu bar, no tool runner, no process execution code path.

------------------------------------------------------------------------

### 6. Process architecture

Preferred conceptual separation:

``` text
Editor menu/UI
    ↓ semantic request
EditorToolRunner
    ↓ command/job description
Platform process layer
    ↓
OS child process
```

Potential names are illustrative, not mandatory:

-   `EditorToolRunner`
-   `EditorToolJob`
-   `EditorToolProcess`
-   `PlatformProcess`
-   `WindowsProcess`

Use the smallest architecture that cleanly isolates OS process details.

Do not introduce a generic engine-wide job system.

------------------------------------------------------------------------

### 7. Command representation

A tool job should conceptually describe:

-   display name;
-   executable;
-   argument list;
-   working directory;
-   optional sequence step metadata.

Avoid building one large shell command string if a direct executable +
arguments API is practical.

Do not pass untrusted level/game data into a shell.

M37 commands are fixed project tooling commands.

------------------------------------------------------------------------

### 8. Windows implementation

The initial implementation targets Windows.

If Win32 APIs are used:

-   isolate them from gameplay;
-   avoid handle leaks;
-   capture stdout/stderr through pipes;
-   avoid blocking reads on the main thread;
-   correctly quote executable/arguments;
-   use Unicode-safe APIs where practical;
-   close process/thread/pipe handles deterministically.

Do not scatter `CreateProcess` calls through editor UI code.

Future Linux support should require replacing/adding a platform process
backend rather than rewriting menu logic.

------------------------------------------------------------------------

### 9. Non-blocking execution

The main editor frame must never perform a blocking wait for tool
completion.

Acceptable designs include:

-   background reader thread plus main-thread polling;
-   carefully implemented non-blocking pipe polling;
-   another minimal architecture justified by the current codebase.

Threading, if used, must have clear ownership and shutdown.

Application exit while a tool is running must be handled deliberately.

M37 must not leave orphaned reader threads or invalid handles.

------------------------------------------------------------------------

### 10. Process termination policy

M37 does not require a user-facing `Cancel Build` feature unless the
implementation makes it trivial and safe.

However, application shutdown with a running tool must be deterministic.

Phase A must choose and document whether shutdown:

-   waits briefly and terminates the child;
-   detaches intentionally;
-   or uses another safe policy.

Do not silently leak a child process.

------------------------------------------------------------------------

### 11. Tool Output window behavior

The Tool Output window is editor/tooling state, not level state.

It must not affect:

-   `LevelDefinition`;
-   workingCopy;
-   active level;
-   Modified;
-   Dirty;
-   BEST;
-   selection;
-   editor camera;
-   physics.

Suggested controls:

-   status line;
-   command/job name;
-   elapsed time;
-   exit code;
-   scrollable log;
-   `Clear` when appropriate.

No terminal emulator is required.

No command prompt text input is required.

Users cannot enter arbitrary shell commands in M37.

------------------------------------------------------------------------

### 12. Workspace integration

Extend M36 workspace state with Tool Output visibility if appropriate.

`View` should expose `Tool Output` if that matches the cleanest M36
pattern.

Alternatively, `Build > Tool Output` may toggle/open it, but there must
be a reliable recovery path after closing the window.

Preferred design:

-   `View > Tool Output`
-   `Build > Tool Output` may additionally focus/open the same window.

One authoritative visibility state only.

------------------------------------------------------------------------

### 13. Build menu enable rules

When no tool is running:

-   Cook Assets: enabled in Development.
-   Build Debug: enabled in Development.
-   Build Development: enabled in Development, subject to the self-build
    policy.
-   Build Release: enabled in Development.
-   Build All: enabled in Development, subject to the self-build policy.

When a tool is running:

-   all job-start commands disabled;
-   Tool Output remains usable;
-   existing editor Level/View/Transform functionality remains usable.

No overlapping tool jobs.

------------------------------------------------------------------------

### 14. Build Development / self-lock handling

Windows may prevent replacing the currently running Development
executable.

Phase A must inspect the actual build output layout and determine the
real risk.

M37 must choose a safe explicit policy.

Acceptable examples:

#### Policy A --- Allow and report

Launch the normal Development build and display the linker/file-lock
failure clearly.

#### Policy B --- Disable unsafe self-build

Disable `Build Development` while running the Development executable if
the build would necessarily overwrite it.

If Build All includes Development, the same policy must be reflected
consistently.

Do not create automatic executable swapping, self-restart, hot reload,
or launcher architecture in M37.

------------------------------------------------------------------------

### 15. Configure policy

Determine whether build commands should:

-   run only `cmake --build --preset ...`, assuming configured build
    tree;
-   or perform a configure step when necessary.

Do not run configure every frame or hide expensive work unnecessarily.

Preferred behavior: preserve the project's current explicit preset
workflow and report missing/not-configured errors clearly unless a small
deterministic preflight is justified.

Phase A must inspect and decide.

------------------------------------------------------------------------

### 16. Cooker behavior

The existing cooker remains authoritative.

M37 must:

-   launch it from repository root;
-   capture its normal output;
-   report exit code;
-   show success/failure;
-   leave source assets untouched except for the normal cooker-defined
    outputs.

No C++ asset conversion implementation.

No new pack format.

------------------------------------------------------------------------

### 17. Build success/failure

A process is successful only if its exit code indicates success.

Do not infer success merely because output contains a word such as
"success".

For Build All:

-   each step has its own label;
-   final status succeeds only if all required steps succeed;
-   failure identifies the failed step.

------------------------------------------------------------------------

### 18. Existing editor regression contract

M37 must preserve all M36 behavior:

-   F1 Metrics;
-   F2 editor;
-   View menu;
-   Transform menu;
-   Level menu;
-   panel close/reopen;
-   Reset Editor Layout;
-   orientation widget;
-   picking;
-   Translate;
-   Resize;
-   nudge;
-   dolly;
-   Development source Save;
-   Debug no source Save;
-   Release editor-free.

Tool running must not hijack these systems.

------------------------------------------------------------------------

### 19. Out of scope

M37 must **not** implement:

-   arbitrary shell/terminal input;
-   terminal emulator;
-   interactive stdin to child processes;
-   Cancel Build unless trivial and explicitly approved during
    implementation;
-   hot reload;
-   executable self-restart;
-   launcher application;
-   automatic game restart after build;
-   automatic source Save before cooker;
-   automatic Apply before cooker;
-   automatic Cooker before every build;
-   automatic Build after every Save;
-   file watching;
-   background continuous build;
-   parallel builds;
-   remote build;
-   Linux process backend unless required for compile hygiene;
-   Android build;
-   iOS build;
-   Web/Emscripten build;
-   deployment;
-   packaging/installer;
-   Add/Delete/Duplicate objects;
-   Level Format v2;
-   rotation gizmo;
-   undo/redo;
-   multiple levels/scenes.

------------------------------------------------------------------------

### 20. Testing strategy

Process launching is testable without launching the full game.

Add focused tests for project-owned logic where practical:

-   job state transitions;
-   only one active job;
-   command construction;
-   repository-root working directory;
-   exit code success/failure;
-   bounded log behavior;
-   Build All sequencing;
-   stop-on-failure sequencing;
-   self-build policy;
-   shutdown/cleanup logic where testable.

A small deterministic helper child process/test command may be used for
process-capture tests if it does not depend on network access.

Do not make unit tests compile the entire project repeatedly.

------------------------------------------------------------------------

### 21. Manual validation

Phase C must validate at least:

1.  Development Build menu visible.
2.  Debug execution policy matches Phase A decision.
3.  Release has no tool UI.
4.  Tool Output opens/closes/restores.
5.  Cook Assets launches.
6.  Cooker output appears incrementally.
7.  Cooker exit code/status correct.
8.  Build Debug launches.
9.  Build Debug output appears.
10. Build Debug success/failure correctly reported.
11. Build Release launches.
12. Build Release output appears.
13. Build Development follows approved self-build policy.
14. Build All follows approved sequence.
15. Build All stops on failure if a step fails.
16. second tool cannot start while one is running.
17. editor remains responsive while tool runs.
18. menu/panels remain usable while tool runs.
19. world picking/gizmos remain unaffected.
20. tool commands work when game was launched from unrelated CWD.
21. command working directory is repository root.
22. no arbitrary shell input exists.
23. no unexpected source level modification.
24. application shutdown with tool state is safe.
25. M36 regressions pass.

------------------------------------------------------------------------

### 22. Phase plan

#### Phase A --- Audit, process abstraction & tests

-   inspect canonical cooker command;
-   inspect CMake presets/build layout;
-   inspect Development executable lock/self-build risk;
-   define Development-only tool policy;
-   design tool job/state model;
-   design process abstraction;
-   define repository-root resolution;
-   define output capture/bounds;
-   define Build All sequence;
-   define shutdown policy;
-   implement/test process layer and pure job logic where appropriate;
-   no live Build menu yet.

#### Phase B --- Live editor integration

-   add Build menu;
-   add Tool Output window;
-   wire Cook Assets;
-   wire Build Debug;
-   wire Build Development according to approved policy;
-   wire Build Release;
-   wire Build All;
-   live incremental output;
-   busy-state disabling;
-   workspace visibility integration;
-   unrelated-CWD safety;
-   builds/tests/smokes.

#### Phase C --- Manual validation

Phase C passed. Live Development Build menu launched the canonical cooker
and CMake presets; Tool Output, one-active-job disabling, Policy A /
LNK1168 when a relink is required, incremental no-op success while the
Development exe is locked, unrelated-CWD root safety, and M36 regressions
were confirmed.

**Source vs cooked vs staged (Phase C finding).** Save Level Source writes
`game/assets/source/`. Cook Assets updates `game/assets/cooked/` only.
Runtime staging is the existing `Platformer3D` POST_BUILD copy into
`build/windows-vs2022/bin/<Config>/assets/`. Cook-only then restarting an
already-built Development exe still loads the previous staged level (FOV
55 vs authored 40 in the Phase C camera test). The current fresh-restart
workflow is Apply Preview → Save → Cook → Build Development → Restart.
Build Development may be a C++ no-op and still stage assets; the exe
timestamp need not change. This separation is intentional in M37.

Milestone 38 is now the dedicated Stage Runtime Assets / Cook & Stage
work. M37 Cook Assets remains cook-only; M37 Build Development remains
`cmake --build --preset windows-development`.

------------------------------------------------------------------------

### 23. Completion criteria

M37 is complete only when:

-   Development editor can launch the existing cooker externally;
-   Development editor can launch supported CMake build presets
    externally;
-   Build All sequences Debug/Development/Release according to approved
    self-build policy;
-   output is captured and visible without freezing the editor;
-   exit status is accurate;
-   concurrent jobs are prevented;
-   repository-root/CWD handling is safe;
-   OS-specific process code is isolated;
-   Debug/Release boundaries are preserved;
-   M36 editor functionality regresses cleanly;
-   automated tests/builds/cooker tests pass;
-   manual Phase C passes;
-   canonical level source remains clean;
-   no M38 work has started.

**Phase C result:** the criteria above are satisfied. Git closure
(commit/push/merge) has not been requested. The authored Level 01 camera
FOV Y is intentionally 40 (not a cooker accident).

------------------------------------------------------------------------

### 24. Deferred backlog

Future milestones may consider:

-   Add/Delete/Duplicate authored objects;
-   dynamic authored collections / Level Format v2;
-   rotation gizmo;
-   undo/redo;
-   multiple levels/scenes;
-   richer build profiles;
-   Linux editor tool process backend;
-   Android/Web build targets;
-   packaging/deployment;
-   optional cancel/restart workflows;
-   explicit Stage Runtime Assets or Cook & Stage (now Milestone 38).

These are not M37.
