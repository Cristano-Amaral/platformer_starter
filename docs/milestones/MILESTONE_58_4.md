## Milestone 58.4 — Item Pickup Target Highlight Intensity

### Status
**CLOSED.** Milestone 59 is the active Item Pickup presentation pass.

### Branch
`milestone/58.4-item-pickup-highlight-intensity`

### Cursor Model
**Grok 4.6 High — Fast OFF**

### Goal
Add a narrow authored control for the strength of the M58.3 gameplay target highlight used by Item Pickups.

M58.3 already introduced:
- golden highlight on the actual targeted model;
- optional transformed interaction bounds;
- `showInteractionBounds`.

M58.4 adds one per-pickup authored scalar:

`float targetHighlightIntensity`

The goal is to let the level designer tune how visible the golden target feedback is for each Item Pickup, because different model/material colors respond differently to the same fixed highlight strength.

This is a calibration milestone, not a new highlight system.

### Preserve Existing Authorities
Inspect the current repository first. Preserve M55 targeting and collection, M58 visual transforms, M58.2 cached model highlight rendering support, M58.3 gameplay target highlight, `showInteractionBounds`, `StaticModelSceneStore`, HUD, workingCopy/active/savedSourceBaseline, Apply/Revert/Save, Level Format v1, renderer state restoration, Release behavior, and current tests.

Do not duplicate target selection or create a second highlight path.

### New Authored Field
Add exactly one new Item Pickup authored presentation field:

`float targetHighlightIntensity`

Preferred default:

`0.70f`

This should preserve approximately the current M58.3 appearance, where the golden tint alpha is approximately `180 / 255`.

The field controls only the strength of the gameplay target highlight.

It must not affect targeting, range, facing, LOS, collection, Inventory, Door unlocking, gameplay interaction position, `showInteractionBounds`, editor selection ghost, editor picking, or gizmos.

### Range
Use a normalized authored range:

`0.0f <= targetHighlightIntensity <= 1.0f`

Semantics:
- `0.0` = no additional golden model highlight contribution;
- `1.0` = strongest supported golden tint contribution;
- intermediate values scale the highlight continuously.

Even at `0.0`, targeting and HUD remain fully functional.

If `showInteractionBounds == true`, the interaction wire remains independently visible.

Do not use the intensity to disable targeting.

### Inspector
Add to Item Pickup Inspector:

**Target Highlight Intensity**

Use a slider or drag control consistent with current editor UI conventions.

Preferred UI range:

`0.00 .. 1.00`

Recommended display precision:

2 decimal places.

Keep it visually near `Show Interaction Bounds`.

Do not call this control `Contrast`; the authored meaning is highlight intensity.

### Add / Duplicate
Add Item Pickup:

`targetHighlightIntensity = 0.70f`

Duplicate Item Pickup:

preserves the source `targetHighlightIntensity`.

No randomization.
No model-specific auto-tuning.
No asset metadata.

### Validation
Validation:
- finite;
- inclusive range `[0.0, 1.0]`.

Reject:
- NaN;
- Inf;
- values below 0;
- values above 1.

Use the same validation authority consistently across parse/apply/save paths.

Do not silently clamp invalid serialized authored data.

Inspector interaction may naturally constrain values through the UI.

### Level Format v1
Extend the current M58.3 Item Pickup syntax narrowly and backward-compatibly.

Current conceptual grammar:

`item_pickup <px> <py> <pz> <quantity> <itemId> [visual ...] [bounds <0|1>] [<modelIdentity...>]`

Because `modelIdentity` may contain spaces, preserve the explicit-marker strategy.

Preferred new marker:

`highlight <float>`

Conceptual grammar:

`item_pickup <px> <py> <pz> <quantity> <itemId> [visual ...] [bounds <0|1>] [highlight <intensity>] [<modelIdentity...>]`

Requirements:
- old M55/M58/M58.3 records remain valid;
- omitted intensity defaults to `0.70`;
- canonical writer emits explicit `highlight` marker deterministically;
- model identities with spaces still round-trip;
- marker order deterministic;
- invalid float rejected;
- out-of-range float rejected;
- no Level Format v2.

Inspect actual parser/writer before implementation and choose the narrowest unambiguous marker placement.

### Runtime Highlight Mapping
Reuse the M58.3 golden tint color.

Do not add authored color.

Preferred base tint:

`RGB(255, 220, 72)`

Scale highlight contribution using:

`targetHighlightIntensity`

Preferred direct mapping:

`alpha = round(255 * targetHighlightIntensity)`

or another direct normalized mapping that preserves `0.70` near the current M58.3 visual result.

Do not introduce nonlinear curves unless the current renderer requires them.

Report the exact mapping.

### 0.0 Behavior
At `targetHighlightIntensity == 0.0`:
- no additional model tint/highlight pass is required;
- pickup is still targeted;
- HUD still appears;
- E still collects;
- bounds still appear if `showInteractionBounds == true`.

This is valid authored behavior.

Do not force a minimum non-zero highlight.

### 1.0 Behavior
At `targetHighlightIntensity == 1.0`:
- use the strongest supported golden target tint;
- keep renderer state valid;
- avoid accidental opaque material replacement that destroys readability if current tinted draw path blends safely.

No bloom.
No post-processing.
No material rewrite.

### Fallback Item Pickup
For Item Pickups with no model identity, preserve the current fallback cube targeting feedback.

Apply `targetHighlightIntensity` to fallback target fill/highlight where practical and narrow.

If the fallback path cannot use the exact same model-tint mechanism, preserve semantic consistency:
- low intensity = subtler fallback target highlight;
- high intensity = stronger fallback target highlight.

`showInteractionBounds` remains independent.

### Missing Model
For missing staged model:
- preserve placeholder behavior;
- apply targetHighlightIntensity to placeholder highlight where practical;
- no crash;
- no source fallback.

### Interaction Bounds Independence
`showInteractionBounds` and `targetHighlightIntensity` are independent controls.

Examples:

#### Model-only strong highlight
- Show Interaction Bounds = false
- Target Highlight Intensity = 0.90

#### Bounds-heavy subtle highlight
- Show Interaction Bounds = true
- Target Highlight Intensity = 0.25

#### Bounds-only
- Show Interaction Bounds = true
- Target Highlight Intensity = 0.00

#### HUD-only
- Show Interaction Bounds = false
- Target Highlight Intensity = 0.00

All are valid.

### Editor Selection Ghost Isolation
M58.4 must not change M58.2 Editor selection ghost intensity.

`targetHighlightIntensity` applies only to Gameplay Item Pickup target highlight.

Do not couple this field to Static Prop editor highlight, Item Pickup editor selection ghost, gizmo hover, or editor selection bounds.

No global highlight theme is introduced.

### HUD
Preserve:

`E Pick Up <itemId> x<quantity>`

HUD does not depend on intensity.

At intensity `0.0`, HUD remains visible.

### Gameplay / Release
Target highlight intensity is player-facing and must work in Release.

No ImGui dependency in runtime rendering.

Inspector remains Development-only authoring UI, but authored value must drive runtime rendering in all gameplay builds.

### Renderer State Safety
Reuse M58.3 rendering path.

If highlight pass is skipped at intensity 0, renderer state must remain correct.

If rendered at non-zero intensity, preserve M58.3 restoration guarantees for matrix stack, depth state, blend state, culling, and material/shader state.

No state leakage.

### Performance
No new per-frame asset work.

Only current targeted Item Pickup may incur highlight rendering.

No extra model loads, file reads, asset metadata lookup, mesh copies, or post-process buffer.

### Authored Lifecycle
`targetHighlightIntensity` participates in:
- workingCopy;
- active;
- savedSourceBaseline;
- authored equality;
- Modified / Dirty;
- Add;
- Duplicate;
- Delete unchanged;
- Apply;
- Revert;
- Save;
- reload.

Runtime targeting and collection do not mutate it.

Checkpoint / Restart do not mutate authored value.

### Explicitly Out of Scope
Do not implement authored highlight color, per-item RGB picker, bloom, post-processing, pulsing highlight, animation curve, hover/target fade timing, global highlight preferences, model-material analysis, automatic contrast detection, luminance-based auto tuning, asset metadata, generic Highlight component, generic Presentation component, generic Interactable system, Static Prop gameplay highlight, Door gameplay highlight, Dynamic Box gameplay highlight, ItemDefinition/catalog, ECS, undo/redo, Level Format v2, or M59 functionality.

