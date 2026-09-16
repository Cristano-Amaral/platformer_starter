#pragma once

// Milestone 61: one short Item Pickup collection sound. Not a general audio
// engine or audio-event framework. Loaded once from the staged runtime path.

#include <filesystem>

namespace platform
{
class ItemPickupCollectionSound
{
public:
    ItemPickupCollectionSound() = default;
    ~ItemPickupCollectionSound();

    ItemPickupCollectionSound(const ItemPickupCollectionSound&) = delete;
    ItemPickupCollectionSound& operator=(const ItemPickupCollectionSound&) = delete;
    ItemPickupCollectionSound(ItemPickupCollectionSound&&) = delete;
    ItemPickupCollectionSound& operator=(ItemPickupCollectionSound&&) = delete;

    void Load();
    void LoadFromPath(const std::filesystem::path& path);
    void Unload();
    void Play() const;
    bool IsLoaded() const;

private:
    bool deviceOwned = false;
    bool loaded = false;
    unsigned int frameCount = 0;
    void* sound = nullptr;
};
}
