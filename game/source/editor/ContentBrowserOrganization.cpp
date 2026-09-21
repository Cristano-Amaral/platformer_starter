#include "editor/ContentBrowserOrganization.h"

#include "assets/RuntimePng.h"
#include "assets/StaticGlb.h"
#include "platform/FileReplace.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <fstream>
#include <sstream>
#include <system_error>
#include <unordered_set>

namespace editor
{
namespace
{
constexpr const char* kUnfiledLabel = "(unfiled)";

std::string TrimCopy(std::string_view text)
{
    std::size_t begin = 0;
    while (begin < text.size()
        && (text[begin] == ' ' || text[begin] == '\t' || text[begin] == '\r'
            || text[begin] == '\n'))
    {
        ++begin;
    }
    std::size_t end = text.size();
    while (end > begin
        && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\r'
            || text[end - 1] == '\n'))
    {
        --end;
    }
    return std::string(text.substr(begin, end - begin));
}

bool IsSupportedAssetIdentity(std::string_view identity)
{
    std::string fileName;
    return assets::TryParseStaticModelIdentity(identity, fileName, nullptr)
        || assets::TryParseRuntimePngIdentity(identity, fileName, nullptr);
}

std::vector<std::string> SplitFolderPath(std::string_view path)
{
    std::vector<std::string> parts;
    if (path.empty())
    {
        return parts;
    }
    std::string current;
    for (char ch : path)
    {
        if (ch == '/')
        {
            parts.push_back(current);
            current.clear();
        }
        else
        {
            current.push_back(ch);
        }
    }
    parts.push_back(current);
    return parts;
}

std::string JoinFolderParts(const std::vector<std::string>& parts)
{
    std::string path;
    for (const std::string& part : parts)
    {
        if (!path.empty())
        {
            path.push_back('/');
        }
        path += part;
    }
    return path;
}

bool FolderNameEquals(std::string_view left, std::string_view right)
{
    return left == right;
}

std::size_t FolderIndex(
    const ContentBrowserOrganization& organization,
    std::string_view folderPath)
{
    for (std::size_t index = 0; index < organization.folders.size(); ++index)
    {
        if (FolderNameEquals(organization.folders[index], folderPath))
        {
            return index;
        }
    }
    return organization.folders.size();
}

std::size_t AssignmentIndex(
    const ContentBrowserOrganization& organization,
    std::string_view identity)
{
    for (std::size_t index = 0; index < organization.assignments.size(); ++index)
    {
        if (organization.assignments[index].canonicalIdentity == identity)
        {
            return index;
        }
    }
    return organization.assignments.size();
}

std::size_t FavoriteIndex(
    const ContentBrowserOrganization& organization,
    std::string_view identity)
{
    for (std::size_t index = 0; index < organization.favorites.size(); ++index)
    {
        if (organization.favorites[index] == identity)
        {
            return index;
        }
    }
    return organization.favorites.size();
}

bool SiblingCollision(
    const ContentBrowserOrganization& organization,
    std::string_view parentPath,
    std::string_view name,
    std::string_view ignorePath = {})
{
    const std::string candidate = JoinContentBrowserFolderPath(parentPath, name);
    for (const std::string& folder : organization.folders)
    {
        if (!ignorePath.empty() && FolderNameEquals(folder, ignorePath))
        {
            continue;
        }
        if (FolderNameEquals(folder, candidate))
        {
            return true;
        }
    }
    return false;
}

bool ReplaceFolderPrefix(
    std::string& path,
    std::string_view oldPrefix,
    std::string_view newPrefix)
{
    if (oldPrefix.empty() || path.size() < oldPrefix.size())
    {
        return false;
    }
    if (path.compare(0, oldPrefix.size(), oldPrefix.data(), oldPrefix.size()) != 0)
    {
        return false;
    }
    if (path.size() != oldPrefix.size() && path[oldPrefix.size()] != '/')
    {
        return false;
    }
    if (newPrefix.empty())
    {
        if (path.size() == oldPrefix.size())
        {
            path.clear();
            return true;
        }
        path.erase(0, oldPrefix.size() + 1);
        return true;
    }
    path.replace(0, oldPrefix.size(), std::string(newPrefix));
    return true;
}
}

