#pragma once

// Development Content Browser query/selection model. Not ImGui and not a
// second asset registry. StaticModelCatalog remains authority.

#include "assets/StaticModelCatalog.h"
#include "editor/ContentBrowserView.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace editor
{
struct ContentBrowserState
{
    assets::StaticModelCatalog catalog;
    std::string filterQuery;
    std::string selectedIdentity;
    std::string statusMessage;
    bool deleteConfirmOpen = false;
    ContentBrowserViewMode viewMode = kDefaultContentBrowserViewMode;
};

bool ContentBrowserQueryMatches(std::string_view haystack, std::string_view query);
std::vector<assets::StaticModelCatalogEntry> FilterContentBrowserEntries(
    const assets::StaticModelCatalog& catalog,
    std::string_view query);
void RefreshContentBrowser(
    ContentBrowserState& state,
    const std::filesystem::path& sourceRoot);
void SelectContentBrowserIdentity(ContentBrowserState& state, std::string_view canonicalIdentity);
void ClearContentBrowserSelection(ContentBrowserState& state);

// Delete confirmation is UI-only. Cancel must not call assets::DeleteStaticModel.
inline bool ContentBrowserDeleteConfirmed(bool confirmed)
{
    return confirmed;
}
}
