# Milestone 63 --- Level Goal / Exit

## Status

**Planned.** Milestone 62 is CLOSED. Begin only from clean synchronized
`main`.

## Canonical Path

`docs/milestones/MILESTONE_63.md`

## Branch

`milestone/63-level-goal`

## Cursor Model

**Grok 4.6 High --- Fast OFF**

## Goal

Add the first explicit end-of-level objective: a concrete authored
**Level Goal** region. When the player enters any active goal during
Gameplay, the current run becomes **Level Completed** exactly once and
shows simple `Level Complete` feedback.

M63 stops there: no next-level loading, campaign, Continue/Next Level,
menu, generic Objective/Trigger/Event/GameState framework, or M64.

## Context / Authority

Follow the post-M60 contract. Read this active file first, then current
code/tests/architecture/relevant docs. Load older milestone files only
for concrete dependencies.

Authority: current code → tests → current architecture/docs → active
milestone → latest relevant current docs → older history.

## Preserve

Preserve Level Format v1 limits/conventions; editor workingCopy → Apply
→ active authority; Save authored state; runtime state transient; all
existing authored categories; Checkpoint/Hazard/Collectible behavior;
Dynamic Box; Pressure Plate; Door; Item Pickup; Inventory/UI; E priority
Drop → Grab → Collect → Unlock; M58--M62 presentation/feedback;
checkpoint/restart semantics; staged assets; Development/Release
separation.

## Authored Data / Level Format

Add a repeatable concrete Level Goal category with center/position and
size only. Preferred syntax:

``` text
level_goal <cx> <cy> <cz> <sx> <sy> <sz>
```

Use that spelling unless repository inspection reveals a concrete
compatibility reason otherwise; report deviations.

Validate finite numbers and strictly positive/practical minimum
dimensions using comparable authored-volume conventions. Respect v1
file/line/category limits and parser error conventions. Choose valid
Palette defaults based on existing volumes. Writer must be deterministic
and round-trip clean.

Multiple goals are allowed; any one may complete the level. No goal ID,
ordering, target level, required item, objective text, model identity,
rotation, visual settings, receiver/action fields, or Level Format v2.

## Editor Integration

Integrate Level Goal as a normal concrete authored category using
existing patterns: - Palette/Add; - selection and picking; - Inspector
center/position and size; - Translate; - Resize/Scale according to
current volume conventions; - Duplicate/Delete; - Apply/Save/reload; -
Dirty/equality; - editor ghost/wire visualization.

Do not add Rotate if meaningless for an axis-aligned volume. Do not
invent generic editor infrastructure.

Editor workingCopy is pending authored state; Apply validates/promotes
to active; Gameplay uses active goals. Runtime completion never mutates
authored goals or Dirty state. Save never persists completion.

## Gameplay Detection

During active Gameplay, detect overlap between the authoritative player
representation and active Level Goal AABBs/volumes using the narrowest
current overlap facilities.

Prefer data-driven overlap; do not create a Jolt body unless current
architecture provides a compelling established trigger-volume
convention.

Only the player can complete a goal. Dynamic Boxes, pickups and other
objects cannot. No E press is required.

## Completion Authority

Add the narrowest runtime state representing current-level completion.

``` text
Playing
  ↓ player overlaps any goal
Level Completed
```

The transition is idempotent. First valid overlap completes exactly
once; remaining inside or entering another goal cannot retrigger.
Completion does not mutate authored data, load/reload/quit, award items,
or require E.

Do not create a generalized state-machine framework.

## Completion Feedback / Post-Completion Behavior

Show a clear runtime `Level Complete` HUD message/panel in Development
Gameplay and Release without ImGui. It may remain visible while
completed.

Keep the current level loaded. Do not automatically transition. Prefer
preserving existing player/runtime behavior after completion rather than
broadly freezing gameplay; report the chosen behavior.

No Continue/Next Level, results/statistics, stars, score, timer, or
campaign progress.

## Goal Visualization

Provide a simple recognizable goal marker/volume with existing
greybox/immediate rendering. Editor visualization must be easy to
see/select; Gameplay/Release must make the goal identifiable.

No model asset, texture, shader, particle framework or authored visual
fields are required. Restore renderer state.

## Lifecycle

Completion is runtime-only: - fresh run: not completed; - Restart:
reset; - Apply: reset; - level reload: reset; - authored edits never
serialize completion; - checkpoint activation/respawn before completion
remains normal and must not fabricate completion; - PhysicsWorld rebuild
must not fabricate completion.