const char* ContentBrowserOrganizationStatusName(ContentBrowserOrganizationStatus status)
{
    switch (status)
    {
    case ContentBrowserOrganizationStatus::Ok:
        return "Ok";
    case ContentBrowserOrganizationStatus::InvalidName:
        return "InvalidName";
    case ContentBrowserOrganizationStatus::Collision:
        return "Collision";
    case ContentBrowserOrganizationStatus::NotFound:
        return "NotFound";
    case ContentBrowserOrganizationStatus::Cycle:
        return "Cycle";
    case ContentBrowserOrganizationStatus::Limit:
        return "Limit";
    case ContentBrowserOrganizationStatus::Error:
        return "Error";
    }
    return "Error";
}

bool ContentBrowserOrganizationSucceeded(ContentBrowserOrganizationStatus status)
{
    return status == ContentBrowserOrganizationStatus::Ok;
}

bool IsReservedContentBrowserFolderName(std::string_view name)
{
    return name == "AllAssets" || name == "Favorites" || name == "Models" || name == "Textures"
        || name == "Folders" || name == "All" || name == "Assets";
}

bool IsSafeContentBrowserFolderName(std::string_view name, std::string* reason)
{
    const auto fail = [&](const char* text) {
        if (reason != nullptr)
        {
            *reason = text;
        }
        return false;
    };
    if (name.empty())
    {
        return fail("folder name is empty");
    }
    if (name.size() > kContentBrowserFolderNameMaxLength)
    {
        return fail("folder name is too long");
    }
    if (name == "." || name == "..")
    {
        return fail("folder name is reserved");
    }
    if (IsReservedContentBrowserFolderName(name))
    {
        return fail("folder name collides with a Content Browser collection");
    }
    for (char ch : name)
    {
        const unsigned char byte = static_cast<unsigned char>(ch);
        if (byte < 32 || ch == '/' || ch == '\\' || ch == ':' || ch == '*' || ch == '?'
            || ch == '"' || ch == '<' || ch == '>' || ch == '|' || ch == ' ')
        {
            return fail("folder name contains an unsafe character");
        }
    }
    return true;
}

bool IsSafeContentBrowserFolderPath(std::string_view path, std::string* reason)
{
    if (path.empty())
    {
        if (reason != nullptr)
        {
            *reason = "folder path is empty";
        }
        return false;
    }
    if (path.find('\\') != std::string_view::npos)
    {
        if (reason != nullptr)
        {
            *reason = "folder path must use posix separators";
        }
        return false;
    }
    const std::vector<std::string> parts = SplitFolderPath(path);
    if (parts.size() > kContentBrowserFolderMaxDepth)
    {
        if (reason != nullptr)
        {
            *reason = "folder hierarchy is too deep";
        }
        return false;
    }
    for (const std::string& part : parts)
    {
        if (!IsSafeContentBrowserFolderName(part, reason))
        {
            return false;
        }
    }
    if (JoinFolderParts(parts) != path)
    {
        if (reason != nullptr)
        {
            *reason = "folder path is not normalized";
        }
        return false;
    }
    return true;
}

std::string ContentBrowserFolderParentPath(std::string_view folderPath)
{
    const std::size_t slash = folderPath.rfind('/');
    if (slash == std::string_view::npos)
    {
        return {};
    }
    return std::string(folderPath.substr(0, slash));
}

std::string JoinContentBrowserFolderPath(std::string_view parentPath, std::string_view name)
{
    if (parentPath.empty())
    {
        return std::string(name);
    }
    std::string path;
    path.reserve(parentPath.size() + 1 + name.size());
    path.append(parentPath);
    path.push_back('/');
    path.append(name);
    return path;
}

bool ContentBrowserFolderIsAncestor(
    std::string_view ancestorPath,
    std::string_view candidatePath)
{
    if (ancestorPath.empty() || candidatePath.size() <= ancestorPath.size())
    {
        return false;
    }
    if (candidatePath.compare(0, ancestorPath.size(), ancestorPath.data(), ancestorPath.size())
        != 0)
    {
        return false;
    }
    return candidatePath[ancestorPath.size()] == '/';
}

