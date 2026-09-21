#pragma once

// Development Content Browser query/selection model. Not ImGui and not a
// second asset registry. StaticModelCatalog remains model authority;
// SourceTextureCatalog remains texture authority.

#include "assets/SourceTextureCatalog.h"
#include "assets/StaticModelCatalog.h"
#include "editor/ContentBrowserOrganization.h"
#include "editor/ContentBrowserView.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace editor
{
enum class ContentBrowserAssetKind
{
    Model,
    Texture,
};

struct ContentBrowserAssetEntry
{
    ContentBrowserAssetKind kind = ContentBrowserAssetKind::Model;
    std::string canonicalIdentity;
    std::string assetType;
    std::string displayName;
    std::string logicalFolder;
    bool favorite = false;
};

struct ContentBrowserState
{
    assets::StaticModelCatalog catalog;
    assets::SourceTextureCatalog textureCatalog;
    ContentBrowserOrganization organization;
    ContentBrowserCollection collection = kDefaultContentBrowserCollection;
    std::string currentFolderPath;
    std::string filterQuery;
    std::string selectedIdentity;
    std::string statusMessage;
    bool deleteConfirmOpen = false;
    bool folderNameDialogOpen = false;
    bool folderRenameDialogOpen = false;
    bool folderDeleteConfirmOpen = false;
    bool folderCreateSubfolder = false;
    std::string folderNameInput;
    ContentBrowserViewMode viewMode = kDefaultContentBrowserViewMode;
};

bool ContentBrowserQueryMatches(std::string_view haystack, std::string_view query);
const char* ContentBrowserAssetKindName(ContentBrowserAssetKind kind);
ContentBrowserAssetKind ClassifyContentBrowserIdentity(std::string_view canonicalIdentity);
bool ContentBrowserIdentityIsModel(std::string_view canonicalIdentity);
bool ContentBrowserIdentityIsTexture(std::string_view canonicalIdentity);

std::vector<assets::StaticModelCatalogEntry> FilterContentBrowserEntries(
    const assets::StaticModelCatalog& catalog,
    std::string_view query);

std::vector<ContentBrowserAssetEntry> CollectContentBrowserAssets(
    const ContentBrowserState& state);
std::vector<ContentBrowserAssetEntry> QueryContentBrowserAssets(
    const ContentBrowserState& state);

void RefreshContentBrowser(
    ContentBrowserState& state,
    const std::filesystem::path& sourceRoot);
void SelectContentBrowserIdentity(ContentBrowserState& state, std::string_view canonicalIdentity);
void ClearContentBrowserSelection(ContentBrowserState& state);
void SetContentBrowserCollection(
    ContentBrowserState& state,
    ContentBrowserCollection collection,
    std::string_view folderPath = {});
bool PersistContentBrowserViewState(const ContentBrowserState& state);
bool PersistContentBrowserOrganization(
    ContentBrowserState& state,
    const std::filesystem::path& sourceRoot);

bool ContentBrowserAssetIsVisibleInFolderScope(
    std::string_view assignedFolder,
    std::string_view scopeFolder,
    bool includeDescendants);

inline bool ContentBrowserDeleteConfirmed(bool confirmed)
{
    return confirmed;
}

// Thumbnail wrap: columns = max(1, floor((availableWidth + spacing) / (tileWidth + spacing))).
// availableWidth is the asset-pane content width, not the application window width.
inline int ComputeContentBrowserThumbnailColumns(
    float availableWidth,
    float tileWidth,
    float spacing)
{
    if (!(availableWidth > 0.0f) || !(tileWidth > 0.0f))
    {
        return 1;
    }
    const float gap = spacing > 0.0f ? spacing : 0.0f;
    const float stride = tileWidth + gap;
    if (!(stride > 0.0f))
    {
        return 1;
    }
    const int columns = static_cast<int>((availableWidth + gap) / stride);
    return columns < 1 ? 1 : columns;
}

inline std::string ContentBrowserStatusFirstLine(std::string_view text)
{
    std::size_t begin = 0;
    while (begin < text.size() && (text[begin] == '\n' || text[begin] == '\r'))
    {
        ++begin;
    }
    std::size_t end = begin;
    while (end < text.size() && text[end] != '\n' && text[end] != '\r')
    {
        ++end;
    }
    return std::string(text.substr(begin, end - begin));
}

inline std::string MakeContentBrowserStatusBarText(
    std::string_view statusMessage,
    std::string_view kindName,
    std::string_view displayName,
    bool hasSelection)
{
    const std::string status = ContentBrowserStatusFirstLine(statusMessage);
    if (!status.empty())
    {
        if (hasSelection && !kindName.empty() && !displayName.empty())
        {
            std::string line;
            line.append(kindName);
            line += " | ";
            line.append(displayName);
            line += " | ";
            line += status;
            return line;
        }
        return status;
    }
    if (!hasSelection)
    {
        return "No asset selected";
    }
    std::string line;
    line.append(kindName);
    if (!displayName.empty())
    {
        line += " | ";
        line.append(displayName);
    }
    return line;
}
}
