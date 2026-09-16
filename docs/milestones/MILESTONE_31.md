## Milestone 31 — External Level File v1

### Status
Phase B implementation complete. Not complete. Awaiting Phase C manual validation.

### Branch
`milestone/31-external-level-file-v1`

### Goal
Move the authored data for the single playable `level_01` out of hard-coded C++ values and into one project-owned external level file, while preserving the M30 `LevelDefinition` as the runtime data model and preserving all M30 gameplay exactly.

This milestone establishes the first real **authoring file → validated LevelDefinition → runtime** path. It is the bridge toward the future Development Level Editor and multiple levels, but it does **not** implement either feature yet.

### Player-visible contract
The game should look and behave exactly like M30:

- same Level 01 geometry and route;
- same Player spawn;
- same checkpoints, hazards, collectibles and goal;
- same moving platform and cyan dynamic box;
- same camera framing;
- same TIME/BEST/COLLECTED/completion behavior;
- same persistent BEST save;
- same Release behavior.

The architectural difference is that Level 01 authored values are loaded from an external runtime level file rather than being compiled into `Level01.cpp`.

### Canonical data flow

```text
Level authoring file
        ↓
strict project-owned loader/parser
        ↓
validated world::LevelDefinition
        ↓
Application
   ├── gameplay meaning/state
   ├── PhysicsWorld
   └── Renderer
```

`world::LevelDefinition` remains the portable runtime representation introduced in M30.

### Scope

#### 1. One external Level 01 file
Create exactly one authored level file for `level_01`.

Preferred project layout:

```text
assets/source/levels/level_01.level
        ↓ cooker/staging
assets/cooked/levels/level_01.level
        ↓ runtime staging
<exe>/assets/levels/level_01.level
```

The exact extension/location may be refined during Phase A if repository inspection shows a cleaner fit, but the level must participate in the existing source → cooked → staged asset pipeline.

#### 2. Project-owned text format v1
Use a small, explicit, versioned, human-readable project format.

Do not add JSON/YAML/TOML/XML or a third-party serialization dependency.

The format must include:

- magic/header;
- format version 1;
- level ID;
- initial spawn;
- kill plane;
- ground;
- six elevated platforms;
- two slopes;
- moving platform;
- two checkpoints;
- two hazards;
- three collectibles;
- goal;
- cyan dynamic box;
- camera offset/FOV.

The final grammar must be documented and deterministic.

#### 3. Strict parser
Implement a project-owned parser that rejects malformed content rather than silently guessing.

Reject at minimum:

- wrong magic;
- unsupported version;
- missing required sections/fields;
- duplicate singleton fields;
- invalid numeric parsing;
- NaN/Inf;
- invalid sizes;
- invalid counts;
- invalid moving-platform path/speed;
- invalid cyan-box mass;
- invalid FOV;
- trailing malformed/unrecognized content according to the chosen grammar.

#### 4. Load status
Provide a small typed result/status suitable for Debug/Development diagnostics, conceptually distinguishing:

- Loaded;
- Missing;
- Invalid;
- UnsupportedVersion;
- Error.

Do not create a generic asset error framework.

#### 5. Failure policy
A missing or invalid canonical Level 01 file is a **fatal initialization error** for this milestone.

Do not silently fall back to a compiled copy of Level 01, because that would recreate two canonical authoring sources and could hide packaging/editor errors.

Failure should be reported clearly in Debug/Development and via the existing application/platform error path as appropriate, then initialization should fail cleanly.

#### 6. Remove compiled Level 01 authored values
After the migration, `Level01.cpp` must no longer contain a second full copy of the Level 01 coordinates.

It may be removed or reduced to a path/bootstrap helper if justified.

Changing CP1, the goal, cyan-box initial position, camera framing, etc. should require editing the external Level 01 authoring file only.

#### 7. Runtime model remains M30
Do not redesign `LevelDefinition` merely because it is now loaded.

The loader populates the existing project-owned types.

Runtime state remains separate:

- active checkpoint;
- respawn position;
- collected flags;
- completion;
- death count;
- moving-platform runtime pose/direction;
- cyan-box runtime physics state;
- timer;
- BEST.

#### 8. Existing asset pipeline
Integrate the level file with the existing cooker/staging process.

For v1, cooking may be a validated/canonicalized copy if no transformation is required, but it must be handled deliberately by the cooker and manifest rather than bypassing the pipeline.

The runtime must load the staged/cooked level file using executable-relative runtime asset paths, not the working directory.

#### 9. Incremental cooker
Preserve the existing incremental/hash behavior where applicable.

Changing the source Level 01 file should cause the cooked Level 01 output to update; unchanged input should not be unnecessarily rewritten if the existing cooker architecture supports that contract.

#### 10. Camera
Persist/load only the M30 level-authored camera values:

- offset;
- FOV.

Dead-zone and smoothing/sharpness remain global controller policy.

#### 11. Save compatibility
M29 save v1 remains unchanged:

```text
PLATFORMER_SAVE 1
best_seconds <double>
```

Do not add level ID and do not create save v2.

#### 12. Debug metrics
Development/Debug should expose read-only level loading information, including at minimum:

