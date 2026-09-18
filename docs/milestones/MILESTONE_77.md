# Milestone 77 --- Viewport Grid & Spatial Authoring Guides

**Status:** implemented, awaiting manual acceptance\
**Branch:** `milestone/77-viewport-grid-spatial-guides`\
**Recommended Cursor model:** Grok 4.6 High --- Fast OFF

## 1. Goal

Improve spatial readability and Level-building speed in the Development
editor by adding a lightweight world-space viewport grid and origin/axis
guides that complement M76 transform snapping.

M76 made authored transforms deterministic and easier to quantize. M77
makes those increments visually understandable in the 3D viewport so the
author can judge spacing, alignment, scale, height, and world origin
without relying only on Inspector numbers.

This is an editor-authoring productivity milestone. It must not change
runtime gameplay, Level Format v1, canonical Level semantics, or the
authority model.

## 2. World grid

Render a Development-editor-only world-space grid on the horizontal XZ
plane.

Required behavior:

-   grid lies on the world XZ plane at Y = 0;
-   centered around world origin;
-   finite, bounded rendering extent;
-   minor lines and visually distinct major lines;
-   visually distinct world X and Z origin axes;
-   no gameplay/Release rendering;
-   no collision, picking, authored object, LevelDefinition entry, Jolt
    body, or runtime gameplay semantics.

The grid is a viewport aid only.

## 3. Relationship to M76 snapping

The grid must complement, not replace, M76 snapping.

Required defaults:

-   minor grid spacing follows the current M76 Translate snap increment;
-   major-line cadence is a small deterministic multiple of minor
    spacing (prefer 4 or 5 after inspecting visual readability and
    current conventions);
-   when Translate snap increment changes, the grid spacing updates
    accordingly.

The grid must remain usable when Snap itself is disabled. The authored
Translate increment remains the grid-spacing source so authors can
visually plan on the same increment that Ctrl/inversion can temporarily
activate.

Do not couple grid visibility to Snap enabled/disabled.

Do not change M76 quantization semantics.

## 4. Adaptive density / readability

Prevent the grid from becoming unusably dense or visually noisy as the
camera moves.

Use the narrowest deterministic approach appropriate to the current
renderer/editor.

Required:

-   avoid drawing an excessive number of lines;
-   preserve stable world alignment;
-   no visible grid swimming caused by camera-relative repositioning;
-   no per-frame allocations proportional to an unbounded world;
-   reasonable near/far readability;
-   grid must not obscure authored geometry or selection gizmos.

If adaptive visual density is necessary, it may skip/fade minor lines at
distance while preserving the same underlying world alignment. Do not
silently change authored snapping increments.

## 5. Origin and axes

Provide clear but unobtrusive spatial orientation:

-   world origin must be visually identifiable;
-   X-axis and Z-axis must be distinguishable from ordinary grid lines;
-   axis rendering is editor-only and non-interactive.

Follow existing project rendering conventions. Do not introduce a
generalized debug-draw framework solely for M77.

## 6. Grid controls

Add compact controls to the existing Development editor near the
transform/snap controls or the narrowest appropriate viewport/tooling
location.

Required:

-   Grid visible toggle;
-   clear display of effective minor spacing;
-   optional compact major-line cadence control only if it can be added
    without unnecessary complexity.

Default:

-   Grid visible = true.

Do not duplicate the Translate snap increment editor. The M76 Translate
increment remains authoritative for minor grid spacing.

## 7. Persistence

Persist M77 viewport-grid preferences using the existing editor
layout/preferences mechanism.

Persist at minimum:

-   Grid visible.

If M77 introduces a configurable major-line cadence, persist it as well.

Older layouts without M77 fields must load safe defaults.

Invalid persisted values must safely fall back/clamp.

Do not create a new settings file or layout format version.

## 8. Camera and viewport behavior

The grid must coexist with all current editor camera/navigation
behavior.

Preserve:

-   existing orbit/pan/dolly/navigation controls;
-   existing M76 Ctrl snap inversion;
-   existing Shift camera/nudge behavior;
-   existing Alt behavior;
-   picking;
-   gizmo manipulation;
-   selection;
-   F2 Development overlay flow.

The grid must never consume gameplay/editor input.

## 9. Rendering authority and portability

Use existing rendering abstractions/conventions where practical.

Requirements:

-   Development editor only;
-   no dependency on editor rendering in Release;
-   no source-asset dependency;
-   no new texture/model asset required merely to draw the grid;
-   portable across the project's supported raylib/OpenGL-oriented
    targets;
-   avoid platform-specific APIs.

Prefer procedural line rendering or the narrowest existing primitive
path.

## 10. Depth / occlusion semantics

The grid must read as a ground-plane authoring aid rather than a screen
overlay.

It should participate in viewport depth in the narrowest way supported
by the existing renderer so geometry above/in front of it remains
readable.

Avoid z-fighting with authored ground geometry at Y = 0. Use a small
rendering-only strategy if necessary, without changing authored
coordinates or collision.

Document the chosen strategy.

## 11. Selection and gizmo priority

Selection outlines, handles, and transform gizmos remain visually and
semantically more important than the grid.

The grid must not:

-   become pickable;
-   interfere with ray casts;
-   change object selection;
-   hide handles;
-   alter gizmo hit testing;
-   alter M76 snapping.

## 12. Future authoring readiness

Keep grid-spacing derivation and any world-grid line
generation/calculation narrow and testable independently of ImGui where
practical.

This supports future automated preview/authoring inspection without
building agent infrastructure now.

Do NOT add:

-   CLI;
-   Agent API;
-   MCP;
-   scripting;
-   generic Authoring Operations framework;
-   generic debug visualization framework.

