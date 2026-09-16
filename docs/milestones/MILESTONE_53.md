## Milestone 53 --- Authored Door & Pressure Plate Link

### Status

**Implemented — awaiting manual acceptance**

Milestone 52 is CLOSED. Do not declare M53 CLOSED.

### Branch

`milestone/53-pressure-plate-door`

### Recommended Cursor Model

**Grok 4.6 High --- Fast OFF**

------------------------------------------------------------------------

### 1. Goal

Close the first complete physical puzzle loop by adding a repeatable
authored **Door** and a narrow authored link from a Pressure Plate to
one Door:

**Grab Dynamic Box → place on Pressure Plate → plate becomes active →
linked Door opens → remove last box → plate becomes inactive → Door
closes.**

M53 deliberately implements one concrete producer/consumer relationship.
It must **not** introduce a generalized
trigger/receiver/event/channel/component framework yet.

The previous roadmap label "Game Feature Configuration" is
deferred/resequenced. The concrete Pressure Plate → Door use case comes
first so any later trigger abstraction is based on real requirements.

------------------------------------------------------------------------

### 2. Existing Architecture to Preserve

Inspect the repository first and preserve current authorities,
especially:

-   Level Format v1;
-   `workingCopy`, `active`, `savedSourceBaseline`;
-   authored repeatable-object lifecycle;
-   session-local structural indices;
-   Hierarchy / Inspector / scene selection;
-   primitive Translate / Resize gizmos;
-   Object Palette / placement conventions;
-   Jolt `PhysicsWorld` transactional rebuild and body budget;
-   Dynamic Box runtime simulation;
-   M46 kill-plane recovery;
-   M51 Grab / Carry;
-   M52 Pressure Plate authored/runtime overlap semantics;
-   Restart Run / checkpoint respawn;
-   Development/Debug/Release behavior;
-   staged runtime authority.

Do not build parallel authorities.

------------------------------------------------------------------------

### 3. Authored Door

Add a repeatable authored `Door` category.

Use the smallest authored data needed for M53:

-   closed position / center;
-   size;
-   opening travel distance (or equivalent narrow scalar/axis
    representation after repository inspection).

Default opening direction for M53 should be deterministic and simple.
Prefer **vertical +Y opening** unless current project conventions
strongly justify another fixed direction.

Do not add rotation, custom model identity, material, animation asset,
GUID, script, lock/key, health, interaction prompt, or generic receiver
component.

The authored position is the **closed** pose.

------------------------------------------------------------------------

### 4. Pressure Plate → Door Link

Extend the authored Pressure Plate with the narrowest reference needed
to control one Door.

Because the repository intentionally has no persistent GUID system, use
a Level Format v1-compatible **authored Door index/reference** if it can
be made safe and deterministic within current save/parse/lifecycle
conventions.

The reference must mean the Door's authored order in the same
LevelDefinition, not a Jolt BodyID or runtime pointer.

Use an explicit "no linked door" representation.

Requirements:

-   a Pressure Plate may link to zero or one Door in M53;
-   multiple Pressure Plates may link to the same Door;
-   one Pressure Plate does not control multiple Doors yet;
-   references are validated;
-   Delete/Duplicate/reordering semantics must not silently retarget
    links;
-   if the current lifecycle makes stable index maintenance unsafe, STOP
    and report the concrete architectural conflict rather than inventing
    GUIDs or a generic ID system.

Do not introduce named channels or persistent GUIDs.

------------------------------------------------------------------------

### 5. Level Format v1

Keep Level Format v1.

Add repeatable Door syntax consistent with current parser conventions,
conceptually:

`door <cx> <cy> <cz> <sx> <sy> <sz> <openDistance>`

Extend Pressure Plate syntax with its optional/narrow Door reference
using the safest repository-consistent form discovered during
inspection.

Requirements:

-   finite values;
-   positive/non-degenerate Door size;
-   finite positive opening distance within a sensible safety bound;
-   valid/no-link Pressure Plate reference;
-   parse/save round-trip;
-   current file/line/count limits;
-   no runtime open fraction/state serialized;
-   no Level Format v2.

Document the exact final syntax.

------------------------------------------------------------------------