bool ContentBrowserFolderContainsPath(
    std::string_view folderPath,
    std::string_view candidatePath,
    bool includeDescendants)
{
    if (folderPath.empty())
    {
        return candidatePath.empty();
    }
    if (FolderNameEquals(folderPath, candidatePath))
    {
        return true;
    }
    return includeDescendants && ContentBrowserFolderIsAncestor(folderPath, candidatePath);
}

std::filesystem::path ContentBrowserOrganizationPath(const std::filesystem::path& sourceRoot)
{
    if (sourceRoot.empty() || !sourceRoot.is_absolute())
    {
        return {};
    }
    return (sourceRoot / std::string(kContentBrowserOrganizationFileName)).lexically_normal();
}

void SortContentBrowserOrganization(ContentBrowserOrganization& organization)
{
    std::sort(organization.folders.begin(), organization.folders.end());
    organization.folders.erase(
        std::unique(organization.folders.begin(), organization.folders.end()),
        organization.folders.end());
    std::sort(
        organization.assignments.begin(),
        organization.assignments.end(),
        [](const ContentBrowserAssetAssignment& left,
            const ContentBrowserAssetAssignment& right) {
            return left.canonicalIdentity < right.canonicalIdentity;
        });
    std::sort(organization.favorites.begin(), organization.favorites.end());
    organization.favorites.erase(
        std::unique(organization.favorites.begin(), organization.favorites.end()),
        organization.favorites.end());
}

void ReconcileContentBrowserOrganization(
    ContentBrowserOrganization& organization,
    const std::vector<std::string>& knownIdentities)
{
    std::unordered_set<std::string> known(knownIdentities.begin(), knownIdentities.end());
    organization.assignments.erase(
        std::remove_if(
            organization.assignments.begin(),
            organization.assignments.end(),
            [&](const ContentBrowserAssetAssignment& assignment) {
                return known.find(assignment.canonicalIdentity) == known.end()
                    || !IsSupportedAssetIdentity(assignment.canonicalIdentity)
                    || (!assignment.folderPath.empty()
                        && FolderIndex(organization, assignment.folderPath)
                            >= organization.folders.size());
            }),
        organization.assignments.end());
    organization.favorites.erase(
        std::remove_if(
            organization.favorites.begin(),
            organization.favorites.end(),
            [&](const std::string& identity) {
                return known.find(identity) == known.end() || !IsSupportedAssetIdentity(identity);
            }),
        organization.favorites.end());
    SortContentBrowserOrganization(organization);
}

bool ContentBrowserFolderExists(
    const ContentBrowserOrganization& organization,
    std::string_view folderPath)
{
    return FolderIndex(organization, folderPath) < organization.folders.size();
}

std::string ContentBrowserAssignedFolder(
    const ContentBrowserOrganization& organization,
    std::string_view canonicalIdentity)
{
    const std::size_t index = AssignmentIndex(organization, canonicalIdentity);
    if (index >= organization.assignments.size())
    {
        return {};
    }
    return organization.assignments[index].folderPath;
}

bool ContentBrowserIsFavorite(
    const ContentBrowserOrganization& organization,
    std::string_view canonicalIdentity)
{
    return FavoriteIndex(organization, canonicalIdentity) < organization.favorites.size();
}

