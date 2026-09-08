#pragma once

// Derived catalog of supported static GLB source models. Identity is the
// normalized project-relative path (models/<file>.glb). Not a scene system,
// not persisted, and not an ImGui authority.

#include "assets/StaticGlb.h"

#include <filesystem>
#include <string>
#include <vector>

namespace assets
{
struct StaticModelCatalogEntry
{
    std::string canonicalIdentity;
    std::string assetType;
};

class StaticModelCatalog
{
public:
    void Refresh(const std::filesystem::path& sourceRoot);
    void Clear();

    const std::vector<StaticModelCatalogEntry>& Entries() const;
    const StaticModelCatalogEntry* Find(std::string_view canonicalIdentity) const;
    std::size_t Count() const;

private:
    std::vector<StaticModelCatalogEntry> entries;
};
}
