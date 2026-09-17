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
    // raylib default: Esc closes the window. Placement mode and the player
    // Inventory UI need Esc without quitting; Application restores the default
    // when idle.
    void SetEscapeClosesWindow(bool enabled);
    // Requests the normal window-close path. Does not terminate the process
    // or skip Shutdown/destructors.
    void RequestClose();

private:
    bool initialized = false;
    bool closeRequested = false;
};
}
