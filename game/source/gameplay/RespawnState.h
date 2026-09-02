#pragma once

#include "core/Vec3.h"
#include "world/RespawnWorld.h"

namespace gameplay
{
enum class RespawnReason
{
    None,
    Fall,
    Manual,
    Hazard,
};

// Application-owned runtime checkpoint/respawn state. Not owned by
// PhysicsWorld or Renderer.
//
// activeCheckpointIndex: -1 = none, 0 = Checkpoint 1, 1 = Checkpoint 2.
struct RespawnState
{
    int activeCheckpointIndex = world::kNoActiveCheckpointIndex;
    core::Vec3 respawnPosition = world::kInitialSpawnVisualCenter;
    int deathCount = 0;
    RespawnReason lastRespawnReason = RespawnReason::None;
};
}