### 6. Door Authored Lifecycle

Door must integrate with:

-   Add;
-   Duplicate;
-   Delete;
-   Apply;
-   Revert;
-   Save;
-   Modified/Dirty;
-   `workingCopy`;
-   `active`;
-   `savedSourceBaseline`.

Hierarchy group: `Doors`.

Inspector exposes only the authored fields actually chosen for M53,
expected:

-   Position;
-   Size;
-   Open Distance.

Reuse primitive Translate / Resize semantics where appropriate.

No Rotate gizmo.

Object Palette/direct Add should follow existing primitive
authored-object conventions and reuse Ground/Platform/Slope placement
infrastructure where appropriate.

------------------------------------------------------------------------

### 7. Pressure Plate Inspector Link

Pressure Plate Inspector should expose a narrow Door link editor.

Prefer a deterministic combo/select control:

-   `None`;
-   Door 0 / Door 1 / ... with useful current editor labeling.

Do not ask the user to type raw indices if current ImGui/editor
conventions allow a safer selector.

Changing the link edits `workingCopy` only and follows Apply/Revert/Save
authority.

No runtime link mutation.

------------------------------------------------------------------------

### 8. Reference Integrity

Door lifecycle operations must preserve semantic links.

At minimum:

-   deleting a Door clears links that pointed to that Door;
-   links to later Doors are remapped if authored indices shift;
-   duplicating a Door does not automatically steal or duplicate
    Pressure Plate links;
-   duplicating a Pressure Plate preserves its current Door link only if
    that is safe and deterministic under current lifecycle conventions;
-   Revert restores prior links;
-   Save/reload preserves links;
-   invalid file references fail validation rather than silently
    retarget.

Implement this narrowly for Pressure Plate → Door only. Do not create a
generic reference graph.

------------------------------------------------------------------------

### 9. Runtime Door Representation

A Door is a solid world obstacle.

Use the narrowest Jolt-compatible representation consistent with
existing architecture, preferably a **kinematic box body** driven
between closed and open positions.

It must collide meaningfully with:

-   player;
-   Dynamic Boxes;
-   carried Dynamic Boxes;
-   other existing solid-world participants as current collision layers
    permit.

Do not implement the Door as visual-only if that would allow the
player/boxes to pass through while closed.

Door body allocation must participate in `kPhysicsMaxBodies = 64`
transactional accounting.

------------------------------------------------------------------------

### 10. Door Activation Semantics

For each Door, derive a runtime desired-open state from applied Pressure
Plates linked to it.

Rule:

**Door desired-open = true if at least one linked Pressure Plate is
Active.**

Therefore:

-   zero linked active plates → close;
-   one linked active plate → open;
-   two linked plates active → open;
-   one of two deactivates while the other remains active → stay open;
-   final linked plate deactivates → close.

This gives useful OR semantics without introducing logic gates.

Do not add AND mode, inversion, latch, toggle, delay, timer, or authored
logic settings.

------------------------------------------------------------------------

### 11. Door Motion

Door moves smoothly between:

-   closed authored position;
-   open position = closed position + deterministic opening travel.

Use narrow tuning constants for speed/responsiveness.

Do not serialize runtime open fraction.

The Door should not teleport between states during ordinary gameplay.

Opening/closing must be deterministic and bounded.

Do not add an animation framework.

------------------------------------------------------------------------

### 12. Obstruction Safety

Closing a Door must not catastrophically crush, launch, tunnel through,
or destabilize the player or Dynamic Boxes.

Use current Jolt kinematic collision behavior and the narrowest safe
policy.

If a player or Dynamic Box blocks closing, prefer a safe behavior such
as pausing/reopening/remaining blocked rather than forcing the Door
through the object.

Do not add a generalized obstruction/door framework.

Report the exact policy.

------------------------------------------------------------------------

### 13. M51 / M52 Integration

M51 remains unchanged:

-   Dynamic Box Grab / Carry continues to work;
-   carried boxes remain physical;
-   Door collision must remain meaningful.

M52 remains unchanged in detection authority:

-   Pressure Plate Active comes only from actual Dynamic Box overlap;
-   player and Static Props do not activate;
-   multiple boxes behave correctly.

