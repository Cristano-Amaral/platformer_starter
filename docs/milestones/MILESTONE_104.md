# Milestone 104 - Equipment Sockets & Visible Equipment Foundation

## Status

Defined. Implementation has not started.

## Branch

`milestone/104-equipment-sockets-visible-equipment`

## Goal

Connect the existing M100 Equipment system and M103 animated Player presentation so equipped items can optionally render as visible models attached to authored character skeleton joints, without changing gameplay authority or introducing a generalized character-instance, combat, or animation-graph system.

The intended bounded data flow is:

`Equipment slot -> equipped ItemDefinition -> optional equipment attachment authoring -> Player animated skeleton joint -> visible attached model`

M104 is a presentation/attachment foundation. Equipment inventory state remains authoritative for what is equipped; M103 remains authoritative for skeletal pose/animation; M102 remains authoritative for effective Player stats.

## Required Scope

### 1. Typed equipment attachment authoring

Extend the existing typed ItemDefinition equipment payload with an optional, bounded attachment definition for Equipment items.

The attachment contract must be explicit and typed, not a generic property bag. It must support at minimum:

- attachment joint/bone name;
- local translation offset;
- local rotation offset;
- local scale.

The existing optional ItemDefinition World Model remains the visual model source where appropriate; do not introduce a duplicate model identity unless repository constraints require a narrowly justified field.

Attachment data is meaningful only for Equipment items. Validation must reject or clearly diagnose invalid category/type combinations rather than silently interpreting them.

### 2. Gameplay definition persistence

Extend the existing `definitions.gameplay` parser/writer for the typed attachment fields.

Requirements:

- deterministic serialization;
- Save/Reload round-trip preservation;
- backward compatibility with definitions that omit attachment data;
- missing or malformed attachment authoring must fail safely and diagnostically;
- no format version bump, JSON conversion, GUID migration, or generic metadata system.

### 3. Item Database editor integration

Extend the existing Development Item Database editor for Equipment items.

For an Equipment item, expose bounded attachment authoring for:

- joint/bone name;
- translation offset;
- rotation offset;
- scale.

When practical with the existing M103 model/animation infrastructure, provide discoverability of available skeleton joint names for the canonical/selected character model rather than requiring blind text entry. A simple bounded selector/list is sufficient; do not build a skeleton hierarchy editor.

The editor must preserve the established working-copy/baseline and Save/Reload behavior.

None, resolved, and missing/invalid attachment states must be distinguishable where applicable.

### 4. Reusable joint attachment transform

Add the minimum reusable runtime support needed to resolve an authored joint name against an evaluated skeleton and compute an attachment transform from:

- the current animated joint pose;
- the character/model world transform;
- the authored local equipment offset.

The transform composition must be deterministic and covered by focused tests.

Do not create a generalized scene-node/component/socket framework.

### 5. Player visible equipment

The Player is the only required runtime consumer in M104.

For each occupied M100 Equipment slot, in deterministic existing slot order:

1. resolve the equipped ItemDefinition;
2. resolve its optional World Model/presentation model;
3. resolve its optional attachment joint;
4. compute the attachment transform from the current M103 animated Player pose;
5. render the equipment model at that transform.

Visible equipment must update immediately after equip, unequip, or swap operations.

Inventory-only items must not render as equipped attachments.

### 6. Safe fallback behavior

The runtime must remain stable when:

- an equipped item has no World Model;
- an Equipment item has no attachment authoring;
- the authored joint name is missing from the Player skeleton;
- the item definition is missing/invalid;
- the model reference is missing;
- the Player character/model/animation reference is unresolved.

These cases must not crash, corrupt equipment state, or alter gameplay stats. Prefer omission of the visual attachment plus clear Development diagnostics over invented fallback placement.

### 7. Rendering integration

Use the existing renderer/model/material pipeline for attached equipment models.

Requirements:

- preserve static-model rendering;
- preserve M103 Player skinning;
- preserve terrain, vegetation, lighting, and material behavior;
- attached equipment follows the animated joint in the main pass;
- if the existing shadow path renders ordinary model geometry for this case, visible equipment must cast a correctly transformed shadow as well;
- do not add equipment skinning or character mesh merging unless an actual required asset proves it necessary.

M104 is primarily for rigid models attached to animated joints (for example, a sword attached to a hand).

### 8. Development diagnostics

Extend Development diagnostics sufficiently to inspect visible-equipment resolution, including at minimum:

- equipment slot;
- equipped item identity;
- item-definition resolution status;
- model resolution status;
- authored attachment joint;
- joint resolution status;
- whether the attachment is currently rendered.

