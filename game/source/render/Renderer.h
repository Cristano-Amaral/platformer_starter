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
    void Draw(const gameplay::Player& player, const gameplay::PlatformerCamera& camera);
};
}
