#include "assets/StaticGlbImport.h"

#include <fstream>
#include <system_error>
#include <utility>
#include <vector>

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

bool RemovePath(const std::filesystem::path& path)
{
    std::error_code error;
    std::filesystem::remove(path, error);
    return !error;
}

bool RelativeStaysInsideRoot(
    const std::filesystem::path& root,
    const std::filesystem::path& relative)
{
    if (relative.empty() || relative.is_absolute() || relative.has_root_name())
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

StaticGlbImportResult MakeResult(
    StaticGlbImportStatus status,
    std::string message,
    std::string identity = {},
    std::filesystem::path destination = {})
{
    StaticGlbImportResult result{};
    result.status = status;
    result.message = std::move(message);
    result.canonicalIdentity = std::move(identity);
    result.destinationPath = std::move(destination);
    return result;
}

StaticGlbImportStatus StatusFromValidation(StaticGlbStatus status)
{
    switch (status)
    {
    case StaticGlbStatus::Ok:
        return StaticGlbImportStatus::Imported;
    case StaticGlbStatus::Missing:
        return StaticGlbImportStatus::Missing;
    case StaticGlbStatus::NotAFile:
        return StaticGlbImportStatus::NotAFile;
    case StaticGlbStatus::UnsupportedExtension:
        return StaticGlbImportStatus::UnsupportedExtension;
    case StaticGlbStatus::UnsafeName:
        return StaticGlbImportStatus::UnsafeName;
    case StaticGlbStatus::InvalidContainer:
        return StaticGlbImportStatus::InvalidContainer;
    case StaticGlbStatus::Incompatible:
        return StaticGlbImportStatus::Incompatible;
    }
    return StaticGlbImportStatus::Error;
}

bool CopyFileToTemporary(
    const std::filesystem::path& source,
    const std::filesystem::path& temporary)
{
    std::ifstream in(source, std::ios::binary);
    if (!in)
    {
        return false;
    }
    std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        return false;
    }
    out << in.rdbuf();
    out.flush();
    return static_cast<bool>(in) && static_cast<bool>(out);
}
}

const char* StaticGlbImportStatusName(StaticGlbImportStatus status)
{
    switch (status)
    {
    case StaticGlbImportStatus::Imported:
        return "Imported";
    case StaticGlbImportStatus::Cancelled:
        return "Cancelled";
    case StaticGlbImportStatus::Missing:
        return "Missing";
    case StaticGlbImportStatus::NotAFile:
        return "NotAFile";
    case StaticGlbImportStatus::UnsupportedExtension:
        return "UnsupportedExtension";
    case StaticGlbImportStatus::UnsafeName:
        return "UnsafeName";
    case StaticGlbImportStatus::UnsafeDestination:
        return "UnsafeDestination";
    case StaticGlbImportStatus::Collision:
        return "Collision";
    case StaticGlbImportStatus::InvalidContainer:
        return "InvalidContainer";
    case StaticGlbImportStatus::Incompatible:
        return "Incompatible";
    case StaticGlbImportStatus::Error:
        return "Error";
    }
    return "Error";
}

bool StaticGlbImportSucceeded(StaticGlbImportStatus status)
{
    return status == StaticGlbImportStatus::Imported;
}

bool TryResolveStaticGlbImportDestination(
    const std::filesystem::path& sourceRoot,
    std::string_view fileName,
    std::string& canonicalIdentity,
    std::filesystem::path& destination,
    std::string& error)
{
    canonicalIdentity.clear();
    destination.clear();
    error.clear();
    if (sourceRoot.empty() || !sourceRoot.is_absolute())
    {
        error = "authoring source root is missing or not absolute";
        return false;
    }
    std::string nameReason;
    if (!IsSafeStaticGlbFileName(fileName, &nameReason))
    {
        error = nameReason;
        return false;
    }
    canonicalIdentity = CanonicalStaticModelIdentity(fileName);
    const std::filesystem::path relativeIdentity =
        std::filesystem::path(canonicalIdentity).lexically_normal();
    if (!RelativeStaysInsideRoot(sourceRoot, relativeIdentity))
    {
        error = "import destination would leave the canonical models root";
        return false;
    }
    const std::filesystem::path modelsRoot =
        (sourceRoot / std::string(kStaticModelsLogicalDirectory)).lexically_normal();
    if (!RelativeStaysInsideRoot(modelsRoot, std::filesystem::path(std::string(fileName))))
    {
        error = "import destination would leave the canonical models root";
        return false;
    }
    destination = (sourceRoot / relativeIdentity).lexically_normal();
    const std::filesystem::path relativeToModels = destination.lexically_relative(modelsRoot);
    if (relativeToModels.empty() || relativeToModels != std::filesystem::path(std::string(fileName)))
    {
        error = "import destination would leave the canonical models root";
        destination.clear();
        return false;
    }
    return true;
}

