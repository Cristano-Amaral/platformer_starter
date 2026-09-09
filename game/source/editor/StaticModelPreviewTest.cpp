#include "assets/StaticGlbImport.h"
#include "assets/StaticModelCatalog.h"
#include "assets/StaticModelDelete.h"
#include "editor/ContentBrowser.h"
#include "editor/EditorCamera.h"
#include "editor/EditorLayout.h"
#include "editor/EditorSelection.h"
#include "editor/EditorWorkspace.h"
#include "editor/StaticModelFraming.h"
#include "world/LevelDefinition.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <system_error>

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

bool FiniteVec(const core::Vec3& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool NearlyEqual(float a, float b, float epsilon = 0.001f)
{
    return std::fabs(a - b) <= epsilon;
}

std::filesystem::path TestStaticGlbPath()
{
#if defined(PLATFORMER_TEST_STATIC_GLB)
    return std::filesystem::path{PLATFORMER_TEST_STATIC_GLB}.lexically_normal();
#else
    return {};
#endif
}

std::filesystem::path MakeTempRoot()
{
    const std::filesystem::path root =
        (std::filesystem::temp_directory_path() / "platformer_m48_2_preview").lexically_normal();
    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::filesystem::create_directories(root, error);
    return root;
}

void RemoveTree(const std::filesystem::path& path)
{
    std::error_code error;
    std::filesystem::remove_all(path, error);
}

bool PathIsRegularFile(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::is_regular_file(path, error) && !error;
}

void CopyFixture(const std::filesystem::path& fixture, const std::filesystem::path& destination)
{
    std::filesystem::create_directories(destination.parent_path());
    std::filesystem::copy_file(
        fixture, destination, std::filesystem::copy_options::overwrite_existing);
}

world::LevelDefinition MakeAuthoredSnapshot()
{
    world::LevelDefinition level{};
    level.id = "level_01";
    level.killPlaneY = -8.0f;
    level.camera.fieldOfViewY = 40.0f;
    level.elevatedPlatforms.resize(6);
    level.checkpoints.resize(2);
    level.hazards.resize(2);
    level.collectibles.resize(3);
    return level;
}
}

