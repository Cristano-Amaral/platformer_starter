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

    static constexpr float kMoveSpeed = 6.0f;
    static constexpr float kJumpSpeed = 8.0f;
    static constexpr float kGravity = 20.0f;

private:
    core::Vec3 position;
    core::Vec3 size;
    float verticalVelocity = 0.0f;
    bool grounded = true;
};
}
