# Milestone 61 --- Item Pickup Collection Feedback

## Status

**Planned.** Milestone 60 is CLOSED. Begin only from clean synchronized
`main`.

## Canonical Path

`docs/milestones/MILESTONE_61.md`

## Branch

`milestone/61-item-pickup-collection-feedback`

## Cursor Model

**Grok 4.6 High --- Fast OFF**

## Goal

Add clear short-lived player feedback when an Item Pickup is
successfully collected: (1) a small transient visual collection effect
at the pickup's current presented world position and (2) one short
collection sound.

M61 builds on M55--M59 without changing collection authority, Inventory,
targeting, authored pickup presentation, Door/Pressure Plate behavior,
or Level Format. This remains Item-Pickup-specific; do not create
generic FX, event, Interactable, audio-event, ItemDefinition, or
component frameworks.

## M60 Context Contract

Read this file first: `docs/milestones/MILESTONE_61.md`. Then inspect
current code, tests, architecture and relevant current docs. Do not
automatically load complete milestone history; read older milestone
files only for a concrete dependency/ambiguity.

Authority: current code → tests → current architecture/docs → active
milestone → latest relevant checkpoint/current docs → older milestone
history.

## Preserve

Preserve M55 targeting and atomic collection; `ItemPickupSpec::position`
gameplay authority; `ItemPickupRunState`; Inventory/UI; E priority Drop
→ Grab → Collect → Unlock; Door item requirements; Pressure Plates; M58
visual transform; M58.1 Rotate; M58.2 editor ghost/picking; M58.3
bounds; M58.4 intensity; M59 Gold Amount + optional idle bob/spin;
staged model authority; StaticModelSceneStore; authored lifecycle;
checkpoint/restart semantics; Debug/Development/Release separation;
Level Format v1.

## Collection Authority

Trigger feedback only after the existing collection path succeeds:

``` text
current valid pickup target
→ E Collect
→ Inventory::TryAdd succeeds
→ pickup becomes collected
→ spawn visual feedback + play sound
```

Failed `TryAdd` emits no feedback. Do not perform a second
target/collection query. One successful collection emits exactly one
feedback event.

## Visual Effect

Add a narrow transient collection effect at the pickup's current
presented visual location when collection succeeds. Preferred direction:
a small restrained golden burst/sparks, approximately 0.25--0.60 s,
modest world-space radius, simple outward/upward motion and fade.

Use existing primitive/immediate rendering where practical. A small
bounded CPU-side representation is preferred if no suitable current
effect path exists. No post-processing, bloom, external particle
middleware, particle editor, or generic particle/FX system.

No authored tuning fields are required.

## Effect Origin

Capture the current presented gameplay visual origin at successful
collection, including authored `position`, `visualOffset`, and current
M59 idle bob when enabled. Spin need not affect the positional origin.
The captured origin is transient runtime data and no longer follows the
collected pickup.

Gameplay targeting remains at logical `position`.

## Runtime Lifecycle

Feedback is runtime-owned and never serialized. Use
bounded/fixed-capacity storage appropriate for the expected small number
of simultaneous effects; avoid per-frame heap churn where practical.

Expired effects are removed/reused. Fresh run, Restart, Apply and reload
clear active effects. Checkpoint restore must not replay feedback for
pickups already collected before the checkpoint. PhysicsWorld rebuild
alone must not fabricate/replay feedback.

## Collection Sound

Play one short sound only after successful collection. Prefer the
repository's existing audio abstraction/path if one exists. If none
exists, use the narrowest cross-build approach consistent with current
dependencies; do not introduce a general audio engine/event framework.

One successful collection → one sound. Failed collection → none. No
looping, per-item sound field, ItemDefinition metadata, or editor
dependency. Must work in Release and fail safely if sound cannot load.

If adding an audio asset, follow current source/runtime staging
conventions; runtime must not read source assets. Use
project-owned/redistributable material and document origin/generation as
appropriate. Do not add an asset catalog framework.

## No Level Format Change

M61 adds no authored Item Pickup field and should not change Item Pickup
grammar. No Level Format v2.

## HUD / Editor Isolation

Preserve `E Pick Up <itemId> x<quantity>`. Do not add loot toasts or
floating labels.

Do not change Content Browser, Item Pickup Inspector, M58 transforms,
M58.1 gizmos, M58.2 ghost/picking, or M59 authored controls. Collection
feedback is Gameplay runtime presentation only.

