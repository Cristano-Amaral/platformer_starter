#pragma once

// Derived catalog of standalone runtime PNG source textures. Identity is
// textures/<file>.png. Not a scene system, not persisted, and not a
// generalized runtime AssetManager. StaticModelCatalog remains independent.

#include "assets/RuntimePng.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace assets
{
struct SourceTextureCatalogEntry
{
    std::string canonicalIdentity;
    std::string assetType;
    std::string displayName;
};

class SourceTextureCatalog
{
public:
    void Refresh(const std::filesystem::path& sourceRoot);
    void Clear();

    const std::vector<SourceTextureCatalogEntry>& Entries() const;
    const SourceTextureCatalogEntry* Find(std::string_view canonicalIdentity) const;
    std::size_t Count() const;
    std::vector<std::string> Identities() const;

private:
    std::vector<SourceTextureCatalogEntry> entries;
};
}
