#pragma once

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
    void DrawWorld(const gameplay::Player& player, const gameplay::PlatformerCamera& camera);
    void EndFrame();
};
}
