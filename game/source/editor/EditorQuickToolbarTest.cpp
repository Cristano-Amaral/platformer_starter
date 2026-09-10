#include "editor/EditorBuildPreference.h"
#include "editor/EditorGizmo.h"
#include "editor/EditorOrientation.h"
#include "editor/EditorPicking.h"
#include "editor/EditorPlacement.h"
#include "editor/EditorToolCommands.h"
#include "editor/EditorWorkspace.h"
#include "editor/LevelEditor.h"
#include "render/CameraView.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
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

bool NearlyEqual(float a, float b, float epsilon = 0.0001f)
{
    return std::fabs(a - b) <= epsilon;
}

bool Vec3Near(core::Vec3 a, core::Vec3 b, float epsilon = 0.0001f)
{
    return NearlyEqual(a.x, b.x, epsilon) && NearlyEqual(a.y, b.y, epsilon)
        && NearlyEqual(a.z, b.z, epsilon);
}

std::filesystem::path MakeTempDir()
{
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / "platformer3d_m43_toolbar";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    return root;
}
}

int main()
{
    using editor::EditorBuildTarget;
    using editor::EditorContentViewport;
    using editor::EditorToolKind;
    using editor::EditorTransformMode;
    using editor::LevelEditorRequest;

    {
        editor::EditorWorkspaceState workspace{};
        Expect(workspace.showQuickToolbar, "default Quick Toolbar visible");
        Expect(editor::AllEditorPanelsVisible(workspace), "defaults include Quick Toolbar");
        workspace.showQuickToolbar = false;
        Expect(!editor::AllEditorPanelsVisible(workspace), "View toggle hides toolbar independently");
        Expect(workspace.showObjectPalette, "hiding toolbar leaves Object Palette");
        Expect(workspace.showContentBrowser, "hiding toolbar leaves Content Browser");
        workspace.showQuickToolbar = true;
        Expect(workspace.showQuickToolbar, "View toggle shows toolbar");
        workspace.showQuickToolbar = false;
        editor::ResetEditorWorkspaceVisibility(workspace);
        Expect(workspace.showQuickToolbar, "Reset restores default toolbar visibility");
        Expect(editor::AllEditorPanelsVisible(workspace), "Reset restores all default panels");
    }

    {
        EditorTransformMode mode = EditorTransformMode::Translate;
        Expect(
            editor::TrySetEditorTransformMode(mode, false, EditorTransformMode::Resize),
            "toolbar Resize writes canonical TransformMode");
        Expect(mode == EditorTransformMode::Resize, "canonical mode is Resize");
        Expect(
            editor::TrySetEditorTransformMode(mode, false, EditorTransformMode::Translate),
            "toolbar Translate writes canonical TransformMode");
        Expect(mode == EditorTransformMode::Translate, "canonical mode is Translate");
        Expect(
            editor::TrySetEditorTransformMode(mode, false, EditorTransformMode::Scale),
            "toolbar Scale writes canonical TransformMode");
        Expect(mode == EditorTransformMode::Scale, "canonical mode is Scale");
        Expect(
            editor::TrySetEditorTransformMode(mode, false, EditorTransformMode::Rotate),
            "toolbar Rotate writes canonical TransformMode");
        Expect(mode == EditorTransformMode::Rotate, "canonical mode is Rotate");
        Expect(
            editor::TrySetEditorTransformMode(mode, false, EditorTransformMode::Translate),
            "toolbar Translate restores after Rotate");
        Expect(mode == EditorTransformMode::Translate, "canonical mode is Translate after Rotate");
        Expect(
            !editor::TrySetEditorTransformMode(mode, true, EditorTransformMode::Resize),
            "toolbar cannot bypass drag lock");
        Expect(mode == EditorTransformMode::Translate, "drag leaves canonical Translate");
    }

    {
        Expect(
            editor::QuickToolbarApplyPreviewRequest() == LevelEditorRequest::ApplyPreview,
            "toolbar Apply is canonical ApplyPreview");
        Expect(
            editor::QuickToolbarRevertWorkingCopyRequest() == LevelEditorRequest::RevertWorkingCopy,
            "toolbar Revert is canonical RevertWorkingCopy");
        Expect(
            editor::QuickToolbarSaveLevelSourceRequest() == LevelEditorRequest::SaveLevelSource,
            "toolbar Save is canonical SaveLevelSource");
        Expect(
            editor::CanApplyPreview(true, true) && !editor::CanApplyPreview(false, true),
            "toolbar Apply enable matches canonical CanApplyPreview");
        Expect(
            editor::CanSaveLevelSource(true, false) && !editor::CanSaveLevelSource(true, true),
            "toolbar Save enable matches canonical CanSaveLevelSource");
        Expect(
            editor::IsResizeSelection({editor::EditorObjectKind::ElevatedPlatform, 0})
                && !editor::IsResizeSelection({editor::EditorObjectKind::Collectible, 0}),
            "Resize availability is canonical IsResizeSelection");
        Expect(
            editor::IsScaleSelection({editor::EditorObjectKind::StaticProp, 0})
                && !editor::IsScaleSelection({editor::EditorObjectKind::ElevatedPlatform, 0}),
            "Scale availability is canonical IsScaleSelection");
        Expect(
            editor::IsRotateSelection({editor::EditorObjectKind::StaticProp, 0})
                && editor::IsRotateSelection({editor::EditorObjectKind::ItemPickup, 0})
                && !editor::IsRotateSelection({editor::EditorObjectKind::ElevatedPlatform, 0}),
            "Rotate availability is canonical IsRotateSelection");
    }

    {
        Expect(
            editor::ParseEditorBuildTarget("") == EditorBuildTarget::Development,
            "first-run/empty persisted value is Development");
        Expect(
            editor::kDefaultEditorBuildTarget == EditorBuildTarget::Development,
            "default enum is Development");
        Expect(
            editor::EditorToolKindForBuildTarget(EditorBuildTarget::Debug)
                == EditorToolKind::BuildDebug,
            "Debug maps to Build Debug");
        Expect(
            editor::EditorToolKindForBuildTarget(EditorBuildTarget::Development)
                == EditorToolKind::BuildDevelopment,
            "Development maps to Build Development");
        Expect(
            editor::EditorToolKindForBuildTarget(EditorBuildTarget::Release)
                == EditorToolKind::BuildRelease,
            "Release maps to Build Release");
        Expect(
            editor::EditorToolKindForBuildTarget(EditorBuildTarget::All)
                == EditorToolKind::BuildAll,
            "All maps to Build All");
        Expect(
            editor::EditorBuildSelectionEditableWhileRunning(),
            "combo stays editable; next job only");
        Expect(
            editor::CanStartEditorToolJob(true, false),
            "Run can start when runner is idle");
        Expect(
            !editor::CanStartEditorToolJob(true, true),
            "busy runner cannot start a second job");
    }

    {
        const std::filesystem::path temp = MakeTempDir();
        const std::filesystem::path path = editor::MakeEditorBuildSelectionPath(temp);
        Expect(!path.empty(), "build selection path joins user-data/Platformer3D");
        Expect(
            editor::LoadEditorBuildSelectionFromPath(path) == EditorBuildTarget::Development,
            "missing file falls back to Development");

        const EditorBuildTarget values[] = {
            EditorBuildTarget::Debug,
            EditorBuildTarget::Development,
            EditorBuildTarget::Release,
            EditorBuildTarget::All};
        for (EditorBuildTarget value : values)
        {
            Expect(editor::SaveEditorBuildSelectionToPath(path, value), "save build selection");
            Expect(
                editor::LoadEditorBuildSelectionFromPath(path) == value,
                std::string("round-trip ") + editor::EditorBuildTargetName(value));
        }

        {
            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            out << "NotAConfig\n";
        }
        Expect(
            editor::LoadEditorBuildSelectionFromPath(path) == EditorBuildTarget::Development,
            "invalid persisted value falls back to Development");

        EditorBuildTarget selected = EditorBuildTarget::All;
        editor::EditorWorkspaceState workspace{};
        workspace.showQuickToolbar = false;
        editor::ResetEditorWorkspaceVisibility(workspace);
        Expect(workspace.showQuickToolbar, "Reset restores toolbar visibility");
        Expect(
            selected == EditorBuildTarget::All,
            "Reset Editor Layout does not reset last build selection");
        std::filesystem::remove_all(temp);
    }

    {
        const float windowW = 1280.0f;
        const float windowH = 720.0f;
        const float menu = 24.0f;
        const float toolbar = 28.0f;

        const EditorContentViewport menuOnly = editor::MakeEditorContentViewport(
            windowW, windowH, editor::LiveEditorChromeHeight(true, menu, toolbar, false));
        Expect(NearlyEqual(menuOnly.y, menu), "menu-only viewport Y is menu height");
        Expect(NearlyEqual(menuOnly.height, windowH - menu), "menu-only viewport height");
        Expect(NearlyEqual(menuOnly.x, 0.0f) && NearlyEqual(menuOnly.width, windowW),
            "content spans window width");

        const EditorContentViewport withToolbar = editor::MakeEditorContentViewport(
            windowW, windowH, editor::LiveEditorChromeHeight(true, menu, toolbar, true));
        Expect(NearlyEqual(withToolbar.y, menu + toolbar), "toolbar-visible viewport Y is chrome");
        Expect(
            NearlyEqual(withToolbar.height, windowH - menu - toolbar),
            "toolbar-visible viewport height");
        Expect(
            editor::LiveEditorChromeHeight(false, menu, toolbar, true) == 0.0f,
            "F2 off: chrome height is 0");
        Expect(
            !editor::PointInEditorContentViewport(10.0f, menu + 2.0f, withToolbar),
            "pointer over toolbar is outside content");
        Expect(
            editor::PointInEditorContentViewport(10.0f, menu + toolbar + 2.0f, withToolbar),
            "pointer below toolbar is in content");
        Expect(
            editor::EditorViewportPointerBlocked(10.0f, menu + 2.0f, withToolbar, false),
            "toolbar region blocks viewport actions");
        Expect(
            editor::EditorViewportPointerBlocked(10.0f, menu + toolbar + 8.0f, withToolbar, true),
            "current-frame ImGui capture blocks viewport");
        Expect(
            !editor::EditorViewportPointerBlocked(
                10.0f, menu + toolbar + 8.0f, withToolbar, false),
            "content pointer is not blocked");
    }

    {
        const float menu = 24.0f;
        const float toolbar = 28.0f;
        const float hiddenInset =
            editor::OrientationWidgetLiveExtraTopInset(true, menu, toolbar, false);
        const float visibleInset =
            editor::OrientationWidgetLiveExtraTopInset(true, menu, toolbar, true);
        Expect(NearlyEqual(hiddenInset, menu + editor::kOrientationWidgetMenuBarGap),
            "hidden toolbar: widget inset is menu plus gap");
        Expect(
            NearlyEqual(visibleInset, menu + toolbar + editor::kOrientationWidgetMenuBarGap),
            "visible toolbar: widget inset includes toolbar");
        const editor::OrientationWidgetLayout hidden =
            editor::MakeOrientationWidgetLayout(1280.0f, 720.0f, hiddenInset);
        const editor::OrientationWidgetLayout visible =
            editor::MakeOrientationWidgetLayout(1280.0f, 720.0f, visibleInset);
        Expect(
            NearlyEqual(visible.originY, hidden.originY + toolbar),
            "orientation widget moves down by toolbar height");
        Expect(NearlyEqual(visible.originX, hidden.originX), "orientation widget X unchanged");
    }

    {
        render::CameraView view{};
        view.position = {0.0f, 3.0f, 12.0f};
        view.target = {0.0f, 3.0f, 11.0f};
        view.up = {0.0f, 1.0f, 0.0f};
        view.fieldOfViewY = 40.0f;

        const EditorContentViewport hidden = editor::MakeEditorContentViewport(
            1280.0f, 720.0f, editor::LiveEditorChromeHeight(true, 24.0f, 28.0f, false));
        const EditorContentViewport shown = editor::MakeEditorContentViewport(
            1280.0f, 720.0f, editor::LiveEditorChromeHeight(true, 24.0f, 28.0f, true));

        const float hiddenMouseY = hidden.y + hidden.height * 0.5f;
        const float shownMouseY = shown.y + shown.height * 0.5f;
        const editor::Ray3 hiddenRay = editor::ScreenToWorldRayFromWindow(
            view, 640.0f, hiddenMouseY, hidden);
        const editor::Ray3 shownRay = editor::ScreenToWorldRayFromWindow(
            view, 640.0f, shownMouseY, shown);
        Expect(Vec3Near(hiddenRay.origin, shownRay.origin), "center rays share origin");
        Expect(Vec3Near(hiddenRay.direction, shownRay.direction, 0.001f),
            "equivalent content-center mouse yields the same look ray");

        Expect(
            editor::EditorViewportPointerBlocked(640.0f, 10.0f, shown, false),
            "toolbar click is not a placement/pick/gizmo/camera content hit");
        Expect(
            !editor::ShouldConfirmPlacement(
                editor::PlacementMode::Collectible,
                true,
                true,
                false,
                false,
                false,
                false,
                true),
            "ImGui toolbar capture prevents placement confirm");
        Expect(
            !editor::ShouldAttemptEditorViewportPick(true, true, false, false, false),
            "ImGui toolbar capture prevents viewport pick");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d editor quick toolbar test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Editor quick toolbar tests passed.\n");
    return 0;
}
