# M118 Attack Foundation — implementation report

Branch: `milestone/118-attack-foundation`. Automated implementation validation passed. Manual acceptance and Git closure remain pending. No commit, push, merge, milestone closure, or M119 work occurred.

## Implemented behavior and architecture

- Optional embedded `animation_attack` and reusable `animation_attack_asset` assignments reuse compatibility validation, humanoid mapping, retargeting, Character Preview and CharacterInstance resolution. Explicit invalid reusable assignments never fall back to an embedded clip. Older authored definitions remain valid.
- RuntimeHealth owns a narrow transient Attack countdown alongside the existing Hit Reaction countdown. Presentation supplies the resolved clip duration at the existing 60 Hz animation sampling rate. Playback clamps for one action duration, regardless of reusable asset looping. Requests require Alive state, a resolvable positive-duration action, and no active Attack or Hit Reaction. Repeated requests are ignored without restart, buffering, combos or queues.
- Player **Q** sets semantic `InputState::attackPressed` and requests Attack through Application's existing gameplay simulation guard. Player physics/locomotion continue. The special Player retains its existing Renderer presentation path; no second Player instance is created.
- NPC/Enemy `CharacterInstance::RequestAttack()` is the shared runtime entry point. F1 Runtime Health exposes a small per-actor Attack button, state, remaining time and resolution reason. Player diagnostics also show resolution. Patrol phase, actual position and runtime/render transform hold for every frame beginning in Attack, including expiry, then resume without catch-up.
- Positive nonlethal damage with a valid Hit Reaction cancels Attack. Damage without a resolved reaction retains Attack and still produces M116 feedback. Lethal damage clears Attack immediately; Defeated actors cannot request it. Health Reset, New Run, Restart, Manual respawn, death completion and placed-instance rebuilds clear transient state. Fall respawn explicitly clears Player Attack while preserving its existing Health and placed-actor lifecycle authority.
- No attack path selects targets, performs spatial detection or applies damage. Existing direct/debug/Hazard damage paths remain separate.

## Character Database authority

Inspection found Save/Reload and saved-catalog promotion on editor exit, but no Character Database Apply-only command. M118 adds the explicit Apply boundary needed by the requested transient setup:

- Working edits remain pending until **Apply** validates and promotes a candidate to active definitions.
- Apply does not write authored files or clear the saved baseline/Dirty flag. Application reloads Player presentation, rebuilds placed instances and clears Attack.
- **Save** persists authored state; it does not promote changed Character assignments, including on editor exit.
- Exit retains active Character state and Character Database source-mapping edits while keeping existing saved Item/Animation catalog updates. Failed saved-catalog reload retains the valid applied snapshot.

This is a narrow Character Database command using existing registry serialization/validation and Application ownership, without a general editor transaction framework.

## Regression coverage

Updated C++ tests:

- `GameplayDefinitionTest`: optional Attack grammar, deterministic round trip and duplicate-slot rejection.
- `CharacterDatabaseEditorTest`: assignment authoring, Apply without Save, invalid Apply preserving active state, saved assignment persistence, source mapping authority and unrelated saved Item/Animation updates.
- `CharacterPreviewTest`: embedded/reusable Attack preview and invalid explicit assignment without embedded fallback.
- `PlayerCharacterStatsTest`: countdown, repeated request rejection, damage feedback without reaction, valid reaction interruption, defeat priority, reset and completion.
- `CharacterInstanceTest`: valid action and changing sampled pose, missing/invalid/incompatible fallback preserving locomotion presentation, completion, reset, repeated requests and retargeting.
- `CharacterPlacementTest`: production Application input without target damage, Player rendered animation selection/completion, incompatible and retargeted resolution, NPC/Enemy actual patrol phase/position/transform freeze and resume, interruption, lethal priority, Defeated rejection, active-action Manual/Fall respawn, stable instance counts, real Character Apply/reapply/exit and Save-only pending state using a temporary file.

Existing animation, equipment, placement, patrol, Health/Damage, Death/Defeat, Damage Feedback, Hit Reaction and lifecycle tests remain green. Python milestone/instruction expectations now recognize M118.

## Exact final validation commands and results

Configure:

```powershell
cmake --preset windows-vs2022
```

PASS.

All configurations built successfully, including the game and all test targets:

```powershell
python build/m118-validation/run_with_normalized_env.py cmake --build --preset windows-debug -- /m:4 /nr:false
python build/m118-validation/run_with_normalized_env.py cmake --build --preset windows-development -- /m:4 /nr:false
python build/m118-validation/run_with_normalized_env.py cmake --build --preset windows-release -- /m:4 /nr:false
```

C++ execution, from repository root:

```powershell
python build/m118-validation/run_with_normalized_env.py python build/m118-validation/run_cpp_tests.py Development all
python build/m118-validation/run_with_normalized_env.py python build/m118-validation/run_cpp_tests.py Debug
python build/m118-validation/run_with_normalized_env.py python build/m118-validation/run_cpp_tests.py Release
```

| Configuration | Executables | Result |
| --- | ---: | --- |
| Development | All 73 `*Test.exe` executables | 73 PASS |
| Debug | 18 focused executables | 18 PASS |
| Release | 18 focused executables | 18 PASS |

Focused executables: AnimationLibraryTest, CharacterAssetValidatorTest, CharacterDatabaseEditorTest, CharacterPreviewTest, CharacterInstanceTest, CharacterPlacementTest, GameplayDefinitionTest, PlayerCharacterStatsTest, PlayerPresentationTest, PlayerAnimationAssetTest, EquipmentAttachmentTest, EquipmentTest, InventoryTest, PlayerHealthTest, SkeletalAnimationTest, PhysicsRebuildTest, GameFlowTest and InventoryUiTest.

Python validation:

```powershell
python tools/test_milestone_docs.py
python tools/test_agent_instructions.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_stage_world_shaders.py
```

All seven suites PASS. Cooked assets were already present; no source asset changes or additional asset cooking setup was needed.

Git checks:

```powershell
git branch --show-current
git status
git diff --check
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
git diff -- game/assets/source/gameplay/definitions.gameplay
git diff --stat
```

Expected branch confirmed; diff check clean; all three protected authored-data diffs empty. No canonical files were restored or normalized.

## Process-only validation workarounds and resolved failures

- The initial unwrapped `cmake --build --preset windows-development` failed with MSBuild MSB6001 because inherited environment keys contained both `PATH` and `Path`. The wrapper creates a case-normalized child environment only; it does not edit machine/user environment settings or source.
- Parallel MSBuild required execution outside the restricted sandbox. `/m:4` supplies worker parallelism; `/nr:false` disables persistent worker reuse for final builds. No CMake or IDE project definition was changed for this.
- The initial Development suite run inside the sandbox passed 72/73 executables; the existing EditorToolRunnerTest could not complete its child shell commands. Rerunning outside the sandbox passed all 73, including EditorToolRunnerTest (4.3 seconds). HostProcess source was not changed.
- Intermediate implementation regressions found an outdated idle-animation snapshot in the expanded patrol test and the new Apply/exit path losing reusable animations when a saved catalog could not load. The fixture snapshot was corrected and the applied-catalog preservation behavior was fixed; final tests above pass.
- Validation wrappers, drivers, full per-test logs and scratch artifacts are retained under ignored `build/m118-validation/`, not as project source changes.

## Canonical authored-data audit

`level_01.level`, `level_02.level` and `definitions.gameplay` have empty diffs. Tests use in-memory assignments, existing animation fixtures and temporary files. No authored Attack assignment was saved to the canonical catalog. No levels, models or animation assets were added or modified.

## Manual acceptance setup and remaining work

Manual acceptance remains the User's responsibility:

1. In Development, verify Player Q with no Attack assignment is safe.
2. In Character Database, temporarily assign `animations/humanoid_jump` to Player Attack (compatible with `models/player.glb`). Use **Apply without Save**, exit F2, press Q and verify visible action/completion, normal locomotion and unchanged target health. This Jump clip is a test action, not a new authored combat animation.
3. For NPC/Enemy patrol tests, use compatible temporary definitions and placements through Apply-only authoring. An exact fixture can use `models/player.glb` with existing humanoid Idle/Move/Jump assets; an existing mapped retarget target can use its compatible retarget asset. `animations/humanoid_jump` is incompatible with `characters/retarget_target` because its source mapping is unavailable; use the mapped retarget asset for that character. Make patrol active and use F1 Runtime Health **Attack** buttons.
4. Verify patrol position/phase hold and resume, use small nonlethal Damage with a compatible Hit Reaction to interrupt, use lethal Damage to verify priority, and use real reset/respawn to verify clearing. Check red feedback remains separate and no residual animation/tint, duplicate instances or patrol jump remains.
5. Restore temporary working assignments or restart the process without saving. No temporary authored setup is currently persisted. Any later manual saved changes must be restored before Git closure.

Intentionally deferred: spatial hits, hitboxes/hurtboxes, weapon collision, automatic damage, combat formulas, combat AI, combos, resources, cooldown frameworks, generalized animation/action systems and all M119 features. No default Attack animation is shipped in canonical definitions. Linux/mobile builds were not run; Windows is the prescribed validation target.

## Files changed