Keep diagnostics bounded and useful; do not build a generalized inspector framework.

### 9. Canonical demonstration data

Provide the minimum canonical authored data/assets needed to manually prove the feature if suitable existing equipment/model data is available.

Prefer reusing existing repository assets and ItemDefinitions. Do not introduce large third-party art packs or unrelated content.

Any canonical `definitions.gameplay` changes must be intentional, minimal, and documented in the final report. Temporary test definitions/assets must not remain after validation.

Canonical `level_01.level` and `level_02.level` must not receive incidental changes.

## Runtime Authority

M104 must preserve the existing authority boundaries:

- M100 Equipment state determines what item occupies each equipment slot.
- M102 Character Stats determines equipment modifier contributions and effective Player stats.
- M103 skeletal animation determines the current Player pose.
- M104 only derives visible attachment presentation from those existing authorities.

Visible equipment must not become a second source of truth for inventory/equipment state or gameplay modifiers.

## Tests

Add focused automated coverage appropriate to the implementation, including at minimum:

- attachment authoring parse/write round trip;
- validation of attachment fields and Equipment-only semantics;
- backward compatibility when attachment data is absent;
- joint-name resolution success and failure;
- deterministic joint/local-offset transform composition;
- equipment slot -> visible attachment selection;
- equip/unequip/swap presentation updates;
- missing ItemDefinition/model/joint safe behavior;
- Item Database Save/Reload for attachment authoring;
- M100 Inventory/Equipment regressions;
- M102 Player-character-stat regressions;
- M103 Player presentation/skeletal animation regressions;
- static-model/rendering regressions affected by the implementation.

If an escaped integration path depends on Raylib or the real canonical asset, prefer an integration regression that exercises that real path rather than relying only on backend-independent unit tests.

## Manual Acceptance

Manual Development-build acceptance is mandatory after automated validation.

At minimum verify:

1. the game starts and reaches normal gameplay;
2. an Equipment item with a visible model can be equipped;
3. the model appears at the authored Player joint;
4. the attachment follows Idle, Move, and Jump animation poses without remaining behind or drifting in world space;
5. local translation/rotation/scale offsets visibly affect placement as authored;
6. unequip removes the visible model immediately;
7. swapping equipment updates the visible model correctly;
8. missing/invalid joint authoring fails safely without a crash;
9. M102 equipment stat behavior remains correct;
10. Player movement, jumping, gravity, collision, and animation state selection remain unchanged;
11. attached equipment rendering/shadows do not cause obvious regressions to static models, terrain, vegetation, lighting, or materials;
12. Item Database Save/Reload preserves attachment authoring.

Any manual test data must be restored to the intended canonical state before Git closure.

## Explicitly Out of Scope

Do not implement any of the following in M104:

- generalized `CharacterInstance` runtime;
- Enemy/NPC/Animal spawning or controllers;
- AI;
- combat, attacks, damage, health/death gameplay;
- weapon hitboxes or traces;
- attack animation states;
- animation events/notifies;
- root motion;
- IK or procedural hand placement;
- retargeting;
- equipment-driven skeletal mesh replacement;
- skinned clothing/armor deformation;
- mesh merging;
- generalized scene graph or socket/component framework;
- generic animation graph/state-machine editor;
- blend trees, layers, masks, or additive animation;
- inventory redesign;
- save-game persistence;
- ECS migration;
- GUID migration;
- JSON conversion;
- Level Format v2;
- M105 features.

## Canonical Data Safety

Before closure, explicitly audit:

- `game/assets/source/gameplay/definitions.gameplay`;
- `game/assets/source/levels/level_01.level`;
- `game/assets/source/levels/level_02.level`.

Do not mechanically normalize line endings or restore unrelated semantic changes.

## Validation

Run the repository-required validation plus all tests affected by the implementation.

At minimum:

```text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release
```

Run relevant C++ tests for GameplayDefinition, Item Database, Inventory, Equipment, Player stats, Player presentation, skeletal animation, model/material/rendering, plus newly added M104 tests.

Run the required Python regression suites defined by the repository/workflow, including:

```text
python tools/test_milestone_docs.py
python tools/test_agent_instructions.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_stage_world_shaders.py
```

Before the implementation report:

```text
git branch --show-current
git status
git diff --check
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
git diff -- game/assets/source/gameplay/definitions.gameplay
git diff --stat
```

## Agent Stop Rule

After implementation and validation, report results and STOP.

Do not commit, push, merge, switch branches, close M104, or begin M105. Manual acceptance and explicit user approval are required before Git closure.
