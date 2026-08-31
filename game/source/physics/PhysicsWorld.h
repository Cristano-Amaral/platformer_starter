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
    void Update(float deltaSeconds);
    void Shutdown();

    bool IsInitialized() const;
    DynamicTestBox GetDynamicTestBox() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