StaticGlbImportResult ImportStaticGlb(
    const std::filesystem::path& originalExternalPath,
    const std::filesystem::path& sourceRoot,
    StaticModelCatalog* catalog)
{
    if (sourceRoot.empty() || !sourceRoot.is_absolute())
    {
        return MakeResult(
            StaticGlbImportStatus::UnsafeDestination,
            "authoring source root is missing or not absolute");
    }
    if (!PathIsDirectory(sourceRoot))
    {
        return MakeResult(
            StaticGlbImportStatus::UnsafeDestination,
            "authoring source root is not a directory");
    }

    if (originalExternalPath.empty())
    {
        return MakeResult(StaticGlbImportStatus::Missing, "input path is empty");
    }

    const StaticGlbValidation validation = ValidateStaticGlbFile(originalExternalPath);
    if (validation.status != StaticGlbStatus::Ok)
    {
        return MakeResult(
            StatusFromValidation(validation.status),
            validation.message);
    }

    const std::string fileName = originalExternalPath.filename().string();
    std::string identity;
    std::filesystem::path destination;
    std::string destinationError;
    if (!TryResolveStaticGlbImportDestination(
            sourceRoot, fileName, identity, destination, destinationError))
    {
        return MakeResult(
            StaticGlbImportStatus::UnsafeDestination,
            destinationError,
            identity,
            destination);
    }

    const std::filesystem::path modelsRoot =
        (sourceRoot / std::string(kStaticModelsLogicalDirectory)).lexically_normal();

    if (PathExists(destination))
    {
        return MakeResult(
            StaticGlbImportStatus::Collision,
            "destination already exists; import refuses to overwrite canonical content: " + identity,
            identity,
            destination);
    }

    std::error_code createError;
    std::filesystem::create_directories(modelsRoot, createError);
    if (createError || !PathIsDirectory(modelsRoot))
    {
        return MakeResult(
            StaticGlbImportStatus::Error,
            "failed to create the canonical models directory",
            identity,
            destination);
    }

    std::filesystem::path temporary = destination;
    temporary += std::string(kStaticGlbImportTempSuffix);
    if (PathExists(temporary))
    {
        if (!RemovePath(temporary))
        {
            return MakeResult(
                StaticGlbImportStatus::Error,
                "stale import temporary could not be removed",
                identity,
                destination);
        }
    }

    if (!CopyFileToTemporary(originalExternalPath, temporary))
    {
        RemovePath(temporary);
        return MakeResult(
            StaticGlbImportStatus::Error,
            "failed to copy the GLB into a temporary import file",
            identity,
            destination);
    }

    std::vector<std::uint8_t> copiedBytes;
    {
        std::ifstream copiedStream(temporary, std::ios::binary);
        if (!copiedStream)
        {
            RemovePath(temporary);
            return MakeResult(
                StaticGlbImportStatus::Error,
                "copied temporary could not be reopened for validation",
                identity,
                destination);
        }
        copiedStream.seekg(0, std::ios::end);
        const std::streamoff copiedSize = copiedStream.tellg();
        if (copiedSize < 0)
        {
            RemovePath(temporary);
            return MakeResult(
                StaticGlbImportStatus::Error,
                "copied temporary size could not be read",
                identity,
                destination);
        }
        copiedStream.seekg(0, std::ios::beg);
        copiedBytes.resize(static_cast<std::size_t>(copiedSize));
        if (copiedSize > 0
            && !copiedStream.read(
                reinterpret_cast<char*>(copiedBytes.data()),
                static_cast<std::streamsize>(copiedSize)))
        {
            RemovePath(temporary);
            return MakeResult(
                StaticGlbImportStatus::Error,
                "copied temporary could not be read",
                identity,
                destination);
        }
    }
    const StaticGlbValidation copied = ValidateStaticGlbBytes(copiedBytes);
    if (copied.status != StaticGlbStatus::Ok)
    {
        RemovePath(temporary);
        return MakeResult(
            StaticGlbImportStatus::Error,
            "copied bytes failed static GLB validation; destination was not created",
            identity,
            destination);
    }

    if (PathExists(destination))
    {
        RemovePath(temporary);
        return MakeResult(
            StaticGlbImportStatus::Collision,
            "destination already exists; import refuses to overwrite canonical content: " + identity,
            identity,
            destination);
    }

    std::error_code renameError;
    std::filesystem::rename(temporary, destination, renameError);
    if (renameError)
    {
        RemovePath(temporary);
        RemovePath(destination);
        return MakeResult(
            StaticGlbImportStatus::Error,
            "failed to promote the imported GLB onto the canonical destination",
            identity,
            destination);
    }

    if (catalog != nullptr)
    {
        catalog->Refresh(sourceRoot);
        if (catalog->Find(identity) == nullptr)
        {
            RemovePath(destination);
            catalog->Refresh(sourceRoot);
            return MakeResult(
                StaticGlbImportStatus::Error,
                "imported file was not discoverable as a valid static model; destination was removed",
                identity,
                destination);
        }
    }

    std::string message = "Imported static GLB: ";
    message += identity;
    message += "\nCanonical source: ";
    message += destination.generic_string();
    message += "\nThis is project source content only. It is not a level object.";
    message += "\nNext: Build > Cook Assets, then Stage Runtime Assets (or Cook & Stage).";
    return MakeResult(StaticGlbImportStatus::Imported, std::move(message), identity, destination);
}
}
