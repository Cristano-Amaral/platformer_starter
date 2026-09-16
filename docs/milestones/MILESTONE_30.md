## Milestone 30 — Level Data v1: Data-Driven Single Level

### Status
**Phase A — repository inspection and Level Data design/scaffolding (current).** Do not mark complete until Phase B, Phase C manual validation, and Git closure are finished. Do not start Milestone 31.

### Branch
`milestone/30-level-data-v1`

### Recommended Cursor model
**Grok 4.6 High — Fast OFF**

### Goal
Introduce the first project-owned **data-driven level definition** without adding multiple playable levels or an editor yet.

M30 converts the current hard-coded M29 world layout into one explicit `LevelDefinition` loaded/constructed as project data, while preserving the exact gameplay behavior validated through M29.

This milestone is the architectural bridge toward:

- multiple levels;
- a Development level editor;
- configurable player/camera/world parameters;
- future enemy and boss spawn data;
- asset packaging by level;
- Web/Android/iOS/embedded portability.

M30 does **not** implement those future systems yet.

---

### Player-visible contract
The game should look and play the same as M29.

Expected current route and systems remain intact:

- player spawn;
- static platforms;
- slopes;
- moving platform;
- two checkpoints;
- two hazards;
- three collectibles;
- goal;
- cyan dynamic physics box;
- camera behavior;
- TIME;
- persistent BEST;
- Release HUD;
- restart/respawn semantics.

The architectural change should not require the player to learn any new controls.

---

### Core architectural objective
Create a project-owned level-data boundary so gameplay systems no longer define the current level by scattering canonical world coordinates/specifications across Application/Renderer/Physics initialization.

The canonical geometry/gameplay layout for the current level should come from one `LevelDefinition` or a comparably small set of level-data structures.

The definition should describe **what the level contains**. Runtime systems remain responsible for **how those objects behave**.

Examples:

- Level data says where a checkpoint is and its trigger/respawn positions.
- Application still decides checkpoint progression semantics.
- Level data says where a hazard is.
- Application still decides that hazard contact causes Hazard respawn.
- Level data says where the goal is.
- Application still owns completion state and BEST logic.
- Level data says moving-platform path/speed.
- Physics/runtime still performs kinematic motion and CharacterVirtual carry.

---

### M30 LevelDefinition scope
The exact structs should be decided after repository inspection, but M30 should centralize the current level's immutable authoring data, including at minimum:

1. player initial spawn;
2. static boxes/platforms;
3. slope definitions;
4. moving-platform definition;
5. checkpoint definitions;
6. hazard definitions;
7. collectible definitions;
8. goal definition;
9. cyan dynamic-box initial definition;
10. level camera tuning that is currently canonical for this level, if inspection confirms it is level-specific rather than global engine policy.

Do not blindly put every constant in `LevelDefinition`. Physics constants, player movement constants, global rendering policy, save paths, UI positions, etc. are not level authoring data.

---

### Level identity
Introduce a minimal stable identity for the current level, for example:

`level_01`

or an equivalent project-owned identifier.

M30 still has exactly **one playable level**.

Do not build a campaign, level-selection screen, level progression system, unlock system, world map, or next-level transition.

---

### Source format
M30 should first establish the **runtime data model**, not prematurely commit the project to a large external serialization/editor format.

Preferred M30 direction:

- project-owned C++ `LevelDefinition` data;
- one canonical factory/data source such as `CreateLevel01Definition()` or equivalent;
- runtime systems consume that definition;
- no JSON dependency;
- no external level parser required yet.

The structures must nevertheless be designed so a later editor/file format can populate the same conceptual data without changing gameplay semantics.

If repository inspection finds a compelling existing lightweight data mechanism, Cursor should report it during Phase A before changing this direction.

---

### Ownership boundaries
#### Level data
Owns immutable authoring/configuration data for the current level.

#### Application
Owns run state and gameplay meaning:

- checkpoint progression;
- deaths/respawn reason;
- completion;
- collectible collected flags;
- run timer;
- session/persistent BEST integration;
- restart priority/order.