int main()
{
    using editor::PreviewLoadAction;
    using editor::StaticModelPreviewOrbit;
    using editor::ThumbnailModelBounds;
    using editor::ThumbnailSourceStamp;

    const std::filesystem::path fixture = TestStaticGlbPath();
    Expect(PathIsRegularFile(fixture), "canonical test_static.glb fixture exists");

    const std::filesystem::path tempRoot = MakeTempRoot();
    const std::filesystem::path sourceRoot = (tempRoot / "source").lexically_normal();
    const std::filesystem::path cookedRoot = (tempRoot / "cooked").lexically_normal();
    const std::filesystem::path stagedDev =
        (tempRoot / "staged" / "Development" / "assets").lexically_normal();
    std::filesystem::create_directories(sourceRoot);
    CopyFixture(fixture, sourceRoot / "models" / "crate.glb");
    CopyFixture(fixture, sourceRoot / "models" / "neighbor.glb");

    editor::ContentBrowserState browser{};
    editor::RefreshContentBrowser(browser, sourceRoot);
    editor::SelectContentBrowserIdentity(browser, "models/crate.glb");
    Expect(browser.selectedIdentity == "models/crate.glb", "selection feeds preview identity");

    ThumbnailSourceStamp crateStamp{};
    Expect(
        editor::ReadThumbnailSourceStamp(sourceRoot / "models" / "crate.glb", crateStamp),
        "source stamp for selected asset");

    {
        Expect(
            editor::ClassifyPreviewLoad("", false, false, {}, "", false, {})
                == PreviewLoadAction::Clear,
            "no selection clears preview");
        Expect(
            editor::ClassifyPreviewLoad(
                "", false, false, {}, browser.selectedIdentity, true, crateStamp)
                == PreviewLoadAction::Load,
            "first valid selection loads the real model");
        Expect(
            editor::ClassifyPreviewLoad(
                "models/crate.glb", true, false, crateStamp, "models/crate.glb", true, crateStamp)
                == PreviewLoadAction::Keep,
            "same selection does not reload");
        Expect(
            editor::ClassifyPreviewLoad(
                "models/crate.glb",
                true,
                false,
                crateStamp,
                "models/neighbor.glb",
                true,
                crateStamp)
                == PreviewLoadAction::Load,
            "selection change replaces the model");
        editor::ClearContentBrowserSelection(browser);
        Expect(
            editor::ClassifyPreviewLoad(
                "models/crate.glb", true, false, crateStamp, browser.selectedIdentity, false, {})
                == PreviewLoadAction::Clear,
            "clearing selection releases preview");
        editor::SelectContentBrowserIdentity(browser, "models/crate.glb");
    }

    {
        Expect(
            editor::ClassifyPreviewLoad(
                "models/crate.glb", false, true, crateStamp, "models/crate.glb", true, crateStamp)
                == PreviewLoadAction::KeepFailed,
            "failed load is not retried every frame");
        ThumbnailSourceStamp empty{};
        Expect(
            editor::ClassifyPreviewLoad(
                "models/crate.glb", false, true, empty, "models/crate.glb", true, crateStamp)
                == PreviewLoadAction::Load,
            "Refresh retry (cleared stamp) becomes eligible");
        Expect(
            editor::ClassifyPreviewLoad(
                "models/neighbor.glb",
                false,
                true,
                crateStamp,
                "models/crate.glb",
                true,
                crateStamp)
                == PreviewLoadAction::Load,
            "reselection of another identity then this one reloads");
    }

    {
        const ThumbnailModelBounds centered{{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
        const auto frame = editor::MakeDefaultStaticModelCameraFrame(centered);
        Expect(NearlyEqual(frame.target.x, 0.0f) && NearlyEqual(frame.target.y, 0.0f),
            "ordinary framing targets the bounds center");
        Expect(FiniteVec(frame.position), "ordinary framing is finite");
        Expect(frame.nearPlane > 0.0f && frame.farPlane > frame.nearPlane, "ordinary near/far");
    }

    {
        const ThumbnailModelBounds offOrigin{{10.0f, 4.0f, -3.0f}, {12.0f, 6.0f, -1.0f}};
        const auto frame = editor::MakeDefaultStaticModelCameraFrame(offOrigin);
        Expect(NearlyEqual(frame.target.x, 11.0f) && NearlyEqual(frame.target.y, 5.0f)
                && NearlyEqual(frame.target.z, -2.0f),
            "off-origin framing uses bounds center");
        Expect(FiniteVec(frame.position), "off-origin camera is finite");
    }

    {
        const ThumbnailModelBounds tiny{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
        const auto tinyFrame = editor::MakeDefaultStaticModelCameraFrame(tiny);
        Expect(FiniteVec(tinyFrame.position), "tiny bounds stay finite");
        const ThumbnailModelBounds huge{{-1000.0f, -50.0f, -800.0f}, {1000.0f, 50.0f, 800.0f}};
        const auto hugeFrame = editor::MakeDefaultStaticModelCameraFrame(huge);
        Expect(FiniteVec(hugeFrame.position), "large bounds stay finite");
        ThumbnailModelBounds nanBounds{};
        nanBounds.min = {std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f};
        nanBounds.max = {1.0f, 1.0f, 1.0f};
        const auto nanFrame = editor::MakeDefaultStaticModelCameraFrame(nanBounds);
        Expect(FiniteVec(nanFrame.position), "NaN bounds fall back to finite camera");
    }

    {
        const ThumbnailModelBounds centered{{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
        StaticModelPreviewOrbit orbit{};
        editor::ResetStaticModelPreviewOrbit(orbit, centered);
        const float startYaw = orbit.yawDegrees;
        const float startPitch = orbit.pitchDegrees;
        const float startDistance = orbit.distance;
        editor::ApplyStaticModelPreviewOrbit(orbit, 40.0f, 0.0f);
        Expect(orbit.yawDegrees > startYaw, "orbit yaw increases with +X drag");
        Expect(NearlyEqual(orbit.pitchDegrees, startPitch), "horizontal orbit leaves pitch");
        editor::ApplyStaticModelPreviewOrbit(orbit, 0.0f, 10000.0f);
        Expect(
            orbit.pitchDegrees >= editor::kStaticModelPreviewMinPitchDegrees
                && orbit.pitchDegrees <= editor::kStaticModelPreviewMaxPitchDegrees,
            "orbit pitch is clamped");
        Expect(std::isfinite(orbit.yawDegrees) && std::isfinite(orbit.pitchDegrees),
            "orbit remains finite");
        const auto orbited = editor::MakeStaticModelCameraFrameFromOrbit(orbit);
        Expect(FiniteVec(orbited.position) && FiniteVec(orbited.target), "orbited camera is finite");

        Expect(editor::ApplyStaticModelPreviewDolly(orbit, 4.0f), "dolly zoom in");
        Expect(orbit.distance < startDistance, "zoom in reduces distance");
        for (int i = 0; i < 40; ++i)
        {
            editor::ApplyStaticModelPreviewDolly(orbit, 8.0f);
            editor::ApplyStaticModelPreviewDolly(orbit, -8.0f);
        }
        Expect(orbit.distance > 0.0f && std::isfinite(orbit.distance), "zoom clamps stay positive finite");

        editor::ApplyStaticModelPreviewOrbit(orbit, 80.0f, -30.0f);
        editor::ApplyStaticModelPreviewDolly(orbit, -3.0f);
        editor::ResetStaticModelPreviewOrbit(orbit, centered);
        Expect(NearlyEqual(orbit.yawDegrees, startYaw), "Reset View restores yaw");
        Expect(NearlyEqual(orbit.pitchDegrees, startPitch), "Reset View restores pitch");
        Expect(NearlyEqual(orbit.distance, startDistance), "Reset View restores distance");
        Expect(NearlyEqual(orbit.target.x, 0.0f), "Reset View restores target");
    }

    {
        editor::EditorCamera viewport{};
        viewport.yawDegrees = 12.0f;
        viewport.pitchDegrees = -8.0f;
        viewport.position = {3.0f, 4.0f, 9.0f};
        StaticModelPreviewOrbit orbit{};
        editor::ResetStaticModelPreviewOrbit(
            orbit, {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}});
        editor::ApplyStaticModelPreviewOrbit(orbit, 25.0f, -10.0f);
        Expect(NearlyEqual(viewport.yawDegrees, 12.0f), "preview orbit does not move viewport yaw");
        Expect(NearlyEqual(viewport.position.z, 9.0f), "preview orbit does not move viewport position");
    }

    {
        Expect(!editor::ResolvePreviewRenderSize(0.0f, 256.0f).valid, "zero width is invalid");
        Expect(!editor::ResolvePreviewRenderSize(256.0f, 0.0f).valid, "zero height is invalid");
        Expect(!editor::ResolvePreviewRenderSize(4.0f, 4.0f).valid, "tiny region is invalid");
        const auto ok = editor::ResolvePreviewRenderSize(320.0f, 240.0f);
        Expect(ok.valid && ok.width == 320 && ok.height == 240, "valid region keeps integer size");
        Expect(
            editor::PreviewRenderTargetNeedsResize(320, 240, 640, 240),
            "width change resizes the target");
        Expect(
            !editor::PreviewRenderTargetNeedsResize(320, 240, 320, 240),
            "unchanged size does not resize");
        Expect(
            editor::ClassifyPreviewLoad(
                "models/crate.glb", true, false, crateStamp, "models/crate.glb", true, crateStamp)
                == PreviewLoadAction::Keep,
            "render-target resize does not classify as model reload");
    }

    {
        Expect(
            editor::ClassifyPreviewLoad(
                "models/crate.glb", true, false, crateStamp, "models/crate.glb", true, crateStamp)
                == PreviewLoadAction::Keep,
            "Refresh unchanged source does not reload");
        {
            std::ofstream append(sourceRoot / "models" / "crate.glb", std::ios::binary | std::ios::app);
            append.put('\0');
        }
        ThumbnailSourceStamp changed{};
        Expect(
            editor::ReadThumbnailSourceStamp(sourceRoot / "models" / "crate.glb", changed),
            "changed source stamp");
        Expect(
            editor::ClassifyPreviewLoad(
                "models/crate.glb", true, false, crateStamp, "models/crate.glb", true, changed)
                == PreviewLoadAction::Load,
            "Refresh after source change reloads");
        CopyFixture(fixture, sourceRoot / "models" / "crate.glb");
        Expect(
            editor::ReadThumbnailSourceStamp(sourceRoot / "models" / "crate.glb", crateStamp),
            "restored source stamp");
    }

    {
        const std::filesystem::path external = tempRoot / "external" / "imported.glb";
        CopyFixture(fixture, external);
        const assets::StaticGlbImportResult imported =
            assets::ImportStaticGlb(external, sourceRoot, &browser.catalog);
        Expect(imported.status == assets::StaticGlbImportStatus::Imported, "import succeeds");
        editor::SelectContentBrowserIdentity(browser, imported.canonicalIdentity);
        ThumbnailSourceStamp importedStamp{};
        Expect(
            editor::ReadThumbnailSourceStamp(sourceRoot / "models" / "imported.glb", importedStamp),
            "imported source stamp");
        Expect(
            editor::ClassifyPreviewLoad(
                "models/crate.glb",
                true,
                false,
                crateStamp,
                browser.selectedIdentity,
                true,
                importedStamp)
                == PreviewLoadAction::Load,
            "selecting imported asset loads preview");
        const std::filesystem::path cacheRoot =
            editor::MakeThumbnailCacheRoot((tempRoot / "userdata").lexically_normal());
        Expect(
            !editor::ThumbnailCacheIsValid("models/imported.glb", importedStamp, cacheRoot),
            "preview load does not require thumbnail cache");
        std::filesystem::remove(sourceRoot / "models" / "imported.glb");
        editor::RefreshContentBrowser(browser, sourceRoot);
        editor::SelectContentBrowserIdentity(browser, "models/crate.glb");
    }

    {
        editor::SelectContentBrowserIdentity(browser, "models/crate.glb");
        Expect(!editor::ContentBrowserDeleteConfirmed(false), "delete cancel");
        Expect(browser.selectedIdentity == "models/crate.glb", "cancel leaves selection");
        Expect(
            editor::ClassifyPreviewLoad(
                "models/crate.glb", true, false, crateStamp, browser.selectedIdentity, true, crateStamp)
                == PreviewLoadAction::Keep,
            "delete cancel keeps preview");

        assets::StaticModelDeleteRoots roots{};
        roots.sourceRoot = sourceRoot;
        roots.cookedRoot = cookedRoot;
        roots.stagedRoots = {stagedDev};
        const assets::StaticModelDeleteResult deleted =
            assets::DeleteStaticModel("models/crate.glb", roots, &browser.catalog);
        Expect(deleted.status == assets::StaticModelDeleteStatus::Deleted, "delete selected asset");
        editor::RefreshContentBrowser(browser, sourceRoot);
        Expect(browser.selectedIdentity.empty(), "deleted selection clears");
        Expect(
            editor::ClassifyPreviewLoad(
                "models/crate.glb", true, false, crateStamp, browser.selectedIdentity, false, {})
                == PreviewLoadAction::Clear,
            "delete success clears preview");
        Expect(
            PathIsRegularFile(sourceRoot / "models" / "neighbor.glb"), "neighbor source remains");
    }

    {
        world::LevelDefinition workingCopy = MakeAuthoredSnapshot();
        world::LevelDefinition active = workingCopy;
        bool modified = false;
        bool dirty = false;
        StaticModelPreviewOrbit orbit{};
        editor::ResetStaticModelPreviewOrbit(
            orbit, {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}});
        editor::ApplyStaticModelPreviewOrbit(orbit, 15.0f, -6.0f);
        editor::ApplyStaticModelPreviewDolly(orbit, 2.0f);
        editor::EditorSelection scene{editor::EditorObjectKind::ElevatedPlatform, 1};
        Expect(
            world::AuthoredLevelDataEqual(workingCopy, active),
            "preview camera does not mutate authored snapshots");
        Expect(!modified && !dirty, "preview does not set Modified or Dirty");
        Expect(scene.kind == editor::EditorObjectKind::ElevatedPlatform, "scene selection untouched");
        Expect(workingCopy.dynamicBoxes.empty(), "no static prop created");
        Expect(workingCopy.staticProps.empty(), "preview does not instantiate Static Props");
        Expect(workingCopy.camera.fieldOfViewY == 40.0f, "FOV unchanged");
        editor::ContentBrowserState previewBrowser{};
        previewBrowser.selectedIdentity = "models/test_static.glb";
        editor::EditorSelection propScene{editor::EditorObjectKind::StaticProp, 0};
        Expect(
            previewBrowser.selectedIdentity == "models/test_static.glb",
            "Static Prop scene selection does not hijack Model Preview identity");
        Expect(propScene.kind == editor::EditorObjectKind::StaticProp, "scene kind stays independent");
    }

    {
        editor::EditorWorkspaceState workspace{};
        Expect(workspace.showModelPreview, "Model Preview defaults visible");
        Expect(editor::AllEditorPanelsVisible(workspace), "defaults include Model Preview");
        workspace.showModelPreview = false;
        Expect(!editor::AllEditorPanelsVisible(workspace), "hiding Model Preview is independent");
        editor::ResetEditorWorkspaceVisibility(workspace);
        Expect(workspace.showModelPreview, "Reset restores Model Preview");
        const editor::EditorLayoutDefaults defaults =
            editor::ComputeDefaultEditorLayout(1280.0f, 720.0f);
        Expect(
            editor::FindDefaultPlacement(defaults, editor::kModelPreviewWindowName) != nullptr,
            "Model Preview has a default layout pose");
        Expect(
            std::strcmp(defaults.modelPreview.name, editor::kModelPreviewWindowName) == 0,
            "layout name is Model Preview");
    }

    RemoveTree(tempRoot);

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d static model preview test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Static model preview tests passed.\n");
    return 0;
}
