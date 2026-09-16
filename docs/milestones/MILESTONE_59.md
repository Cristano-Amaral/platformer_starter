## Milestone 59 --- Item Pickup Presentation Polish

### Status

**Implemented.** Awaiting manual acceptance. Do not mark CLOSED. Milestone
58.4 is CLOSED. Do not start Milestone 60.

### Branch

`milestone/59-item-pickup-presentation-polish`

### Cursor Model

**Grok 4.6 High --- Fast OFF**

### Goal

Finish the current Item Pickup presentation pass with two concrete
improvements:

1.  Add an optional, authored, visual-only idle presentation (vertical
    bob + Y spin) so world pickups are easier to read before targeting.
2.  Improve M58.4 target-highlight tuning by separating overall
    highlight intensity from how strongly the model is pushed toward the
    existing golden target color.

This remains specific to Item Pickups. Do not create generic animation,
highlight, presentation, Interactable, ItemDefinition, or component
frameworks.

### Preserve Existing Authorities

Inspect the repository first and preserve the closed M55--M58.4
behavior: `ItemPickupSpec::position` targeting authority;
range/facing/LOS/nearest/tie; collection and `ItemPickupRunState`;
Inventory/UI; Door item requirements and E arbitration; Pressure Plates;
M58 visual offset/rotation/scale; M58.1 Rotate; M58.2 editor
ghost/picking; M58.3 `showInteractionBounds`; M58.4
`targetHighlightIntensity`; `StaticModelSceneStore`; staged runtime
authority; authored lifecycle; Level Format v1; render-state
restoration; canonical Level 01 safety.

### Part A --- Authored Idle Presentation

Add exactly:

``` cpp
bool idleAnimationEnabled = false;
float idleBobAmplitude = 0.15f;
float idleBobSpeed = 1.0f;
float idleSpinSpeedDegrees = 45.0f;
```

Defaults preserve existing scenes because idle animation is disabled.

When enabled, derive transient visual-only bob and Y spin from an
existing/shared runtime time source. Do not mutate authored fields per
frame and do not serialize runtime phase.

Conceptually:

``` text
visual position = position + visualOffset + {0, bobOffsetY, 0}
visual rotation = visualRotationDegrees + {0, idleSpinDegrees, 0}
visual scale    = visualScale
```

Use the repository's actual Euler/matrix convention.

Validation: - bob amplitude `[0.0, 2.0]` - bob speed `[0.0, 10.0]` -
spin speed `[-720.0, 720.0]` deg/s - all finite - negative spin reverses
direction - zero speed valid

Inspector: - Idle Animation - Bob Amplitude - Bob Speed - Spin Speed
(deg/s)

No curves, easing editor, axis selector, randomization, or per-pickup
phase field.

### Part B --- Target Highlight Gold Amount

Preserve:

``` cpp
float targetHighlightIntensity = 0.70f;
```

Add exactly:

``` cpp
float targetHighlightGoldAmount = 0.70f;
```

Valid finite range `[0.0, 1.0]`.

`targetHighlightIntensity` remains the overall contribution/opacity of
the extra target-highlight pass.

`targetHighlightGoldAmount` controls how strongly that pass pushes the
rendered pickup toward the existing golden target color.

The two controls MUST produce visibly distinct effects. Do not implement
Gold Amount as merely a second multiplier on the exact same alpha; that
would reproduce the M58.4 limitation.

Keep the existing gold approximately `RGB(255,220,72)`. No authored
color picker.

At Gold Amount 0, preserve original model coloration as much as the
current renderer permits. At Gold Amount 1, provide the strongest
supported push toward gold. At Intensity 0, there is no extra
model-highlight contribution regardless of Gold Amount.

Use the narrowest rendering change possible. Prefer extending the
current tinted model path. If a tiny shader/material change is truly
required to separate tint amount from pass opacity, keep it narrowly
scoped, restore state, support all builds, and report why the previous
path was insufficient. No post-processing or bloom.

