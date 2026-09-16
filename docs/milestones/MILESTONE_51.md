## Milestone 51 --- Dynamic Box Grab / Carry

### Status

**Implemented, awaiting manual acceptance**

Milestone 50 is CLOSED. Do not declare M51 CLOSED until after user
manual acceptance and the separate Git closure workflow.

### Branch

`milestone/51-dynamic-box-grab-carry`

### Recommended Cursor Model

**Grok 4.6 High --- Fast OFF**

### 1. Goal

Introduce the first direct player-to-physics-object interaction: **Grab
/ Carry** for authored Dynamic Boxes.

The player can target a nearby Dynamic Box, grab it through one explicit
gameplay action, carry it in front of the player while preserving safe
physics/world collision, and release/drop it back into normal
simulation.

Gameplay loop:

**Approach Dynamic Box → Target → Grab → Carry → Move → Drop**

M51 is deliberately narrow. It is not a generic interaction framework.

### 2. Existing Architecture to Preserve

Inspect the current repository first and reuse its actual authorities: -
Jolt Physics and current CharacterVirtual/player architecture. -
Repeatable authored Dynamic Boxes. - Authored transform distinct from
transient simulated transform. - Runtime Dynamic Box rendering from
current Jolt pose. - Restart Run restores authored box transforms and
zeros velocities. - Checkpoint respawn does not globally reset Dynamic
Boxes. - M46 per-box recovery below `active.killPlane`. - Apply/reload
reconstruct runtime Dynamic Boxes from authored definitions. - Existing
input, collision-layer, physics-query, and runtime lifecycle
conventions. - Static Props remain non-physical and non-grabbable.

Do not replace adequate current architecture.

### 3. Core Behavior

#### 3.1 Grabbable scope

Only authored Dynamic Boxes are grabbable in M51.

Do not make Static Props or other authored/world objects grabbable. Do
not add a generic `Interactable` component or interaction-policy
framework.

#### 3.2 Runtime-only carry state

Carry state is transient runtime state and must not change Level Format
v1, `workingCopy`, `active` authored definitions, saved source,
Modified/Dirty, or authored Dynamic Box transforms.

Track only the runtime identity/reference needed to know whether and
which current Dynamic Box is carried. No GUIDs or serialized body
handles.

#### 3.3 Target acquisition

When not carrying, determine an eligible Dynamic Box reasonably in front
of the player/camera and within a finite maximum grab distance.

Requirements: - finite max range; - deterministic selection if several
candidates qualify; - only current runtime Dynamic Boxes; - aligned with
player/view intent; - no grabbing across the level.

Prefer an existing Jolt ray/query path if appropriate. Otherwise add the
narrowest Dynamic-Box-specific query. Do not create a generic
interaction targeting framework.

#### 3.4 Obstruction

Do not grab through obvious solid world geometry. Existing
Ground/Platform/Slope or equivalent solid collision between player and
candidate must prevent the grab where current physics authority can
establish this.

#### 3.5 Input

Inspect current bindings and add one clear action: **Grab / Drop**.

The same action may toggle: - not carrying → attempt Grab; - carrying →
Drop.

Follow current keyboard/gamepad conventions. No input-remapping system.

#### 3.6 Carry target

Carry the box at a stable, finite target position in front of the
player/camera. It follows player movement and relevant facing/view
direction.

Use existing fixed physics timing. Do not parent the body to the camera
or create a scene graph.

#### 3.7 Physics while carried

The carried box remains represented by its physics body. Drive it toward
the carry target with the narrowest stable Jolt-compatible mechanism
available in the current architecture.

Do not detach a visual-only model from physics. Do not permanently alter
authored state. Do not build a generalized joint/constraint framework
solely for M51.

#### 3.8 Collision policy

World collision must remain meaningful while carrying. The box must not
simply teleport through Ground/Platforms/Slopes/solid world geometry.

