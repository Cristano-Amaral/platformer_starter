#include "assets/RuntimePngDelete.h"

#include <string>
#include <system_error>
#include <utility>

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
    const std::filesystem::path relative =
        std::filesystem::path(std::string(identity)).lexically_normal();
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

RuntimePngDeleteResult MakeResult(
    RuntimePngDeleteStatus status,
    std::string message,
    std::string identity = {})
{
    RuntimePngDeleteResult result{};
    result.status = status;
    result.message = std::move(message);
    result.canonicalIdentity = std::move(identity);
    return result;
}

bool RemoveAuthorizedPng(const std::filesystem::path& path, std::string& error)
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
    if (path.extension().string() != std::string(kRuntimePngExtension))
    {
        error = "refusing to delete a non-.png path: " + path.generic_string();
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

const char* RuntimePngDeleteStatusName(RuntimePngDeleteStatus status)
{
    switch (status)
    {
    case RuntimePngDeleteStatus::Deleted:
        return "Deleted";
    case RuntimePngDeleteStatus::NoSelection:
        return "NoSelection";
    case RuntimePngDeleteStatus::InvalidIdentity:
        return "InvalidIdentity";
    case RuntimePngDeleteStatus::UnsafePath:
        return "UnsafePath";
    case RuntimePngDeleteStatus::MissingSource:
        return "MissingSource";
    case RuntimePngDeleteStatus::Error:
        return "Error";
    }
    return "Error";
}

bool RuntimePngDeleteSucceeded(RuntimePngDeleteStatus status)
{
    return status == RuntimePngDeleteStatus::Deleted;
}

bool TryResolveRuntimePngDeletePaths(
    std::string_view canonicalIdentity,
    const RuntimePngDeleteRoots& roots,
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
    if (!TryParseRuntimePngIdentity(canonicalIdentity, fileName, &error))
    {
        return false;
    }

    if (!JoinIdentityUnderRoot(roots.sourceRoot, canonicalIdentity, sourcePath, error, "source"))
    {
        return false;
    }
    if (!JoinIdentityUnderRoot(roots.cookedRoot, canonicalIdentity, cookedPath, error, "cooked"))
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

RuntimePngDeleteResult DeleteRuntimePng(
    std::string_view canonicalIdentity,
    const RuntimePngDeleteRoots& roots,
    SourceTextureCatalog* catalog)
{
    if (canonicalIdentity.empty())
    {
        return MakeResult(
            RuntimePngDeleteStatus::NoSelection, "no asset is selected for delete");
    }

    std::string fileName;
    std::string parseError;
    if (!TryParseRuntimePngIdentity(canonicalIdentity, fileName, &parseError))
    {
        return MakeResult(
            RuntimePngDeleteStatus::InvalidIdentity,
            parseError,
            std::string(canonicalIdentity));
    }
    (void)fileName;

    std::filesystem::path sourcePath;
    std::filesystem::path cookedPath;
    std::vector<std::filesystem::path> stagedPaths;
    std::string resolveError;
    if (!TryResolveRuntimePngDeletePaths(
            canonicalIdentity, roots, sourcePath, cookedPath, stagedPaths, resolveError))
    {
        return MakeResult(
            RuntimePngDeleteStatus::UnsafePath,
            resolveError,
            std::string(canonicalIdentity));
    }
    if (!PathIsDirectory(roots.sourceRoot))
    {
        return MakeResult(
            RuntimePngDeleteStatus::UnsafePath,
            "source root is not a directory",
            std::string(canonicalIdentity));
    }
    if (!PathExists(sourcePath))
    {
        return MakeResult(
            RuntimePngDeleteStatus::MissingSource,
            "canonical source asset is missing; generated files were not deleted",
            std::string(canonicalIdentity));
    }
    if (!PathIsRegularFile(sourcePath))
    {
        return MakeResult(
            RuntimePngDeleteStatus::UnsafePath,
            "canonical source path is not a regular file",
            std::string(canonicalIdentity));
    }

    RuntimePngDeleteResult result =
        MakeResult(RuntimePngDeleteStatus::Error, {}, std::string(canonicalIdentity));
    std::string removeError;
    if (!PathExists(cookedPath))
    {
        result.cookedWasMissing = true;
    }
    else if (!RemoveAuthorizedPng(cookedPath, removeError))
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
        if (!RemoveAuthorizedPng(stagedPath, removeError))
        {
            result.message = "staged cleanup failed; source was not deleted. " + removeError;
            return result;
        }
        ++result.stagedRemovedCount;
    }

    if (!RemoveAuthorizedPng(sourcePath, removeError))
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
    result.status = RuntimePngDeleteStatus::Deleted;
    result.message = "Deleted texture: ";
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
