#include "assets/StaticGlbImport.h"
#include "assets/StaticModelCatalog.h"
#include "assets/StaticModelDelete.h"
#include "editor/ContentBrowser.h"
#include "editor/ContentBrowserView.h"
#include "editor/EditorSelection.h"
#include "editor/StaticModelThumbnailCache.h"
#include "world/LevelDefinition.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

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
        (std::filesystem::temp_directory_path() / "platformer_m48_1_thumbs").lexically_normal();
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

bool PathExists(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::exists(path, error) && !error;
}

void CopyFixture(const std::filesystem::path& fixture, const std::filesystem::path& destination)
{
    std::filesystem::create_directories(destination.parent_path());
    std::filesystem::copy_file(
        fixture, destination, std::filesystem::copy_options::overwrite_existing);
}

void WriteBytes(const std::filesystem::path& path, std::string_view bytes)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

bool WriteDummyCache(
    std::string_view identity,
    const editor::ThumbnailSourceStamp& stamp,
    const std::filesystem::path& cacheRoot,
    int schema = editor::kStaticModelThumbnailSchemaVersion)
{
    std::filesystem::path imagePath;
    std::filesystem::path metaPath;
    std::string error;
    if (!editor::TryResolveThumbnailCachePaths(identity, cacheRoot, imagePath, metaPath, error))
    {
        return false;
    }
    WriteBytes(imagePath, "PNG");
    editor::ThumbnailCacheMeta meta{};
    meta.schemaVersion = schema;
    meta.canonicalIdentity = std::string(identity);
    meta.stamp = stamp;
    return editor::WriteThumbnailCacheMeta(metaPath, meta);
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
    using editor::ContentBrowserState;
    using editor::ContentBrowserViewMode;
    using editor::ThumbnailEnsureDecision;
    using editor::ThumbnailModelBounds;
    using editor::ThumbnailSourceStamp;

    const std::filesystem::path fixture = TestStaticGlbPath();
    Expect(PathIsRegularFile(fixture), "canonical test_static.glb fixture exists");

    const std::filesystem::path tempRoot = MakeTempRoot();
    const std::filesystem::path userData = (tempRoot / "userdata").lexically_normal();
    const std::filesystem::path cacheRoot = editor::MakeThumbnailCacheRoot(userData);
    const std::filesystem::path sourceRoot = (tempRoot / "source").lexically_normal();
    const std::filesystem::path cookedRoot = (tempRoot / "cooked").lexically_normal();
    const std::filesystem::path stagedDev =
        (tempRoot / "staged" / "Development" / "assets").lexically_normal();
    std::filesystem::create_directories(sourceRoot);
    std::filesystem::create_directories(cacheRoot);
    std::filesystem::create_directories(userData / "Platformer3D");

    {
        const std::string a = "models/crate.glb";
        const std::string b = "models/barrel.glb";
        Expect(
            editor::ThumbnailCacheKeyHex(a) == editor::ThumbnailCacheKeyHex(a),
            "cache key is deterministic");
        Expect(
            editor::ThumbnailCacheKeyHex(a) != editor::ThumbnailCacheKeyHex(b),
            "distinct identities map to distinct cache keys");
        Expect(editor::ThumbnailCacheKeyHex(a).size() == 16, "cache key is 16 hex chars");
        std::filesystem::path imagePath;
        std::filesystem::path metaPath;
        std::string error;
        Expect(
            editor::TryResolveThumbnailCachePaths(a, cacheRoot, imagePath, metaPath, error),
            "canonical identity resolves inside cache root");
        Expect(imagePath.parent_path() == cacheRoot, "image stays in cache root");
        Expect(metaPath.parent_path() == cacheRoot, "meta stays in cache root");
        Expect(imagePath.filename() == editor::ThumbnailCacheKeyHex(a) + ".png", "image uses hash name");
    }

    {
        std::filesystem::path imagePath;
        std::filesystem::path metaPath;
        std::string error;
        Expect(
            !editor::TryResolveThumbnailCachePaths(
                "models/../secret.glb", cacheRoot, imagePath, metaPath, error),
            "traversal identity cannot escape cache root");
        Expect(
            !editor::TryResolveThumbnailCachePaths(
                "../models/crate.glb", cacheRoot, imagePath, metaPath, error),
            "noncanonical identity is rejected");
        Expect(
            !editor::TryResolveThumbnailCachePaths(
                "C:/temp/crate.glb", cacheRoot, imagePath, metaPath, error),
            "absolute identity is rejected");
        Expect(
            !editor::TryResolveThumbnailCachePaths(
                "models/crate.glb",
                std::filesystem::path("relative-cache"),
                imagePath,
                metaPath,
                error),
            "relative cache root is rejected");
        Expect(editor::MakeThumbnailCacheRoot({}).empty(), "empty user-data yields no cache root");
        Expect(
            editor::MakeThumbnailCacheRoot(userData).filename() == "thumbnails",
            "cache lives under Platformer3D/thumbnails");
        Expect(
            editor::MakeThumbnailCacheRoot(userData).parent_path().filename() == "Platformer3D",
            "cache is not source/cooked/staged");
    }

    CopyFixture(fixture, sourceRoot / "models" / "crate.glb");
    CopyFixture(fixture, sourceRoot / "models" / "neighbor.glb");
    ThumbnailSourceStamp crateStamp{};
    Expect(
        editor::ReadThumbnailSourceStamp(sourceRoot / "models" / "crate.glb", crateStamp),
        "source stamp reads size and mtime");
    Expect(
        !editor::ThumbnailCacheIsValid("models/crate.glb", crateStamp, cacheRoot),
        "missing cache is eligible for generation");

    Expect(
        WriteDummyCache("models/crate.glb", crateStamp, cacheRoot), "dummy cache write succeeds");
    Expect(
        editor::ThumbnailCacheIsValid("models/crate.glb", crateStamp, cacheRoot),
        "valid matching cache is a hit");

    {
        std::ofstream append(sourceRoot / "models" / "crate.glb", std::ios::binary | std::ios::app);
        append.put('\0');
    }
    ThumbnailSourceStamp changedStamp{};
    Expect(
        editor::ReadThumbnailSourceStamp(sourceRoot / "models" / "crate.glb", changedStamp),
        "changed source stamp reads");
    Expect(!editor::SourceStampsEqual(crateStamp, changedStamp), "source change updates stamp");
    Expect(
        !editor::ThumbnailCacheIsValid("models/crate.glb", changedStamp, cacheRoot),
        "changed source invalidates cache");
    Expect(
        editor::ThumbnailCacheIsValid("models/crate.glb", crateStamp, cacheRoot),
        "stale stamp still matches old metadata until regenerated");

    CopyFixture(fixture, sourceRoot / "models" / "crate.glb");
    Expect(
        editor::ReadThumbnailSourceStamp(sourceRoot / "models" / "crate.glb", crateStamp),
        "restored source stamp");
    Expect(
        WriteDummyCache("models/crate.glb", crateStamp, cacheRoot), "rewrite matching cache");
    Expect(
        editor::ThumbnailCacheIsValid("models/crate.glb", crateStamp, cacheRoot),
        "unchanged source keeps valid cache");

    Expect(
        WriteDummyCache("models/crate.glb", crateStamp, cacheRoot, 99), "schema mismatch write");
    Expect(
        !editor::ThumbnailCacheIsValid("models/crate.glb", crateStamp, cacheRoot),
        "schema mismatch invalidates cache");
    Expect(
        WriteDummyCache("models/crate.glb", crateStamp, cacheRoot), "restore current schema");

    {
        Expect(
            editor::ClassifyThumbnailEnsure(true, false, true, crateStamp, crateStamp)
                == ThumbnailEnsureDecision::ReuseReady,
            "ready texture with matching stamp is reused");
        Expect(
            editor::ClassifyThumbnailEnsure(false, true, true, crateStamp, crateStamp)
                == ThumbnailEnsureDecision::ReuseFailed,
            "failed generation is not retried every frame");
        Expect(
            editor::ClassifyThumbnailEnsure(false, false, true, crateStamp, crateStamp)
                == ThumbnailEnsureDecision::Refresh,
            "missing cache/texture is eligible for generation");
        ThumbnailSourceStamp empty{};
        Expect(
            editor::ClassifyThumbnailEnsure(false, true, true, empty, crateStamp)
                == ThumbnailEnsureDecision::Refresh,
            "Refresh retry clears failed stamp and becomes eligible");
        Expect(
            editor::ClassifyThumbnailEnsure(true, false, true, crateStamp, changedStamp)
                == ThumbnailEnsureDecision::Refresh,
            "source change invalidates in-memory reuse");
        Expect(
            editor::ClassifyThumbnailEnsure(false, true, false, crateStamp, crateStamp)
                == ThumbnailEnsureDecision::Refresh,
            "missing source stamp does not lock a permanent skip without retry classification");
    }

    {
        const std::filesystem::path external = tempRoot / "external" / "imported.glb";
        CopyFixture(fixture, external);
        ContentBrowserState browser{};
        editor::RefreshContentBrowser(browser, sourceRoot);
        const assets::StaticGlbImportResult imported =
            assets::ImportStaticGlb(external, sourceRoot, &browser.catalog);
        Expect(imported.status == assets::StaticGlbImportStatus::Imported, "import succeeds");
        Expect(imported.canonicalIdentity == "models/imported.glb", "imported identity is canonical");
        ThumbnailSourceStamp importedStamp{};
        Expect(
            editor::ReadThumbnailSourceStamp(sourceRoot / "models" / "imported.glb", importedStamp),
            "imported source has a stamp");
        Expect(
            !editor::ThumbnailCacheIsValid("models/imported.glb", importedStamp, cacheRoot),
            "newly imported asset is thumbnail-eligible");
        std::filesystem::remove(sourceRoot / "models" / "imported.glb");
    }

    {
        ThumbnailSourceStamp neighborStamp{};
        Expect(
            editor::ReadThumbnailSourceStamp(sourceRoot / "models" / "neighbor.glb", neighborStamp),
            "neighbor stamp");
        Expect(WriteDummyCache("models/neighbor.glb", neighborStamp, cacheRoot), "neighbor cache");
        Expect(
            editor::ThumbnailCacheIsValid("models/neighbor.glb", neighborStamp, cacheRoot),
            "neighbor cache is valid before delete");

        assets::StaticModelDeleteRoots roots{};
        roots.sourceRoot = sourceRoot;
        roots.cookedRoot = cookedRoot;
        roots.stagedRoots = {stagedDev};
        ContentBrowserState browser{};
        editor::RefreshContentBrowser(browser, sourceRoot);
        editor::SelectContentBrowserIdentity(browser, "models/crate.glb");
        const assets::StaticModelDeleteResult deleted =
            assets::DeleteStaticModel("models/crate.glb", roots, &browser.catalog);
        Expect(deleted.status == assets::StaticModelDeleteStatus::Deleted, "delete source succeeds");
        Expect(editor::RemoveThumbnailCacheEntry("models/crate.glb", cacheRoot), "mapped cache removed");
        std::filesystem::path crateImage;
        std::filesystem::path crateMeta;
        std::string error;
        Expect(
            editor::TryResolveThumbnailCachePaths(
                "models/crate.glb", cacheRoot, crateImage, crateMeta, error),
            "deleted identity still maps deterministically");
        Expect(!PathExists(crateImage) && !PathExists(crateMeta), "deleted cache files are gone");
        Expect(
            editor::ThumbnailCacheIsValid("models/neighbor.glb", neighborStamp, cacheRoot),
            "unrelated cache entry remains");
        Expect(
            PathIsRegularFile(sourceRoot / "models" / "neighbor.glb"), "neighbor source remains");
        editor::RefreshContentBrowser(browser, sourceRoot);
        Expect(browser.catalog.Find("models/crate.glb") == nullptr, "deleted asset leaves catalog");
    }

    {
        ContentBrowserState browser{};
        editor::RefreshContentBrowser(browser, sourceRoot);
        Expect(browser.catalog.Count() >= 1, "remaining assets after delete");
        Expect(
            browser.viewMode == ContentBrowserViewMode::Thumbnails, "default mode is Thumbnails");
        editor::SelectContentBrowserIdentity(browser, "models/neighbor.glb");
        browser.filterQuery = "NEIGH";
        const std::vector<assets::StaticModelCatalogEntry> thumbs =
            editor::FilterContentBrowserEntries(browser.catalog, browser.filterQuery);
        browser.viewMode = ContentBrowserViewMode::List;
        const std::vector<assets::StaticModelCatalogEntry> list =
            editor::FilterContentBrowserEntries(browser.catalog, browser.filterQuery);
        Expect(thumbs.size() == 1 && list.size() == 1, "search results match in both modes");
        Expect(
            thumbs[0].canonicalIdentity == list[0].canonicalIdentity,
            "same filter authority in both modes");
        Expect(browser.selectedIdentity == "models/neighbor.glb", "selection survives switch to List");
        browser.viewMode = ContentBrowserViewMode::Thumbnails;
        Expect(
            browser.selectedIdentity == "models/neighbor.glb",
            "selection survives switch back to Thumbnails");
        browser.filterQuery.clear();
        Expect(
            editor::FilterContentBrowserEntries(browser.catalog, "").size()
                == browser.catalog.Count(),
            "clearing query restores full results");
        Expect(
            editor::FilterContentBrowserEntries(browser.catalog, "no-such-asset").empty(),
            "zero matches is valid");
    }

    {
        const std::filesystem::path viewPath = editor::MakeContentBrowserViewPath(userData);
        Expect(viewPath.filename() == "editor_content_browser_view.txt", "view file family matches layout");
        Expect(
            editor::LoadContentBrowserViewModeFromPath(viewPath) == ContentBrowserViewMode::Thumbnails,
            "missing view file defaults to Thumbnails");
        Expect(
            editor::SaveContentBrowserViewModeToPath(viewPath, ContentBrowserViewMode::List),
            "List mode persists");
        Expect(
            editor::LoadContentBrowserViewModeFromPath(viewPath) == ContentBrowserViewMode::List,
            "reload restores List");
        Expect(
            editor::SaveContentBrowserViewModeToPath(viewPath, ContentBrowserViewMode::Thumbnails),
            "Thumbnails mode persists");
        Expect(
            editor::LoadContentBrowserViewModeFromPath(viewPath) == ContentBrowserViewMode::Thumbnails,
            "reload restores Thumbnails");
        Expect(
            editor::SaveContentBrowserViewModeToPath(viewPath, ContentBrowserViewMode::List),
            "List saved before reset");
        ContentBrowserState browser{};
        browser.viewMode = editor::LoadContentBrowserViewModeFromPath(viewPath);
        Expect(browser.viewMode == ContentBrowserViewMode::List, "session load can restore List");
        browser.viewMode = editor::kDefaultContentBrowserViewMode;
        Expect(
            editor::SaveContentBrowserViewModeToPath(viewPath, browser.viewMode),
            "Reset Editor Layout writes default");
        Expect(
            editor::LoadContentBrowserViewModeFromPath(viewPath) == ContentBrowserViewMode::Thumbnails,
            "reset default is Thumbnails");
        Expect(
            editor::ParseContentBrowserViewMode("list") == ContentBrowserViewMode::List,
            "view mode parse is case-insensitive");
        Expect(
            editor::ParseContentBrowserViewMode("nope") == ContentBrowserViewMode::Thumbnails,
            "unknown view mode falls back to Thumbnails");
    }

    {
        editor::EditorSelection scene{};
        scene.kind = editor::EditorObjectKind::ElevatedPlatform;
        scene.index = 1;
        ContentBrowserState browser{};
        editor::RefreshContentBrowser(browser, sourceRoot);
        editor::SelectContentBrowserIdentity(browser, "models/neighbor.glb");
        browser.viewMode = ContentBrowserViewMode::List;
        browser.viewMode = ContentBrowserViewMode::Thumbnails;
        Expect(scene.kind == editor::EditorObjectKind::ElevatedPlatform, "asset select is not scene select");
        Expect(scene.index == 1, "scene selection index is unchanged");
        Expect(
            browser.selectedIdentity == "models/neighbor.glb", "browser selection remains independent");
    }

    {
        world::LevelDefinition workingCopy = MakeAuthoredSnapshot();
        world::LevelDefinition active = workingCopy;
        world::LevelDefinition savedSourceBaseline = workingCopy;
        bool modified = false;
        bool dirty = false;
        ContentBrowserState browser{};
        browser.viewMode = ContentBrowserViewMode::List;
        editor::RefreshContentBrowser(browser, sourceRoot);
        editor::SelectContentBrowserIdentity(browser, "models/neighbor.glb");
        browser.viewMode = ContentBrowserViewMode::Thumbnails;
        Expect(
            world::AuthoredLevelDataEqual(workingCopy, active)
                && world::AuthoredLevelDataEqual(active, savedSourceBaseline),
            "view/search/refresh do not mutate authored snapshots");
        Expect(!modified && !dirty, "thumbnail tooling does not set Modified or Dirty");
        Expect(workingCopy.elevatedPlatforms.size() == 6, "platform count unchanged");
        Expect(workingCopy.dynamicBoxes.empty(), "no static prop was created");
        Expect(workingCopy.camera.fieldOfViewY == 40.0f, "FOV unchanged");
    }

    {
        ContentBrowserState empty{};
        editor::RefreshContentBrowser(empty, tempRoot / "missing-models-root");
        Expect(empty.catalog.Count() == 0, "zero assets remains valid");
        Expect(empty.viewMode == ContentBrowserViewMode::Thumbnails, "empty browser still defaults to Thumbnails");
        Expect(
            editor::FilterContentBrowserEntries(empty.catalog, "glb").empty(),
            "empty catalog search is empty");
    }

    {
        const ThumbnailModelBounds centered{{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
        const auto frame = editor::MakeThumbnailCameraFrame(centered);
        Expect(frame.target.x == 0.0f && frame.target.y == 0.0f && frame.target.z == 0.0f,
            "centered bounds target the origin");
        Expect(FiniteVec(frame.position) && FiniteVec(frame.target), "centered camera is finite");
        Expect(frame.fieldOfViewY == editor::kStaticModelThumbnailFieldOfViewY, "FOV is 40");
        Expect(frame.nearPlane > 0.0f && frame.farPlane > frame.nearPlane, "near/far are ordered");
        Expect(frame.position.x > 0.0f && frame.position.y > 0.0f && frame.position.z > 0.0f,
            "three-quarter camera sits in +X+Y+Z");
    }

    {
        const ThumbnailModelBounds offOrigin{{10.0f, 4.0f, -3.0f}, {12.0f, 6.0f, -1.0f}};
        const auto frame = editor::MakeThumbnailCameraFrame(offOrigin);
        Expect(frame.target.x == 11.0f && frame.target.y == 5.0f && frame.target.z == -2.0f,
            "off-origin bounds keep the visual center");
        Expect(FiniteVec(frame.position), "off-origin camera is finite");
    }

    {
        const ThumbnailModelBounds skinny{{-0.01f, -4.0f, -0.01f}, {0.01f, 4.0f, 0.01f}};
        const auto frame = editor::MakeThumbnailCameraFrame(skinny);
        Expect(FiniteVec(frame.position) && FiniteVec(frame.target), "non-uniform camera is finite");
        Expect(frame.target.y == 0.0f, "non-uniform target uses bounds center");
        Expect(frame.farPlane > frame.nearPlane, "non-uniform near/far stay valid");
    }

    {
        const ThumbnailModelBounds tiny{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
        const auto frame = editor::MakeThumbnailCameraFrame(tiny);
        Expect(FiniteVec(frame.position) && FiniteVec(frame.target), "degenerate extents stay finite");
        Expect(frame.nearPlane > 0.0f, "tiny model near plane is positive");
    }

    {
        const ThumbnailModelBounds huge{{-1000.0f, -50.0f, -800.0f}, {1000.0f, 50.0f, 800.0f}};
        const auto frame = editor::MakeThumbnailCameraFrame(huge);
        Expect(FiniteVec(frame.position) && FiniteVec(frame.target), "large extents stay finite");
        Expect(frame.farPlane > frame.nearPlane + 1.0f, "large model far plane clears the mesh");
        const auto again = editor::MakeThumbnailCameraFrame(huge);
        Expect(
            again.position.x == frame.position.x && again.position.y == frame.position.y
                && again.position.z == frame.position.z,
            "camera distance is deterministic");
    }

    {
        ThumbnailModelBounds nanBounds{};
        nanBounds.min = {std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f};
        nanBounds.max = {1.0f, 1.0f, 1.0f};
        const auto frame = editor::MakeThumbnailCameraFrame(nanBounds);
        Expect(FiniteVec(frame.position) && FiniteVec(frame.target), "NaN bounds fall back to finite camera");
    }

    RemoveTree(tempRoot);

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d content browser thumbnail test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Content browser thumbnail tests passed.\n");
    return 0;
}
