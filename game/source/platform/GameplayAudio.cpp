#include "platform/GameplayAudio.h"

#include "platform/RuntimePaths.h"

#include "raylib.h"

#include <array>
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
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

int CueIndex(GameplaySfxCue cue)
{
    const int index = static_cast<int>(cue);
    if (index < 0 || index >= kGameplaySfxCueCount)
    {
        return -1;
    }
    return index;
}

std::string_view CueLogicalId(GameplaySfxCue cue)
{
    switch (cue)
    {
    case GameplaySfxCue::Pickup:
        return kItemPickupCollectionSoundLogicalId;
    case GameplaySfxCue::Collectible:
        return kCollectibleCollectSoundLogicalId;
    case GameplaySfxCue::Damage:
        return kPlayerDamageSoundLogicalId;
    case GameplaySfxCue::Death:
        return kPlayerDeathSoundLogicalId;
    case GameplaySfxCue::Respawn:
        return kPlayerRespawnSoundLogicalId;
    case GameplaySfxCue::Footstep:
        return kPlayerFootstepSoundLogicalId;
    case GameplaySfxCue::Jump:
        return kPlayerJumpSoundLogicalId;
    case GameplaySfxCue::Landing:
        return kPlayerLandSoundLogicalId;
    case GameplaySfxCue::CheckpointActivate:
        return kCheckpointActivateSoundLogicalId;
    case GameplaySfxCue::PressurePlateActivate:
        return kPressurePlateActivateSoundLogicalId;
    case GameplaySfxCue::PressurePlateDeactivate:
        return kPressurePlateDeactivateSoundLogicalId;
    case GameplaySfxCue::DoorUnlock:
        return kDoorUnlockSoundLogicalId;
    case GameplaySfxCue::LevelGoalComplete:
        return kLevelGoalCompleteSoundLogicalId;
    }
    return {};
}

constexpr std::array<GameplaySfxCue, kGameplaySfxCueCount> kAllCues = {
    GameplaySfxCue::Pickup,
    GameplaySfxCue::Collectible,
    GameplaySfxCue::Damage,
    GameplaySfxCue::Death,
    GameplaySfxCue::Respawn,
    GameplaySfxCue::Footstep,
    GameplaySfxCue::Jump,
    GameplaySfxCue::Landing,
    GameplaySfxCue::CheckpointActivate,
    GameplaySfxCue::PressurePlateActivate,
    GameplaySfxCue::PressurePlateDeactivate,
    GameplaySfxCue::DoorUnlock,
    GameplaySfxCue::LevelGoalComplete,
};
}

GameplayAudio::~GameplayAudio()
{
    Unload();
}

void GameplayAudio::Load()
{
    if (loadAttempted)
    {
        return;
    }
    loadAttempted = true;
    ++loadAttemptCount;
    for (GameplaySfxCue cue : kAllCues)
    {
        LoadCueFromPath(cue, RuntimeAssetPath(CueLogicalId(cue)));
    }
}

void GameplayAudio::LoadCueFromPath(GameplaySfxCue cue, const std::filesystem::path& path)
{
    const int index = CueIndex(cue);
    if (index < 0)
    {
        return;
    }

    if (cues[index] != nullptr)
    {
        auto owned = std::unique_ptr<OwnedSound>(static_cast<OwnedSound*>(cues[index]));
        UnloadSound(owned->value);
        cues[index] = nullptr;
    }
    loaded[index] = false;
    frameCounts[index] = 0;

    auto logMissing = [this, index, cue]() {
        if (missingLogged[index])
        {
            return;
        }
        missingLogged[index] = true;
        std::fprintf(
            stderr,
            "GameplayAudio: missing or invalid staged sound: %s\n",
            std::string(CueLogicalId(cue)).c_str());
    };

    if (path.empty() || !path.is_absolute())
    {
        logMissing();
        return;
    }

    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error))
    {
        logMissing();
        return;
    }

    if (!IsAudioDeviceReady())
    {
        InitAudioDevice();
        if (!IsAudioDeviceReady())
        {
            logMissing();
            return;
        }
        deviceOwned = true;
    }

    const std::string native = path.string();
    Sound loadedSound = LoadSound(native.c_str());
    if (!SoundHasFrames(loadedSound))
    {
        UnloadSound(loadedSound);
        logMissing();
        return;
    }

    SetSoundVolume(loadedSound, kGameplaySfxVolume);
    auto owned = std::make_unique<OwnedSound>();
    owned->value = loadedSound;
    cues[index] = owned.release();
    frameCounts[index] = loadedSound.frameCount;
    loaded[index] = true;
}

void GameplayAudio::Unload()
{
    for (int index = 0; index < kGameplaySfxCueCount; ++index)
    {
        if (cues[index] != nullptr)
        {
            auto owned = std::unique_ptr<OwnedSound>(static_cast<OwnedSound*>(cues[index]));
            UnloadSound(owned->value);
            cues[index] = nullptr;
        }
        loaded[index] = false;
        frameCounts[index] = 0;
        missingLogged[index] = false;
    }
    loadAttempted = false;
    if (deviceOwned)
    {
        if (IsAudioDeviceReady())
        {
            CloseAudioDevice();
        }
        deviceOwned = false;
    }
}

void GameplayAudio::PlayCue(GameplaySfxCue cue) const
{
    const int index = CueIndex(cue);
    if (index < 0 || !loaded[index] || cues[index] == nullptr)
    {
        return;
    }
    const OwnedSound* owned = static_cast<const OwnedSound*>(cues[index]);
    if (!SoundHasFrames(owned->value))
    {
        return;
    }
    PlaySound(owned->value);
}

void GameplayAudio::PlayPickup() const
{
    PlayCue(GameplaySfxCue::Pickup);
}

void GameplayAudio::PlayCollectible() const
{
    PlayCue(GameplaySfxCue::Collectible);
}

void GameplayAudio::PlayDamage() const
{
    PlayCue(GameplaySfxCue::Damage);
}

void GameplayAudio::PlayDeath() const
{
    PlayCue(GameplaySfxCue::Death);
}

void GameplayAudio::PlayRespawn() const
{
    PlayCue(GameplaySfxCue::Respawn);
}

void GameplayAudio::PlayFootstep() const
{
    PlayCue(GameplaySfxCue::Footstep);
}

void GameplayAudio::PlayJump() const
{
    PlayCue(GameplaySfxCue::Jump);
}

void GameplayAudio::PlayLanding() const
{
    PlayCue(GameplaySfxCue::Landing);
}

void GameplayAudio::PlayCheckpointActivate() const
{
    PlayCue(GameplaySfxCue::CheckpointActivate);
}

void GameplayAudio::PlayPressurePlateActivate() const
{
    PlayCue(GameplaySfxCue::PressurePlateActivate);
}

void GameplayAudio::PlayPressurePlateDeactivate() const
{
    PlayCue(GameplaySfxCue::PressurePlateDeactivate);
}

void GameplayAudio::PlayDoorUnlock() const
{
    PlayCue(GameplaySfxCue::DoorUnlock);
}

void GameplayAudio::PlayLevelGoalComplete() const
{
    PlayCue(GameplaySfxCue::LevelGoalComplete);
}

bool GameplayAudio::IsCueLoaded(GameplaySfxCue cue) const
{
    const int index = CueIndex(cue);
    return index >= 0 && loaded[index] && cues[index] != nullptr && frameCounts[index] > 0;
}

int GameplayAudio::LoadAttemptCount() const
{
    return loadAttemptCount;
}
}
