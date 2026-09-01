# Architecture

## Direction
The game is a 3D platformer with a side/platform-style presentation. The player moves in a constrained gameplay plane/track while the world may use full 3D geometry. Camera behavior belongs to gameplay, while camera/input/window implementation details stay behind engine/backend boundaries.

## Dependency direction
`gameplay -> core abstractions`
`ui -> core/gameplay public state`
`render backend -> raylib initially`
`physics backend -> Jolt CharacterVirtual / Jolt world`
`platform backend -> OS/raylib platform services`

Gameplay must not depend on backend implementation headers.

## Runtime path
```
semantic input
    ->
Player gameplay movement policy (Player-relative)
    ->
PhysicsWorld project-owned API
    ->
Jolt kinematic moving platform (MoveKinematic)
    ->
CharacterVirtual ground velocity + Player-relative velocity
    ->
Jolt CharacterVirtual / Jolt world
    ->
project-owned Player state
    ->
Renderer / PlatformerCamera / DebugMetrics
```

Jolt CharacterVirtual is the authoritative Player collision and physical-position backend. There is no second custom Player AABB collision path.

Player owns gameplay policy: semantic movement intent, horizontal acceleration/deceleration relative to supporting ground, vertical gameplay velocity, jump, coyote time, and jump buffer. PhysicsWorld owns Jolt runtime state, CharacterVirtual, static greybox bodies, the kinematic moving platform, and the dynamic test box.

## Shared greybox geometry
`world::GreyboxWorld` is the project-owned source of truth for current static level boxes (ground and elevated platforms). Renderer and PhysicsWorld both derive from those definitions. Ground and platform coordinates are not duplicated inside PhysicsWorld.

The test kinematic platform is specified by `world::kTestMovingPlatform` in `MovingPlatform.h`. Renderer and PhysicsWorld both derive from that spec. Path and size are not duplicated in PhysicsWorld.

Static test slopes are specified by `world::kWalkableSlope` (30 degrees) and `world::kSteepSlope` (60 degrees) in `Slope.h`. Renderer and PhysicsWorld both derive from those specs. CharacterVirtual max slope remains 50 degrees. Walkable `OnGround` is valid gameplay support; `OnSteepGround` is not.

## Moving ground
The Player rides kinematic ground through CharacterVirtual: `UpdateGroundVelocity` then `GetGroundVelocity`, added to Player-relative horizontal speed. The Player is not parented to the platform and does not receive a manual position delta.

## Physics boundary
Jolt types stay inside `PhysicsWorld.cpp`. Public physics headers expose only project-owned types (`PlayerMoveCommand`, `PlayerPhysicsState`, `PlayerGroundSupport`, `DynamicTestBox`, `MovingPlatformState`). The project has one physics backend: Jolt v5.6.0. There is no abstract `IPhysicsEngine`.

## Early abstraction points
Only abstract boundaries that are known to vary by target:
- application/platform lifecycle;
- input device mapping to semantic actions;
- filesystem/save location;
- timing;
- graphics/window backend exposure;
- physics integration boundary;
- asset location/loading.

Do not build a general-purpose engine before the game needs it.

## Asset pipeline
Authored files live in `game/assets/source/`. The Python cooker writes runtime-ready copies to `game/assets/cooked/`. The game never loads from `source/`.

Cook from the repository root:

    python tools/cook_assets.py

The cooker uses the Python standard library plus cooker-only **Pillow 12.3.0** (`python -m pip install -r tools/requirements.txt`; not a CMake or game runtime dependency). It processes known authored assets by explicit `KNOWN_ASSETS` identity, identifies content with SHA-256, skips rewriting an identical cooked file, and writes `game/assets/cooked/manifest.json` with portable relative paths (no absolute paths, timestamps, or machine names). Assets are not auto-discovered: a PNG under `source/textures/` is a runtime texture only if it is listed. Blender authoring PNGs (for example `test_textured_basecolor.png`) stay out of the cooker.

Standalone runtime PNGs (`kind: runtime_png`) use recipe `runtime_png.max512.lanczos.v1`: maximum dimension **512 px**, aspect ratio preserved, no upscale, no crop, Pillow `LANCZOS` when downscaling. Sources already within the limit are copied byte-for-byte. GLBs remain opaque byte copies; images embedded in `models/test_textured.glb` are not resized. Changing the recipe (max dimension, filter, or encoding) recooks even if source bytes are unchanged. See `tools/README.md`.

CMake does not cook. After configure, a POST_BUILD step stages cooked files next to the executable:

    <executable directory>/assets/textures/test_checker.png
    <executable directory>/assets/models/test_static.glb
    <executable directory>/assets/models/test_authored.glb
    <executable directory>/assets/models/test_textured.glb

Runtime lookup uses `platform::RuntimeAssetPath`, which joins `<executable directory>/assets/` with the logical relative path. The executable directory is queried from the OS in the platform layer (`GetModuleFileNameW` on Windows, `/proc/self/exe` on Linux). The process current working directory is never used, and non-absolute results are rejected so raylib file loads cannot silently resolve against CWD. The renderer does not hard-code `game/assets/cooked`.

Logical identities:
- Milestone 15 test texture: `textures/test_checker.png`
- Milestone 16 test model: `models/test_static.glb`
- Milestone 17 authored model: `models/test_authored.glb`
- Milestone 18 textured model: `models/test_textured.glb`

The M18 Base Color PNG `game/assets/source/textures/test_textured_basecolor.png` is Blender authoring input only. It is not cooked or staged. The exported GLB must embed the image. See `docs/BLENDER_WORKFLOW.md`.

Editable Blender files live in `game/assets/source/blender/`. They are not cooked and are not runtime assets. See `docs/BLENDER_WORKFLOW.md`. The cooker copies exported GLBs unchanged. Blender is not a build or runtime dependency.

The checker appears on a dedicated visual quad at `(0, 1.5, 2.5)`. The Milestone 16 test GLB is drawn at `(2.5, 1.0, 2.5)`. The Blender-authored GLB is drawn at `(-2.5, 1.0, 2.5)`. The Milestone 18 textured GLB is drawn at `(4.0, 1.0, 2.5)`. None of these are greybox geometry and none have collision.

The checker, Milestone 16 pyramid, Blender-authored model, and textured model are visual-only technical tests: no GreyboxWorld entries, no Jolt bodies, no mesh collision.

If a required cooked file is missing at CMake configure time, configure fails and tells the developer to run the cooker. If a staged runtime file is missing at load time, the renderer logs the logical id and draws a simple visual fallback. Texture and models are each loaded once after the window/graphics context exists and unloaded before window shutdown.
