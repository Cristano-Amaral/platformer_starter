#pragma once

// Dear ImGui helpers for M34 editor-window placement. Not compiled into
// Release UI stubs. Not a docking/workspace manager.

#include "editor/EditorLayout.h"

#include "imgui.h"

namespace editor
{
inline void ApplyKnownEditorWindowPlacement(
    const char* windowName,
    float viewportWidth,
    float viewportHeight,
    bool forceDefaultLayout)
{
    const EditorLayoutDefaults defaults =
        ComputeDefaultEditorLayout(viewportWidth, viewportHeight);
    const EditorWindowPlacement* placement = FindDefaultPlacement(defaults, windowName);
    if (placement == nullptr)
    {
        return;
    }

    const ImGuiCond condition = forceDefaultLayout ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
    ImGui::SetNextWindowPos(ImVec2(placement->x, placement->y), condition);
    ImGui::SetNextWindowSize(ImVec2(placement->width, placement->height), condition);
}

inline void RecoverKnownEditorWindowIfOffscreen(
    const char* windowName,
    float viewportWidth,
    float viewportHeight,
    bool recover)
{
    if (!recover)
    {
        return;
    }

    const ImVec2 pos = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    EditorWindowPlacement current{windowName, pos.x, pos.y, size.x, size.y};
    if (!EditorWindowNeedsClamp(current, viewportWidth, viewportHeight))
    {
        return;
    }

    const EditorWindowPlacement clamped =
        ClampEditorWindowPlacement(current, viewportWidth, viewportHeight);
    ImGui::SetWindowPos(ImVec2(clamped.x, clamped.y), ImGuiCond_Always);
    ImGui::SetWindowSize(ImVec2(clamped.width, clamped.height), ImGuiCond_Always);
}

inline void SnapKnownEditorWindowsToDefaults(float viewportWidth, float viewportHeight)
{
    const EditorLayoutDefaults defaults =
        ComputeDefaultEditorLayout(viewportWidth, viewportHeight);
    const EditorWindowPlacement* windows[] = {
        &defaults.metrics,
        &defaults.hierarchy,
        &defaults.inspector,
        &defaults.levelEditor};
    for (const EditorWindowPlacement* placement : windows)
    {
        ImGui::SetWindowPos(
            placement->name, ImVec2(placement->x, placement->y), ImGuiCond_Always);
        ImGui::SetWindowSize(
            placement->name, ImVec2(placement->width, placement->height), ImGuiCond_Always);
    }
}
}
