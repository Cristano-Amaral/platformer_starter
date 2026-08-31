#pragma once

#include "core/Vec3.h"
#include "input/InputState.h"

namespace gameplay
{
class Player
{
public:
    Player(core::Vec3 position, core::Vec3 size);

    void Update(const input::InputState& input, float deltaSeconds);

    const core::Vec3& Position() const;
    const core::Vec3& Size() const;

    // Units per second along X. Tune here.
    static constexpr float kMoveSpeed = 6.0f;

private:
    core::Vec3 position;
    core::Vec3 size;
};
}