Inspector: - Show Interaction Bounds - Target Highlight Intensity -
Target Highlight Gold Amount

Preferred Gold Amount slider: `0.00 .. 1.00`, two decimals.

### Target / Bounds Attachment

In Gameplay, the current target highlight and optional transformed
target bounds must follow the currently rendered visual pickup,
including transient idle bob/spin, so feedback remains attached to the
model.

This does NOT change targeting. M55 target eligibility remains based on
logical `ItemPickupSpec::position`.

### Editor Isolation

The M58.2 editor selection ghost and Translate/Scale/Rotate gizmos
remain stable authored tools. They must not chase transient gameplay
bob/spin.

Do not turn idle animation into an authored gizmo transform or add
animation gizmos/timeline UI.

### Fallback / Missing Model

No-model fallback remains safe and may use the same visual-only bob/spin
where naturally supported. Missing staged model preserves placeholder
behavior. Target feedback remains safe. No source fallback.

### Authored Lifecycle

All five new/preserved presentation values involved here participate
correctly in workingCopy/active/savedSourceBaseline, equality,
Modified/Dirty, Add, Duplicate, Apply, Revert, Save, reload. Duplicate
preserves values. Runtime phase/time never enters
Dirty/equality/serialization.

### Level Format v1

Extend the explicit-marker Item Pickup grammar backward-compatibly.

Current conceptual M58.4 form:

``` text
item_pickup <px> <py> <pz> <quantity> <itemId>
    [visual ...]
    [bounds <0|1>]
    [highlight <intensity>]
    [<modelIdentity...>]
```

Preferred new markers before modelIdentity:

``` text
gold <amount>
idle <0|1> <bobAmplitude> <bobSpeed> <spinSpeedDegrees>
```

Conceptual canonical form:

``` text
item_pickup <px> <py> <pz> <quantity> <itemId>
    [visual <ox> <oy> <oz> <rx> <ry> <rz> <sx> <sy> <sz>]
    [bounds <0|1>]
    [highlight <intensity>]
    [gold <amount>]
    [idle <0|1> <bobAmplitude> <bobSpeed> <spinSpeedDegrees>]
    [<modelIdentity...>]
```

Inspect the actual parser/writer first. M55/M58/M58.3/M58.4 records must
remain valid and receive M59 defaults. Canonical writer emits
deterministic markers/order. Bool is exact `0|1`; floats finite/in
range; identities with spaces round-trip; no ambiguous parsing; no Level
Format v2.

### Gameplay Isolation

Do not change targeting position/range/facing/LOS/nearest/tie,
collection atomicity, Inventory/UI, E arbitration, Door
`requiredItemId`, Pressure Plates, Dynamic Box Grab/Carry, checkpoint
collected state, Restart/Apply/reload semantics, or PhysicsWorld
authority.

Idle animation is presentation only.

### Performance / Render Safety

Reuse cached models. No per-frame filesystem I/O, GLB parse, mesh
duplication, asset scans, source fallback, or post-process buffer.
Restore matrix/depth/blend/cull/material/shader state after highlight
rendering.

### Focused Tests

Cover: 1. idle disabled default; 2. bob amplitude/speed/spin defaults;
3. Gold Amount default 0.70; 4. Add defaults; 5. Duplicate preserves; 6.
equality/Dirty detects each field; 7. Revert/Apply/Save/reload; 8. range
endpoints valid; 9. invalid ranges/NaN/Inf rejected; 10. old M58.4
record gets M59 defaults; 11. canonical marker order; 12. full
round-trip; 13. model identity with spaces unchanged; 14. malformed
bool/float rejected; 15. idle disabled equals M58.4 visual transform;
16. bob affects visual Y only; 17. spin affects visual Y rotation only;
18. authored transform unchanged by runtime animation; 19. logical
targeting position unchanged; 20. collected pickup not
rendered/animated; 21. target highlight follows animated visual; 22.
target bounds follow animated visual; 23. editor ghost/gizmo origin does
not chase runtime animation; 24. Intensity 0 suppresses extra model
highlight regardless of Gold Amount; 25. Gold Amount 0 materially
differs from 1 at fixed nonzero Intensity; 26. Intensity changes
contribution at fixed Gold Amount; 27. Gold Amount changes gold
coloration at fixed Intensity; 28. `showInteractionBounds` independent;
29. HUD independent; 30. current target only highlighted; 31.
fallback/missing model safe; 32. no source fallback; 33. cached model
reuse; 34. render state restored; 35. M58.2 editor ghost and
Translate/Scale/Rotate unchanged; 36. targeting/facing/LOS/tie
unchanged; 37. collection/Inventory unchanged; 38. Door/Pressure Plate/E
arbitration unchanged.

