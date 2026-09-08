#include "editor/ContentBrowser.h"

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
    for (const assets::StaticModelCatalogEntry& entry : state.catalog.Entries())
    {
        if (entry.canonicalIdentity == canonicalIdentity)
        {
            state.selectedIdentity = entry.canonicalIdentity;
            return;
        }
    }
    ClearContentBrowserSelection(state);
}

void RefreshContentBrowser(ContentBrowserState& state, const std::filesystem::path& sourceRoot)
{
    const std::string previousIdentity = state.selectedIdentity;
    state.catalog.Refresh(sourceRoot);
    if (previousIdentity.empty())
    {
        state.deleteConfirmOpen = false;
        return;
    }
    SelectContentBrowserIdentity(state, previousIdentity);
}
}