#### PhysicsWorld
Consumes relevant level geometry/physics definitions and creates runtime bodies.

#### Renderer
Consumes level data/runtime state read-only for visuals.

#### Player
Keeps player movement/CharacterVirtual responsibilities.

#### Platform
Keeps OS/path/time/window/file-replacement responsibilities.

#### Persistence
Keeps M29 BEST save responsibilities only.

---

### Runtime state must remain separate from authoring data
Do not put mutable run state into `LevelDefinition`.

Examples that must remain outside immutable level data:

- active checkpoint index;
- collected flags;
- death count;
- completion flag;
- current moving-platform position/phase;
- cyan-box current transform;
- current player transform;
- current TIME;
- BEST;
- load/save statuses.

A fresh run should instantiate/reset runtime state from the immutable level definition where appropriate.

---

### Restart/respawn preservation
Preserve all M20–M29 semantics.

Ordinary respawn from R/Fall/Hazard:

- teleports/reset Player as before;
- preserves active checkpoint;
- preserves collectibles;
- preserves completion;
- preserves timer behavior;
- preserves BEST;
- does not reset moving platform;
- does not reset cyan box.

Enter fresh restart:

- resets run-owned state;
- restores Player from level initial spawn;
- restores moving platform from its level definition;
- restores cyan dynamic box from its level definition;
- clears checkpoints/deaths/completion/collectibles/current TIME;
- preserves persistent/session BEST.

---

### Static validation
Where useful, keep or migrate compile-time/runtime validation that protects authored data assumptions.

Examples:

- exactly two checkpoints for the current M30 level;
- exactly two hazards;
- exactly three collectibles;
- valid positive sizes;
- checkpoint indices/order consistent;
- spawn/respawn points valid;
- level identity non-empty;
- moving-platform path valid.

Do not build a generic schema-validation framework.

---

### Debug/Development
Add a small Level Data section to existing DebugMetrics/ImGui showing useful read-only information such as:

- level id/name;
- player initial spawn;
- static box count;
- slope count;
- checkpoint count;
- hazard count;
- collectible count;
- goal presence;
- moving platform presence;
- dynamic-box presence.

If camera tuning is moved into level data, show the effective values.

Do not create the level editor in M30.

---

### Release
Release behavior must remain equivalent to M29:

- no ImGui;
- same playable level;
- same HUD;
- persistent BEST works;
- runtime assets remain executable-relative;
- save remains user-data-relative;
- no dependency on CWD.

---

### Portability
Level data must be project-owned standard C++ and contain no:

- Win32 types/APIs;
- raylib types in gameplay/authoring structures;
- filesystem policy;
- Android/iOS/Web APIs.

Prefer existing project-owned math types such as `core::Vec3` where suitable.

This is important for the future Web/Android/iOS/Raspberry Pi targets and Development editor.

---

### Asset pipeline
M30 does not change the cooker or runtime asset packaging.

Do not implement packs yet.

Current cooked/staged assets continue working exactly as in M29.

---

### Save compatibility
M29 save v1 must remain valid without migration.

M30 must not change:

- `PLATFORMER_SAVE 1`;
- `best_seconds` semantics;
- `%LOCALAPPDATA%/Platformer3D/best_time_v1.txt` Windows policy;
- load/save timing.

The current save does not need a level id because M30 still has one playable level and persists only the global/session best established by M29.

Do not create save v2.

---

### Phase A — Repository inspection and Level Data design/scaffolding
Cursor must inspect the actual M29 code before implementation and report:

- every location that currently owns canonical level coordinates/specs;
- duplicate geometry/spec data between Application, PhysicsWorld, Renderer, GreyboxWorld or other files;
- exact M29 initialization/restart flow;
- what is immutable authoring data vs mutable runtime state;
- proposed LevelDefinition structures;
- exact ownership and file placement;
- how PhysicsWorld will consume level geometry;
- how Renderer will consume level data;
- how Application will consume checkpoints/hazards/collectibles/goal;
- moving-platform definition/runtime split;
- cyan-box definition/runtime split;
- whether camera settings should be level data now;
- migration plan preserving exact M29 coordinates/behavior;
- validation strategy;
- future editor compatibility without implementing editor/file parsing;
- build impact.

