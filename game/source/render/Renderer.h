#pragma once

#include "core/Vec3.h"

namespace gameplay
{
class Player;
class PlatformerCamera;
}

namespace render
{
class Renderer
{
public:
    void BeginFrame();
    void DrawWorld(
        const gameplay::Player& player,
        const gameplay::PlatformerCamera& camera,
        core::Vec3 physicsTestBoxPosition,
        core::Vec3 physicsTestBoxSize,
        core::Vec3 movingPlatformPosition,
        core::Vec3 movingPlatformSize);
    void EndFrame();
};
}
