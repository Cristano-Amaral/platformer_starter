#include "assets/RuntimePng.h"
#include "assets/RuntimePngImport.h"
#include "assets/SourceTextureCatalog.h"
#include "assets/StaticGlbImport.h"
#include "assets/StaticModelCatalog.h"
#include "editor/ContentBrowser.h"
#include "editor/ContentBrowserOrganization.h"
#include "editor/ContentBrowserView.h"
#include "editor/TerrainTextureReference.h"
#include "editor/TextureThumbnailLifecycle.h"
#include "world/LevelDefinition.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
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

std::filesystem::path TestCheckerPngPath()
{
#if defined(PLATFORMER_TEST_CHECKER_PNG)
    return std::filesystem::path{PLATFORMER_TEST_CHECKER_PNG}.lexically_normal();
#else
    return {};
#endif
}

std::filesystem::path MakeTempRoot()
{
    const std::filesystem::path root =
        (std::filesystem::temp_directory_path() / "platformer_m89_browser").lexically_normal();
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

void CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination)
{
    std::filesystem::create_directories(destination.parent_path());
    std::filesystem::copy_file(
        source, destination, std::filesystem::copy_options::overwrite_existing);
}

world::LevelDefinition MakeLevel()
{
    world::LevelDefinition level{};
    level.id = "level_01";
    level.killPlaneY = -8.0f;
    level.camera.fieldOfViewY = 40.0f;
    level.elevatedPlatforms.resize(1);
    return level;
}
}

