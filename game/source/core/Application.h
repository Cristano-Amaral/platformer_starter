#pragma once

#include "gameplay/PlatformerCamera.h"
#include "gameplay/Player.h"
#include "platform/Window.h"
#include "render/Renderer.h"
#include "world/GreyboxWorld.h"

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
    bool initialized = false;
};
}
