#pragma once

#include <filesystem>

namespace platform
{
// Promote a completed sibling temporary file onto finalPath.
// Persistence may call this after a successful flush/close of the temp file.
// This is not a generic filesystem API.
//
// Windows (M29 validated):
//   If finalPath already exists as a regular file, ReplaceFileW promotes
//   the temp onto that name. The previous final is not deleted as a
//   separate step before the replacement call. If ReplaceFileW fails, the
//   original final remains according to ReplaceFileW semantics.
//   If finalPath does not exist (first-ever save), MoveFileExW with
//   MOVEFILE_WRITE_THROUGH performs the promote. No destination is deleted.
// POSIX: std::filesystem::rename (compile-compatible; not M29-validated).
//
// Returns false for empty or non-absolute paths, or when the OS operation
// fails. Callers must treat false as Save Error without rolling back
// in-memory BEST.
bool ReplaceFileWithTemporary(
    const std::filesystem::path& temporaryPath,
    const std::filesystem::path& finalPath);
}
