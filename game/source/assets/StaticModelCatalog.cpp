#include "assets/StaticModelCatalog.h"

#include <algorithm>
#include <system_error>

namespace assets
{
namespace
{
bool PathIsRegularFile(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::is_regular_file(path, error) && !error;
}

bool PathIsDirectory(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::is_directory(path, error) && !error;
}
}

void StaticModelCatalog::Clear()
{
    entries.clear();
}

void StaticModelCatalog::Refresh(const std::filesystem::path& sourceRoot)
{
    entries.clear();
    if (sourceRoot.empty() || !sourceRoot.is_absolute() || !PathIsDirectory(sourceRoot))
    {
        return;
    }

    const std::filesystem::path modelsRoot =
        (sourceRoot / std::string(kStaticModelsLogicalDirectory)).lexically_normal();
    if (!PathIsDirectory(modelsRoot))
    {
        return;
    }

    std::error_code iteratorError;
    const std::filesystem::directory_iterator end{};
    for (std::filesystem::directory_iterator it(modelsRoot, iteratorError);
         !iteratorError && it != end;
         it.increment(iteratorError))
    {
        if (iteratorError)
        {
            break;
        }
        const std::filesystem::path& path = it->path();
        if (!PathIsRegularFile(path))
        {
            continue;
        }
        const std::string fileName = path.filename().string();
        std::string nameReason;
        if (!IsSafeStaticGlbFileName(fileName, &nameReason))
        {
            continue;
        }
        const StaticGlbValidation validation = ValidateStaticGlbFile(path);
        if (validation.status != StaticGlbStatus::Ok)
        {
            continue;
        }
        StaticModelCatalogEntry entry{};
        entry.canonicalIdentity = CanonicalStaticModelIdentity(fileName);
        entry.assetType = std::string(kStaticGlbAssetType);
        entry.displayName = fileName;
        entries.push_back(std::move(entry));
    }

    std::sort(
        entries.begin(),
        entries.end(),
        [](const StaticModelCatalogEntry& left, const StaticModelCatalogEntry& right) {
            return left.canonicalIdentity < right.canonicalIdentity;
        });
    entries.erase(
        std::unique(
            entries.begin(),
            entries.end(),
            [](const StaticModelCatalogEntry& left, const StaticModelCatalogEntry& right) {
                return left.canonicalIdentity == right.canonicalIdentity;
            }),
        entries.end());
}

const std::vector<StaticModelCatalogEntry>& StaticModelCatalog::Entries() const
{
    return entries;
}

const StaticModelCatalogEntry* StaticModelCatalog::Find(std::string_view canonicalIdentity) const
{
    for (const StaticModelCatalogEntry& entry : entries)
    {
        if (entry.canonicalIdentity == canonicalIdentity)
        {
            return &entry;
        }
    }
    return nullptr;
}

std::size_t StaticModelCatalog::Count() const
{
    return entries.size();
}
}
