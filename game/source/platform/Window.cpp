#include "platform/Window.h"

#include "raylib.h"

namespace platform
{
namespace
{
constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;
constexpr int kTargetFps = 60;
constexpr const char* kWindowTitle = "Platformer3D";
}

bool Window::Initialize()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(kWindowWidth, kWindowHeight, kWindowTitle);
    if (!IsWindowReady())
    {
        return false;
    }

    SetExitKey(KEY_ESCAPE);
    SetTargetFPS(kTargetFps);
    initialized = true;
    return true;
}

void Window::Shutdown()
{
    if (!initialized)
    {
        return;
    }

    CloseWindow();
    initialized = false;
}

Window::~Window()
{
    Shutdown();
}

bool Window::ShouldClose() const
{
    return WindowShouldClose();
}
}
