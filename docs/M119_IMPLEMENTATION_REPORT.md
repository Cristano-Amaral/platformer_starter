# Milestone 119 Implementation Report — Melee Hit Detection Foundation

## Implementation

`CharacterDefinition` now has an optional single melee box. The deterministic grammar is `melee_hit <start> <end> <cx> <cy> <cz> <hx> <hy> <hz>` in a Character definition; omission disables detection and preserves older definitions. Start and end are normalized resolved Attack progress (`0 <= start < end <= 1`), inclusive at start and exclusive at end. The center is a local offset; the three positive half extents describe a local 3D box. Values must be finite; center components are bounded to `[-100, 100]`, half extents to `[0.001, 100]`. Character Database exposes these fields in its existing working copy. Apply validates and promotes without Save, invalid Apply retains active definitions, and Save/Reload persists valid authoring.

The query rotates and scales the box by the attacker's *current* runtime world transform (`Rz * Ry * Rx` for placed instances). The Player uses its actual visual position and facing yaw, which already follows authoritative Player movement. A query tests live Player and placed NPC/Enemy center spheres in full 3D. Placed instances use their actual world positions and a radius derived from their runtime visual scale; the Player uses a fixed 0.5 unit radius near its visual width. These are gameplay query regions, not Jolt bodies or visible actors. The sphere approximation is intentionally narrow because placed characters currently have presentation transforms but no gameplay collision shape.

Contact identities are session-local: Player is `1`; a placed instance is its unique `CharacterInstance` handle plus one. `Application::MeleeContacts()` exposes newly contacted `(attacker, target)` pairs for the current simulation step. Application evaluates the start and end transforms of a positive simulation advance when the elapsed interval intersects the window, so a step crossing the entire window still detects a stationary overlap before Attack expiry. `RuntimeHealth` stores contacted target identities for one Attack. Requesting a later Attack clears the set. Completion, explicit clear, Hit Reaction interruption, Defeat, health reset, respawn, and placed-instance rebuild clear transient state. Repeated Attack requests still fail under M118 authority. Contacts neither call Damage nor change Health, AttackPower/Defense, feedback, reaction, Defeat, knockback, or any other combat effect. M120 retains Damage integration ownership.

F1 Runtime Health diagnostics show configured/active hit window, contact count and last target identity alongside existing Attack resolution diagnostics. No debug volume draw was added; the current diagnostics and explicit world-space test assertions provide the narrow observability for this milestone.

## Architectural findings

The repository has no placed-character gameplay collider; `CharacterInstance` owns a transient presentation world transform, and NPC/Enemy runtime actors borrow it. A simple 3D sphere target query uses those existing runtime positions without adding a physics actor or generalized shape system. M118 previously cleared only Player Attack on Fall respawn. M119 clears placed Attack/contact state on every respawn as required by this milestone; other Health state still follows existing respawn rules.

The target spheres approximate character occupancy rather than mesh or physics collision. Very thin or unusually scaled visuals can therefore contact earlier or later than their visible surface. The current query samples spatial transforms at simulation step boundaries and does not sweep moving geometry between them. Debug wireframes and spatial swept contact are deferred. The prescribed validation builds Windows only; Linux/mobile compilation was not part of M119 acceptance.

## Automated coverage

- `GameplayDefinitionTest`: missing-field compatibility, parse/write/parse equality and invalid/duplicate melee grammar.
- `CharacterDatabaseEditorTest`: Apply without Save, invalid melee Apply preserving active state, Save/Reload.
- `PlayerHealthTest`: exact hit-window boundaries and a step crossing the complete window, oriented 3D box query, self-exclusion, distinct target memory, later Attack and Defeated cleanup.
- `CharacterPlacementTest`: real Application Character Database Apply without Save and invalid melee Apply preservation; production Player and placed-instance queries, no contact outside the window, a step crossing the complete window, multi-target first contact, continuous overlap and exit/re-entry de-duplication, subsequent Attack, reaction interrupt, Manual/Fall respawn and stable counts, with Health/feedback/reaction unchanged by contact.

## Validation

Final results on `milestone/119-melee-hit-detection-foundation`:

| Command / suite | Result |
| --- | --- |
| `cmake --preset windows-vs2022` | PASS |
| `cmake --build --preset windows-debug` | PASS |
| `cmake --build --preset windows-development` | PASS |
| `cmake --build --preset windows-release` | PASS |
| Debug focused C++ executables | 18/18 PASS |
| Development C++ executables | 73/73 PASS |
| Release focused C++ executables | 18/18 PASS |
| `python tools/test_milestone_docs.py` | 7/7 PASS |
| `python tools/test_agent_instructions.py` | 9/9 PASS |
| `python tools/test_stage_runtime_assets.py` | 13/13 PASS |
| `python tools/test_cook_level_v1.py` | 29/29 PASS |
| `python tools/test_cook_runtime_png.py` | 23/23 PASS |
| `python tools/test_import_static_glb.py` | 13/13 PASS |
| `python tools/test_stage_world_shaders.py` | 4/4 PASS |

Final builds used the prescribed CMake presets through the ignored `build/m118-validation/run_with_normalized_env.py` process wrapper with `/m:4 /nr:false`. The wrapper removes duplicate inherited case variants of `PATH` for MSBuild; parallel workers ran outside the restricted sandbox. The first unwrapped Debug attempt failed with MSBuild `MSB6001` because inherited `PATH` and `Path` collided; the final wrapped Debug build passed. An intermediate placement regression failed because its fixture patrol moved manually positioned targets back to authored origins; the fixture was corrected and final Debug, Development, and Release runs pass. These workarounds changed no project source or machine environment settings.

`git diff --check` exited 0. Git emitted only its existing LF/CRLF working-copy warnings; no line endings were normalized. `git diff --stat` reported 17 tracked files, 417 insertions and 20 deletions; three untracked files are excluded from that stat.

## Canonical data audit

The required `git diff` checks for canonical `level_01.level`, `level_02.level`, and `definitions.gameplay` were all empty. No Attack or melee assignment was persisted to canonical content. The supplied `MILESTONE_119.md` was already untracked before implementation; its Status and Branch lines were updated after validation. The other untracked files are this report and `MeleeHitDetection.h`.

## Manual acceptance setup

1. In Development, confirm Q with the default Player definition (no Attack or melee fields) is safe.
2. In F2 Character Database, assign `animations/humanoid_jump` as Player Attack and enable melee with window `0.25`–`0.75`, local center `(0,0,1)`, half extents `(0.5,0.8,0.5)`. **Apply without Save**. The Jump clip is a temporary test action.
3. Place two compatible NPC/Enemy targets at different positions in front of the Player through the Level editor's transient working placement/Apply path, or use an already loaded test placement. Use F1 Runtime Health diagnostics to observe the window and count. Confirm no contact outside the 3D box or before/after the window, one contact per target during overlap and re-entry, and another contact on the next Q Attack.
4. Check Player and target Health, feedback, and reaction remain unchanged by the contacts. Use the existing F1 direct Damage control to trigger a valid Hit Reaction or lethal Defeat during Attack and confirm the window closes. Temporarily assign a compatible Attack and melee box to an NPC/Enemy, Apply, and use its F1 Attack button to check the same query toward Player.
5. Use real respawn/restart and verify zero stale contacts and stable placed-instance counts. Discard working edits or restart without Save. Preserve canonical authored files.

Manual acceptance and explicit Git closure approval remain with the User. No commit, push, merge, closure, or M120 work was performed.

## Changed files

`AGENTS.md`, `docs/ARCHITECTURE.md`, `docs/MILESTONES.md`, `docs/M119_IMPLEMENTATION_REPORT.md`, `docs/milestones/MILESTONE_119.md`, `game/source/core/Application.cpp`, `game/source/core/Application.h`, `game/source/editor/CharacterDatabaseEditor.cpp`, `game/source/editor/CharacterDatabaseEditorTest.cpp`, `game/source/gameplay/CharacterDefinition.h`, `game/source/gameplay/GameplayDefinitionFile.cpp`, `game/source/gameplay/GameplayDefinitionTest.cpp`, `game/source/gameplay/MeleeHitDetection.h`, `game/source/gameplay/PlayerHealthTest.cpp`, `game/source/gameplay/RuntimeHealth.h`, `game/source/render/CharacterPlacementTest.cpp`, `game/source/render/LevelCharacters.h`, `game/source/ui/debug/DebugUi.cpp`, `tools/test_agent_instructions.py`, and `tools/test_milestone_docs.py`.
