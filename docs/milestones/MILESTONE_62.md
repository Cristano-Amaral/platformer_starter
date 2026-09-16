# Milestone 62 --- Item Pickup Collection HUD Notification

## Status

**Planned.** Milestone 61 is CLOSED. Begin only from clean synchronized
`main`.

## Canonical Path

`docs/milestones/MILESTONE_62.md`

## Branch

`milestone/62-item-pickup-collection-hud-notification`

## Cursor Model

**Grok 4.6 High --- Fast OFF**

## Goal

Add one small transient HUD confirmation after each successful Item
Pickup collection, complementing the M61 world-space burst and sound.

Preferred text: `Picked Up <itemId> x<quantity>`.

This is presentation only and consumes the same successful collection
outcome as M61. It must not create a second targeting, collection,
Inventory, or interaction authority.

## Context / Authority

Follow the post-M60 contract. Read `docs/milestones/MILESTONE_62.md`
first, then current code/tests/architecture/relevant docs. Do not load
milestone history unless a concrete dependency requires it.

Authority: current code → tests → current architecture/docs → active
milestone → latest relevant current documentation → older history.

## Preserve

Preserve M55 targeting/atomic collection; logical pickup `position`;
Inventory/UI; E priority Drop → Grab → Collect → Unlock; Door
requirements; Pressure Plates; M58 transforms; M58.1 Rotate; M58.2
ghost/picking; M58.3 bounds; M58.4 intensity; M59 Gold Amount + idle;
M61 burst/sound; lifecycle semantics; Release; Level Format v1.

## Trigger Authority

Only the existing successful collection path may emit M62 feedback:

``` text
valid target → E Collect → Inventory::TryAdd succeeds → collected
             → M61 burst + sound
             → M62 HUD notification
```

Failure emits none. Do not re-query targeting, infer collection by
polling Inventory, scan collected flags, or create an event bus. One
success creates one notification.

## Content

Show the actual collected `itemId` and quantity. Preferred wording is
`Picked Up <itemId> x<quantity>` or the narrowest equivalent matching
current HUD conventions.

Use authored/runtime item IDs directly. No ItemDefinition display names,
localization, rarity, icons, metadata, or catalogs.

## Presentation

Use current HUD font/immediate drawing. Keep it non-modal, readable,
visually separate from the existing targeting prompt, and
non-obstructive. It must not pause gameplay or steal input.

Use a short hold/fade with total lifetime approximately 1.5--2.5
seconds. A small existing-style background/shadow is acceptable if
needed.

Report final placement, duration and fade constants.

## Rapid Collections

Use a small bounded Item-Pickup-specific runtime list/queue so rapid
collections do not silently overwrite each other.

Preferred capacity: 3--5 entries unless current HUD architecture
suggests a narrower equivalent. New entries append deterministically;
entries expire independently; overflow deterministically drops/reuses
the oldest. A short vertical stack is preferred.

Do not build a generic notification/toast manager.

## Runtime Lifecycle

Notification state is transient runtime presentation only: not authored,
serialized, Inventory data, ItemPickupSpec data, editor Dirty/equality,
or checkpoint data.

Fresh run, Restart, Apply and reload clear entries. Checkpoint respawn
and PhysicsWorld rebuild must not fabricate/replay notifications. Do not
replay historical collections.

Follow current gameplay presentation timing/pause conventions. If M61
freezes while Inventory/editor pause is active, M62 may freeze
consistently rather than introducing a new real-time clock. Report the
final timing authority.

## M61 / HUD Integration

M61 burst and sound remain unchanged and are siblings of M62 under the
same successful collection result. M62 must not delay/retrigger them or
duplicate collection callbacks.

Preserve the existing pre-collection HUD exactly in behavior:
`E Pick Up <itemId> x<quantity>`. M62 is post-collection confirmation.

Do not modify Inventory UI semantics. Follow the narrowest existing HUD
layering convention while Inventory UI is open and report the behavior.

## Editor / Release / Format