Prevent catastrophic player↔carried-box feedback. A narrow
suppression/adjustment of player-vs-current-carried-box collision is
acceptable if required, while preserving carried-box vs world collision.
Do not globally disable Dynamic Box collision.

Document the exact policy.

#### 3.9 Carry obstruction

If the desired carry point is obstructed, do not blindly teleport
through geometry. Let world collision constrain the body or narrowly
clamp/shorten the target using existing physics queries.

Prefer stable behavior over a complex gravity-gun simulation.

#### 3.10 Drop

Grab/Drop while carrying releases the box from its current runtime pose
into ordinary Dynamic Box simulation.

Normal Drop: - current runtime pose retained; - normal gravity/dynamics
resume; - authored transform unchanged; - no automatic authored reset; -
no arbitrary large release impulse.

#### 3.11 Lifecycle invalidation

Clear carry safely whenever runtime ownership/references can become
invalid, including relevant cases such as: - Restart Run; -
Apply/reload/rebuild; - runtime/level reconstruction; - carried-box
kill-plane recovery; - gameplay-state destruction/transition.

Never retain a dangling index/body reference.

#### 3.12 Checkpoint respawn

Preserve the established rule: checkpoint respawn does **not** globally
reset Dynamic Boxes.

Choose the narrowest safe carry policy during player checkpoint respawn.
Prefer releasing the carried box if continuing carry across player
teleport/reconstruction is ambiguous. Document/test it.

#### 3.13 Kill-plane recovery

M46 recovery remains authoritative and per-box. If the carried box is
recovered below `active.killPlane`, clear carrying safely before/with
recovery. Do not disable recovery while carrying or reset unrelated
boxes.

#### 3.14 Restart Run

Restart clears carry. All Dynamic Boxes still return to applied authored
transforms with zero velocities. No box remains implicitly carried
afterward.

#### 3.15 Apply/reload

Apply/reload clears carry before/as runtime Dynamic Boxes are
reconstructed. Do not preserve transient carry across reconstruction.

### 4. Player Feedback

Provide minimal readable runtime feedback for target/carry state using
current HUD/gameplay conventions.

A small prompt, reticle state, simple text, tint, or bounds indicator is
enough. No generic interaction UI and no inventory UI.

A lightweight target highlight may reuse existing rendering conventions;
do not build outline post-processing solely for this.

### 5. Runtime Identity Safety

Use the narrowest session/runtime identity compatible with current
Dynamic Box/physics architecture. No persistent GUIDs.

Any runtime index/body reference must be invalidated correctly on
rebuild, restart, reload, and recovery.

### 6. Physics Tuning

Use small explicit constants following repository conventions for
concepts such as max grab distance, carry distance, responsiveness,
correction velocity/force, and safety margin if needed.

Do not add these to Level Format or an editor physics panel.

Default expectation: existing authored Dynamic Boxes are grabbable
regardless of mass unless repository constraints prove this unsafe. If a
mass restriction becomes necessary, STOP and report rather than silently
inventing a gameplay rule.

### 7. Editor / Authoring

No new Dynamic Box Inspector property or `Grabbable` checkbox. Dynamic
Box itself is the explicit grabbable type in M51.

No Level Format change. Existing Dynamic Box authoring/lifecycle/gizmos
remain unchanged.

### 8. Explicitly Out of Scope

Do not implement: - Static Prop collision/physics/grabbing; - generic
Interactable/component/policy architecture; - inventory or inventory
pickup; - pressure plates/triggers/event graphs; - scripting; -
throwing/charged throw; - weapons/melee use of boxes; - held-object
rotation controls; - adjustable carry distance; - multiple carried
objects; - authored grab points/sockets; - mass/friction/restitution
editor; - new physics-layer editor; - GUIDs/ECS/prefabs/undo-redo; -
Level Format v2; - M52+ functionality.

### 9. Focused Tests

