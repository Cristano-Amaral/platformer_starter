#include "editor/ContentBrowser.h"

#include "assets/RuntimePng.h"
#include "assets/StaticGlb.h"

#include <algorithm>
#include <cctype>

namespace editor
{
namespace
{
char AsciiToLower(char ch)
{
    return static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
}

std::string AsciiToLowerCopy(std::string_view text)
{
    std::string lowered(text);
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), AsciiToLower);
    return lowered;
}

ContentBrowserAssetEntry MakeModelEntry(
    const assets::StaticModelCatalogEntry& entry,
    const ContentBrowserOrganization& organization)
{
    ContentBrowserAssetEntry asset{};
    asset.kind = ContentBrowserAssetKind::Model;
    asset.canonicalIdentity = entry.canonicalIdentity;
    asset.assetType = entry.assetType;
    asset.displayName = entry.displayName;
    asset.logicalFolder = ContentBrowserAssignedFolder(organization, entry.canonicalIdentity);
    asset.favorite = ContentBrowserIsFavorite(organization, entry.canonicalIdentity);
    return asset;
}

ContentBrowserAssetEntry MakeTextureEntry(
    const assets::SourceTextureCatalogEntry& entry,
    const ContentBrowserOrganization& organization)
{
    ContentBrowserAssetEntry asset{};
    asset.kind = ContentBrowserAssetKind::Texture;
    asset.canonicalIdentity = entry.canonicalIdentity;
    asset.assetType = entry.assetType;
    asset.displayName = entry.displayName;
    asset.logicalFolder = ContentBrowserAssignedFolder(organization, entry.canonicalIdentity);
    asset.favorite = ContentBrowserIsFavorite(organization, entry.canonicalIdentity);
    return asset;
}

std::vector<std::string> KnownIdentities(const ContentBrowserState& state)
{
    std::vector<std::string> identities;
    identities.reserve(state.catalog.Count() + state.textureCatalog.Count());
    for (const assets::StaticModelCatalogEntry& entry : state.catalog.Entries())
    {
        identities.push_back(entry.canonicalIdentity);
    }
    for (const assets::SourceTextureCatalogEntry& entry : state.textureCatalog.Entries())
    {
        identities.push_back(entry.canonicalIdentity);
    }
    return identities;
}
}

bool ContentBrowserQueryMatches(std::string_view haystack, std::string_view query)
{
    if (query.empty())
    {
        return true;
    }
    const std::string loweredHaystack = AsciiToLowerCopy(haystack);
    const std::string loweredQuery = AsciiToLowerCopy(query);
    return loweredHaystack.find(loweredQuery) != std::string::npos;
}

const char* ContentBrowserAssetKindName(ContentBrowserAssetKind kind)
{
    switch (kind)
    {
    case ContentBrowserAssetKind::Model:
        return "Model";
    case ContentBrowserAssetKind::Texture:
        return "Texture";
    }
    return "Model";
}

ContentBrowserAssetKind ClassifyContentBrowserIdentity(std::string_view canonicalIdentity)
{
    if (ContentBrowserIdentityIsTexture(canonicalIdentity))
    {
        return ContentBrowserAssetKind::Texture;
    }
    return ContentBrowserAssetKind::Model;
}

bool ContentBrowserIdentityIsModel(std::string_view canonicalIdentity)
{
    std::string fileName;
    return assets::TryParseStaticModelIdentity(canonicalIdentity, fileName, nullptr);
}

bool ContentBrowserIdentityIsTexture(std::string_view canonicalIdentity)
{
    std::string fileName;
    return assets::TryParseRuntimePngIdentity(canonicalIdentity, fileName, nullptr);
}

std::vector<assets::StaticModelCatalogEntry> FilterContentBrowserEntries(
    const assets::StaticModelCatalog& catalog,
    std::string_view query)
{
    std::vector<assets::StaticModelCatalogEntry> filtered;
    for (const assets::StaticModelCatalogEntry& entry : catalog.Entries())
    {
        if (ContentBrowserQueryMatches(entry.canonicalIdentity, query)
            || ContentBrowserQueryMatches(entry.displayName, query)
            || ContentBrowserQueryMatches(entry.assetType, query))
        {
            filtered.push_back(entry);
        }
    }
    return filtered;
}

bool ContentBrowserAssetIsVisibleInFolderScope(
    std::string_view assignedFolder,
    std::string_view scopeFolder,
    bool includeDescendants)
{
    if (scopeFolder.empty())
    {
        return assignedFolder.empty();
    }
    return ContentBrowserFolderContainsPath(scopeFolder, assignedFolder, includeDescendants);
}

std::vector<ContentBrowserAssetEntry> CollectContentBrowserAssets(const ContentBrowserState& state)
{
    std::vector<ContentBrowserAssetEntry> assets;
    assets.reserve(state.catalog.Count() + state.textureCatalog.Count());
    for (const assets::StaticModelCatalogEntry& entry : state.catalog.Entries())
    {
        assets.push_back(MakeModelEntry(entry, state.organization));
    }
    for (const assets::SourceTextureCatalogEntry& entry : state.textureCatalog.Entries())
    {
        assets.push_back(MakeTextureEntry(entry, state.organization));
    }
    std::sort(
        assets.begin(),
        assets.end(),
        [](const ContentBrowserAssetEntry& left, const ContentBrowserAssetEntry& right) {
            if (left.canonicalIdentity == right.canonicalIdentity)
            {
                return static_cast<int>(left.kind) < static_cast<int>(right.kind);
            }
            return left.canonicalIdentity < right.canonicalIdentity;
        });
    return assets;
}