Phase A may add compile-only structs/factories and documentation, but the game must not yet switch to the new definition if doing so would make the migration partially live.

Phase A scaffolding (M29 gameplay remains live):
- `world/LevelDefinition.h`: immutable aggregate (`level_01`) reusing `Box`, `SlopeSpec`, `MovingPlatformSpec`, `CheckpointSpec`, `HazardSpec`, `CollectibleSpec`, `LevelGoalSpec`;
- `world/Level01.cpp`: `CreateLevel01Definition()` copies current live world constants (not a second authoring source);
- Initialize checks that the factory matches live greybox/spawn/checkpoints/hazards/collectibles/goal, PlatformerCamera framing defaults, and the cyan box initial pose;
- no Application/Renderer/PhysicsWorld consumer migration;
- no DebugMetrics Level Data panel yet (Phase B);
- no editor, JSON, multi-level, or LevelManager.

Build Debug/Development/Release.

Do not commit/push/merge.

---

### Phase B — Data-driven single-level implementation
After Phase A approval:

- create the canonical Level 01 definition;
- migrate current authored level constants into it;
- make Application/PhysicsWorld/Renderer consume the definition through clean boundaries;
- preserve runtime state separately;
- remove obsolete duplicated canonical level constants;
- preserve all M29 behavior;
- add DebugMetrics level-data diagnostics;
- update documentation;
- build all configurations;
- smoke Development and Release from unrelated CWD.

M30 remains incomplete until Phase C.

**Phase B:** Implementation complete, awaiting Phase C manual validation. `game/source/world/Level01.cpp` is the canonical authored Level 01 source. `CreateLevel01Definition()` populates literals directly (no copy from legacy `kGround` / `kCheckpoints` / similar globals). Application owns one immutable `LevelDefinition` constructed at member initialization. PhysicsWorld creates statics/slopes/moving platform/cyan box from that definition. Renderer draws from the same definition plus runtime state. Debug/Development expose a Level Data section. M29 save v1 is unchanged. No editor, external level file, LevelManager, or second level.

---

### Phase C — Manual validation
At minimum validate:

1. initial spawn unchanged;
2. complete traversal remains possible;
3. static platforms/slopes unchanged;
4. moving platform path/speed/carry unchanged;
5. cyan box behavior unchanged;
6. CP1/CP2 progression and respawns unchanged;
7. both hazards unchanged;
8. all three collectibles unchanged;
9. goal/completion unchanged;
10. R/Fall/Hazard priority unchanged;
11. Enter fresh restart resets from level definition correctly;
12. TIME behavior unchanged;
13. M29 BEST loads/saves unchanged;
14. existing M29 save v1 remains compatible;
15. Debug/Development Level Data metrics correct;
16. Release has no ImGui;
17. Development/Release work from unrelated CWD;
18. no new gameplay controls;
19. no external level file/editor exists yet;
20. only one playable level exists.

---

### Explicitly out of scope
M30 must NOT implement:

- Milestone 31;
- multiple playable levels;
- campaign progression;
- level selection;
- next-level transition;
- level editor/editor mode;
- gizmos;
- external JSON/YAML/TOML level files;
- generic serialization framework;
- enemies;
- enemy spawns;
- bosses;
- combat;
- health;
- audio/music systems;
- asset packs;
- asset streaming;
- Web build;
- Android/iOS port;
- Raspberry Pi port;
- save v2;
- per-level BEST records;
- ECS;
- event bus;
- LevelManager/GameManager/WorldManager unless a concrete current requirement proves one is necessary.

---

### Completion criteria
M30 is complete only when:

- the current level has one canonical project-owned data definition;
- runtime systems consume that definition instead of owning duplicated authoring coordinates;
- immutable level data and mutable run state are clearly separated;
- the game behaves equivalently to M29;
- M29 save v1 remains compatible;
- Debug/Development expose useful level-data diagnostics;
- Debug/Development/Release build successfully;
- unrelated-CWD smoke passes;
- Phase C manual validation passes;
- no M31 scope has been implemented.