M53 consumes the M52 derived active state. It must not replace or
duplicate Pressure Plate overlap logic.

------------------------------------------------------------------------

### 14. Restart / Checkpoint / Recovery / Rebuild

#### Restart Run

Dynamic Boxes reset according to existing semantics. Door runtime
motion/state must recompute from resulting Pressure Plate states. No
stale open fraction/reference survives improperly.

#### Checkpoint respawn

Dynamic Boxes are not globally reset. If a box remains on a linked
plate, the Door should remain/open according to that plate. Player
respawn alone must not reset the puzzle.

#### Kill-plane recovery

If an activating box is recovered and leaves the plate, M52 deactivates
the plate and the linked Door responds accordingly.

#### Apply/reload/rebuild

Reconstruct Door bodies and links from `active`. No stale body IDs,
runtime pointers, open-state ownership, or invalid references.

Pending `workingCopy` changes do not affect Gameplay until Apply.

------------------------------------------------------------------------

### 15. Visual Representation

Use a simple primitive Door representation consistent with current
greybox rendering.

The visual transform must match the runtime collision body closely
enough for manual testing.

No custom GLB requirement, material system, VFX, audio, or animation
assets.

Pressure Plate M52 green/inactive feedback remains unchanged.

------------------------------------------------------------------------

### 16. Physics Body Budget

Doors are expected to consume Jolt bodies if implemented as solid
kinematic boxes.

Update body accounting transactionally.

Current known accounting before M53:

-   fixed bodies = 5;
-   Platforms + Dynamic Boxes share the remaining 59;
-   M52 Pressure Plates consume no bodies.

M53 must update the formula based on the actual implementation, expected
conceptually:

`fixedBodies + platformCount + dynamicBoxCount + doorCount <= 64`

Preserve `TryRebuild` transactional failure semantics.

No body leak across rebuilds.

------------------------------------------------------------------------

### 17. Explicitly Out of Scope

Do not implement:

-   generic Trigger system;
-   generic Receiver system;
-   event bus;
-   named channels;
-   GUIDs;
-   generic object references;
-   one plate → many doors;
-   arbitrary trigger → arbitrary receiver;
-   switches/buttons;
-   proximity triggers;
-   player-activated plates;
-   door interaction with E;
-   keys/locks;
-   inventory;
-   door rotation/hinges;
-   sliding-axis editor;
-   horizontal/custom opening direction;
-   animation curves;
-   audio;
-   VFX;
-   custom Door GLB;
-   logic gates;
-   AND/NOT/XOR;
-   delays/timers;
-   latch/toggle;
-   scripting;
-   visual scripting;
-   ECS;
-   prefabs;
-   undo/redo;
-   Level Format v2;
-   the deferred generalized Game Feature Configuration milestone;
-   M54/M55 functionality.

------------------------------------------------------------------------

### 18. Focused Tests

Add focused regression coverage for actual equivalents of:

1.  Door Level Format parse;
2.  Door save/round-trip;
3.  invalid Door position/size/open distance rejected;
4.  repeatable Doors;
5.  Pressure Plate no-link parse/save;
6.  Pressure Plate valid Door link parse/save;
7.  invalid Door link rejected;
8.  Add/Duplicate/Delete Door lifecycle;
9.  Apply/Revert/Save Door lifecycle;
10. Door Hierarchy/Inspector;
11. Door Translate/Resize;
12. Pressure Plate link selector edits workingCopy only;
13. deleting linked Door clears/remaps references correctly;
14. deleting an earlier Door remaps later references;
15. duplicating Door does not silently retarget plates;
16. duplicating linked Pressure Plate has deterministic documented
    behavior;
17. one inactive linked plate keeps Door closed;
18. active linked plate opens Door;
19. removing box/deactivating plate closes Door;
20. two plates linked to one Door use OR semantics;
21. one of two remaining active keeps Door open;
22. final deactivation closes;
23. unlinked active plate does not affect Door;
24. plate linked to Door A does not affect Door B;
25. smooth bounded opening motion;
26. smooth bounded closing motion;
27. closed Door collision blocks player;
28. closed Door collision blocks Dynamic Box;
29. carried Dynamic Box interacts safely with Door;
30. obstruction during closing follows safe documented policy;
31. runtime motion does not mutate authored Door position;
32. runtime open state is not serialized;
33. Restart recomputes puzzle state;
34. checkpoint respawn preserves puzzle when box remains on plate;
35. kill-plane recovery propagates plate deactivation to Door;
36. Apply/reload/rebuild has no stale references/body IDs;
37. pending workingCopy Door/link edits do not affect runtime before
    Apply;
