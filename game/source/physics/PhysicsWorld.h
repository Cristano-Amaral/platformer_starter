#pragma once

#include "core/Vec3.h"

#include <memory>

namespace physics
{
struct DynamicTestBox
{
    core::Vec3 position{};
    core::Vec3 size{1.0f, 1.0f, 1.0f};
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
    void MovePlayer(const PlayerMoveCommand& command, float deltaSeconds);
    void Update(float deltaSeconds);
    void Shutdown();

    bool IsInitialized() const;
    DynamicTestBox GetDynamicTestBox() const;
    PlayerPhysicsState GetPlayerPhysicsState() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
