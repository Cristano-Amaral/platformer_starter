#include "assets/RuntimePngImport.h"

#include <cstdint>
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

std::vector<std::uint8_t> ReadAllBytes(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        return {};
    }
    stream.seekg(0, std::ios::end);
    const std::streamoff size = stream.tellg();
    if (size < 0)
    {
        return {};
    }
    stream.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    if (size > 0
        && !stream.read(
            reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size)))
    {
        return {};
    }
    return bytes;
}

RuntimePngImportResult MakeResult(
    RuntimePngImportStatus status,
    std::string message,
    std::string identity = {},
    std::filesystem::path destination = {})
{
    RuntimePngImportResult result{};
    result.status = status;
    result.message = std::move(message);
    result.canonicalIdentity = std::move(identity);
    result.destinationPath = std::move(destination);
    result.recipe = std::string(kRuntimePngRecipe);
    return result;
}

bool HasLowercasePngExtension(std::string_view fileName)
{
    return HasRuntimePngExtension(fileName);
}
}

const char* RuntimePngImportStatusName(RuntimePngImportStatus status)
{
    switch (status)
    {
    case RuntimePngImportStatus::Imported:
        return "Imported";
    case RuntimePngImportStatus::Cancelled:
        return "Cancelled";
    case RuntimePngImportStatus::Missing:
        return "Missing";
    case RuntimePngImportStatus::NotAFile:
        return "NotAFile";
    case RuntimePngImportStatus::UnsupportedExtension:
        return "UnsupportedExtension";
    case RuntimePngImportStatus::UnsafeName:
        return "UnsafeName";
    case RuntimePngImportStatus::UnsafeDestination:
        return "UnsafeDestination";
    case RuntimePngImportStatus::Collision:
        return "Collision";
    case RuntimePngImportStatus::InvalidPng:
        return "InvalidPng";
    case RuntimePngImportStatus::Error:
        return "Error";
    }
    return "Error";
}

const char* RuntimePngCookWriteStatusName(RuntimePngCookWriteStatus status)
{
    switch (status)
    {
    case RuntimePngCookWriteStatus::NotRequested:
        return "NotRequested";
    case RuntimePngCookWriteStatus::CopiedUnchanged:
        return "CopiedUnchanged";
    case RuntimePngCookWriteStatus::NeedsExternalRecipeCook:
        return "NeedsExternalRecipeCook";
    case RuntimePngCookWriteStatus::Failed:
        return "Failed";
    }
    return "NotRequested";
}

bool RuntimePngImportSucceeded(RuntimePngImportStatus status)
{
    return status == RuntimePngImportStatus::Imported;
}

bool TryResolveRuntimePngImportDestination(
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
    if (!IsSafeRuntimePngFileName(fileName, &nameReason))
    {
        error = nameReason;
        return false;
    }
    canonicalIdentity = CanonicalRuntimePngIdentity(fileName);
    const std::filesystem::path relativeIdentity =
        std::filesystem::path(canonicalIdentity).lexically_normal();
    if (!RelativeStaysInsideRoot(sourceRoot, relativeIdentity))
    {
        error = "import destination would leave the canonical textures root";
        return false;
    }
    const std::filesystem::path texturesRoot =
        (sourceRoot / std::string(kRuntimeTexturesLogicalDirectory)).lexically_normal();
    if (!RelativeStaysInsideRoot(texturesRoot, std::filesystem::path(std::string(fileName))))
    {
        error = "import destination would leave the canonical textures root";
        return false;
    }
    destination = (sourceRoot / relativeIdentity).lexically_normal();
    const std::filesystem::path relativeToTextures = destination.lexically_relative(texturesRoot);
    if (relativeToTextures.empty()
        || relativeToTextures != std::filesystem::path(std::string(fileName)))
    {
        error = "import destination would leave the canonical textures root";
        destination.clear();
        return false;
    }
    return true;
}

bool TryResolveRuntimePngCookDestination(
    const std::filesystem::path& cookedRoot,
    std::string_view canonicalIdentity,
    std::filesystem::path& cookedPath,
    std::string& error)
{
    cookedPath.clear();
    error.clear();
    if (cookedRoot.empty() || !cookedRoot.is_absolute())
    {
        error = "cooked root is missing or not absolute";
        return false;
    }
    std::string fileName;
    if (!TryParseRuntimePngIdentity(canonicalIdentity, fileName, &error))
    {
        return false;
    }
    const std::filesystem::path relativeIdentity =
        std::filesystem::path(std::string(canonicalIdentity)).lexically_normal();
    if (!RelativeStaysInsideRoot(cookedRoot, relativeIdentity))
    {
        error = "cook destination would leave the cooked textures root";
        return false;
    }
    cookedPath = (cookedRoot / relativeIdentity).lexically_normal();
    const std::filesystem::path relativeCheck = cookedPath.lexically_relative(cookedRoot);
    if (relativeCheck.empty() || relativeCheck.generic_string() != std::string(canonicalIdentity))
    {
        error = "cook destination would leave the cooked textures root";
        cookedPath.clear();
        return false;
    }
    return true;
}