## Fallback

Feedback must work safely for model-backed pickups, fallback primitive
pickups and missing staged model placeholders. It depends on successful
collection, not model availability. No source fallback.

## Performance / Render Safety

No per-frame filesystem I/O, GLB parse, sound-file loading, mesh
duplication, source scan, or unbounded particle accumulation. Cache
audio according to current lifetime conventions. Restore render state
after visual feedback. Avoid broad renderer architecture changes.

## Focused Tests

Cover equivalents of: 1. successful collection emits exactly one
feedback event; 2. failed Inventory add emits none; 3.
untargeted/collected pickup emits none; 4. effect origin uses current
presented position; 5. visualOffset and enabled idle bob affect origin;
6. logical targeting remains unchanged; 7. effect expires and capacity
is bounded; 8. Restart/Apply/reload clear active effects; 9. checkpoint
restore does not replay old effects; 10. PhysicsWorld rebuild does not
fabricate effects; 11. sound triggers once only on success; 12. missing
sound fails safely; 13. model/fallback/missing-model cases safe; 14. HUD
and M55 range/facing/LOS/nearest/tie unchanged; 15. collection
atomicity/Inventory/E arbitration unchanged; 16. Door/Pressure Plate
unchanged; 17. M59 idle/highlight unchanged; 18. M58.2 editor
ghost/gizmos unchanged; 19. Release contains feedback; 20. renderer
state restored and no source fallback.

Do not expose broad generic APIs solely for tests.

## Manual Acceptance

Use disposable Development fixtures.

1.  Collect a model-backed pickup: it disappears as before, one short
    visual effect appears at the pickup, one sound plays, Inventory
    increments exactly once.
2.  Use non-zero Visual Offset + enabled visible idle bob; collect while
    displaced and confirm the burst originates at the current presented
    visual location, not merely logical `position`.
3.  Collect several pickups sequentially; each gets one independent
    short effect/sound and no stale effect persists.
4.  Confirm targeting range/facing/LOS and E priority remain unchanged;
    Door requirements, Pressure Plates and Inventory UI regressions
    remain correct.
5.  Trigger feedback then Restart; confirm it clears and does not
    replay. Verify checkpoint restore does not replay already-collected
    feedback; Apply/reload leaves no stale feedback.
6.  Test no-model fallback and, if practical, missing staged model.
7.  Run Release and confirm visual + sound feedback without editor/ImGui
    dependency.

Remove all disposable fixtures before Git closure.

## Validation

Run relevant C++ tests for Item Pickup targeting/collection,
Inventory/UI, Door/LockedDoor, Pressure Plate, M58/M59 presentation,
renderer state, lifecycle/checkpoint/restart, asset/audio integration
and canonical cleanup.

Run:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
```

Run any new narrow audio staging test explicitly.

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
docs only where current conventions require. Do not restore the
monolithic corpus or copy historical milestone bodies into
always-applied Cursor context.

## Canonical Data Safety

Before closure Level 01 remains: Item Pickups 0; Doors 0; Pressure
Plates 0; Dynamic Boxes 0; Static Props 0. Keep absent
`dynamic_box 0 5 0 1 1 1 30`. Leave no disposable fixtures.

## Out of Scope

No generic particle/FX system or editor; generic audio-event system;
per-item FX/sound settings; arbitrary effect colors; loot/inventory
toasts; floating labels; rarity; ItemDefinition/catalog; generic
Interactable/Presentation/Event bus; ECS; undo/redo;
post-processing/bloom; Level Format v2; M62 functionality.

## Completion Criteria

Ready for manual acceptance when successful collection produces exactly
one short visual effect and sound; failure produces neither; visual
origin matches current presented pickup location; feedback is
transient/bounded/runtime-only; lifecycle does not replay stale effects;
collection/Inventory/targeting/E/Door/Plate behavior is unchanged;
fallback/missing-model safe; Release works; tests/builds pass; canonical
Level 01 is clean; and no generic FX/audio/Interactable/ItemDefinition
framework, Level Format v2, or M62 functionality was introduced.

## STOP

After implementation, validation, documentation and report: **STOP**. Do
not commit, push, merge, start M62, or declare M61 CLOSED. Wait for user
manual acceptance and separate Git closure.
