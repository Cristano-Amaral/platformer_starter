#include "ui/debug/DebugUiBackend.h"

#if defined(PLATFORMER_ENABLE_DEBUG_UI)

#include "editor/EditorLayout.h"
#include "editor/EditorSnap.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "raylib.h"
#include "rlImGui.h"

#include <cstring>
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

    static ImGuiSettingsHandler snapHandler;
    snapHandler.TypeName = editor::kEditorSnapIniTypeName;
    snapHandler.TypeHash = ImHashStr(snapHandler.TypeName);
    snapHandler.ReadOpenFn = [](ImGuiContext*, ImGuiSettingsHandler*, const char* name) -> void* {
        if (name == nullptr || std::strcmp(name, editor::kEditorSnapIniEntryName) != 0)
        {
            return nullptr;
        }
        editor::EditorSnapPreferences* prefs = editor::BoundEditorSnapPreferences();
        return prefs == nullptr ? nullptr : prefs;
    };
    snapHandler.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line) {
        if (entry == nullptr || line == nullptr)
        {
            return;
        }
        editor::ParseEditorSnapPreferenceLine(
            *static_cast<editor::EditorSnapPreferences*>(entry), line);
        editor::SanitizeEditorSnapPreferences(
            *static_cast<editor::EditorSnapPreferences*>(entry));
    };
    snapHandler.WriteAllFn = [](ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* outBuf) {
        editor::EditorSnapPreferences* prefs = editor::BoundEditorSnapPreferences();
        if (prefs == nullptr || handler == nullptr || outBuf == nullptr)
        {
            return;
        }
        const std::string section = editor::SerializeEditorSnapPreferencesSection(*prefs);
        outBuf->append(section.c_str(), section.c_str() + section.size());
        if (section.empty() || section.back() != '\n')
        {
            outBuf->append("\n");
        }
    };
    ImGui::AddSettingsHandler(&snapHandler);

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

bool DebugUiBackend::WantsTextInput() const
{
    if (!initialized)
    {
        return false;
    }

    return ImGui::GetIO().WantTextInput;
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
bool DebugUiBackend::WantsTextInput() const
{
    return false;
}
}

#endif