RuntimePngCookWriteStatus WriteRecipeCompatibleCookedRuntimePng(
    const std::filesystem::path& sourcePng,
    const std::filesystem::path& cookedPng,
    std::string& message,
    int* width,
    int* height)
{
    message.clear();
    if (width != nullptr)
    {
        *width = 0;
    }
    if (height != nullptr)
    {
        *height = 0;
    }
    if (sourcePng.empty() || cookedPng.empty() || !sourcePng.is_absolute()
        || !cookedPng.is_absolute())
    {
        message = "source or cooked PNG path is missing or not absolute";
        return RuntimePngCookWriteStatus::Failed;
    }
    const std::vector<std::uint8_t> bytes = ReadAllBytes(sourcePng);
    int parsedWidth = 0;
    int parsedHeight = 0;
    std::string pngReason;
    if (!TryReadRuntimePngDimensions(
            bytes.data(), bytes.size(), parsedWidth, parsedHeight, &pngReason))
    {
        message = pngReason.empty() ? "source PNG could not be validated for cook" : pngReason;
        return RuntimePngCookWriteStatus::Failed;
    }
    if (width != nullptr)
    {
        *width = parsedWidth;
    }
    if (height != nullptr)
    {
        *height = parsedHeight;
    }
    if (!RuntimePngDimensionsWithinRecipeLimit(parsedWidth, parsedHeight))
    {
        message =
            "source PNG exceeds 512px; reuse python tools/cook_assets.py --cook-runtime-png "
            "(recipe runtime_png.max512.lanczos.v1)";
        return RuntimePngCookWriteStatus::NeedsExternalRecipeCook;
    }

    std::error_code createError;
    std::filesystem::create_directories(cookedPng.parent_path(), createError);
    if (createError)
    {
        message = "failed to create the cooked textures directory";
        return RuntimePngCookWriteStatus::Failed;
    }

    std::filesystem::path temporary = cookedPng;
    temporary += std::string(kRuntimePngImportTempSuffix);
    if (PathExists(temporary) && !RemovePath(temporary))
    {
        message = "stale cooked temporary could not be removed";
        return RuntimePngCookWriteStatus::Failed;
    }
    if (!CopyFileToTemporary(sourcePng, temporary))
    {
        RemovePath(temporary);
        message = "failed to copy the PNG into a cooked temporary";
        return RuntimePngCookWriteStatus::Failed;
    }
    std::error_code renameError;
    std::filesystem::rename(temporary, cookedPng, renameError);
    if (renameError)
    {
        RemovePath(temporary);
        message = "failed to promote the cooked PNG onto the destination";
        return RuntimePngCookWriteStatus::Failed;
    }
    message = "cooked copy is byte-identical (recipe ";
    message += kRuntimePngRecipe;
    message += ")";
    return RuntimePngCookWriteStatus::CopiedUnchanged;
}

