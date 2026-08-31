#pragma once

namespace ui
{
class DebugUiBackend
{
public:
    DebugUiBackend() = default;
    ~DebugUiBackend();

    DebugUiBackend(const DebugUiBackend&) = delete;
    DebugUiBackend& operator=(const DebugUiBackend&) = delete;
    DebugUiBackend(DebugUiBackend&&) = delete;
    DebugUiBackend& operator=(DebugUiBackend&&) = delete;

    void Initialize();
    void Shutdown();
    void BeginFrame();
    void EndFrame();
    bool ConsumeTogglePressed();

private:
    bool initialized = false;
};
}