M63 does not define post-completion checkpoint continuation or
next-level behavior.

## Existing Gameplay Isolation

Goal must not interfere with Checkpoints, Hazards, Collectibles, Dynamic
Box grab/drop, Pressure Plates, Doors, Item Pickups, Inventory,
locked-door consumption, M61/M62 feedback, targeting or E arbitration.

## Release / Performance

Goal detection, visualization and completion HUD work in Release without
editor/ImGui.

No per-frame filesystem I/O/source access, unbounded runtime
history/allocation, or unnecessary physics rebuild. Bounded iteration
over authored goals is acceptable under v1 limits.

## Focused Tests

Cover equivalents of: 1. parse one/multiple level_goal; 2. malformed
token count/non-finite/invalid dimensions rejected; 3. deterministic
writer and round-trip; 4. valid default; 5. editor
add/duplicate/delete/equality; 6. Apply promotes workingCopy; 7.
Save/reload authored goals; 8. runtime completion not serialized; 9.
outside does not complete; 10. player overlap completes; 11. non-player
does not complete; 12. first completion exactly once; 13. staying
inside/second goal does not retrigger; 14. Restart/Apply/reload reset;
15. checkpoint respawn/PhysicsWorld rebuild do not fabricate completion;
16. completion does not mutate authored data/Dirty; 17. no E required
and E arbitration unchanged; 18. Item Pickup/Inventory/M61/M62
unchanged; 19. Door/Pressure Plate unchanged; 20. editor
selection/gizmo/picking works; 21. Release detection/visualization/HUD
works; 22. renderer state restored; 23. canonical cleanup correct.

Avoid generic production APIs solely for tests.

## Manual Acceptance

Use disposable Development fixtures.

1.  Add a Level Goal; verify visualization, selection, Inspector
    center/size, Translate, supported Resize/Scale, Duplicate/Delete and
    Dirty/Apply.
2.  Save/reload and verify authored round-trip.
3.  Apply, enter Gameplay, approach without entering: incomplete.
4.  Enter with player: `Level Complete` appears exactly once, without E.
5.  Remain inside and then enter another goal: no retrigger.
6.  Restart: completion resets and the goal can complete again.
7.  Apply/reload: completion resets.
8.  Checkpoint respawn before completion must not fabricate completion.
9.  Recheck Item Pickup + M61/M62, Inventory, Door/locked Door, Pressure
    Plate, Dynamic Box and E priority.
10. Run Release with a safe disposable level fixture if current workflow
    supports it; verify goal visualization/detection/HUD without
    editor/ImGui.

Remove all fixtures before Git closure.

## Validation

Run relevant current C++ tests for LevelFile/parser/writer, editor
authored categories, selection/gizmos, lifecycle/checkpoints/player
overlap, renderer state, Item Pickup, Inventory, Door, Pressure Plate,
Dynamic Box and canonical cleanup.

Run:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_item_pickup_collect_sound.py
```

Add/update narrow cooker/staging tests if Level Goal requires them.

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run `git diff --check`.

## Documentation

Update compact milestone index/status and current architecture/workflow
docs only where post-M60 conventions require. Document concrete Level
Goal syntax and semantics in appropriate current docs/tooling
references. Do not restore the monolithic corpus.

## Canonical Data Safety

Before closure canonical Level 01 remains fixture-free: Item Pickups 0,
Doors 0, Pressure Plates 0, Dynamic Boxes 0, Static Props 0, Level Goals
0. Keep absent `dynamic_box 0 5 0 1 1 1 30`.

## Out of Scope

No next-level loading; multi-level progression; target level/path;
campaign graph/save/progress; Continue/Next Level; results screen; main
menu; score/stars/timer; generic
Objective/Trigger/Receiver/Event/GameState framework; generic
Interactable; goal requirements/IDs/models/textures; generic VFX; ECS;
undo/redo; Level Format v2; M64.

## Completion Criteria

Ready for manual acceptance when Level Goal is a repeatable v1 authored
category; editor fully authors it; player overlap with any active goal
completes exactly once; non-player objects do not; `Level Complete` is
visible; current level stays loaded; lifecycle resets correctly without
fabricated completion; runtime never mutates authored data; existing
gameplay is preserved; Development/Release work; tests/builds pass;
canonical Level 01 is clean; and no generic
Objective/Trigger/Event/GameState framework, next-level loading, v2 or
M64 was added.

## STOP

After implementation, validation, documentation and report: **STOP**. Do
not commit, push, merge, start M64, or declare M63 CLOSED. Wait for user
manual acceptance and separate Git closure.
