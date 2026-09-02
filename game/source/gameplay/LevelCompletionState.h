#pragma once

namespace gameplay
{
// Application-owned one-way run completion. Not owned by Player,
// PhysicsWorld, Renderer, or PlatformerCamera. PerformRespawn must not clear it.
struct LevelCompletionState
{
    bool completed = false;
};
}
