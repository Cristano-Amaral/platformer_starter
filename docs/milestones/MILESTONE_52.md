## Milestone 52 --- Pressure Plate / Dynamic Box Trigger

### Status

**Implemented — awaiting manual acceptance**

Milestone 51 is CLOSED. Runtime activation uses a direct AABB overlap
between the applied authored Pressure Plate region and current Dynamic
Box runtime bounds. Pressure Plates do not consume Jolt body slots.
Canonical Level 01 remains 0 Pressure Plates, 0 Dynamic Boxes, and 0
Static Props.

Do not declare M52 CLOSED until user manual acceptance.

M52 begins from clean synchronized `main`.

### Branch

`milestone/52-pressure-plate-box-trigger`

### Cursor

**Grok 4.6 High --- Fast OFF**

### Goal

Add the first repeatable authored gameplay trigger: a **Pressure Plate**
whose transient runtime state is active while at least one runtime
Dynamic Box overlaps its trigger region.

Puzzle loop: **Grab box → place on plate → plate activates → remove last
box → plate deactivates.**

M52 proves concrete detection/state only. It does not add doors,
outputs, event graphs, scripting, or generic trigger/action
architecture.

### Authored data

Add repeatable Pressure Plates to the existing LevelDefinition/authored
lifecycle with the smallest data required: - position/center; - trigger
size/extents.

Use current primitive-object conventions. No GUID, alias, target,
action, script, or serialized active state.

Extend **Level Format v1** with narrow repeatable syntax consistent with
the repository, conceptually:
`pressure_plate <cx> <cy> <cz> <sx> <sy> <sz>`

Validate finite position and positive/non-degenerate size; preserve
existing file/line/count limits and parse/save round-trip. No Level
Format v2.

### Lifecycle/editor

Integrate Pressure Plates with existing: - `workingCopy`, `active`,
`savedSourceBaseline`; - Add, Duplicate, Delete; - Apply, Revert,
Save; - Modified/Dirty; - Hierarchy and Inspector; - Position and Size
editing; - primitive Translate and Resize gizmos.

Integrate with existing Add/Object Palette placement conventions where
narrow and appropriate, reusing Ground/Platform/Slope placement
infrastructure rather than creating a new placement system. Do not alter
M50 Static Prop placement.

### Runtime

Runtime state is applied authored trigger region + transient activation
state.

Use the narrowest current Jolt-compatible representation. Prefer a
sensor/trigger if it integrates cleanly. A deterministic direct overlap
test between authored trigger AABB and current Dynamic Box runtime
bounds is also acceptable if substantially narrower/safer. Do not build
a generalized trigger framework.

A plate is **Active iff at least one current runtime Dynamic Box
overlaps it**.

Multiple boxes are supported: removing one keeps the plate active while
another remains; removing the last deactivates it. No latch/toggle.

Only Dynamic Boxes activate it. Player, Static Props, Platforms,
Collectibles, Hazards, Checkpoints and other plates do not.

M51 carry state is not itself an activation condition: a carried box
activates only through actual runtime overlap.

### Runtime lifecycle semantics

-   M46 kill-plane recovery: recompute overlap; no stale active state.
-   Restart Run: boxes restore as already defined; plate activation is
    recomputed, not restored from saved runtime state.
-   Checkpoint respawn: boxes are not globally reset; a box remaining on
    a plate keeps it active.
-   Apply/reload/rebuild: reconstruct/recompute without stale overlap
    references.
-   `workingCopy` changes do not affect runtime before Apply.
-   Runtime activation never marks Modified/Dirty and is never
    serialized.

### Visual feedback

Render a simple visible plate using current primitive world-render
conventions, with clear inactive vs active visual state (for example a
narrow tint/color convention). No animation/material/VFX/audio
framework.

Use current F1 metrics narrowly if appropriate (e.g. Pressure Plates /
Active Pressure Plates).

### Physics body budget

Preserve `kPhysicsMaxBodies = 64`. If plates use Jolt sensor bodies,
update transactional body accounting correctly. If direct overlap tests
use no body, explicitly preserve/test existing accounting.

### Out of scope

No doors, moving-platform control, hazard control, lights, target
references, channels, event bus, generic
Trigger/Activator/Action/Receiver components, scripting, visual
scripting, logic gates, timers, latching, player activation, Static Prop
activation, weight thresholds, required box counts, filters/tags, custom
plate assets, audio, animation framework, GUIDs, ECS, prefabs,
undo/redo, Level Format v2, or M53+ features.

### Tests

Add focused coverage for: - Level Format
parse/save/validation/repeatability; -
Add/Duplicate/Delete/Apply/Revert/Save; -
Hierarchy/Inspector/Translate/Resize; - inactive with zero boxes; -
active with one overlapping box; - deactivate when last box leaves; -
correct two-box occupancy; - player and Static Props do not activate; -
carried box activates only by overlap; - Drop-on-plate and re-grab-away
behavior; - kill-plane recovery removes stale activation; - Restart
recomputes; - checkpoint respawn preserves activation when box
remains; - Apply/reload has no stale overlap state; - pending
workingCopy edits do not affect active runtime; - activation does not
mutate authored state; - activation is not serialized; - multiple plates
are independent; - physics body accounting remains valid; - M51
Grab/Carry remains green; - M49/M50 Static Prop/clip-plane regressions
remain green; - no generic trigger/output system.

Do not expose public production APIs solely for tests; reuse established
test-access conventions.

### Manual acceptance

In Development: 1. Add Pressure Plate; verify
Hierarchy/Inspector/Translate/Resize; Apply. 2. Add/apply Dynamic Box.
3. Gameplay: use M51 Grab/Carry to put box on plate; plate becomes
active. 4. Remove box; plate becomes inactive. 5. Test two boxes: remove
one, still active; remove last, inactive. 6. Player standing on plate
alone must not activate it. 7. Static Prop must not activate it. 8.
Carried box activation follows actual overlap. 9. Checkpoint respawn
with box left on plate preserves box/activation. 10. Restart Run
recomputes from restored box positions. 11. Kill-plane recovery leaves
no stale active plate. 12. Apply/Revert/Save obey authored authority;
active state never persists. 13. M51 Grab/Carry still behaves normally.
14. Clean canonical Level 01 before closure.

Expected canonical closure state: 0 Pressure Plates, 0 Dynamic Boxes, 0
Static Props; legacy `dynamic_box 0 5 0 1 1 1 30` remains absent.

### Validation

Run relevant current C++ tests for LevelFile, authored lifecycle,
editor/gizmos, physics rebuild/body budget, Dynamic Box recovery, M51
Grab/Carry, restart/checkpoint, cook/stage/reload, canonical cleanup,
and M49/M50 regressions.

Run:

``` text
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
```

Run `python tools/test_import_static_glb.py` too if it remains in the
standard full suite.

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run `git diff --check`.

### Completion

Ready for manual acceptance only when authored Pressure Plates fully
integrate, Dynamic Box overlap drives correct transient active/inactive
state, M51 works naturally with them, lifecycle/recovery/rebuild
semantics are correct, body budget is safe, no generalized
trigger/output architecture was introduced, tests/builds pass, and
canonical data is clean.

### STOP

After implementation and validation, report and STOP. Do not commit,
push, merge, start M53, or declare M52 CLOSED. Closure occurs only after
user manual acceptance and separate Git closure.