std::vector<ContentBrowserAssetEntry> QueryContentBrowserAssets(const ContentBrowserState& state)
{
    std::vector<ContentBrowserAssetEntry> visible;
    for (const ContentBrowserAssetEntry& entry : CollectContentBrowserAssets(state))
    {
        switch (state.collection)
        {
        case ContentBrowserCollection::Models:
            if (entry.kind != ContentBrowserAssetKind::Model)
            {
                continue;
            }
            break;
        case ContentBrowserCollection::Textures:
            if (entry.kind != ContentBrowserAssetKind::Texture)
            {
                continue;
            }
            break;
        case ContentBrowserCollection::Favorites:
            if (!entry.favorite)
            {
                continue;
            }
            break;
        case ContentBrowserCollection::Folders:
            if (!ContentBrowserAssetIsVisibleInFolderScope(
                    entry.logicalFolder,
                    state.currentFolderPath,
                    kContentBrowserFolderSearchIncludesDescendants))
            {
                continue;
            }
            break;
        case ContentBrowserCollection::AllAssets:
            break;
        }
        if (!ContentBrowserQueryMatches(entry.canonicalIdentity, state.filterQuery)
            && !ContentBrowserQueryMatches(entry.displayName, state.filterQuery)
            && !ContentBrowserQueryMatches(entry.assetType, state.filterQuery)
            && !ContentBrowserQueryMatches(ContentBrowserAssetKindName(entry.kind), state.filterQuery)
            && !ContentBrowserQueryMatches(entry.logicalFolder, state.filterQuery))
        {
            continue;
        }
        visible.push_back(entry);
    }
    return visible;
}

void ClearContentBrowserSelection(ContentBrowserState& state)
{
    state.selectedIdentity.clear();
    state.deleteConfirmOpen = false;
}

void SelectContentBrowserIdentity(ContentBrowserState& state, std::string_view canonicalIdentity)
{
    if (canonicalIdentity.empty())
    {
        ClearContentBrowserSelection(state);
        return;
    }
    if (state.catalog.Find(canonicalIdentity) != nullptr
        || state.textureCatalog.Find(canonicalIdentity) != nullptr)
    {
        state.selectedIdentity = std::string(canonicalIdentity);
        return;
    }
    ClearContentBrowserSelection(state);
}

void RefreshContentBrowser(ContentBrowserState& state, const std::filesystem::path& sourceRoot)
{
    const std::string previousIdentity = state.selectedIdentity;
    state.catalog.Refresh(sourceRoot);
    state.textureCatalog.Refresh(sourceRoot);
    std::string loadMessage;
    (void)LoadContentBrowserOrganization(
        ContentBrowserOrganizationPath(sourceRoot), state.organization, loadMessage);
    const std::size_t assignmentCount = state.organization.assignments.size();
    const std::size_t favoriteCount = state.organization.favorites.size();
    ReconcileContentBrowserOrganization(state.organization, KnownIdentities(state));
    if (state.organization.assignments.size() != assignmentCount
        || state.organization.favorites.size() != favoriteCount)
    {
        std::string saveMessage;
        (void)SaveContentBrowserOrganization(
            ContentBrowserOrganizationPath(sourceRoot), state.organization, saveMessage);
    }
    if (!state.currentFolderPath.empty()
        && !ContentBrowserFolderExists(state.organization, state.currentFolderPath))
    {
        state.currentFolderPath.clear();
    }
    if (previousIdentity.empty())
    {
        state.deleteConfirmOpen = false;
        return;
    }
    SelectContentBrowserIdentity(state, previousIdentity);
}

void SetContentBrowserCollection(
    ContentBrowserState& state,
    ContentBrowserCollection collection,
    std::string_view folderPath)
{
    state.collection = collection;
    if (collection == ContentBrowserCollection::Folders)
    {
        state.currentFolderPath = std::string(folderPath);
    }
    else
    {
        state.currentFolderPath.clear();
    }
}

bool PersistContentBrowserViewState(const ContentBrowserState& state)
{
    ContentBrowserViewState view{};
    view.viewMode = state.viewMode;
    view.collection = state.collection;
    view.folderPath = state.currentFolderPath;
    return SaveContentBrowserViewState(view);
}

bool PersistContentBrowserOrganization(
    ContentBrowserState& state,
    const std::filesystem::path& sourceRoot)
{
    std::string message;
    const ContentBrowserOrganizationStatus status = SaveContentBrowserOrganization(
        ContentBrowserOrganizationPath(sourceRoot), state.organization, message);
    if (!message.empty())
    {
        state.statusMessage = message;
    }
    return ContentBrowserOrganizationSucceeded(status);
}
}
