#pragma once

// Milestone 71/72: narrow Application-owned gameplay SFX owner. Loads the
// fixed staged cue set once, plays semantic one-shots, and unloads before
// the audio device/window shut down. Not a generic AudioEngine,
// ResourceManager, mixer, event bus, or music system.

#include <filesystem>

namespace platform
{
inline constexpr float kGameplaySfxVolume = 1.0f;

enum class GameplaySfxCue
{
    Pickup,
    Damage,
    Death,
    Respawn,
    Footstep,
    Jump,
    Landing,
};

inline constexpr int kGameplaySfxCueCount = 7;

class GameplayAudio
{
public:
    GameplayAudio() = default;
    ~GameplayAudio();

    GameplayAudio(const GameplayAudio&) = delete;
    GameplayAudio& operator=(const GameplayAudio&) = delete;
    GameplayAudio(GameplayAudio&&) = delete;
    GameplayAudio& operator=(GameplayAudio&&) = delete;

    void Load();
    void LoadCueFromPath(GameplaySfxCue cue, const std::filesystem::path& path);
    void Unload();

    void PlayPickup() const;
    void PlayDamage() const;
    void PlayDeath() const;
    void PlayRespawn() const;
    void PlayFootstep() const;
    void PlayJump() const;
    void PlayLanding() const;

    bool IsCueLoaded(GameplaySfxCue cue) const;
    int LoadAttemptCount() const;

private:
    void PlayCue(GameplaySfxCue cue) const;

    bool deviceOwned = false;
    bool loadAttempted = false;
    int loadAttemptCount = 0;
    void* cues[kGameplaySfxCueCount]{};
    unsigned int frameCounts[kGameplaySfxCueCount]{};
    bool loaded[kGameplaySfxCueCount]{};
    bool missingLogged[kGameplaySfxCueCount]{};
};
}
