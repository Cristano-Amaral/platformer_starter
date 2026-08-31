#pragma once

#include "gameplay/PlatformerCamera.h"
#include "gameplay/Player.h"
#include "platform/Window.h"
#include "render/Renderer.h"

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
    gameplay::Player player{{0.0f, 0.8f, 0.0f}, {0.8f, 1.6f, 0.8f}};
    gameplay::PlatformerCamera camera;
    bool initialized = false;
};
}
