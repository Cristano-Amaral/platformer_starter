#pragma once

#include <string_view>

namespace gameplay
{
enum class PlayerAnimationState { Idle, Move, Jump };

inline constexpr float kPlayerAnimationCrossFadeSeconds = 0.12f;

inline const char* PlayerAnimationStateName(PlayerAnimationState state)
{
    switch (state)
    {
    case PlayerAnimationState::Idle: return "Idle";
    case PlayerAnimationState::Move: return "Move";
    case PlayerAnimationState::Jump: return "Jump";
    }
    return "Idle";
}

inline PlayerAnimationState SelectPlayerAnimationState(
    bool grounded, float horizontalVelocity, float meaningfulMovementThreshold)
{
    if (!grounded) return PlayerAnimationState::Jump;
    return horizontalVelocity > meaningfulMovementThreshold
            || horizontalVelocity < -meaningfulMovementThreshold
        ? PlayerAnimationState::Move : PlayerAnimationState::Idle;
}

struct PlayerAnimationPlayback
{
    PlayerAnimationState state = PlayerAnimationState::Idle;
    PlayerAnimationState previousState = PlayerAnimationState::Idle;
    float playbackTimeSeconds = 0.0f;
    float previousPlaybackTimeSeconds = 0.0f;
    float transitionTimeSeconds = kPlayerAnimationCrossFadeSeconds;
};

inline void ResetPlayerAnimation(PlayerAnimationPlayback& playback)
{
    playback = {};
}

inline void UpdatePlayerAnimation(
    PlayerAnimationPlayback& playback, bool grounded, float horizontalVelocity,
    float meaningfulMovementThreshold, float deltaSeconds)
{
    const PlayerAnimationState selected = SelectPlayerAnimationState(
        grounded, horizontalVelocity, meaningfulMovementThreshold);
    if (selected != playback.state)
    {
        playback.previousState = playback.state;
        playback.previousPlaybackTimeSeconds = playback.playbackTimeSeconds;
        playback.state = selected;
        playback.playbackTimeSeconds = 0.0f;
        playback.transitionTimeSeconds = 0.0f;
    }
    const float safeDelta = deltaSeconds > 0.0f ? deltaSeconds : 0.0f;
    playback.playbackTimeSeconds += safeDelta;
    playback.previousPlaybackTimeSeconds += safeDelta;
    playback.transitionTimeSeconds += safeDelta;
    if (playback.transitionTimeSeconds > kPlayerAnimationCrossFadeSeconds)
        playback.transitionTimeSeconds = kPlayerAnimationCrossFadeSeconds;
}

inline float PlayerAnimationBlendAmount(const PlayerAnimationPlayback& playback)
{
    return kPlayerAnimationCrossFadeSeconds > 0.0f
        ? playback.transitionTimeSeconds / kPlayerAnimationCrossFadeSeconds : 1.0f;
}
}
