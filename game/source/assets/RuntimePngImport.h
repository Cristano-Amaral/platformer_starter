#pragma once

// Copies a valid external PNG into canonical source/textures/. Reuses the
// existing runtime_png.max512.lanczos.v1 cook mapping for cooked output.
// Does not stage, rewrite Levels, or mutate editor authored state.

#include "assets/RuntimePng.h"
#include "assets/SourceTextureCatalog.h"

#include <filesystem>
#include <string>

namespace assets
{
enum class RuntimePngImportStatus
{
    Imported,
    Cancelled,
    Missing,
    NotAFile,
    UnsupportedExtension,
    UnsafeName,
    UnsafeDestination,
    Collision,
    InvalidPng,
    Error,
};

enum class RuntimePngCookWriteStatus
{
    NotRequested,
    CopiedUnchanged,
    NeedsExternalRecipeCook,
    Failed,
};

const char* RuntimePngImportStatusName(RuntimePngImportStatus status);
const char* RuntimePngCookWriteStatusName(RuntimePngCookWriteStatus status);

struct RuntimePngImportResult
{
    RuntimePngImportStatus status = RuntimePngImportStatus::Error;
    RuntimePngCookWriteStatus cookStatus = RuntimePngCookWriteStatus::NotRequested;
    std::string canonicalIdentity;
    std::filesystem::path destinationPath;
    std::filesystem::path cookedPath;
    std::string recipe;
    int sourceWidth = 0;
    int sourceHeight = 0;
    std::string message;
};

bool RuntimePngImportSucceeded(RuntimePngImportStatus status);

bool TryResolveRuntimePngImportDestination(
    const std::filesystem::path& sourceRoot,
    std::string_view fileName,
    std::string& canonicalIdentity,
    std::filesystem::path& destination,
    std::string& error);

bool TryResolveRuntimePngCookDestination(
    const std::filesystem::path& cookedRoot,
    std::string_view canonicalIdentity,
    std::filesystem::path& cookedPath,
    std::string& error);

// Writes cooked/textures/<file>.png using the existing recipe rule: files
// already within 512 are a byte-identical copy. Larger files require the
// Python cooker (Pillow LANCZOS) and are not written here.
RuntimePngCookWriteStatus WriteRecipeCompatibleCookedRuntimePng(
    const std::filesystem::path& sourcePng,
    const std::filesystem::path& cookedPng,
    std::string& message,
    int* width = nullptr,
    int* height = nullptr);

// sourceRoot is the canonical authored root (game/assets/source). Destination
// is always sourceRoot/textures/<filename>.png. originalExternalPath is input
// only and is never stored as identity. Optional cookedRoot writes a
// recipe-compatible cooked copy when dimensions are within 512.
RuntimePngImportResult ImportRuntimePng(
    const std::filesystem::path& originalExternalPath,
    const std::filesystem::path& sourceRoot,
    SourceTextureCatalog* catalog = nullptr,
    const std::filesystem::path& cookedRoot = {});
}