Inspect current tests and add narrow production-boundary coverage for
equivalents of: 1. no target without an eligible box; 2.
in-range/in-front Dynamic Box can target; 3. beyond-range box cannot
target; 4. non-Dynamic-Box cannot target; 5. deterministic
multiple-candidate choice; 6. solid obstruction prevents through-world
grab; 7. valid Grab enters carrying; 8. invalid Grab does nothing; 9.
exactly one box carried; 10. carried reference identifies correct
runtime box; 11. carry target follows movement; 12. carry target follows
facing/view; 13. carry distance bounded; 14. world collision remains
active; 15. no catastrophic player feedback; 16. carry does not mutate
authored transform; 17. no `workingCopy` mutation; 18. no authored
`active` mutation; 19. no Modified/Dirty mutation; 20. Drop clears
carry; 21. dropped box resumes simulation; 22. Drop does not
authored-reset; 23. no arbitrary large release impulse; 24. Restart
clears carry; 25. Restart still authored-resets all boxes; 26.
checkpoint respawn keeps non-global-reset semantics; 27. checkpoint
carry policy safe/deterministic; 28. Apply/reload clears carry; 29.
rebuild leaves no stale carry reference; 30. carried-box kill-plane
recovery clears carry; 31. recovery still resets only that box; 32.
other boxes unaffected; 33. repeated Grab/Drop stable; 34. grab another
box after drop; 35. feedback matches target; 36. feedback matches
carrying; 37. Static Props non-grabbable; 38. Dynamic Box authored
lifecycle unchanged; 39. Level Format unchanged; 40. physics body
accounting remains valid; 41. builds/configurations remain appropriate;
42. no M52+ architecture introduced.

Do not expose public production APIs solely for tests; reuse established
test-access conventions.

### 10. Manual Acceptance

Test at least: - nearby/in-front target feedback; - out-of-range
rejection; - Grab and stable carry; - movement/turning while carrying; -
carried-box collision against world geometry; - no catastrophic player
launch/push-through; - Drop and resumed gravity/dynamics; - repeated
re-grab/drop; - two Dynamic Boxes, only one carried; - obstruction
blocks through-wall grab; - Restart clears carry and restores all
boxes; - checkpoint respawn does not globally reset boxes and follows
documented carry policy; - kill-plane recovery remains per-box and
cannot leave stale carry; - Apply/reload cannot leave stale carry; -
runtime carry never marks authored level Modified and never persists
transient pose; - Static Props cannot be grabbed.

### 11. Regression Validation

Run relevant current C++ tests for Dynamic Box runtime/recovery, physics
rebuild/body budget, authored lifecycle, LevelFile, player movement,
restart/checkpoint behavior, rendering, editor reconstruction, canonical
cleanup, and preserve M49/M50 Static Prop/clip-plane regressions.

Run:

``` text
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
```

If still part of the standard suite:

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

``` text
git diff --check
```

### 12. Canonical Data Safety

Do not leave manual M51 Dynamic Box fixtures in
`game/assets/source/levels/level_01.level` unless explicitly authorized.

The intentionally removed legacy line remains absent:

`dynamic_box 0 5 0 1 1 1 30`

Do not mechanically overwrite unrelated semantic changes.

### 13. Completion Criteria

M51 is ready for manual acceptance when: - nearby/in-front Dynamic Boxes
can be targeted; - solid obstruction prevents through-world grab; -
exactly one box can be grabbed/carried; - carry follows player/view at
bounded distance; - world collision remains meaningful and stable; -
Drop safely resumes normal simulation; - authored state never changes
from carry; - Restart/checkpoint/recovery/rebuild semantics remain
correct; - no stale runtime reference survives reconstruction; - Static
Props remain non-grabbable; - no generic interaction framework was
introduced; - required tests/builds pass; - canonical Level 01 remains
safe.

### 14. STOP Rule

After implementation, validation, documentation, and report, STOP.

Do not commit, push, merge, start M52, or declare M51 CLOSED.

M51 closes only after user manual acceptance and the separate Git
closure workflow.