- [AGENTS.md](C:/dev/platformer_cursor_starter/AGENTS.md)
- [README.md](C:/dev/platformer_cursor_starter/README.md)
- [docs/ARCHITECTURE.md](C:/dev/platformer_cursor_starter/docs/ARCHITECTURE.md)
- [docs/M118_IMPLEMENTATION_REPORT.md](C:/dev/platformer_cursor_starter/docs/M118_IMPLEMENTATION_REPORT.md)
- [docs/MILESTONES.md](C:/dev/platformer_cursor_starter/docs/MILESTONES.md)
- [docs/milestones/MILESTONE_118.md](C:/dev/platformer_cursor_starter/docs/milestones/MILESTONE_118.md)
- [game/source/animation/CharacterAssetValidator.cpp](C:/dev/platformer_cursor_starter/game/source/animation/CharacterAssetValidator.cpp)
- [game/source/animation/CharacterAssetValidator.h](C:/dev/platformer_cursor_starter/game/source/animation/CharacterAssetValidator.h)
- [game/source/core/Application.cpp](C:/dev/platformer_cursor_starter/game/source/core/Application.cpp)
- [game/source/core/Application.h](C:/dev/platformer_cursor_starter/game/source/core/Application.h)
- [game/source/editor/CharacterDatabaseEditor.cpp](C:/dev/platformer_cursor_starter/game/source/editor/CharacterDatabaseEditor.cpp)
- [game/source/editor/CharacterDatabaseEditor.h](C:/dev/platformer_cursor_starter/game/source/editor/CharacterDatabaseEditor.h)
- [game/source/editor/CharacterDatabaseEditorTest.cpp](C:/dev/platformer_cursor_starter/game/source/editor/CharacterDatabaseEditorTest.cpp)
- [game/source/editor/CharacterPreview.cpp](C:/dev/platformer_cursor_starter/game/source/editor/CharacterPreview.cpp)
- [game/source/editor/CharacterPreview.h](C:/dev/platformer_cursor_starter/game/source/editor/CharacterPreview.h)
- [game/source/editor/CharacterPreviewTest.cpp](C:/dev/platformer_cursor_starter/game/source/editor/CharacterPreviewTest.cpp)
- [game/source/gameplay/CharacterDefinition.h](C:/dev/platformer_cursor_starter/game/source/gameplay/CharacterDefinition.h)
- [game/source/gameplay/EnemyRuntime.h](C:/dev/platformer_cursor_starter/game/source/gameplay/EnemyRuntime.h)
- [game/source/gameplay/GameplayDefinitionFile.cpp](C:/dev/platformer_cursor_starter/game/source/gameplay/GameplayDefinitionFile.cpp)
- [game/source/gameplay/GameplayDefinitionTest.cpp](C:/dev/platformer_cursor_starter/game/source/gameplay/GameplayDefinitionTest.cpp)
- [game/source/gameplay/NpcRuntime.h](C:/dev/platformer_cursor_starter/game/source/gameplay/NpcRuntime.h)
- [game/source/gameplay/PlayerCharacterStatsTest.cpp](C:/dev/platformer_cursor_starter/game/source/gameplay/PlayerCharacterStatsTest.cpp)
- [game/source/gameplay/RuntimeHealth.h](C:/dev/platformer_cursor_starter/game/source/gameplay/RuntimeHealth.h)
- [game/source/input/Input.cpp](C:/dev/platformer_cursor_starter/game/source/input/Input.cpp)
- [game/source/input/InputState.h](C:/dev/platformer_cursor_starter/game/source/input/InputState.h)
- [game/source/render/CharacterInstance.cpp](C:/dev/platformer_cursor_starter/game/source/render/CharacterInstance.cpp)
- [game/source/render/CharacterInstance.h](C:/dev/platformer_cursor_starter/game/source/render/CharacterInstance.h)
- [game/source/render/CharacterInstanceTest.cpp](C:/dev/platformer_cursor_starter/game/source/render/CharacterInstanceTest.cpp)
- [game/source/render/CharacterPlacementTest.cpp](C:/dev/platformer_cursor_starter/game/source/render/CharacterPlacementTest.cpp)
- [game/source/render/Renderer.cpp](C:/dev/platformer_cursor_starter/game/source/render/Renderer.cpp)
- [game/source/render/Renderer.h](C:/dev/platformer_cursor_starter/game/source/render/Renderer.h)
- [game/source/ui/debug/DebugMetrics.h](C:/dev/platformer_cursor_starter/game/source/ui/debug/DebugMetrics.h)
- [game/source/ui/debug/DebugUi.cpp](C:/dev/platformer_cursor_starter/game/source/ui/debug/DebugUi.cpp)
- [tools/test_agent_instructions.py](C:/dev/platformer_cursor_starter/tools/test_agent_instructions.py)
- [tools/test_milestone_docs.py](C:/dev/platformer_cursor_starter/tools/test_milestone_docs.py)
