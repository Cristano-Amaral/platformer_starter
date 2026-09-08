#pragma once

#include "core/Vec3.h"
#include "physics/PhysicsCapacity.h"

#include <memory>
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
    void Shutdown();

    bool IsInitialized() const;
    int StaticBodyCount() const;
    int DynamicBodyCount() const;
    std::vector<DynamicBoxRuntimeState> GetDynamicBoxes() const;
    MovingPlatformState GetMovingPlatform() const;
    PlayerPhysicsState GetPlayerPhysicsState() const;

private:
    friend struct PhysicsWorldTestAccess;
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
