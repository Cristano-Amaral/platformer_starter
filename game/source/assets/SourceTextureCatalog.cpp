#include "assets/SourceTextureCatalog.h"

namespace assets
{
void SourceTextureCatalog::Clear()
{
    entries.clear();
}

void SourceTextureCatalog::Refresh(const std::filesystem::path& sourceRoot)
{
    entries.clear();
    const std::vector<std::string> identities = CollectSourceRuntimePngIdentities(sourceRoot);
    entries.reserve(identities.size());
    for (const std::string& identity : identities)
    {
        SourceTextureCatalogEntry entry{};
        entry.canonicalIdentity = identity;
        entry.assetType = std::string(kRuntimePngAssetType);
        entry.displayName = RuntimePngDisplayName(identity);
        entries.push_back(std::move(entry));
    }
}

const std::vector<SourceTextureCatalogEntry>& SourceTextureCatalog::Entries() const
{
    return entries;
}

const SourceTextureCatalogEntry* SourceTextureCatalog::Find(std::string_view canonicalIdentity) const
{
    for (const SourceTextureCatalogEntry& entry : entries)
    {
        if (entry.canonicalIdentity == canonicalIdentity)
        {
            return &entry;
        }
    }
    return nullptr;
}

std::size_t SourceTextureCatalog::Count() const
{
    return entries.size();
}

std::vector<std::string> SourceTextureCatalog::Identities() const
{
    std::vector<std::string> identities;
    identities.reserve(entries.size());
    for (const SourceTextureCatalogEntry& entry : entries)
    {
        identities.push_back(entry.canonicalIdentity);
    }
    return identities;
}
}
