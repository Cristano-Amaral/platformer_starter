#include "render/Renderer.h"

#include "core/Vec3.h"
#include "gameplay/PlatformerCamera.h"
#include "gameplay/Player.h"
#include "world/GreyboxWorld.h"

#include "raylib.h"

namespace render
{
namespace
{
constexpr Color kBackgroundColor{32, 36, 48, 255};
constexpr Color kGroundColor{78, 84, 96, 255};
constexpr Color kPlatformColor{110, 118, 132, 255};
constexpr Color kPlatformAccentColor{96, 104, 118, 255};
constexpr Color kPlayerColor{216, 96, 72, 255};
constexpr Color kWireColor{24, 26, 32, 255};

constexpr int kGridSlices = 20;
constexpr float kGridSpacing = 1.0f;

Vector3 ToRaylib(core::Vec3 value)
{
    return Vector3{value.x, value.y, value.z};
}

void DrawGreyboxBox(core::Vec3 center, core::Vec3 size, Color fill)
{
    const Vector3 position = ToRaylib(center);
    DrawCube(position, size.x, size.y, size.z, fill);
    DrawCubeWires(position, size.x, size.y, size.z, kWireColor);
}

Camera3D MakeCamera(const gameplay::PlatformerCamera& camera)
{
    const core::Vec3 target = camera.Target();
    Camera3D view{};
    view.position = ToRaylib(target + camera.offset);
    view.target = ToRaylib(target);
    view.up = Vector3{0.0f, 1.0f, 0.0f};
    view.fovy = camera.fieldOfViewY;
    view.projection = CAMERA_PERSPECTIVE;
    return view;
}
}

void Renderer::Draw(const gameplay::Player& player, const gameplay::PlatformerCamera& camera)
{
    BeginDrawing();
    ClearBackground(kBackgroundColor);

    const Camera3D view = MakeCamera(camera);
    BeginMode3D(view);

    DrawGrid(kGridSlices, kGridSpacing);
    DrawGreyboxBox(world::kGround.center, world::kGround.size, kGroundColor);
    DrawGreyboxBox(
        world::kElevatedPlatforms[0].center,
        world::kElevatedPlatforms[0].size,
        kPlatformColor);
    DrawGreyboxBox(
        world::kElevatedPlatforms[1].center,
        world::kElevatedPlatforms[1].size,
        kPlatformAccentColor);
    DrawGreyboxBox(player.Position(), player.Size(), kPlayerColor);

    EndMode3D();
    EndDrawing();
}
}