## 13. Canonical Level safety

M77 is editor tooling.

Do not intentionally modify:

-   `game/assets/source/levels/level_01.level`
-   `game/assets/source/levels/level_02.level`

The grid is not authored Level data.

Do not normalize EOLs.

Before completion:

``` text
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
```

must be semantically empty.

## 14. Regression coverage

Add/extend narrow automated tests proving at minimum:

1.  default Grid visibility is enabled;
2.  effective minor spacing derives from the M76 Translate increment;
3.  changing Translate increment changes effective grid spacing without
    changing authored data;
4.  Snap enabled/disabled does not control Grid visibility;
5.  invalid spacing inputs inherited from layout/preferences remain safe
    through M76 validation;
6.  world-grid line positions remain aligned to deterministic world
    multiples;
7.  origin axes are generated/identified separately from ordinary lines;
8.  bounded rendering does not generate an unbounded line count;
9.  layout round-trip preserves Grid visibility;
10. older layout without M77 fields loads defaults;
11. Grid does not enter `LevelDefinition`/workingCopy/Dirty semantics;
12. Grid does not affect picking/gizmo hit testing;
13. M76 snapping tests remain green;
14. normal gameplay/runtime behavior is unchanged;
15. Release remains free of editor-only behavior;
16. canonical Levels remain unchanged.

Prefer pure tests for grid-spacing/line-generation math plus the
narrowest existing editor/render integration coverage.

## 15. Validation

Run directly affected current C++ tests, especially those covering:

-   EditorGizmo / M76 snapping;
-   EditorWorkspace;
-   EditorQuickToolbar;
-   editor layout persistence;
-   EditorPicking;
-   editor orientation/render helpers;
-   authored lifecycle;
-   Level file behavior.

Run newer relevant tests discovered during repository inspection.

Run:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
```

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Then:

``` text
git diff --check
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
```

## 16. Manual acceptance

Manual Development acceptance must verify:

-   Grid visible by default;
-   world origin and X/Z orientation are understandable;
-   grid visually aligns with objects placed using M76 Translate snap;
-   changing Translate increment updates grid spacing coherently;
-   disabling Snap does not hide Grid;
-   Grid toggle works;
-   preference survives restart according to current layout semantics;
-   camera navigation remains unchanged;
-   picking remains correct;
-   Translate/Resize/Scale/Rotate gizmos remain correct;
-   Ctrl M76 inversion remains correct;
-   grid does not obscure gizmos/selection;
-   ground/platform geometry remains readable without severe z-fighting;
-   F2 roundtrip remains correct;
-   gameplay is unchanged;
-   Release build behaves normally;
-   canonical Levels have no semantic changes.

Automated green is not sufficient.

## 17. Out of scope

Do not implement:

-   object-to-object snapping;
-   surface snapping;
-   vertex snapping;
-   smart guides/alignment;
-   rulers/measurement tools;
-   multi-selection;
-   group transforms;
-   Undo/Redo;
-   drag-and-drop viewport placement;
-   arbitrary grid planes;
-   local/object-aligned grids;
-   authored grid objects;
-   CLI;
-   Agent API;
-   MCP;
-   scripting;
-   generic debug-draw framework;
-   new gameplay mechanics;
-   new object types;
-   enemies;
-   Level Format v2;
-   M78 functionality.

## 18. Documentation

Canonical active document:

`docs/milestones/MILESTONE_77.md`

Preserve:

-   M75 --- UI Audio & Menu Feedback --- CLOSED
-   M76 --- Transform Snapping & Authoring Productivity --- CLOSED

Keep `docs/MILESTONES.md` compact.

Update architecture/README/AGENTS only where actual implementation
requires it.

After implementation M77 is **implemented, awaiting manual acceptance**,
not CLOSED.

## 18.1 Implementation notes

-   Major-line cadence is **4**. Default Translate `0.25` therefore places
    major lines on 1.0 world units, matching the historical raylib
    `DrawGrid` spacing and remaining the narrower of the 4-or-5 choice.
    That legacy `DrawGrid` is no longer drawn in Gameplay or Release.
-   Render-only Y offset is **+0.008**. Canonical Ground tops at Y = 0
    (`ground 0 -0.25 0 ... 0.5 ...`). Authored coordinates, physics, and
    picking stay at Y = 0.
-   Grid visibility is persisted as `Platformer3D.ViewportGrid` / `Settings`
    / `Visible` inside existing `editor_layout.ini`. Major cadence is not a
    user setting.
-   Spacing derivation and line generation live in
    `editor/EditorViewportGrid.h` and are callable without ImGui.
-   The viewport grid is a Development editor authoring aid only. Normal
    Gameplay and Release render no grid.

## 19. Cursor report and STOP

Report:

1.  files changed;
2.  existing editor viewport/render architecture discovered;
3.  grid rendering design;
4.  effective minor spacing derivation from M76;
5.  major-line cadence;
6.  origin/X/Z axis presentation;
7.  bounded/adaptive density behavior;
8.  depth/z-fighting strategy;
9.  Grid controls;
10. persistence/backward compatibility;
11. input/camera/picking/gizmo preservation;
12. M76 snapping preservation;
13. editor-only/Release boundary;
14. future automation-readiness boundary;
15. C++ test results;
16. Python test results;
17. Debug/Development/Release build results;
18. `git diff --check`;
19. canonical Level diff confirmation;
20. explicit confirmation that no smart/object/surface/vertex snapping,
    Undo, multi-select, CLI/Agent API/MCP, generic framework, new
    gameplay, or M78 scope was added.

Then STOP.

Do NOT commit.\
Do NOT push.\
Do NOT merge.\
Do NOT start M78.\
Do NOT mark M77 CLOSED.
