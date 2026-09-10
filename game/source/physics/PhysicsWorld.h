#pragma once

#include "core/Vec3.h"
#include "physics/DynamicBoxGrab.h"
#include "physics/PhysicsCapacity.h"
#include "world/Door.h"
#include "world/PressurePlate.h"

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace world
{
struct LevelDefinition;
}

namespace physics
{
struct DynamicBoxRuntimeState
{
    core::Vec3 center{};
    core::Vec3 size{};
    core::Vec3 linearVelocity{};
    core::Vec3 angularVelocity{};
    float massKg = 0.0f;
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    float rotationZ = 0.0f;
    float rotationW = 1.0f;
    bool valid = false;
    bool active = false;
};

struct PressurePlateRuntimeState
{
    core::Vec3 center{};
    core::Vec3 size{};
    bool active = false;
};

struct DoorRuntimeState
{
    core::Vec3 closedCenter{};
    core::Vec3 center{};
    core::Vec3 size{};
    float openDistance = 0.0f;
    float openFraction = 0.0f;
    bool desiredOpen = false;
    bool blockedClosing = false;
    bool unlocked = true;
    bool valid = false;
};

enum class PlayerGroundSupport
{
    OnGround,
    OnSteepGround,
    NotSupported,
    InAir,
};

struct PlayerMoveCommand
{
    float horizontalVelocity = 0.0f;
    float verticalVelocity = 0.0f;
};

struct PlayerPhysicsState
{
    core::Vec3 visualCenter{};
    float horizontalVelocity = 0.0f;
    float verticalVelocity = 0.0f;
    bool supported = false;
    PlayerGroundSupport groundSupport = PlayerGroundSupport::InAir;
    int contactCount = 0;
    bool characterInitialized = false;
    core::Vec3 groundVelocity{};
    bool supportingGroundMoving = false;
    core::Vec3 groundNormal{};
    float groundSlopeAngleDegrees = 0.0f;
    bool currentSupportWalkable = false;
    core::Vec3 worldVelocity{};
    const char* supportBodyKind = "None";
    bool dynamicContact = false;
    bool characterInnerBodyActive = false;
};

struct MovingPlatformState
{
    core::Vec3 position{};
    core::Vec3 size{};
    core::Vec3 velocity{};
    float direction = 1.0f;
    float pathMinX = 0.0f;
    float pathMaxX = 0.0f;
    float speed = 0.0f;
    bool valid = false;
};

struct PhysicsWorldTestAccess;

class PhysicsWorld
{
public:
    PhysicsWorld();
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;
    PhysicsWorld(PhysicsWorld&&) = delete;
    PhysicsWorld& operator=(PhysicsWorld&&) = delete;

    bool Initialize(const world::LevelDefinition& level);
    bool InitializePlayer(core::Vec3 visualCenter, core::Vec3 visualSize);
    // Build a replacement world first; swap only on success so a failed
    // rebuild leaves this instance intact. Used by Apply Preview and reload.
    bool TryRebuild(
        const world::LevelDefinition& level,
        core::Vec3 playerVisualCenter,
        core::Vec3 playerVisualSize);
    void ResetCharacter(const core::Vec3& visualCenter, const core::Vec3& velocity);
    void ResetMovingPlatform();
    // Full run restart: authored pose, zero linear/angular velocity.
    // Checkpoint respawn does not call this.
    void ResetDynamicBoxes();
    // Per-body kill-plane recovery using the applied authored kill plane copied
    // at Initialize / TryRebuild. Restores only bodies whose runtime center Y
    // is strictly below that plane. Does not rebuild PhysicsWorld and does not
    // mutate authored Dynamic Box specs.
    void RecoverFallenDynamicBoxes();
    void UpdateMovingPlatform(float deltaSeconds);
    void MovePlayer(const PlayerMoveCommand& command, float deltaSeconds);
    void Update(float deltaSeconds);
    // Runtime-only Grab / Carry. Does not mutate authored Dynamic Box specs.
    // facingX should be +1 or -1 (player facing along the gameplay X axis).
    void SetGrabAim(core::Vec3 playerVisualCenter, float facingX);
    void HandleGrabDrop();
    void ClearCarry();
    DynamicBoxGrabState GetGrabState() const;
    // Session lock flags owned by Application. Size should match applied Doors.
    // Missing entries fall back to authored requiresKey. Does not snap motion.
    void SetDoorRuntimeUnlocked(std::span<const std::uint8_t> unlocked);
    // Solid world LOS used by M51 grab and M55 pickup. Ignores Dynamic Boxes
    // and the CharacterVirtual inner body. Static Props have no Jolt body.
    bool WorldSolidBlocksSegment(core::Vec3 from, core::Vec3 to) const;
    // Same LOS, ignoring the named applied Door body so a locked Door can be
    // targeted without its own solid blocking the ray to its center.
    bool WorldSolidBlocksSegmentIgnoringDoor(
        core::Vec3 from, core::Vec3 to, int doorIndex) const;
    void Shutdown();

    bool IsInitialized() const;
    int StaticBodyCount() const;
    int DynamicBodyCount() const;
    std::vector<DynamicBoxRuntimeState> GetDynamicBoxes() const;
    std::vector<PressurePlateRuntimeState> GetPressurePlates() const;
    std::vector<DoorRuntimeState> GetDoors() const;
    int DoorBodyCount() const;
    MovingPlatformState GetMovingPlatform() const;
    PlayerPhysicsState GetPlayerPhysicsState() const;

private:
    friend struct PhysicsWorldTestAccess;
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