ContentBrowserOrganizationStatus LoadContentBrowserOrganization(
    const std::filesystem::path& path,
    ContentBrowserOrganization& organization,
    std::string& message)
{
    organization = {};
    message.clear();
    if (path.empty())
    {
        message = "organization path is empty";
        return ContentBrowserOrganizationStatus::Error;
    }
    std::error_code error;
    if (!std::filesystem::exists(path, error) || error)
    {
        organization.schemaVersion = kContentBrowserOrganizationSchemaVersion;
        message = "organization file is missing; using empty project organization";
        return ContentBrowserOrganizationStatus::Ok;
    }
    std::ifstream in(path);
    if (!in)
    {
        message = "organization file could not be opened";
        return ContentBrowserOrganizationStatus::Error;
    }
    bool sawSchema = false;
    std::string line;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        const std::string trimmed = TrimCopy(line);
        if (trimmed.empty())
        {
            continue;
        }
        std::istringstream tokens(trimmed);
        std::string keyword;
        tokens >> keyword;
        if (keyword == "schema")
        {
            int version = 0;
            tokens >> version;
            if (version != kContentBrowserOrganizationSchemaVersion)
            {
                message = "unsupported organization schema";
                organization = {};
                return ContentBrowserOrganizationStatus::Error;
            }
            organization.schemaVersion = version;
            sawSchema = true;
            continue;
        }
        if (keyword == "folder")
        {
            std::string folderPath;
            tokens >> folderPath;
            std::string reason;
            if (!IsSafeContentBrowserFolderPath(folderPath, &reason))
            {
                continue;
            }
            organization.folders.push_back(folderPath);
            continue;
        }
        if (keyword == "assign")
        {
            std::string identity;
            std::string folderPath;
            tokens >> identity >> folderPath;
            if (!IsSupportedAssetIdentity(identity)
                || !IsSafeContentBrowserFolderPath(folderPath, nullptr))
            {
                continue;
            }
            ContentBrowserAssetAssignment assignment{};
            assignment.canonicalIdentity = identity;
            assignment.folderPath = folderPath;
            organization.assignments.push_back(std::move(assignment));
            continue;
        }
        if (keyword == "favorite")
        {
            std::string identity;
            tokens >> identity;
            if (!IsSupportedAssetIdentity(identity))
            {
                continue;
            }
            organization.favorites.push_back(identity);
        }
    }
    if (!sawSchema)
    {
        organization.schemaVersion = kContentBrowserOrganizationSchemaVersion;
    }
    SortContentBrowserOrganization(organization);
    message = "loaded project Content Browser organization";
    return ContentBrowserOrganizationStatus::Ok;
}

ContentBrowserOrganizationStatus SaveContentBrowserOrganization(
    const std::filesystem::path& path,
    const ContentBrowserOrganization& organization,
    std::string& message)
{
    message.clear();
    if (path.empty() || !path.is_absolute())
    {
        message = "organization path is missing or not absolute";
        return ContentBrowserOrganizationStatus::Error;
    }
    ContentBrowserOrganization sorted = organization;
    SortContentBrowserOrganization(sorted);
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error)
    {
        message = "failed to create the organization directory";
        return ContentBrowserOrganizationStatus::Error;
    }
    std::ostringstream text;
    text << "schema " << kContentBrowserOrganizationSchemaVersion << '\n';
    for (const std::string& folder : sorted.folders)
    {
        text << "folder " << folder << '\n';
    }
    for (const ContentBrowserAssetAssignment& assignment : sorted.assignments)
    {
        if (assignment.folderPath.empty())
        {
            continue;
        }
        text << "assign " << assignment.canonicalIdentity << ' ' << assignment.folderPath << '\n';
    }
    for (const std::string& identity : sorted.favorites)
    {
        text << "favorite " << identity << '\n';
    }
    const std::string payload = text.str();
    std::filesystem::path temporary = path;
    temporary += ".tmp";
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            message = "failed to write organization temporary";
            return ContentBrowserOrganizationStatus::Error;
        }
        out << payload;
        if (!out)
        {
            message = "failed to write organization temporary";
            return ContentBrowserOrganizationStatus::Error;
        }
    }
    if (!platform::ReplaceFileWithTemporary(temporary, path))
    {
        std::filesystem::remove(temporary, error);
        message = "failed to promote organization metadata";
        return ContentBrowserOrganizationStatus::Error;
    }
    message = "saved project Content Browser organization";
    return ContentBrowserOrganizationStatus::Ok;
}

ContentBrowserOrganizationStatus CreateContentBrowserFolder(
    ContentBrowserOrganization& organization,
    std::string_view parentPath,
    std::string_view name,
    std::string& createdPath,
    std::string& message)
{
    createdPath.clear();
    message.clear();
    std::string reason;
    if (!parentPath.empty() && !IsSafeContentBrowserFolderPath(parentPath, &reason))
    {
        message = reason;
        return ContentBrowserOrganizationStatus::InvalidName;
    }
    if (!parentPath.empty() && !ContentBrowserFolderExists(organization, parentPath))
    {
        message = "parent folder does not exist";
        return ContentBrowserOrganizationStatus::NotFound;
    }
    if (!IsSafeContentBrowserFolderName(name, &reason))
    {
        message = reason;
        return ContentBrowserOrganizationStatus::InvalidName;
    }
    const std::string candidate = JoinContentBrowserFolderPath(parentPath, name);
    if (!IsSafeContentBrowserFolderPath(candidate, &reason))
    {
        message = reason;
        return ContentBrowserOrganizationStatus::InvalidName;
    }
    if (organization.folders.size() >= kContentBrowserFolderMaxCount)
    {
        message = "folder limit reached";
        return ContentBrowserOrganizationStatus::Limit;
    }
    if (SiblingCollision(organization, parentPath, name))
    {
        message = "a sibling folder already uses that name";
        return ContentBrowserOrganizationStatus::Collision;
    }
    organization.folders.push_back(candidate);
    SortContentBrowserOrganization(organization);
    createdPath = candidate;
    message = "created logical folder ";
    message += candidate;
    return ContentBrowserOrganizationStatus::Ok;
}

