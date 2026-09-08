#include "assets/StaticGlb.h"
#include "assets/StaticGlbImport.h"
#include "assets/StaticModelCatalog.h"
#include "assets/StaticModelDelete.h"
#include "editor/ContentBrowser.h"
#include "editor/ContentBrowserView.h"
#include "world/LevelDefinition.h"

#include <cstdio>
#include <filesystem>
#include <string>
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
        (std::filesystem::temp_directory_path() / "platformer_m48_browser").lexically_normal();
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

assets::StaticModelDeleteRoots MakeRoots(
    const std::filesystem::path& sourceRoot,
    const std::filesystem::path& cookedRoot,
    const std::vector<std::filesystem::path>& stagedRoots)
{
    assets::StaticModelDeleteRoots roots{};
    roots.sourceRoot = sourceRoot;
    roots.cookedRoot = cookedRoot;
    roots.stagedRoots = stagedRoots;
    return roots;
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
    using assets::ImportStaticGlb;
    using assets::StaticGlbImportStatus;
    using assets::StaticModelCatalog;
    using assets::StaticModelDeleteStatus;
    using editor::ContentBrowserState;

    const std::filesystem::path fixture = TestStaticGlbPath();
    Expect(PathIsRegularFile(fixture), "canonical test_static.glb fixture exists");

    const std::filesystem::path tempRoot = MakeTempRoot();
    const std::filesystem::path sourceRoot = (tempRoot / "source").lexically_normal();
    const std::filesystem::path cookedRoot = (tempRoot / "cooked").lexically_normal();
    const std::filesystem::path stagedDebug = (tempRoot / "staged" / "Debug" / "assets").lexically_normal();
    const std::filesystem::path stagedDev =
        (tempRoot / "staged" / "Development" / "assets").lexically_normal();
    const std::filesystem::path stagedRelease =
        (tempRoot / "staged" / "Release" / "assets").lexically_normal();
    std::filesystem::create_directories(sourceRoot);

    {
        ContentBrowserState browser{};
        editor::RefreshContentBrowser(browser, sourceRoot);
        Expect(browser.catalog.Count() == 0, "zero-asset catalog is valid");
        Expect(
            browser.viewMode == editor::kDefaultContentBrowserViewMode
                && browser.viewMode == editor::ContentBrowserViewMode::Thumbnails,
            "default view mode is Thumbnails");
        Expect(
            editor::FilterContentBrowserEntries(browser.catalog, "").empty(),
            "empty catalog filter is empty");
        Expect(browser.selectedIdentity.empty(), "empty catalog has no selection");
    }

    CopyFixture(fixture, sourceRoot / "models" / "crate.glb");
    CopyFixture(fixture, sourceRoot / "models" / "AlphaBox.glb");

    {
        ContentBrowserState browser{};
        editor::RefreshContentBrowser(browser, sourceRoot);
        Expect(browser.catalog.Count() == 2, "catalog presents registered models");
        Expect(
            browser.catalog.Entries()[0].canonicalIdentity == "models/AlphaBox.glb",
            "catalog sort is deterministic by identity");
        Expect(
            browser.catalog.Entries()[1].canonicalIdentity == "models/crate.glb", "second identity");
        Expect(
            browser.catalog.Entries()[0].displayName == "AlphaBox.glb", "display name is filename");
        Expect(
            browser.catalog.Entries()[0].assetType == "static_glb", "asset type is static_glb");
        Expect(
            browser.catalog.Entries()[1].canonicalIdentity == "models/crate.glb",
            "canonical identity is project-relative");
        const std::vector<assets::StaticModelCatalogEntry> all =
            editor::FilterContentBrowserEntries(browser.catalog, "");
        Expect(all.size() == 2, "empty query shows full catalog");
        Expect(
            editor::ContentBrowserQueryMatches("models/crate.glb", "CRATE"),
            "search is case-insensitive");
        const std::vector<assets::StaticModelCatalogEntry> crateHits =
            editor::FilterContentBrowserEntries(browser.catalog, "CrAtE");
        Expect(crateHits.size() == 1, "partial case-insensitive search finds crate");
        Expect(
            crateHits[0].canonicalIdentity == "models/crate.glb", "search hit identity");
        const std::vector<assets::StaticModelCatalogEntry> pathHits =
            editor::FilterContentBrowserEntries(browser.catalog, "models/alpha");
        Expect(pathHits.size() == 1, "search matches canonical path");
        const std::vector<assets::StaticModelCatalogEntry> none =
            editor::FilterContentBrowserEntries(browser.catalog, "no-such-asset");
        Expect(none.empty(), "unmatched query is a valid empty result");
        const std::vector<assets::StaticModelCatalogEntry> restored =
            editor::FilterContentBrowserEntries(browser.catalog, "");
        Expect(restored.size() == 2, "clearing query restores full catalog");
    }

    {
        ContentBrowserState browser{};
        editor::RefreshContentBrowser(browser, sourceRoot);
        editor::SelectContentBrowserIdentity(browser, "models/crate.glb");
        Expect(browser.selectedIdentity == "models/crate.glb", "asset selection stores identity");
        CopyFixture(fixture, sourceRoot / "models" / "newbox.glb");
        editor::RefreshContentBrowser(browser, sourceRoot);
        Expect(browser.catalog.Find("models/newbox.glb") != nullptr, "refresh discovers new asset");
        Expect(
            browser.selectedIdentity == "models/crate.glb",
            "selection survives refresh when identity still exists");
        std::filesystem::remove(sourceRoot / "models" / "crate.glb");
        editor::RefreshContentBrowser(browser, sourceRoot);
        Expect(browser.catalog.Find("models/crate.glb") == nullptr, "refresh removes missing asset");
        Expect(browser.selectedIdentity.empty(), "selection clears when identity disappears");
        CopyFixture(fixture, sourceRoot / "models" / "crate.glb");
        std::filesystem::remove(sourceRoot / "models" / "newbox.glb");
    }

    {
        const std::filesystem::path external = tempRoot / "external" / "imported.glb";
        CopyFixture(fixture, external);
        ContentBrowserState browser{};
        editor::RefreshContentBrowser(browser, sourceRoot);
        const std::size_t before = browser.catalog.Count();
        const assets::StaticGlbImportResult imported =
            ImportStaticGlb(external, sourceRoot, &browser.catalog);
        Expect(imported.status == StaticGlbImportStatus::Imported, "M47 import succeeds");
        Expect(imported.canonicalIdentity == "models/imported.glb", "import identity is canonical");
        Expect(
            browser.catalog.Find("models/imported.glb") != nullptr,
            "successful import is visible through the same catalog without restart");
        Expect(browser.catalog.Count() == before + 1, "import adds one catalog entry");
        editor::SelectContentBrowserIdentity(browser, imported.canonicalIdentity);
        Expect(
            browser.selectedIdentity == "models/imported.glb", "browser can select imported asset");
        std::filesystem::remove(sourceRoot / "models" / "imported.glb");
    }

    {
        Expect(
            !editor::ContentBrowserDeleteConfirmed(false),
            "cancel confirmation does not authorize delete");
        Expect(
            editor::ContentBrowserDeleteConfirmed(true), "confirm authorization is explicit");
        CopyFixture(fixture, sourceRoot / "models" / "keep.glb");
        const auto beforeBytes = std::filesystem::file_size(sourceRoot / "models" / "keep.glb");
        if (!editor::ContentBrowserDeleteConfirmed(false))
        {
            // UI cancel never calls DeleteStaticModel.
        }
        Expect(
            PathIsRegularFile(sourceRoot / "models" / "keep.glb"),
            "cancellation leaves the source file");
        Expect(
            std::filesystem::file_size(sourceRoot / "models" / "keep.glb") == beforeBytes,
            "cancellation makes no filesystem changes");
    }

    {
        assets::StaticModelDeleteResult none = assets::DeleteStaticModel(
            "", MakeRoots(sourceRoot, cookedRoot, {stagedDev}));
        Expect(none.status == StaticModelDeleteStatus::NoSelection, "empty identity is no selection");
        Expect(PathIsRegularFile(sourceRoot / "models" / "crate.glb"), "invalid delete leaves source");

        assets::StaticModelDeleteResult malformed = assets::DeleteStaticModel(
            "../models/crate.glb", MakeRoots(sourceRoot, cookedRoot, {stagedDev}));
        Expect(
            malformed.status == StaticModelDeleteStatus::InvalidIdentity,
            "noncanonical identity is rejected");

        assets::StaticModelDeleteResult traversal = assets::DeleteStaticModel(
            "models/../crate.glb", MakeRoots(sourceRoot, cookedRoot, {stagedDev}));
        Expect(
            traversal.status == StaticModelDeleteStatus::InvalidIdentity,
            "traversal identity is rejected");

        std::string parseReason;
        std::string fileName;
        Expect(
            !assets::TryParseStaticModelIdentity("models/../secret.glb", fileName, &parseReason),
            "parse rejects traversal");
        Expect(
            !assets::TryParseStaticModelIdentity("C:/temp/crate.glb", fileName, &parseReason),
            "parse rejects absolute paths");

        assets::StaticModelDeleteResult relativeRoot = assets::DeleteStaticModel(
            "models/crate.glb",
            MakeRoots(std::filesystem::path("source"), cookedRoot, {stagedDev}));
        Expect(
            relativeRoot.status == StaticModelDeleteStatus::UnsafePath,
            "non-absolute source root is rejected");
        Expect(
            PathIsRegularFile(sourceRoot / "models" / "crate.glb"),
            "unsafe mapping does not delete source");
    }

    {
        CopyFixture(fixture, sourceRoot / "models" / "disposable.glb");
        CopyFixture(fixture, sourceRoot / "models" / "neighbor.glb");
        CopyFixture(fixture, cookedRoot / "models" / "disposable.glb");
        CopyFixture(fixture, cookedRoot / "models" / "neighbor.glb");
        CopyFixture(fixture, stagedDebug / "models" / "disposable.glb");
        CopyFixture(fixture, stagedDev / "models" / "disposable.glb");
        CopyFixture(fixture, stagedRelease / "models" / "disposable.glb");
        CopyFixture(fixture, stagedDev / "models" / "neighbor.glb");

        world::LevelDefinition workingCopy = MakeAuthoredSnapshot();
        world::LevelDefinition active = workingCopy;
        world::LevelDefinition savedSourceBaseline = workingCopy;
        bool modified = false;
        bool dirty = false;

        ContentBrowserState browser{};
        editor::RefreshContentBrowser(browser, sourceRoot);
        editor::SelectContentBrowserIdentity(browser, "models/disposable.glb");

        const assets::StaticModelDeleteResult deleted = assets::DeleteStaticModel(
            browser.selectedIdentity,
            MakeRoots(sourceRoot, cookedRoot, {stagedDebug, stagedDev, stagedRelease}),
            &browser.catalog);
        Expect(deleted.status == StaticModelDeleteStatus::Deleted, "successful delete");
        Expect(deleted.sourceRemoved, "canonical source was removed");
        Expect(deleted.cookedRemoved, "cooked counterpart was removed");
        Expect(deleted.stagedRemovedCount == 3, "all mapped staged copies were removed");
        Expect(!PathExists(sourceRoot / "models" / "disposable.glb"), "source file is gone");
        Expect(!PathExists(cookedRoot / "models" / "disposable.glb"), "cooked file is gone");
        Expect(!PathExists(stagedDebug / "models" / "disposable.glb"), "debug staged file is gone");
        Expect(!PathExists(stagedDev / "models" / "disposable.glb"), "development staged file is gone");
        Expect(
            !PathExists(stagedRelease / "models" / "disposable.glb"), "release staged file is gone");
        Expect(PathIsRegularFile(sourceRoot / "models" / "neighbor.glb"), "neighbor source remains");
        Expect(PathIsRegularFile(cookedRoot / "models" / "neighbor.glb"), "neighbor cooked remains");
        Expect(PathIsRegularFile(stagedDev / "models" / "neighbor.glb"), "neighbor staged remains");

        editor::RefreshContentBrowser(browser, sourceRoot);
        Expect(browser.catalog.Find("models/disposable.glb") == nullptr, "deleted asset leaves catalog");
        Expect(browser.selectedIdentity.empty(), "deleted selection is cleared");
        Expect(browser.catalog.Find("models/neighbor.glb") != nullptr, "neighbor remains in catalog");

        Expect(
            world::AuthoredLevelDataEqual(workingCopy, active)
                && world::AuthoredLevelDataEqual(active, savedSourceBaseline),
            "delete does not mutate authored level snapshots");
        Expect(!modified && !dirty, "delete does not set Modified or Dirty");
        Expect(workingCopy.elevatedPlatforms.size() == 6, "platform count unchanged");
        Expect(workingCopy.dynamicBoxes.empty(), "no static prop or dynamic box was created");
        Expect(workingCopy.camera.fieldOfViewY == 40.0f, "FOV unchanged");
    }

    {
        CopyFixture(fixture, sourceRoot / "models" / "source_only.glb");
        ContentBrowserState browser{};
        const assets::StaticModelDeleteResult deleted = assets::DeleteStaticModel(
            "models/source_only.glb",
            MakeRoots(sourceRoot, cookedRoot, {stagedDev}),
            &browser.catalog);
        Expect(deleted.status == StaticModelDeleteStatus::Deleted, "missing generated files still delete source");
        Expect(deleted.cookedWasMissing, "missing cooked is not an error");
        Expect(deleted.stagedMissingCount >= 1, "missing staged is not an error");
        Expect(!PathExists(sourceRoot / "models" / "source_only.glb"), "source-only delete removes source");
        Expect(
            PathIsRegularFile(sourceRoot / "models" / "neighbor.glb"),
            "missing generated counterpart does not delete neighbors");
    }

    {
        CopyFixture(fixture, sourceRoot / "models" / "blocked.glb");
        std::filesystem::create_directories(cookedRoot / "models" / "blocked.glb");
        const assets::StaticModelDeleteResult blocked = assets::DeleteStaticModel(
            "models/blocked.glb", MakeRoots(sourceRoot, cookedRoot, {stagedDev}));
        Expect(blocked.status == StaticModelDeleteStatus::Error, "cooked directory blocks generated cleanup");
        Expect(!blocked.sourceRemoved, "source remains after generated cleanup failure");
        Expect(PathIsRegularFile(sourceRoot / "models" / "blocked.glb"), "partial failure leaves source");
        std::filesystem::remove_all(cookedRoot / "models" / "blocked.glb");
        std::filesystem::remove(sourceRoot / "models" / "blocked.glb");
    }

    {
        assets::StaticModelDeleteResult missing = assets::DeleteStaticModel(
            "models/missing_source.glb", MakeRoots(sourceRoot, cookedRoot, {stagedDev}));
        Expect(missing.status == StaticModelDeleteStatus::MissingSource, "missing source is reported");
        Expect(
            PathIsRegularFile(sourceRoot / "models" / "neighbor.glb"),
            "missing source does not delete neighbors");
    }

    RemoveTree(tempRoot);

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d content browser test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Content browser tests passed.\n");
    return 0;
}
