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
};

// Application-owned runtime checkpoint/respawn state. Not owned by
// PhysicsWorld or Renderer.
struct RespawnState
{
    bool checkpointActive = false;
    core::Vec3 respawnPosition = world::kInitialSpawnVisualCenter;
    int deathCount = 0;
    RespawnReason lastRespawnReason = RespawnReason::None;
};
}
