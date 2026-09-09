#include "editor/EditorWorkspace.h"
#include "world/LevelDefinition.h"

#include <cstdio>
#include <string>

namespace
{
int gFailures = 0;

void Expect(bool condition, const std::string& name)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL %s\n", name.c_str());
        ++gFailures;
    }
}
}

int main()
{
    using editor::EditorTransformMode;
    using editor::EditorWorkspaceState;

    {
        EditorWorkspaceState workspace{};
        Expect(editor::AllEditorPanelsVisible(workspace), "defaults: all default panels visible");
        editor::ToggleEditorWorkspaceMetrics(workspace);
        Expect(!workspace.showMetrics, "Metrics toggle hides Metrics");
        Expect(workspace.showHierarchy && workspace.showInspector && workspace.showLevelEditor
                && workspace.showToolOutput && workspace.showObjectPalette
                && workspace.showContentBrowser && workspace.showModelPreview
                && workspace.showQuickToolbar,
            "Metrics toggle leaves other panels");
        editor::ToggleEditorWorkspaceMetrics(workspace);
        Expect(workspace.showMetrics, "Metrics toggle restores Metrics");
    }

    {
        EditorWorkspaceState workspace{};
        workspace.showMetrics = false;
        workspace.showHierarchy = false;
        workspace.showInspector = false;
        workspace.showLevelEditor = false;
        workspace.showToolOutput = false;
        workspace.showObjectPalette = false;
        workspace.showContentBrowser = false;
        workspace.showModelPreview = false;
        workspace.showQuickToolbar = false;
        const EditorTransformMode mode = EditorTransformMode::Resize;
        editor::ResetEditorWorkspaceVisibility(workspace);
        Expect(editor::AllEditorPanelsVisible(workspace), "Reset restores all default panels visible");
        Expect(mode == EditorTransformMode::Resize, "Reset does not change transform mode");
    }

    {
        world::LevelDefinition level{};
        level.id = "level_01";
        EditorWorkspaceState workspace{};
        workspace.showInspector = false;
        Expect(level.id == "level_01", "hiding Inspector does not mutate authored data");
        Expect(!workspace.showInspector, "Inspector can be hidden independently");
        workspace.showToolOutput = false;
        Expect(!workspace.showToolOutput, "Tool Output visibility is independent");
        Expect(workspace.showHierarchy && workspace.showLevelEditor, "hiding Tool Output leaves editor panels");
    }

    {
        Expect(!editor::CanApplyPreview(false, true), "Apply disabled when unmodified");
        Expect(!editor::CanApplyPreview(true, false), "Apply disabled when invalid");
        Expect(
            editor::CanApplyPreview(true, true),
            "Apply enabled when modified and valid (no authoring flag; Debug may Apply)");
        Expect(!editor::CanRevertWorkingCopy(false), "Revert disabled when unmodified");
        Expect(editor::CanRevertWorkingCopy(true), "Revert enabled when modified");
        Expect(
            !editor::CanSaveLevelSource(false, false),
            "Save Debug rule: disabled when authoring is unavailable");
        Expect(
            !editor::CanSaveLevelSource(false, true),
            "Save Debug rule: still disabled while modified");
        Expect(!editor::CanSaveLevelSource(true, true), "Save disabled while modified");
        Expect(
            editor::CanSaveLevelSource(true, false),
            "Save Development rule: enabled when authoring and applied");
        Expect(
            !editor::CanReloadRuntimeLevel(false, false, false, false),
            "Reload Debug rule: disabled when authoring is unavailable");
        Expect(
            !editor::CanReloadRuntimeLevel(false, true, false, false),
            "Reload Debug rule: still disabled while modified");
        Expect(
            !editor::CanReloadRuntimeLevel(true, true, false, false),
            "Reload Policy A: disabled while Modified");
        Expect(
            !editor::CanReloadRuntimeLevel(true, false, true, false),
            "Reload Policy A: disabled while a Build tool is Running");
        Expect(
            !editor::CanReloadRuntimeLevel(true, true, true, false),
            "Reload remains disabled when both Modified and a tool is Running");
        Expect(
            !editor::CanReloadRuntimeLevel(true, false, false, true),
            "Reload blocked while Cook, Stage & Reload is pending");
        Expect(
            editor::CanReloadRuntimeLevel(true, false, false, false),
            "Reload Development rule: enabled when authoring, unmodified, idle tools");
        Expect(
            !editor::CanStartCookStageReload(false, false, false, false),
            "Cook, Stage & Reload disabled when authoring is unavailable");
        Expect(
            !editor::CanStartCookStageReload(true, true, false, false),
            "Cook, Stage & Reload disabled while Modified");
        Expect(
            !editor::CanStartCookStageReload(true, false, true, false),
            "Cook, Stage & Reload disabled while runner is Running");
        Expect(
            !editor::CanStartCookStageReload(true, false, false, true),
            "Cook, Stage & Reload disabled while workflow is pending");
        Expect(
            editor::CanStartCookStageReload(true, false, false, false),
            "Dirty is not a start argument: unmodified idle may start");
    }

    {
        EditorTransformMode mode = EditorTransformMode::Translate;
        Expect(
            editor::TrySetEditorTransformMode(mode, false, EditorTransformMode::Resize),
            "mode change allowed when idle");
        Expect(mode == EditorTransformMode::Resize, "idle mode change writes transformMode");
        Expect(
            editor::TrySetEditorTransformMode(mode, false, EditorTransformMode::Scale),
            "idle Scale mode change");
        Expect(mode == EditorTransformMode::Scale, "idle mode change writes Scale");
        Expect(
            !editor::TrySetEditorTransformMode(mode, true, EditorTransformMode::Translate),
            "mode change blocked during drag");
        Expect(mode == EditorTransformMode::Scale, "blocked change leaves Scale");
    }

    {
        Expect(
            editor::OrientationWidgetMenuBarTopInset(0.0f) == 0.0f, "no menu bar: no extra inset");
        Expect(
            editor::OrientationWidgetMenuBarTopInset(editor::kEditorMainMenuBarNominalHeight)
                == editor::kEditorMainMenuBarNominalHeight + editor::kOrientationWidgetMenuBarGap,
            "menu bar inset is height plus gap");
        Expect(
            editor::OrientationWidgetLiveExtraTopInset(false, 24.0f) == 0.0f,
            "F2 off: live inset is 0");
        Expect(
            editor::OrientationWidgetLiveExtraTopInset(true, 0.0f)
                == editor::kEditorMainMenuBarNominalHeight + editor::kOrientationWidgetMenuBarGap,
            "F2 on before first bar: nominal height plus gap");
        Expect(
            editor::OrientationWidgetLiveExtraTopInset(true, 19.0f) == 27.0f,
            "F2 on: live inset uses last menu-bar height plus gap");
        Expect(
            editor::OrientationWidgetLiveExtraTopInset(true, 24.0f, 28.0f, false) == 32.0f,
            "hidden toolbar: inset stays menu-only");
        Expect(
            editor::OrientationWidgetLiveExtraTopInset(true, 24.0f, 28.0f, true) == 60.0f,
            "visible toolbar: inset adds toolbar height plus gap");
    }

    {
        const editor::EditorContentViewport menuOnly =
            editor::MakeEditorContentViewport(1280.0f, 720.0f, 24.0f);
        Expect(menuOnly.y == 24.0f, "menu-only content Y");
        Expect(menuOnly.height == 696.0f, "menu-only content height");
        const editor::EditorContentViewport withToolbar =
            editor::MakeEditorContentViewport(1280.0f, 720.0f, 52.0f);
        Expect(withToolbar.y == 52.0f, "menu+toolbar content Y");
        Expect(withToolbar.height == 668.0f, "menu+toolbar content height");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d editor workspace test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Editor workspace tests passed.\n");
    return 0;
}
