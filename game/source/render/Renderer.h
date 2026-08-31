#pragma once

namespace gameplay
{
class Player;
struct PlatformerCamera;
}

namespace render
{
class Renderer
{
public:
    void Draw(const gameplay::Player& player, const gameplay::PlatformerCamera& camera);
};
}
