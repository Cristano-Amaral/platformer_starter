#include "ui/debug/DebugUiBackend.h"

#if defined(PLATFORMER_ENABLE_DEBUG_UI)

#include "editor/EditorLayout.h"
#include "imgui.h"
#include "raylib.h"
#include "rlImGui.h"

#include <filesystem>

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
    // Set after CreateContext and before the first NewFrame. Default ImGui
    // would otherwise write ./imgui.ini in the process CWD.
    iniFilenameStorage.clear();
    const std::filesystem::path layoutPath = editor::EditorLayoutPath();
    if (!layoutPath.empty())
    {
        editor::EnsureEditorLayoutDirectory();
        iniFilenameStorage = layoutPath.string();
        ImGui::GetIO().IniFilename = iniFilenameStorage.c_str();
    }
    else
    {
        ImGui::GetIO().IniFilename = nullptr;
    }
    initialized = true;
}

void DebugUiBackend::SaveIniSettings()
{
    if (!initialized || iniFilenameStorage.empty())
    {
        return;
    }

    ImGui::SaveIniSettingsToDisk(iniFilenameStorage.c_str());
}

void DebugUiBackend::Shutdown()
{
    if (!initialized)
    {
        return;
    }

    rlImGuiShutdown();
    iniFilenameStorage.clear();
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

bool DebugUiBackend::WantsMouseCapture() const
{
    if (!initialized)
    {
        return false;
    }

    return ImGui::GetIO().WantCaptureMouse;
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
void DebugUiBackend::SaveIniSettings() {}
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
bool DebugUiBackend::WantsMouseCapture() const
{
    return false;
}
}

#endif