int main()
{
    using editor::ContentBrowserOrganization;
    using editor::ContentBrowserState;

    const std::filesystem::path glb = TestStaticGlbPath();
    const std::filesystem::path png = TestCheckerPngPath();
    Expect(PathIsRegularFile(glb), "glb fixture exists");
    Expect(PathIsRegularFile(png), "png fixture exists");

    const std::filesystem::path tempRoot = MakeTempRoot();
    const std::filesystem::path sourceRoot = (tempRoot / "source").lexically_normal();
    std::filesystem::create_directories(sourceRoot);

    CopyFile(glb, tempRoot / "external" / "crate.glb");
    CopyFile(png, tempRoot / "external" / "grass.png");
    Expect(
        assets::ImportStaticGlb(tempRoot / "external" / "crate.glb", sourceRoot).status
            == assets::StaticGlbImportStatus::Imported,
        "seed model import");
    Expect(
        assets::ImportRuntimePng(tempRoot / "external" / "grass.png", sourceRoot).status
            == assets::RuntimePngImportStatus::Imported,
        "seed texture import");

    ContentBrowserState browser{};
    editor::RefreshContentBrowser(browser, sourceRoot);
    Expect(browser.catalog.Find("models/crate.glb") != nullptr, "models remain in StaticModelCatalog");
    Expect(
        browser.textureCatalog.Find("textures/grass.png") != nullptr,
        "textures appear in shared SourceTextureCatalog");
    Expect(
        browser.textureCatalog.Identities()
            == assets::CollectSourceRuntimePngIdentities(sourceRoot),
        "M88 listing uses the same catalog identities");

    world::LevelDefinition working = MakeLevel();
    world::LevelDefinition active = working;
    const world::LevelDefinition beforeOrg = working;

    std::string created;
    std::string message;
    Expect(
        editor::CreateContentBrowserFolder(browser.organization, {}, "Environment", created, message)
            == editor::ContentBrowserOrganizationStatus::Ok,
        "create Environment");
    Expect(created == "Environment", "Environment path");
    Expect(
        editor::CreateContentBrowserFolder(
            browser.organization, "Environment", "Flora", created, message)
            == editor::ContentBrowserOrganizationStatus::Ok,
        "create Environment/Flora");
    Expect(
        editor::CreateContentBrowserFolder(
            browser.organization, "Environment/Flora", "Grass", created, message)
            == editor::ContentBrowserOrganizationStatus::Ok,
        "create Environment/Flora/Grass");
    Expect(
        editor::CreateContentBrowserFolder(
            browser.organization, "Environment/Flora", "Trees", created, message)
            == editor::ContentBrowserOrganizationStatus::Ok,
        "create Environment/Flora/Trees");
    Expect(
        editor::CreateContentBrowserFolder(
            browser.organization, "Environment", "Flora", created, message)
            == editor::ContentBrowserOrganizationStatus::Collision,
        "sibling collision rejected");
    Expect(
        editor::CreateContentBrowserFolder(browser.organization, {}, "bad/name", created, message)
            == editor::ContentBrowserOrganizationStatus::InvalidName,
        "separator in one component rejected");
    Expect(
        editor::CreateContentBrowserFolder(browser.organization, {}, "", created, message)
            == editor::ContentBrowserOrganizationStatus::InvalidName,
        "empty folder name rejected");
    Expect(
        editor::CreateContentBrowserFolder(browser.organization, {}, "Favorites", created, message)
            == editor::ContentBrowserOrganizationStatus::InvalidName,
        "reserved collection name rejected");

    Expect(
        editor::MoveContentBrowserAsset(
            browser.organization, "textures/grass.png", "Environment/Flora/Grass", message)
            == editor::ContentBrowserOrganizationStatus::Ok,
        "move texture into logical folder");
    Expect(
        editor::MoveContentBrowserAsset(
            browser.organization, "models/crate.glb", "Environment/Flora/Trees", message)
            == editor::ContentBrowserOrganizationStatus::Ok,
        "move model into logical folder");
    Expect(
        editor::ContentBrowserAssignedFolder(browser.organization, "textures/grass.png")
            == "Environment/Flora/Grass",
        "texture folder assignment stored");
    Expect(PathIsRegularFile(sourceRoot / "textures" / "grass.png"), "physical texture was not moved");
    Expect(PathIsRegularFile(sourceRoot / "models" / "crate.glb"), "physical model was not moved");

    std::string renamed;
    Expect(
        editor::RenameContentBrowserFolder(
            browser.organization, "Environment/Flora", "Plants", renamed, message)
            == editor::ContentBrowserOrganizationStatus::Ok,
        "rename Flora to Plants");
    Expect(renamed == "Environment/Plants", "renamed path");
    Expect(
        editor::ContentBrowserAssignedFolder(browser.organization, "textures/grass.png")
            == "Environment/Plants/Grass",
        "assignments follow renamed ancestors");
    Expect(
        editor::RenameContentBrowserFolder(
            browser.organization, "Environment/Plants", "Flora", renamed, message)
            == editor::ContentBrowserOrganizationStatus::Ok,
        "rename back to Flora");

    Expect(
        editor::SetContentBrowserFavorite(browser.organization, "models/crate.glb", true, message)
            == editor::ContentBrowserOrganizationStatus::Ok,
        "favorite model");
    Expect(
        editor::SetContentBrowserFavorite(browser.organization, "textures/grass.png", true, message)
            == editor::ContentBrowserOrganizationStatus::Ok,
        "favorite texture");
    Expect(editor::ContentBrowserIsFavorite(browser.organization, "models/crate.glb"), "model favorited");
    Expect(
        editor::ContentBrowserIsFavorite(browser.organization, "textures/grass.png"),
        "texture favorited");
    Expect(
        editor::SetContentBrowserFavorite(browser.organization, "textures/grass.png", false, message)
            == editor::ContentBrowserOrganizationStatus::Ok,
        "unfavorite texture");
    Expect(
        !editor::ContentBrowserIsFavorite(browser.organization, "textures/grass.png"),
        "texture unfavorited");
    Expect(
        editor::SetContentBrowserFavorite(browser.organization, "textures/grass.png", true, message)
            == editor::ContentBrowserOrganizationStatus::Ok,
        "re-favorite texture");

    Expect(
        editor::SaveContentBrowserOrganization(
            editor::ContentBrowserOrganizationPath(sourceRoot), browser.organization, message)
            == editor::ContentBrowserOrganizationStatus::Ok,
        "organization save");
    ContentBrowserOrganization loaded{};
    Expect(
        editor::LoadContentBrowserOrganization(
            editor::ContentBrowserOrganizationPath(sourceRoot), loaded, message)
            == editor::ContentBrowserOrganizationStatus::Ok,
        "organization load");
    Expect(loaded.folders == browser.organization.folders, "folders persist");
    Expect(loaded.favorites == browser.organization.favorites, "favorites persist");
    Expect(
        editor::ContentBrowserAssignedFolder(loaded, "textures/grass.png")
            == "Environment/Flora/Grass",
        "assignments persist");
    Expect(
        editor::SaveContentBrowserOrganization(
            editor::ContentBrowserOrganizationPath(sourceRoot), browser.organization, message)
            == editor::ContentBrowserOrganizationStatus::Ok,
        "organization overwrite save");

    Expect(world::AuthoredLevelDataEqual(working, beforeOrg), "organization does not dirty Level");
    Expect(world::AuthoredLevelDataEqual(working, active), "workingCopy unchanged by organization");

    browser.organization = loaded;
    browser.collection = editor::ContentBrowserCollection::AllAssets;
    browser.filterQuery.clear();
    std::vector<editor::ContentBrowserAssetEntry> all = editor::QueryContentBrowserAssets(browser);
    Expect(all.size() == 2, "All Assets shows models and textures");
    browser.collection = editor::ContentBrowserCollection::Models;
    Expect(editor::QueryContentBrowserAssets(browser).size() == 1, "Models filter");
    Expect(
        editor::QueryContentBrowserAssets(browser)[0].canonicalIdentity == "models/crate.glb",
        "Models filter identity");
    browser.collection = editor::ContentBrowserCollection::Textures;
    Expect(editor::QueryContentBrowserAssets(browser).size() == 1, "Textures filter");
    browser.collection = editor::ContentBrowserCollection::Favorites;
    Expect(editor::QueryContentBrowserAssets(browser).size() == 2, "Favorites shows both");
    browser.filterQuery = "grass";
    Expect(editor::QueryContentBrowserAssets(browser).size() == 1, "Favorites + search");
    browser.collection = editor::ContentBrowserCollection::AllAssets;
    Expect(editor::QueryContentBrowserAssets(browser).size() == 1, "All Assets + search");
    browser.collection = editor::ContentBrowserCollection::Models;
    Expect(editor::QueryContentBrowserAssets(browser).empty(), "Models + non-matching search");
    browser.filterQuery.clear();
    browser.collection = editor::ContentBrowserCollection::Folders;
    browser.currentFolderPath = "Environment/Flora/Grass";
    Expect(editor::QueryContentBrowserAssets(browser).size() == 1, "direct folder scope");
    browser.currentFolderPath = "Environment";
    Expect(
        editor::QueryContentBrowserAssets(browser).size() == 2,
        "folder search includes descendants");
    browser.filterQuery = "crate";
    Expect(editor::QueryContentBrowserAssets(browser).size() == 1, "folder scope + search");
    browser.filterQuery.clear();
    browser.currentFolderPath.clear();
    Expect(editor::QueryContentBrowserAssets(browser).empty(), "unfiled folder scope is empty");

    Expect(
        editor::MoveContentBrowserAsset(browser.organization, "models/crate.glb", {}, message)
            == editor::ContentBrowserOrganizationStatus::Ok,
        "move model back to unfiled");
    Expect(
        editor::ContentBrowserAssignedFolder(browser.organization, "models/crate.glb").empty(),
        "unfiled assignment is empty");

    Expect(
        editor::DeleteContentBrowserFolder(browser.organization, "Environment/Flora", message)
            == editor::ContentBrowserOrganizationStatus::Ok,
        "delete Flora reparents contents");
    Expect(
        !editor::ContentBrowserFolderExists(browser.organization, "Environment/Flora"),
        "deleted folder is gone");
    Expect(
        editor::ContentBrowserFolderExists(browser.organization, "Environment/Grass"),
        "Grass reparented under Environment");
    Expect(
        editor::ContentBrowserAssignedFolder(browser.organization, "textures/grass.png")
            == "Environment/Grass",
        "texture assignment reparented");
    Expect(PathIsRegularFile(sourceRoot / "textures" / "grass.png"), "delete folder keeps physical texture");
    Expect(PathIsRegularFile(sourceRoot / "models" / "crate.glb"), "delete folder keeps physical model");

    browser.organization.assignments.push_back({"textures/missing.png", "Environment/Grass"});
    browser.organization.favorites.push_back("models/missing.glb");
    editor::ReconcileContentBrowserOrganization(
        browser.organization,
        {"models/crate.glb", "textures/grass.png"});
    Expect(
        editor::ContentBrowserAssignedFolder(browser.organization, "textures/missing.png").empty(),
        "stale assignment reconciled");
    Expect(
        !editor::ContentBrowserIsFavorite(browser.organization, "models/missing.glb"),
        "stale favorite reconciled");

    working.hasTerrain = true;
    working.terrain.textureIdentity = "textures/grass.png";
    Expect(
        editor::AuthoredLevelsProtectTerrainTextureIdentity(working, active, active, "textures/grass.png"),
        "loaded Terrain reference blocks delete");
    Expect(
        !editor::AuthoredLevelsProtectTerrainTextureIdentity(active, active, active, "textures/grass.png"),
        "unreferenced texture is not protected");
    Expect(
        world::TryAddTerrainMaterialLayer(working.terrain, "textures/dirt.png"),
        "extra Terrain layer fixture");
    Expect(
        editor::AuthoredLevelsProtectTerrainTextureIdentity(working, active, active, "textures/dirt.png"),
        "loaded Terrain extra layer blocks delete");
    Expect(
        editor::AuthoredLevelsProtectTerrainTextureIdentity(working, active, active, "textures/grass.png"),
        "base Terrain layer remains protected");
    Expect(
        working.terrain.extraLayers[0].textureIdentity == "textures/dirt.png",
        "assigned extra layer identity is the Texture thumbnail request");

    editor::ThumbnailSourceStamp missingStamp{};
    editor::ThumbnailSourceStamp presentStamp{};
    presentStamp.size = 16;
    Expect(
        editor::ClassifyTextureThumbnailEnsure(false, false, missingStamp, false, missingStamp)
            == editor::TextureThumbnailEnsureDecision::Missing,
        "missing texture thumbnail is a placeholder");
    Expect(
        editor::ClassifyTextureThumbnailEnsure(false, false, missingStamp, true, presentStamp)
            == editor::TextureThumbnailEnsureDecision::Reload,
        "first ensure loads the source PNG");
    Expect(
        editor::ClassifyTextureThumbnailEnsure(true, false, presentStamp, true, presentStamp)
            == editor::TextureThumbnailEnsureDecision::ReuseReady,
        "unchanged stamp reuses GPU texture");
    Expect(
        editor::ClassifyTextureThumbnailEnsure(false, false, missingStamp, true, presentStamp)
            == editor::TextureThumbnailEnsureDecision::Reload,
        "Terrain Materials thumbnail uses the shared TextureThumbnailStore path");
    std::vector<std::string> stale;
    editor::CollectStaleTextureThumbnailIdentities(
        {"textures/grass.png", "textures/gone.png"},
        {"textures/grass.png"},
        stale);
    Expect(stale.size() == 1 && stale[0] == "textures/gone.png", "stale preview identities are forgotten");
    float drawW = 0.0f;
    float drawH = 0.0f;
    editor::ComputeTextureThumbnailDrawSize(32, 16, 96.0f, drawW, drawH);
    Expect(drawW == 96.0f && drawH == 48.0f, "thumbnail aspect is preserved");

    Expect(editor::ComputeContentBrowserThumbnailColumns(96.0f, 96.0f, 8.0f) == 1, "exact one-column width");
    Expect(editor::ComputeContentBrowserThumbnailColumns(95.0f, 96.0f, 8.0f) == 1, "narrower than one tile");
    Expect(editor::ComputeContentBrowserThumbnailColumns(1.0f, 96.0f, 8.0f) == 1, "very narrow positive width");
    Expect(editor::ComputeContentBrowserThumbnailColumns(0.0f, 96.0f, 8.0f) == 1, "zero width is one column");
    Expect(editor::ComputeContentBrowserThumbnailColumns(-32.0f, 96.0f, 8.0f) == 1, "negative width is one column");
    Expect(editor::ComputeContentBrowserThumbnailColumns(200.0f, 96.0f, 8.0f) == 2, "exact two-column boundary");
    Expect(editor::ComputeContentBrowserThumbnailColumns(199.0f, 96.0f, 8.0f) == 1, "just below two-column boundary");
    Expect(editor::ComputeContentBrowserThumbnailColumns(201.0f, 96.0f, 8.0f) == 2, "just above two-column boundary");
    Expect(editor::ComputeContentBrowserThumbnailColumns(512.0f, 96.0f, 8.0f) == 5, "several columns");
    Expect(
        editor::ComputeContentBrowserThumbnailColumns(512.0f, 96.0f, 8.0f)
            > editor::ComputeContentBrowserThumbnailColumns(200.0f, 96.0f, 8.0f),
        "wider pane yields more columns");
    Expect(
        editor::ComputeContentBrowserThumbnailColumns(200.0f, 96.0f, 8.0f)
            == editor::ComputeContentBrowserThumbnailColumns(200.0f, 96.0f, 8.0f),
        "same width is stable");
    Expect(
        editor::ComputeContentBrowserThumbnailColumns(96.0f, 96.0f, 8.0f)
            == editor::ComputeContentBrowserThumbnailColumns(
                200.0f - (200.0f - 96.0f), 96.0f, 8.0f),
        "wide then narrow recovers to one column");

    Expect(
        editor::ContentBrowserStatusFirstLine("Imported texture: textures/grass.png\nRecipe: x")
            == "Imported texture: textures/grass.png",
        "status bar uses the first line only");
    Expect(
        editor::MakeContentBrowserStatusBarText(
            "Imported texture: textures/grass.png\nCanonical source: C:/tmp/grass.png",
            "Texture",
            "grass.png",
            true)
            == "Texture | grass.png | Imported texture: textures/grass.png",
        "status bar is one compact line");
    Expect(
        editor::MakeContentBrowserStatusBarText(
            "Cannot delete textures/grass.png: it is referenced by the currently loaded Terrain",
            "Texture",
            "grass.png",
            true)
            .find("Cannot delete") != std::string::npos,
        "delete refusal stays on the status bar");
    Expect(
        editor::MakeContentBrowserStatusBarText({}, "Texture", "grass.png", true)
            == "Texture | grass.png",
        "selection summary when no operation status");
    Expect(
        editor::MakeContentBrowserStatusBarText({}, {}, {}, false) == "No asset selected",
        "empty browser status");

    Expect(
        editor::FilterContentBrowserEntries(browser.catalog, "crate").size() == 1,
        "existing model filter remains");
    browser.collection = editor::ContentBrowserCollection::AllAssets;
    browser.currentFolderPath.clear();
    browser.filterQuery.clear();
    browser.viewMode = editor::ContentBrowserViewMode::List;
    Expect(
        editor::QueryContentBrowserAssets(browser).size()
            == editor::CollectContentBrowserAssets(browser).size(),
        "list mode uses the same query model");

    const std::filesystem::path viewPath = tempRoot / "user" / "editor_content_browser_view.txt";
    editor::ContentBrowserViewState viewState{};
    viewState.viewMode = editor::ContentBrowserViewMode::List;
    viewState.collection = editor::ContentBrowserCollection::Textures;
    viewState.folderPath = "Environment/Grass";
    Expect(editor::SaveContentBrowserViewStateToPath(viewPath, viewState), "save browser layout");
    const editor::ContentBrowserViewState restored =
        editor::LoadContentBrowserViewStateFromPath(viewPath);
    Expect(restored.viewMode == editor::ContentBrowserViewMode::List, "Thumbnails/List persists");
    Expect(restored.collection == editor::ContentBrowserCollection::Textures, "collection persists");
    Expect(restored.folderPath == "Environment/Grass", "folder layout persists");
    Expect(world::AuthoredLevelDataEqual(working, working), "layout persistence is not Level data");

    RemoveTree(tempRoot);
    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d ContentBrowserAssetLibraryTest checks failed\n", gFailures);
        return 1;
    }
    return 0;
}
