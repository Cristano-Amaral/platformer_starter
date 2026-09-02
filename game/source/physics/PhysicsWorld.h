#pragma once

#include "core/Vec3.h"

#include <memory>

namespace physics
{
struct DynamicTestBox
{
    core::Vec3 position{};
    core::Vec3 size{1.0f, 1.0f, 1.0f};
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

class PhysicsWorld
{
public:
    PhysicsWorld();
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;
    PhysicsWorld(PhysicsWorld&&) = delete;
    PhysicsWorld& operator=(PhysicsWorld&&) = delete;

    bool Initialize();
    bool InitializePlayer(core::Vec3 visualCenter, core::Vec3 visualSize);
    void ResetCharacter(const core::Vec3& visualCenter, const core::Vec3& velocity);
    void UpdateMovingPlatform(float deltaSeconds);
    void MovePlayer(const PlayerMoveCommand& command, float deltaSeconds);
    void Update(float deltaSeconds);
    void Shutdown();

    bool IsInitialized() const;
    int StaticBodyCount() const;
    bool IsDynamicTestBodyValid() const;
    DynamicTestBox GetDynamicTestBox() const;
    MovingPlatformState GetMovingPlatform() const;
    PlayerPhysicsState GetPlayerPhysicsState() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
