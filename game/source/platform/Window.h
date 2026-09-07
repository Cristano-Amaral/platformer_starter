#pragma once

namespace platform
{
class Window
{
public:
    Window() = default;
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    bool Initialize();
    void Shutdown();
    bool ShouldClose() const;
    int Width() const;
    int Height() const;
    // raylib default: Esc closes the window. Placement mode needs Esc to
    // cancel without quitting; Application restores the default when idle.
    void SetEscapeClosesWindow(bool enabled);

private:
    bool initialized = false;
};
}