### Focused Tests
Cover:
1. default approximately 0.70;
2. Add uses default;
3. Duplicate preserves;
4. equality detects change;
5. Revert restores;
6. Apply promotes;
7. Save/reload persists;
8. `0.0` valid;
9. `1.0` valid;
10. mid-range valid;
11. negative rejected;
12. >1 rejected;
13. NaN rejected;
14. Inf rejected;
15. legacy record without marker defaults to 0.70;
16. canonical writer emits `highlight`;
17. round-trip preserves value;
18. modelIdentity with spaces remains unchanged;
19. malformed highlight token rejected;
20. no Level Format v2;
21. intensity 0 suppresses model highlight contribution;
22. intensity near default approximates M58.3 visual;
23. intensity 1 produces strongest tint;
24. current target only is highlighted;
25. non-targeted pickup unaffected;
26. collected pickup unaffected;
27. fallback remains safe;
28. missing staged model remains safe;
29. bounds false + intensity >0 works;
30. bounds true + intensity 0 works;
31. bounds false + intensity 0 leaves HUD/targeting intact;
32. M58.2 editor ghost unaffected;
33. targeting range/facing/LOS unchanged;
34. E arbitration unchanged;
35. Door/Pressure Plate/Inventory unchanged;
36. zero-intensity skip path safe;
37. non-zero path restores render state;
38. subsequent rendering unaffected.

Do not expose broad public APIs solely for tests.

### Manual Acceptance
Use disposable Development fixtures.

#### Dark model
1. Add Item Pickup with a dark GLB.
2. Set Show Interaction Bounds off.
3. Test Target Highlight Intensity at approximately 0.20, 0.70, and 1.00.
4. Apply/run after each authored change.
5. Confirm visual strength changes clearly.
6. Confirm HUD and targeting remain unchanged.

#### Light model
7. Use a light-colored GLB.
8. Compare 0.20 / 0.70 / 1.00.
9. Confirm designer can find a useful value.

#### Independence
10. Bounds on + intensity 0.
11. Confirm only bounds + HUD communicate target.
12. Bounds off + intensity high.
13. Confirm only model highlight + HUD communicate target.
14. Bounds off + intensity 0.
15. Confirm targeting and HUD still work with no world-space visual target cue.

#### Fallback
16. Test no-model Item Pickup at low/high intensity.
17. Confirm safe feedback.

#### Lifecycle
18. Change intensity.
19. Revert.
20. Confirm old value returns.
21. Apply.
22. Save/reload.
23. Confirm persisted value.

#### Regression
24. Confirm M58.2 Editor selection ghost remains unchanged.
25. Confirm Translate/Scale/Rotate unchanged.
26. Confirm collection, Inventory, Door required item, and Pressure Plates remain unchanged.

Remove all disposable fixtures before closure.

### Validation
Run relevant C++ tests including M58.4 plus regressions for LevelFile, LevelWriter, Item Pickup, Item Pickup target highlight, renderer state restoration, M55 targeting/collection, M56 Inventory UI, M57/M57.1 Doors, M58 visual transform, M58.1 Rotate, M58.2 Editor selection preview, M58.3 interaction bounds, Pressure Plates, authored lifecycle, and canonical cleanup.

Run:
```
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
```

Build:
```
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run `git diff --check`.

### Canonical Data Safety
Before closure, Level 01 remains fixture-free:
- Item Pickups: 0
- Doors: 0
- Pressure Plates: 0
- Dynamic Boxes: 0
- Static Props: 0

Keep absent:
`dynamic_box 0 5 0 1 1 1 30`

Do not mechanically overwrite unrelated semantic changes.

### Completion Criteria
Ready for manual acceptance when Item Pickup has authored `targetHighlightIntensity`; Inspector exposes normalized 0..1 control; default preserves approximately current M58.3 appearance; intensity changes gameplay model-highlight strength; intensity 0 is valid and does not affect targeting/HUD; `showInteractionBounds` remains independent; legacy records remain valid; model identities with spaces still round-trip; Release uses authored intensity; renderer state remains safe; M58.2/M58.3 behavior remains intact; all builds/tests pass; canonical Level 01 is clean; and no color picker, auto-contrast, bloom, generic Highlight/Interactable framework, Level Format v2, or M59 functionality was introduced.

### STOP
After implementation, validation and report: STOP.

Do not commit, push, merge, start M59, or declare M58.4 CLOSED. Wait for user manual acceptance and separate Git closure.