ContentBrowserOrganizationStatus RenameContentBrowserFolder(
    ContentBrowserOrganization& organization,
    std::string_view folderPath,
    std::string_view newName,
    std::string& renamedPath,
    std::string& message)
{
    renamedPath.clear();
    message.clear();
    std::string reason;
    if (!IsSafeContentBrowserFolderPath(folderPath, &reason))
    {
        message = reason;
        return ContentBrowserOrganizationStatus::InvalidName;
    }
    if (!ContentBrowserFolderExists(organization, folderPath))
    {
        message = "folder does not exist";
        return ContentBrowserOrganizationStatus::NotFound;
    }
    if (!IsSafeContentBrowserFolderName(newName, &reason))
    {
        message = reason;
        return ContentBrowserOrganizationStatus::InvalidName;
    }
    const std::string parent = ContentBrowserFolderParentPath(folderPath);
    const std::string candidate = JoinContentBrowserFolderPath(parent, newName);
    if (!IsSafeContentBrowserFolderPath(candidate, &reason))
    {
        message = reason;
        return ContentBrowserOrganizationStatus::InvalidName;
    }
    if (SiblingCollision(organization, parent, newName, folderPath))
    {
        message = "a sibling folder already uses that name";
        return ContentBrowserOrganizationStatus::Collision;
    }
    if (ContentBrowserFolderContainsPath(folderPath, candidate, true)
        && !FolderNameEquals(folderPath, candidate))
    {
        message = "rename would create a folder cycle";
        return ContentBrowserOrganizationStatus::Cycle;
    }
    for (std::string& folder : organization.folders)
    {
        ReplaceFolderPrefix(folder, folderPath, candidate);
    }
    for (ContentBrowserAssetAssignment& assignment : organization.assignments)
    {
        ReplaceFolderPrefix(assignment.folderPath, folderPath, candidate);
    }
    SortContentBrowserOrganization(organization);
    renamedPath = candidate;
    message = "renamed logical folder to ";
    message += candidate;
    return ContentBrowserOrganizationStatus::Ok;
}

