#include "core/Application.h"

#include "input/Input.h"
#include "platform/Time.h"

namespace core
{
int Application::Run()
{
    Initialize();
    if (!initialized)
    {
        return 1;
    }

    while (!window.ShouldClose())
    {
        const float deltaSeconds = platform::DeltaSeconds();
        const input::InputState inputState = input::Poll();
        player.Update(inputState, deltaSeconds);
        renderer.Draw(player, camera);
    }

    Shutdown();
    return 0;
}

void Application::Initialize()
{
    initialized = window.Initialize();
}

void Application::Shutdown()
{
    window.Shutdown();
    initialized = false;
}
}
