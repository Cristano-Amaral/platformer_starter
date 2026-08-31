#pragma once

#include "gameplay/PlatformerCamera.h"
#include "gameplay/Player.h"
#include "physics/PhysicsWorld.h"
#include "platform/Window.h"
#include "render/Renderer.h"
#include "world/GreyboxWorld.h"

#if defined(PLATFORMER_ENABLE_DEBUG_UI)
#include "ui/debug/DebugUi.h"
#endif

namespace core
{
class Application
{
public:
    int Run();

private:
    void Initialize();
    void Shutdown();

    platform::Window window;
    render::Renderer renderer;
    gameplay::Player player{
        {0.0f, world::GroundSurfaceY() + 1.6f * 0.5f, 0.0f},
        {0.8f, 1.6f, 0.8f}};
    gameplay::PlatformerCamera camera;
    physics::PhysicsWorld physicsWorld;
#if defined(PLATFORMER_ENABLE_DEBUG_UI)
    ui::DebugUi debugUi;
#endif
    bool initialized = false;
};
}
