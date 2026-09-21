#pragma once

// Content Browser logical folders and Favorites. Project organization
// metadata only: never moves physical files, never changes runtime/authored
// identity, never dirties a Level, and is not a runtime asset registry.

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace editor
{
inline constexpr std::string_view kContentBrowserOrganizationFileName =
    "content_browser_organization.v1.txt";
inline constexpr int kContentBrowserOrganizationSchemaVersion = 1;
inline constexpr std::size_t kContentBrowserFolderNameMaxLength = 64;
inline constexpr std::size_t kContentBrowserFolderMaxDepth = 8;
inline constexpr std::size_t kContentBrowserFolderMaxCount = 256;

// Folder names are case-sensitive. Sibling "Grass" and "grass" are distinct.
// Search in a logical folder includes that folder and its descendants.
inline constexpr bool kContentBrowserFolderNamesAreCaseSensitive = true;
inline constexpr bool kContentBrowserFolderSearchIncludesDescendants = true;

enum class ContentBrowserOrganizationStatus
{
    Ok,
    InvalidName,
    Collision,
    NotFound,
    Cycle,
    Limit,
    Error,
};

struct ContentBrowserAssetAssignment
{
    std::string canonicalIdentity;
    std::string folderPath;
};

struct ContentBrowserOrganization
{
    int schemaVersion = kContentBrowserOrganizationSchemaVersion;
    std::vector<std::string> folders;
    std::vector<ContentBrowserAssetAssignment> assignments;
    std::vector<std::string> favorites;
};

const char* ContentBrowserOrganizationStatusName(ContentBrowserOrganizationStatus status);
bool ContentBrowserOrganizationSucceeded(ContentBrowserOrganizationStatus status);

bool IsReservedContentBrowserFolderName(std::string_view name);
bool IsSafeContentBrowserFolderName(std::string_view name, std::string* reason = nullptr);
bool IsSafeContentBrowserFolderPath(std::string_view path, std::string* reason = nullptr);

std::string ContentBrowserFolderParentPath(std::string_view folderPath);
std::string JoinContentBrowserFolderPath(std::string_view parentPath, std::string_view name);
bool ContentBrowserFolderIsAncestor(
    std::string_view ancestorPath,
    std::string_view candidatePath);
bool ContentBrowserFolderContainsPath(
    std::string_view folderPath,
    std::string_view candidatePath,
    bool includeDescendants);

std::filesystem::path ContentBrowserOrganizationPath(const std::filesystem::path& sourceRoot);

ContentBrowserOrganizationStatus LoadContentBrowserOrganization(
    const std::filesystem::path& path,
    ContentBrowserOrganization& organization,
    std::string& message);
ContentBrowserOrganizationStatus SaveContentBrowserOrganization(
    const std::filesystem::path& path,
    const ContentBrowserOrganization& organization,
    std::string& message);

void SortContentBrowserOrganization(ContentBrowserOrganization& organization);
void ReconcileContentBrowserOrganization(
    ContentBrowserOrganization& organization,
    const std::vector<std::string>& knownIdentities);

bool ContentBrowserFolderExists(
    const ContentBrowserOrganization& organization,
    std::string_view folderPath);
std::string ContentBrowserAssignedFolder(
    const ContentBrowserOrganization& organization,
    std::string_view canonicalIdentity);
bool ContentBrowserIsFavorite(
    const ContentBrowserOrganization& organization,
    std::string_view canonicalIdentity);

ContentBrowserOrganizationStatus CreateContentBrowserFolder(
    ContentBrowserOrganization& organization,
    std::string_view parentPath,
    std::string_view name,
    std::string& createdPath,
    std::string& message);
ContentBrowserOrganizationStatus RenameContentBrowserFolder(
    ContentBrowserOrganization& organization,
    std::string_view folderPath,
    std::string_view newName,
    std::string& renamedPath,
    std::string& message);
ContentBrowserOrganizationStatus DeleteContentBrowserFolder(
    ContentBrowserOrganization& organization,
    std::string_view folderPath,
    std::string& message);
ContentBrowserOrganizationStatus MoveContentBrowserAsset(
    ContentBrowserOrganization& organization,
    std::string_view canonicalIdentity,
    std::string_view folderPath,
    std::string& message);
ContentBrowserOrganizationStatus SetContentBrowserFavorite(
    ContentBrowserOrganization& organization,
    std::string_view canonicalIdentity,
    bool favorite,
    std::string& message);
}
