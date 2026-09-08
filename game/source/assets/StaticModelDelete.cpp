#include "assets/StaticModelDelete.h"

#include <system_error>
#include <utility>
#include <string>

namespace assets
{
namespace
{
bool PathExists(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::exists(path, error) && !error;
}

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

bool RelativeStaysInsideRoot(
    const std::filesystem::path& root,
    const std::filesystem::path& relative)
{
    if (root.empty() || !root.is_absolute() || relative.empty() || relative.is_absolute()
        || relative.has_root_name())
    {
        return false;
    }
    for (const std::filesystem::path& part : relative)
    {
        if (part == "..")
        {
            return false;
        }
    }
    const std::filesystem::path resolved = (root / relative).lexically_normal();
    if (!resolved.is_absolute())
    {
        return false;
    }
    const std::filesystem::path relativeToRoot = resolved.lexically_relative(root);
    if (relativeToRoot.empty())
    {
        return false;
    }
    for (const std::filesystem::path& part : relativeToRoot)
    {
        if (part == "..")
        {
            return false;
        }
    }
    return true;
}

bool JoinIdentityUnderRoot(
    const std::filesystem::path& root,
    std::string_view identity,
    std::filesystem::path& joined,
    std::string& error,
    const char* rootLabel)
{
    joined.clear();
    if (root.empty() || !root.is_absolute())
    {
        error = std::string(rootLabel) + " root is missing or not absolute";
        return false;
    }
    const std::filesystem::path relative = std::filesystem::path(std::string(identity)).lexically_normal();
    if (!RelativeStaysInsideRoot(root, relative))
    {
        error = std::string(rootLabel) + " mapping would leave the authorized root";
        return false;
    }
    joined = (root / relative).lexically_normal();
    const std::filesystem::path relativeCheck = joined.lexically_relative(root);
    if (relativeCheck.empty() || relativeCheck.generic_string() != std::string(identity))
    {
        error = std::string(rootLabel) + " mapping would leave the authorized root";
        joined.clear();
        return false;
    }
    return true;
}

StaticModelDeleteResult MakeResult(
    StaticModelDeleteStatus status,
    std::string message,
    std::string identity = {})
{
    StaticModelDeleteResult result{};
    result.status = status;
    result.message = std::move(message);
    result.canonicalIdentity = std::move(identity);
    return result;
}

bool RemoveAuthorizedGlb(const std::filesystem::path& path, std::string& error)
{
    if (!PathExists(path))
    {
        return true;
    }
    if (!PathIsRegularFile(path))
    {
        error = "authorized path exists but is not a regular file: " + path.generic_string();
        return false;
    }
    if (path.extension().string() != std::string(kStaticGlbExtension))
    {
        error = "refusing to delete a non-.glb path: " + path.generic_string();
        return false;
    }
    std::error_code removeError;
    if (!std::filesystem::remove(path, removeError) || removeError)
    {
        error = "failed to remove " + path.generic_string();
        return false;
    }
    return true;
}
}

const char* StaticModelDeleteStatusName(StaticModelDeleteStatus status)
{
    switch (status)
    {
    case StaticModelDeleteStatus::Deleted:
        return "Deleted";
    case StaticModelDeleteStatus::NoSelection:
        return "NoSelection";
    case StaticModelDeleteStatus::InvalidIdentity:
        return "InvalidIdentity";
    case StaticModelDeleteStatus::UnsafePath:
        return "UnsafePath";
    case StaticModelDeleteStatus::MissingSource:
        return "MissingSource";
    case StaticModelDeleteStatus::Error:
        return "Error";
    }
    return "Error";
}

bool StaticModelDeleteSucceeded(StaticModelDeleteStatus status)
{
    return status == StaticModelDeleteStatus::Deleted;
}

bool TryResolveStaticModelDeletePaths(
    std::string_view canonicalIdentity,
    const StaticModelDeleteRoots& roots,
    std::filesystem::path& sourcePath,
    std::filesystem::path& cookedPath,
    std::vector<std::filesystem::path>& stagedPaths,
    std::string& error)
{
    sourcePath.clear();
    cookedPath.clear();
    stagedPaths.clear();
    error.clear();

    std::string fileName;
    if (!TryParseStaticModelIdentity(canonicalIdentity, fileName, &error))
    {
        return false;
    }

    if (!JoinIdentityUnderRoot(
            roots.sourceRoot, canonicalIdentity, sourcePath, error, "source"))
    {
        return false;
    }
    if (!JoinIdentityUnderRoot(
            roots.cookedRoot, canonicalIdentity, cookedPath, error, "cooked"))
    {
        return false;
    }

    for (const std::filesystem::path& stagedRoot : roots.stagedRoots)
    {
        if (stagedRoot.empty())
        {
            continue;
        }
        std::filesystem::path stagedPath;
        if (!JoinIdentityUnderRoot(stagedRoot, canonicalIdentity, stagedPath, error, "staged"))
        {
            sourcePath.clear();
            cookedPath.clear();
            stagedPaths.clear();
            return false;
        }
        stagedPaths.push_back(std::move(stagedPath));
    }
    return true;
}

StaticModelDeleteResult DeleteStaticModel(
    std::string_view canonicalIdentity,
    const StaticModelDeleteRoots& roots,
    StaticModelCatalog* catalog)
{
    if (canonicalIdentity.empty())
    {
        return MakeResult(
            StaticModelDeleteStatus::NoSelection, "no asset is selected for delete");
    }

    std::string fileName;
    std::string parseError;
    if (!TryParseStaticModelIdentity(canonicalIdentity, fileName, &parseError))
    {
        return MakeResult(
            StaticModelDeleteStatus::InvalidIdentity,
            parseError,
            std::string(canonicalIdentity));
    }
    (void)fileName;

    std::filesystem::path sourcePath;
    std::filesystem::path cookedPath;
    std::vector<std::filesystem::path> stagedPaths;
    std::string resolveError;
    if (!TryResolveStaticModelDeletePaths(
            canonicalIdentity, roots, sourcePath, cookedPath, stagedPaths, resolveError))
    {
        return MakeResult(
            StaticModelDeleteStatus::UnsafePath,
            resolveError,
            std::string(canonicalIdentity));
    }

    if (!PathIsDirectory(roots.sourceRoot))
    {
        return MakeResult(
            StaticModelDeleteStatus::UnsafePath,
            "source root is not a directory",
            std::string(canonicalIdentity));
    }
    if (!PathExists(sourcePath))
    {
        return MakeResult(
            StaticModelDeleteStatus::MissingSource,
            "canonical source asset is missing; generated files were not deleted",
            std::string(canonicalIdentity));
    }
    if (!PathIsRegularFile(sourcePath))
    {
        return MakeResult(
            StaticModelDeleteStatus::UnsafePath,
            "canonical source path is not a regular file",
            std::string(canonicalIdentity));
    }

    StaticModelDeleteResult result =
        MakeResult(StaticModelDeleteStatus::Error, {}, std::string(canonicalIdentity));
    std::string removeError;

    if (!PathExists(cookedPath))
    {
        result.cookedWasMissing = true;
    }
    else if (!RemoveAuthorizedGlb(cookedPath, removeError))
    {
        result.message = "cooked cleanup failed; source was not deleted. " + removeError;
        return result;
    }
    else
    {
        result.cookedRemoved = true;
    }

    for (const std::filesystem::path& stagedPath : stagedPaths)
    {
        if (!PathExists(stagedPath))
        {
            ++result.stagedMissingCount;
            continue;
        }
        if (!RemoveAuthorizedGlb(stagedPath, removeError))
        {
            result.message =
                "staged cleanup failed; source was not deleted. " + removeError;
            return result;
        }
        ++result.stagedRemovedCount;
    }

    if (!RemoveAuthorizedGlb(sourcePath, removeError))
    {
        result.message =
            "source delete failed after generated cleanup. Retry delete. " + removeError;
        return result;
    }
    result.sourceRemoved = true;

    if (catalog != nullptr)
    {
        catalog->Refresh(roots.sourceRoot);
    }

    result.status = StaticModelDeleteStatus::Deleted;
    result.message = "Deleted static GLB: ";
    result.message += canonicalIdentity;
    result.message += "\nSource removed.";
    if (result.cookedRemoved)
    {
        result.message += " Cooked copy removed.";
    }
    else
    {
        result.message += " Cooked copy was already absent.";
    }
    result.message += " Staged copies removed: ";
    result.message += std::to_string(result.stagedRemovedCount);
    result.message += ".";
    result.message += "\nThis is not a level edit.";
    return result;
}
}