M62 is Gameplay HUD presentation. No Inspector fields, Content Browser
changes, gizmo/ghost changes, authored settings, or Dirty changes.

It must work in Release without ImGui/Development dependencies.

No new authored fields, Item Pickup grammar changes, or Level Format v2.

## Performance

Bounded storage; avoid per-frame heap churn where practical; no
filesystem I/O, source asset access, or unbounded string/history
accumulation. Use existing HUD rendering.

## Focused Tests

Cover equivalents of: 1. success emits exactly one notification; 2.
failed TryAdd/no target/already collected emits none; 3. correct itemId
and quantity; 4. rapid successes create deterministic bounded entries;
5. overflow removes/reuses oldest deterministically; 6. entries
expire/fade finitely; 7. fresh run/Restart/Apply/reload clear entries;
8. checkpoint respawn/PhysicsWorld rebuild do not replay/fabricate
entries; 9. target HUD unchanged; 10. M61 burst/sound still exactly
once; 11. collection atomicity, Inventory and E arbitration unchanged;
12. Door and Pressure Plate unchanged; 13. M58/M59 presentation/editor
regressions unchanged; 14. Release includes notification; 15. no Level
Format change.

Avoid broad generic APIs solely for tests.

## Manual Acceptance

Use disposable Development fixtures.

1.  Collect a pickup with recognizable itemId/quantity. Confirm existing
    target prompt, then E causes M61 burst/sound plus one M62
    notification with correct itemId/quantity; Inventory changes exactly
    once; notification disappears automatically.
2.  Collect several pickups rapidly. Confirm readable deterministic
    stacking, independent expiration, bounded safe behavior, no
    indefinite entries.
3.  If practical exercise Inventory-add failure; confirm no
    post-collection notification.
4.  Trigger notification then Restart; confirm clear. Confirm
    Apply/reload clear stale entries and checkpoint/PhysicsWorld rebuild
    do not replay/fabricate them.
5.  Recheck target prompt, E priority, Door required-item behavior,
    Pressure Plates, Inventory UI, M61 feedback and relevant M58/M59
    behavior.
6.  Run Release and confirm notification works without editor/ImGui.

Remove all fixtures before Git closure.

## Validation

Run relevant current C++ tests for Item Pickup targeting/collection, M61
feedback, HUD, Inventory/UI, Door/LockedDoor, Pressure Plate, lifecycle,
M58/M59 regressions and canonical cleanup.

Run:

``` text
python tools/test_milestone_docs.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_item_pickup_collect_sound.py
```

If the M61 sound test has a different current name/path, use the
repository truth and report it.

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
docs only where post-M60 conventions require. Do not restore the
monolithic corpus.

## Canonical Data Safety

Before closure Level 01 remains Item Pickups 0, Doors 0, Pressure Plates
0, Dynamic Boxes 0, Static Props 0. Keep absent
`dynamic_box 0 5 0 1 1 1 30`. Leave no disposable fixtures.

## Out of Scope

No generic toast/notification system; generic event bus/Interactable;
ItemDefinition/catalog/display-name database; localization framework;
icons; rarity/per-item colors; loot history screen; persistent
notification history; per-pickup notification fields; new Inventory UI
features; generic UI/animation framework; ECS; undo/redo; Level Format
v2; M63.

## Completion Criteria

Ready for manual acceptance when every successful collection creates one
transient correct HUD confirmation; failure creates none; rapid
collections remain readable in a small deterministic bounded list;
entries expire; lifecycle does not preserve/replay stale feedback;
existing target HUD and M61 feedback remain unchanged;
targeting/collection/Inventory/E/Door/Plate behavior is unchanged;
Release works; validations pass; canonical Level 01 is clean; and no
generic notification/event/ItemDefinition framework, Level Format v2 or
M63 was introduced.

## STOP

After implementation, validation, documentation and report: **STOP**. Do
not commit, push, merge, start M63, or declare M62 CLOSED. Wait for user
manual acceptance and separate Git closure.
