#pragma once

// Development authored-source Level discovery and narrow New/Open helpers.
// Not a Content Browser, asset database, campaign graph, or SceneManager.
// Gameplay/Release never call this; staged runtime remains the load authority.

#include "world/LevelDefinition.h"
#include "world/LevelFile.h"
#include "world/LevelGoal.h"
#include "world/LevelIdentity.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace editor
{
struct AuthoredLevelEntry
{
    std::string id;
};

class AuthoredLevelCatalog
{
public:
    void Refresh(const std::filesystem::path& sourceRoot);
    void Clear();

    const std::vector<AuthoredLevelEntry>& Entries() const;
    const AuthoredLevelEntry* Find(std::string_view levelId) const;
    bool Contains(std::string_view levelId) const;
    std::size_t Count() const;

private:
    std::vector<AuthoredLevelEntry> entries;
};

inline bool AuthoredLevelSwitchNeedsDiscard(bool modified, bool dirty)
{
    return modified || dirty;
}

// Absolute source path for a validated identity under an already-resolved
// authored source root. Empty when the identity or root is unsafe.
std::filesystem::path MakeAuthoredLevelSourcePath(
    const std::filesystem::path& sourceRoot,
    std::string_view levelId);

// Narrowest valid/playable Level Format v1 using required singleton defaults.
// Not a template/wizard/procedural generator.
world::LevelDefinition MakeMinimalPlayableLevel(std::string_view levelId);

enum class AuthoredLevelCreateStatus
{
    Created,
    InvalidId,
    Duplicate,
    Exists,
    Unsafe,
    Invalid,
    Error,
};

struct AuthoredLevelCreateResult
{
    AuthoredLevelCreateStatus status = AuthoredLevelCreateStatus::Error;
    std::string message;
    std::string levelId;
    std::filesystem::path path;
};

const char* AuthoredLevelCreateStatusName(AuthoredLevelCreateStatus status);

// Never overwrites. Rejects unsafe/duplicate IDs before touching the file.
AuthoredLevelCreateResult TryCreateAuthoredLevel(
    const std::filesystem::path& sourceRoot,
    std::string_view levelId,
    const AuthoredLevelCatalog& catalog);

enum class AuthoredLevelOpenStatus
{
    Ready,
    Missing,
    Invalid,
    Unsafe,
    Error,
};

struct AuthoredLevelOpenPrepareResult
{
    AuthoredLevelOpenStatus status = AuthoredLevelOpenStatus::Error;
    std::string message;
    world::LevelDefinition candidate{};
    world::LoadLevelFileStatus loadStatus = world::LoadLevelFileStatus::Error;
};

const char* AuthoredLevelOpenStatusName(AuthoredLevelOpenStatus status);

// Load and validate an authored source Level. Never uses CWD, cooked, or staged
// trees. Failure leaves candidate empty.
AuthoredLevelOpenPrepareResult PrepareAuthoredLevelOpen(
    const std::filesystem::path& sourceAbsolutePath,
    std::string_view expectedLevelId);

struct NextLevelSelectorEntry
{
    std::string id;
    bool missing = false;
};

// None (empty id) first, then discovered identities, then a missing authored
// value preserved at the end when it is absent from discovery.
std::vector<NextLevelSelectorEntry> MakeNextLevelSelectorEntries(
    const AuthoredLevelCatalog& catalog,
    std::string_view currentNextLevelId);

// Writes a logical identity or empty terminal into nextLevelId. Invalid tokens
// are rejected without mutating the spec.
bool ApplyNextLevelSelectorId(world::LevelGoalSpec& goal, std::string_view chosenId);
}
