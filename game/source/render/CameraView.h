#pragma once

// Project-owned view used by the renderer. Neither raylib Camera3D nor
// gameplay::PlatformerCamera. Gameplay builds this from PlatformerCamera;
// the editor builds it from EditorCamera. Renderer does not own camera state.

#include "core/Vec3.h"

namespace render
{
struct CameraView
{
    core::Vec3 position{};
    core::Vec3 target{};
    core::Vec3 up{0.0f, 1.0f, 0.0f};
    float fieldOfViewY = 40.0f;
};
}
