#include "core/Application.h"

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
        window.BeginFrame();
        window.ClearSolidBackground();
        window.EndFrame();
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