- level source/runtime path;
- load status;
- format version if loaded;
- Level ID;
- existing M30 Level Data metrics.

No editor controls yet.

#### 13. Release
Release must load the external staged Level 01 file and remain CWD-independent.

Release must not contain Dear ImGui.

A properly packaged Release therefore depends on its runtime asset content, including the level file.

### Architecture rules

- Application owns the active `LevelDefinition`.
- Loader/parser owns syntax and validation, not gameplay meaning.
- PhysicsWorld derives Jolt runtime representations from `LevelDefinition`.
- Renderer consumes `LevelDefinition` read-only.
- No raylib/Jolt/Win32 types in the level format/runtime data model.
- Filesystem/runtime path policy remains behind existing project/platform boundaries.
- No generic serialization framework.
- No LevelManager/SceneManager/EntityManager/ECS/event bus.

### Portability
The format and parser must use portable C++ and project-owned types so the same level content can later be used on Windows, Web, Raspberry Pi, Android and iOS.

Platform-specific differences in runtime filesystem/access can be handled behind platform/runtime-asset boundaries later without changing the level grammar or gameplay systems.

### Future editor compatibility
This milestone deliberately establishes the file format that a future Development editor can eventually write.

However, M31 does **not** implement:

- editor mode;
- gizmos;
- object selection;
- property panels;
- save-from-editor;
- undo/redo.

The loader should not depend on editor code.

### Future multi-level compatibility
The format must carry a level ID and be capable of representing another `LevelDefinition` later, but M31 still loads exactly one canonical `level_01` path.

Do not add:

- Level 02;
- registry;
- campaign;
- level selection;
- next-level transition.

### Validation
Build and validate:

- Debug;
- Development;
- Release;
- Development from unrelated CWD;
- Release from unrelated CWD.

Manual Phase C must regress the full M30 route and also validate:

- valid external level load;
- level load diagnostics;
- CWD independence;
- source edit → cooker → runtime effect using one safe temporary authored value change, then restore it;
- missing level file fails cleanly;
- malformed level file fails cleanly;
- unsupported version fails cleanly;
- restored valid file launches normally;
- no compiled fallback hides failures.

### Explicitly out of scope

- Milestone 32;
- multiple playable levels;
- campaign progression;
- level transitions;
- Development editor;
- gizmos;
- external editor application;
- enemies;
- bosses;
- combat;
- health;
- audio systems;
- asset packs;
- asset streaming;
- Web build;
- Android/iOS/Raspberry Pi ports;
- save v2;
- per-level BEST;
- JSON/YAML/TOML/XML;
- third-party serialization libraries;
- generic serialization/reflection;
- LevelManager/SceneManager/ECS/event bus.

### Completion criteria
M31 is complete only when:

1. the external Level 01 file is the sole canonical authored source for the current level;
2. it passes through the cooker/staging pipeline;
3. the runtime loads and strictly validates it into `LevelDefinition`;
4. missing/invalid/unsupported content fails cleanly without compiled fallback;
5. all M30 gameplay is manually regression-tested successfully;
6. Debug/Development diagnostics expose the load state;
7. Release remains CWD-independent and loads packaged level data;
8. M29 persistent BEST remains compatible;
9. no editor or multiple-level system has been prematurely introduced;
10. Phase C is approved before commit/merge.

### Phase A — Inspection, format design, parser scaffolding
**Phase A:** Complete. Parser, cooker/staging, canonical source file, and `LevelFileTest` were added while live gameplay still used `CreateLevel01Definition()`. That compiled factory was removed in Phase B.

### Phase B — Live runtime migration
**Phase B:** Implementation complete, awaiting Phase C manual validation.

The sole live authored source for Level 01 is:

```text
game/assets/source/levels/level_01.level
        → cooker (level_v1, header check + byte copy)
game/assets/cooked/levels/level_01.level
        → CMake POST_BUILD staging
<exe>/assets/levels/level_01.level
        → LoadLevelFile / ParseLevelText
world::LevelDefinition
```

`LevelDefinition.id` is an owning `std::string`. Application owns `levelDefinition{}` until Initialize. It loads `platform::RuntimeAssetPath("levels/level_01.level")` once, requires `Loaded` plus `id == "level_01"`, then initializes camera framing, respawn, PhysicsWorld, and Player from the loaded definition. Player is constructed at `{0,0,0}` only as a pre-load placeholder and is initialized from the file spawn before any playable frame. `Level01.cpp` and `CreateLevel01Definition()` are gone. Missing/invalid/unsupported/I/O/wrong-ID Level 01 fails Initialize with stderr diagnostics and no compiled fallback. BEST save remains nonfatal and still loads after successful required init. RestartRun and ordinary respawns do not reread the file. PhysicsWorld/Renderer remain data consumers. Debug/Development add a Level Loading section; Release HUD is unchanged. Format v1 is unchanged. No editor, no LevelManager, no second level.

Do not commit/push/merge until Phase C is approved. Do not mark M31 complete.

### Final status

Complete (manually approved) and merged as `Milestone 31 - External level file v1`.

---

---
