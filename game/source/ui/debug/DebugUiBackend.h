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
    // True while an ImGui widget owns the keyboard (typing in a field). Kept
    // here so ImGui knowledge stays in the debug-UI backend rather than
    // leaking into input or gameplay. False where ImGui is not compiled in.
    bool WantsKeyboardCapture() const;
    // Same boundary for the mouse: clicks, drags and wheel on an ImGui panel
    // must not look, pick, or dolly the editor world behind it.
    bool WantsMouseCapture() const;

private:
    bool initialized = false;
};
}
