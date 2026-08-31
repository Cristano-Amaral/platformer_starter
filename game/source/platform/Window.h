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
    void BeginFrame();
    void EndFrame();
    void ClearSolidBackground();

private:
    bool initialized = false;
};
}