38. body budget includes Doors correctly;
39. over-budget rebuild fails transactionally;
40. no body leak after repeated rebuild;
41. M51 Grab/Carry regressions;
42. M52 Pressure Plate regressions;
43. M49/M50 Static Prop regressions;
44. canonical Level 01 cleanup;
45. no generic trigger/receiver/event system introduced.

Do not expose public production APIs solely for tests. Reuse established
test-access patterns.

------------------------------------------------------------------------

### 19. Manual Acceptance

In Development, using disposable fixtures:

1.  Add a Door; verify Hierarchy, Inspector, Translate, Resize.
2.  Add a Pressure Plate and link it to the Door in Inspector.
3.  Add/apply a Dynamic Box.
4.  Gameplay: closed Door blocks passage.
5.  Grab/carry/drop box onto linked plate.
6.  Plate turns active and Door opens smoothly.
7.  Walk/pass through the opened doorway.
8.  Remove the box; Door closes smoothly.
9.  Verify an unlinked plate does not control the Door.
10. If practical, link two plates to one Door and verify OR semantics.
11. Put an object/player in the closing path and verify safe obstruction
    behavior.
12. Verify M51 Grab/Carry remains stable around Door collision.
13. Leave a box on plate and checkpoint-respawn; puzzle state remains
    derived from the box.
14. Restart Run; puzzle recomputes from restored box positions.
15. Exercise kill-plane recovery; no stale Door-open state.
16. Test Apply/Revert/Save/reload of Door and link.
17. Ensure runtime Door motion never changes saved closed position.
18. Remove all manual M53 fixtures before Git closure.

Expected canonical closure state:

-   Pressure Plates: 0;
-   Doors: 0;
-   Dynamic Boxes: 0;
-   Static Props: 0.

The intentionally removed legacy Dynamic Box line remains absent.

------------------------------------------------------------------------

### 20. Validation

Run all relevant C++ tests, including LevelFile, lifecycle, editor,
gizmo, physics rebuild/body budget, M46, M51, M52, restart/checkpoint,
cook/stage/reload, M49/M50 regressions and canonical cleanup.

Run:

``` text
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
```

If still standard:

``` text
python tools/test_import_static_glb.py
```

Build:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run:

`git diff --check`

------------------------------------------------------------------------

### 21. Canonical Data Safety

Do not retain manual M53 Door/Pressure Plate/Dynamic Box fixtures in:

`game/assets/source/levels/level_01.level`

unless explicitly authorized.

Expected closure state: 0 Doors, 0 Pressure Plates, 0 Dynamic Boxes, 0
Static Props.

Keep the intentionally removed line absent:

`dynamic_box 0 5 0 1 1 1 30`

Do not mechanically overwrite unrelated semantic changes.

------------------------------------------------------------------------

### 22. Completion Criteria

M53 is ready for manual acceptance when:

-   authored repeatable Doors exist;
-   Pressure Plates can safely reference one authored Door;
-   reference integrity survives lifecycle operations;
-   linked active plate opens the Door;
-   final linked plate deactivation closes it;
-   multiple linked plates provide simple OR semantics;
-   Door is a meaningful solid runtime obstacle;
-   motion is smooth and bounded;
-   closing obstruction is safe;
-   M51/M52 behavior remains authoritative;
-   runtime state never mutates authored state;
-   body accounting is transactional and safe;
-   no generalized trigger/receiver/event architecture was introduced;
-   tests/builds pass;
-   canonical data is clean.

------------------------------------------------------------------------

### 23. STOP Rule

After implementation, validation and report, Cursor must STOP.

Do not commit, push, merge, start the next milestone, or declare M53
CLOSED.

Closure occurs only after user manual acceptance and the separate Git
closure workflow.
