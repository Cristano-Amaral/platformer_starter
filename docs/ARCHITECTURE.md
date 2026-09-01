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
