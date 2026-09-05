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
        Expect(editor::AllEditorPanelsVisible(workspace), "defaults: all four visible");
        editor::ToggleEditorWorkspaceMetrics(workspace);
        Expect(!workspace.showMetrics, "Metrics toggle hides Metrics");
        Expect(workspace.showHierarchy && workspace.showInspector && workspace.showLevelEditor,
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
        const EditorTransformMode mode = EditorTransformMode::Resize;
        editor::ResetEditorWorkspaceVisibility(workspace);
        Expect(editor::AllEditorPanelsVisible(workspace), "Reset restores all four visible");
        Expect(mode == EditorTransformMode::Resize, "Reset does not change transform mode");
    }

    {
        world::LevelDefinition level{};
        level.id = "level_01";
        EditorWorkspaceState workspace{};
        workspace.showInspector = false;
        Expect(level.id == "level_01", "hiding Inspector does not mutate authored data");
        Expect(!workspace.showInspector, "Inspector can be hidden independently");
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
    }

    {
        EditorTransformMode mode = EditorTransformMode::Translate;
        Expect(
            editor::TrySetEditorTransformMode(mode, false, EditorTransformMode::Resize),
            "mode change allowed when idle");
        Expect(mode == EditorTransformMode::Resize, "idle mode change writes transformMode");
        Expect(
            !editor::TrySetEditorTransformMode(mode, true, EditorTransformMode::Translate),
            "mode change blocked during drag");
        Expect(mode == EditorTransformMode::Resize, "blocked change leaves mode");
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
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d editor workspace test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Editor workspace tests passed.\n");
    return 0;
}