### Manual Acceptance

Use disposable Development fixtures.

#### Gold Amount

Use a dark model-backed pickup, bounds off, fixed Intensity around
`0.80`. Compare Gold Amount `0.00`, `0.50`, `1.00`. Confirm coloration
changes materially toward gold and is more perceptible than
intensity-only tuning. Repeat on a light model.

#### Independence

Keep Gold Amount fixed and compare low/high Intensity. Confirm distinct
roles. Test Intensity `0` + Gold `1`: no extra model highlight, but
HUD/targeting/collection remain. Test bounds independently.

#### Idle presentation

Enable Idle Animation. Confirm subtle vertical bob and Y spin. Change
amplitude/speed/spin; negative spin reverses. Disable and confirm static
M58.4 presentation returns.

#### Attachment

Target animated pickup. Highlight and optional bounds must follow
animated visual. Repeat with non-zero Visual Offset/Rotation and
non-uniform Scale.

#### Gameplay invariance

Target acquisition remains around logical `position`, not bobbed visual.
Collection, Inventory, Door required-item flow, and Pressure Plates
remain unchanged.

#### Editor regression

F2 selection ghost stays stable/aligned to authored transform.
Translate/Scale/Rotate work normally.

#### Lifecycle

Change new fields, Revert, Apply, Save/reload. Authored values persist;
runtime phase is never saved.

Remove all fixtures before Git closure.

### Validation

Run relevant C++ tests for LevelFile/Writer, Item Pickup, target
highlight, renderer state, StaticModelSceneStore, authored lifecycle,
M55 targeting/collection, Inventory/UI, Doors, Pressure Plates, M58
visual transform, M58.1 Rotate, M58.2 selection preview, M58.3 bounds,
M58.4 intensity, and canonical cleanup.

Run:

``` text
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

Run `git diff --check`.

### Canonical Data Safety

Before closure Level 01 remains: - Item Pickups 0 - Doors 0 - Pressure
Plates 0 - Dynamic Boxes 0 - Static Props 0

Keep absent: `dynamic_box 0 5 0 1 1 1 30`

### Explicitly Out of Scope

No generic animation system, clips/timeline, arbitrary animation axes,
curves/easing editor, random phase authoring, particles/sounds, authored
highlight RGB/color picker, auto-contrast/luminance/material analysis,
bloom/post-processing, generic Highlight/Presentation/Interactable,
ItemDefinition/catalog, generic Transform/Render component, ECS,
undo/redo, Level Format v2, or M60 functionality.

### Completion Criteria

Ready for manual acceptance when optional visual-only bob + Y-spin
works; targeting remains at logical position; target highlight/bounds
follow animated visual; Gold Amount materially changes how golden the
target becomes and is distinct from Intensity; bounds/HUD remain
independent; editor ghost/gizmos stay stable; old records remain valid;
model identities with spaces round-trip; Release works; render/cache
state is safe; all tests/builds pass; canonical Level 01 is clean; and
no generic animation/highlight/Interactable framework or M60
functionality was introduced.

### STOP

After implementation, validation, documentation and report: STOP.

Do not commit, push, merge, start M60, or declare M59 CLOSED. Wait for
user manual acceptance and separate Git closure.
