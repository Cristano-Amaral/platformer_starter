#include "render/TextureThumbnail.h"

#include "editor/TextureThumbnailLifecycle.h"

#include "raylib.h"

#include <algorithm>
#include <vector>

namespace render
{
namespace
{
constexpr int kMaxTextureThumbnailLoadsPerFrame = 1;
}

TextureThumbnailStore::~TextureThumbnailStore()
{
    Shutdown();
}

void TextureThumbnailStore::BeginFrame()
{
    loadsThisFrame = 0;
}

void TextureThumbnailStore::Shutdown()
{
    for (auto& pair : entries)
    {
        UnloadEntry(pair.second);
    }
    entries.clear();
    lastFailureMessage.clear();
}

void TextureThumbnailStore::UnloadEntry(Entry& entry)
{
    if (entry.gpuId != 0)
    {
        Texture2D texture{};
        texture.id = entry.gpuId;
        texture.width = entry.width;
        texture.height = entry.height;
        texture.mipmaps = 1;
        texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        UnloadTexture(texture);
        entry.gpuId = 0;
        entry.width = 0;
        entry.height = 0;
    }
}

bool TextureThumbnailStore::LoadFromSource(
    Entry& entry,
    std::string_view canonicalIdentity,
    const std::filesystem::path& sourcePath,
    const editor::ThumbnailSourceStamp& stamp)
{
    const Texture2D texture = LoadTexture(sourcePath.string().c_str());
    if (texture.id == 0)
    {
        UnloadEntry(entry);
        entry.failed = true;
        entry.stamp = stamp;
        lastFailureMessage = "Texture thumbnail failed to load: ";
        lastFailureMessage += canonicalIdentity;
        return false;
    }
    UnloadEntry(entry);
    entry.gpuId = texture.id;
    entry.width = texture.width;
    entry.height = texture.height;
    entry.stamp = stamp;
    entry.failed = false;
    return true;
}

void TextureThumbnailStore::Ensure(
    std::string_view canonicalIdentity,
    const std::filesystem::path& sourcePath)
{
    const std::string identity(canonicalIdentity);
    editor::ThumbnailSourceStamp sourceStamp{};
    const bool sourceExists = editor::ReadThumbnailSourceStamp(sourcePath, sourceStamp);
    auto found = entries.find(identity);
    const bool hasEntry = found != entries.end();
    const editor::TextureThumbnailEnsureDecision decision = editor::ClassifyTextureThumbnailEnsure(
        hasEntry,
        hasEntry && found->second.failed,
        hasEntry ? found->second.stamp : editor::ThumbnailSourceStamp{},
        sourceExists,
        sourceStamp);
    if (decision == editor::TextureThumbnailEnsureDecision::ReuseReady
        || decision == editor::TextureThumbnailEnsureDecision::ReuseFailed)
    {
        return;
    }
    Entry& entry = entries[identity];
    if (decision == editor::TextureThumbnailEnsureDecision::Missing)
    {
        UnloadEntry(entry);
        entry.failed = true;
        entry.stamp = {};
        lastFailureMessage = "Texture thumbnail source is missing: ";
        lastFailureMessage += identity;
        return;
    }
    if (loadsThisFrame >= kMaxTextureThumbnailLoadsPerFrame)
    {
        return;
    }
    ++loadsThisFrame;
    (void)LoadFromSource(entry, identity, sourcePath, sourceStamp);
}

unsigned int TextureThumbnailStore::TextureGpuId(std::string_view canonicalIdentity) const
{
    const auto found = entries.find(std::string(canonicalIdentity));
    if (found == entries.end() || found->second.failed)
    {
        return 0;
    }
    return found->second.gpuId;
}

int TextureThumbnailStore::TextureWidth(std::string_view canonicalIdentity) const
{
    const auto found = entries.find(std::string(canonicalIdentity));
    if (found == entries.end())
    {
        return 0;
    }
    return found->second.width;
}

int TextureThumbnailStore::TextureHeight(std::string_view canonicalIdentity) const
{
    const auto found = entries.find(std::string(canonicalIdentity));
    if (found == entries.end())
    {
        return 0;
    }
    return found->second.height;
}

bool TextureThumbnailStore::HasReadyTexture(std::string_view canonicalIdentity) const
{
    return TextureGpuId(canonicalIdentity) != 0;
}

bool TextureThumbnailStore::IsFailed(std::string_view canonicalIdentity) const
{
    const auto found = entries.find(std::string(canonicalIdentity));
    return found != entries.end() && found->second.failed;
}

void TextureThumbnailStore::Forget(std::string_view canonicalIdentity)
{
    const auto found = entries.find(std::string(canonicalIdentity));
    if (found == entries.end())
    {
        return;
    }
    UnloadEntry(found->second);
    entries.erase(found);
}

void TextureThumbnailStore::Reconcile(const std::vector<std::string>& catalogIdentities)
{
    std::vector<std::string> loaded;
    loaded.reserve(entries.size());
    for (const auto& pair : entries)
    {
        loaded.push_back(pair.first);
    }
    std::vector<std::string> stale;
    editor::CollectStaleTextureThumbnailIdentities(loaded, catalogIdentities, stale);
    for (const std::string& identity : stale)
    {
        Forget(identity);
    }
}

void TextureThumbnailStore::AllowRetryAll()
{
    for (auto& pair : entries)
    {
        if (pair.second.failed)
        {
            UnloadEntry(pair.second);
            pair.second.failed = false;
            pair.second.stamp = {};
        }
    }
}

bool TextureThumbnailStore::ConsumeLastFailure(std::string& message)
{
    if (lastFailureMessage.empty())
    {
        return false;
    }
    message = lastFailureMessage;
    lastFailureMessage.clear();
    return true;
}
}