RuntimePngImportResult ImportRuntimePng(
    const std::filesystem::path& originalExternalPath,
    const std::filesystem::path& sourceRoot,
    SourceTextureCatalog* catalog,
    const std::filesystem::path& cookedRoot)
{
    if (sourceRoot.empty() || !sourceRoot.is_absolute())
    {
        return MakeResult(
            RuntimePngImportStatus::UnsafeDestination,
            "authoring source root is missing or not absolute");
    }
    if (!PathIsDirectory(sourceRoot))
    {
        return MakeResult(
            RuntimePngImportStatus::UnsafeDestination,
            "authoring source root is not a directory");
    }
    if (originalExternalPath.empty())
    {
        return MakeResult(RuntimePngImportStatus::Missing, "input path is empty");
    }
    if (!PathExists(originalExternalPath))
    {
        return MakeResult(RuntimePngImportStatus::Missing, "input path does not exist");
    }
    if (!PathIsRegularFile(originalExternalPath))
    {
        return MakeResult(RuntimePngImportStatus::NotAFile, "input path is not a regular file");
    }

    const std::string fileName = originalExternalPath.filename().string();
    if (!HasLowercasePngExtension(fileName))
    {
        return MakeResult(
            RuntimePngImportStatus::UnsupportedExtension,
            "only lowercase .png files can be imported as textures");
    }
    std::string nameReason;
    if (!IsSafeRuntimePngFileName(fileName, &nameReason))
    {
        return MakeResult(RuntimePngImportStatus::UnsafeName, nameReason);
    }

    const std::vector<std::uint8_t> originalBytes = ReadAllBytes(originalExternalPath);
    int width = 0;
    int height = 0;
    std::string pngReason;
    if (!TryReadRuntimePngDimensions(
            originalBytes.data(), originalBytes.size(), width, height, &pngReason))
    {
        return MakeResult(
            RuntimePngImportStatus::InvalidPng,
            pngReason.empty() ? "input is not a valid PNG" : pngReason);
    }

    std::string identity;
    std::filesystem::path destination;
    std::string destinationError;
    if (!TryResolveRuntimePngImportDestination(
            sourceRoot, fileName, identity, destination, destinationError))
    {
        return MakeResult(
            RuntimePngImportStatus::UnsafeDestination,
            destinationError,
            identity,
            destination);
    }

    if (PathExists(destination))
    {
        return MakeResult(
            RuntimePngImportStatus::Collision,
            "destination already exists; import refuses to overwrite canonical content: " + identity,
            identity,
            destination);
    }

    const std::filesystem::path texturesRoot =
        (sourceRoot / std::string(kRuntimeTexturesLogicalDirectory)).lexically_normal();
    std::error_code createError;
    std::filesystem::create_directories(texturesRoot, createError);
    if (createError || !PathIsDirectory(texturesRoot))
    {
        return MakeResult(
            RuntimePngImportStatus::Error,
            "failed to create the canonical textures directory",
            identity,
            destination);
    }

    std::filesystem::path temporary = destination;
    temporary += std::string(kRuntimePngImportTempSuffix);
    if (PathExists(temporary) && !RemovePath(temporary))
    {
        return MakeResult(
            RuntimePngImportStatus::Error,
            "stale import temporary could not be removed",
            identity,
            destination);
    }
    if (!CopyFileToTemporary(originalExternalPath, temporary))
    {
        RemovePath(temporary);
        return MakeResult(
            RuntimePngImportStatus::Error,
            "failed to copy the PNG into a temporary import file",
            identity,
            destination);
    }

    const std::vector<std::uint8_t> copiedBytes = ReadAllBytes(temporary);
    int copiedWidth = 0;
    int copiedHeight = 0;
    std::string copiedReason;
    if (!TryReadRuntimePngDimensions(
            copiedBytes.data(), copiedBytes.size(), copiedWidth, copiedHeight, &copiedReason))
    {
        RemovePath(temporary);
        return MakeResult(
            RuntimePngImportStatus::Error,
            "copied bytes failed PNG validation; destination was not created",
            identity,
            destination);
    }
    if (PathExists(destination))
    {
        RemovePath(temporary);
        return MakeResult(
            RuntimePngImportStatus::Collision,
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
            RuntimePngImportStatus::Error,
            "failed to promote the imported PNG onto the canonical destination",
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
                RuntimePngImportStatus::Error,
                "imported file was not discoverable as a valid texture; destination was removed",
                identity,
                destination);
        }
    }

    RuntimePngImportResult result = MakeResult(
        RuntimePngImportStatus::Imported, {}, identity, destination);
    result.sourceWidth = copiedWidth;
    result.sourceHeight = copiedHeight;
    result.recipe = std::string(kRuntimePngRecipe);
    if (!cookedRoot.empty())
    {
        std::string cookError;
        if (!TryResolveRuntimePngCookDestination(
                cookedRoot, identity, result.cookedPath, cookError))
        {
            RemovePath(destination);
            if (catalog != nullptr)
            {
                catalog->Refresh(sourceRoot);
            }
            result.status = RuntimePngImportStatus::Error;
            result.cookStatus = RuntimePngCookWriteStatus::Failed;
            result.message = "imported source was removed because cooked mapping failed: " + cookError;
            return result;
        }
        std::string cookMessage;
        result.cookStatus = WriteRecipeCompatibleCookedRuntimePng(
            destination, result.cookedPath, cookMessage, &result.sourceWidth, &result.sourceHeight);
        if (result.cookStatus == RuntimePngCookWriteStatus::Failed)
        {
            RemovePath(destination);
            RemovePath(result.cookedPath);
            if (catalog != nullptr)
            {
                catalog->Refresh(sourceRoot);
            }
            result.status = RuntimePngImportStatus::Error;
            result.message =
                "imported source was removed because cooked write failed: " + cookMessage;
            return result;
        }
    }

    result.message = "Imported texture: ";
    result.message += identity;
    result.message += "\nCanonical source: ";
    result.message += destination.generic_string();
    result.message += "\nRuntime/authored identity is unchanged by Content Browser folders.";
    result.message += "\nRecipe: ";
    result.message += kRuntimePngRecipe;
    if (result.cookStatus == RuntimePngCookWriteStatus::CopiedUnchanged)
    {
        result.message += "\nCooked copy written (within 512, byte-identical).";
    }
    else if (result.cookStatus == RuntimePngCookWriteStatus::NeedsExternalRecipeCook)
    {
        result.message +=
            "\nSource copied. Cook the PNG with python tools/cook_assets.py --cook-runtime-png "
            "because it exceeds 512px.";
    }
    else
    {
        result.message += "\nThis is project source content only. It is not a level object.";
        result.message += "\nNext: cook with the existing runtime PNG recipe, then Stage if referenced.";
    }
    return result;
}
}
