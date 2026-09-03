#include "ui/debug/DebugUiBackend.h"

#if defined(PLATFORMER_ENABLE_DEBUG_UI)

#include "imgui.h"
#include "raylib.h"
#include "rlImGui.h"

namespace ui
{
DebugUiBackend::~DebugUiBackend()
{
    Shutdown();
}

void DebugUiBackend::Initialize()
{
    if (initialized)
    {
        return;
    }

    rlImGuiSetup(true);
    ImGui::GetIO().IniFilename = nullptr;
    initialized = true;
}

void DebugUiBackend::Shutdown()
{
    if (!initialized)
    {
        return;
    }

    rlImGuiShutdown();
    initialized = false;
}

bool DebugUiBackend::ConsumeTogglePressed()
{
    return IsKeyPressed(KEY_F1);
}

bool DebugUiBackend::WantsKeyboardCapture() const
{
    if (!initialized)
    {
        return false;
    }

    // Reflects the most recently completed ImGui frame. Application polls
    // input before this frame's rlImGuiBegin, which is the normal way to ask
    // whether the UI is currently swallowing keys.
    return ImGui::GetIO().WantCaptureKeyboard;
}

void DebugUiBackend::BeginFrame()
{
    if (!initialized)
    {
        return;
    }

    rlImGuiBegin();
}

void DebugUiBackend::EndFrame()
{
    if (!initialized)
    {
        return;
    }

    rlImGuiEnd();
}
}

#else

namespace ui
{
DebugUiBackend::~DebugUiBackend() = default;
void DebugUiBackend::Initialize() {}
void DebugUiBackend::Shutdown() {}
void DebugUiBackend::BeginFrame() {}
void DebugUiBackend::EndFrame() {}
bool DebugUiBackend::ConsumeTogglePressed()
{
    return false;
}
bool DebugUiBackend::WantsKeyboardCapture() const
{
    return false;
}
}

#endif
