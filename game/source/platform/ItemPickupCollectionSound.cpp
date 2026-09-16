#include "platform/ItemPickupCollectionSound.h"

#include "platform/RuntimePaths.h"

#include "raylib.h"

#include <memory>
#include <string>
#include <system_error>
#include <utility>

namespace platform
{
namespace
{
struct OwnedSound
{
    Sound value{};
};

bool SoundHasFrames(const Sound& sound)
{
    return sound.frameCount > 0;
}
}

ItemPickupCollectionSound::~ItemPickupCollectionSound()
{
    Unload();
}

void ItemPickupCollectionSound::Load()
{
    LoadFromPath(RuntimeAssetPath(kItemPickupCollectionSoundLogicalId));
}

void ItemPickupCollectionSound::LoadFromPath(const std::filesystem::path& path)
{
    Unload();
    if (path.empty() || !path.is_absolute())
    {
        return;
    }

    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error))
    {
        return;
    }

    if (!IsAudioDeviceReady())
    {
        InitAudioDevice();
        if (!IsAudioDeviceReady())
        {
            return;
        }
        deviceOwned = true;
    }

    const std::string native = path.string();
    Sound loadedSound = LoadSound(native.c_str());
    if (!SoundHasFrames(loadedSound))
    {
        UnloadSound(loadedSound);
        if (deviceOwned)
        {
            CloseAudioDevice();
            deviceOwned = false;
        }
        return;
    }

    auto owned = std::make_unique<OwnedSound>();
    owned->value = loadedSound;
    sound = owned.release();
    frameCount = loadedSound.frameCount;
    loaded = true;
}

void ItemPickupCollectionSound::Unload()
{
    if (sound != nullptr)
    {
        auto owned = std::unique_ptr<OwnedSound>(static_cast<OwnedSound*>(sound));
        UnloadSound(owned->value);
        sound = nullptr;
    }
    loaded = false;
    frameCount = 0;
    if (deviceOwned)
    {
        if (IsAudioDeviceReady())
        {
            CloseAudioDevice();
        }
        deviceOwned = false;
    }
}

void ItemPickupCollectionSound::Play() const
{
    if (!loaded || sound == nullptr)
    {
        return;
    }
    const OwnedSound* owned = static_cast<const OwnedSound*>(sound);
    if (!SoundHasFrames(owned->value))
    {
        return;
    }
    PlaySound(owned->value);
}

bool ItemPickupCollectionSound::IsLoaded() const
{
    return loaded && sound != nullptr && frameCount > 0;
}
}