ContentBrowserOrganizationStatus DeleteContentBrowserFolder(
    ContentBrowserOrganization& organization,
    std::string_view folderPath,
    std::string& message)
{
    message.clear();
    std::string reason;
    if (!IsSafeContentBrowserFolderPath(folderPath, &reason))
    {
        message = reason;
        return ContentBrowserOrganizationStatus::InvalidName;
    }
    if (!ContentBrowserFolderExists(organization, folderPath))
    {
        message = "folder does not exist";
        return ContentBrowserOrganizationStatus::NotFound;
    }

    const std::string parent = ContentBrowserFolderParentPath(folderPath);
    std::vector<std::string> childFolders;
    for (const std::string& folder : organization.folders)
    {
        if (ContentBrowserFolderParentPath(folder) == folderPath)
        {
            childFolders.push_back(folder);
        }
    }
    for (const std::string& child : childFolders)
    {
        const std::string childName = std::string(
            std::string_view(child).substr(folderPath.size() + 1));
        if (SiblingCollision(organization, parent, childName, child))
        {
            message =
                "cannot delete folder: reparenting a subfolder would collide with an existing sibling";
            return ContentBrowserOrganizationStatus::Collision;
        }
    }

    const std::string oldPrefix = std::string(folderPath);
    for (std::string& folder : organization.folders)
    {
        if (FolderNameEquals(folder, oldPrefix))
        {
            continue;
        }
        ReplaceFolderPrefix(folder, oldPrefix, parent);
    }
    organization.folders.erase(
        std::remove_if(
            organization.folders.begin(),
            organization.folders.end(),
            [&](const std::string& folder) { return FolderNameEquals(folder, oldPrefix); }),
        organization.folders.end());
    for (ContentBrowserAssetAssignment& assignment : organization.assignments)
    {
        if (FolderNameEquals(assignment.folderPath, oldPrefix))
        {
            assignment.folderPath = parent;
        }
        else
        {
            ReplaceFolderPrefix(assignment.folderPath, oldPrefix, parent);
        }
    }
    organization.assignments.erase(
        std::remove_if(
            organization.assignments.begin(),
            organization.assignments.end(),
            [](const ContentBrowserAssetAssignment& assignment) {
                return assignment.folderPath.empty();
            }),
        organization.assignments.end());
    SortContentBrowserOrganization(organization);
    message = "deleted logical folder ";
    message += oldPrefix;
    message += "; physical assets were not deleted";
    if (!parent.empty())
    {
        message += " (contents reparented to ";
        message += parent;
        message += ")";
    }
    else
    {
        message += " (contents reparented to ";
        message += kUnfiledLabel;
        message += ")";
    }
    return ContentBrowserOrganizationStatus::Ok;
}

ContentBrowserOrganizationStatus MoveContentBrowserAsset(
    ContentBrowserOrganization& organization,
    std::string_view canonicalIdentity,
    std::string_view folderPath,
    std::string& message)
{
    message.clear();
    if (!IsSupportedAssetIdentity(canonicalIdentity))
    {
        message = "asset identity is not a supported Model or Texture";
        return ContentBrowserOrganizationStatus::InvalidName;
    }
    if (!folderPath.empty())
    {
        std::string reason;
        if (!IsSafeContentBrowserFolderPath(folderPath, &reason))
        {
            message = reason;
            return ContentBrowserOrganizationStatus::InvalidName;
        }
        if (!ContentBrowserFolderExists(organization, folderPath))
        {
            message = "destination folder does not exist";
            return ContentBrowserOrganizationStatus::NotFound;
        }
    }
    const std::size_t index = AssignmentIndex(organization, canonicalIdentity);
    if (folderPath.empty())
    {
        if (index < organization.assignments.size())
        {
            organization.assignments.erase(organization.assignments.begin() + static_cast<std::ptrdiff_t>(index));
        }
        message = "moved asset to ";
        message += kUnfiledLabel;
        SortContentBrowserOrganization(organization);
        return ContentBrowserOrganizationStatus::Ok;
    }
    if (index < organization.assignments.size())
    {
        organization.assignments[index].folderPath = std::string(folderPath);
    }
    else
    {
        ContentBrowserAssetAssignment assignment{};
        assignment.canonicalIdentity = std::string(canonicalIdentity);
        assignment.folderPath = std::string(folderPath);
        organization.assignments.push_back(std::move(assignment));
    }
    SortContentBrowserOrganization(organization);
    message = "moved asset to ";
    message += folderPath;
    return ContentBrowserOrganizationStatus::Ok;
}

ContentBrowserOrganizationStatus SetContentBrowserFavorite(
    ContentBrowserOrganization& organization,
    std::string_view canonicalIdentity,
    bool favorite,
    std::string& message)
{
    message.clear();
    if (!IsSupportedAssetIdentity(canonicalIdentity))
    {
        message = "asset identity is not a supported Model or Texture";
        return ContentBrowserOrganizationStatus::InvalidName;
    }
    const std::size_t index = FavoriteIndex(organization, canonicalIdentity);
    if (favorite)
    {
        if (index >= organization.favorites.size())
        {
            organization.favorites.emplace_back(canonicalIdentity);
        }
        SortContentBrowserOrganization(organization);
        message = "added to Favorites";
        return ContentBrowserOrganizationStatus::Ok;
    }
    if (index < organization.favorites.size())
    {
        organization.favorites.erase(
            organization.favorites.begin() + static_cast<std::ptrdiff_t>(index));
    }
    message = "removed from Favorites";
    return ContentBrowserOrganizationStatus::Ok;
}
}
