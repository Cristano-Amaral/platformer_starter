#include "render/Renderer.h"

#include "core/Vec3.h"
#include "gameplay/PlatformerCamera.h"
#include "gameplay/Player.h"

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

constexpr core::Vec3 kGroundCenter{0.0f, -0.25f, 0.0f};
constexpr core::Vec3 kGroundSize{24.0f, 0.5f, 8.0f};

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

Camera3D MakeCamera(const gameplay::Player& player, const gameplay::PlatformerCamera& camera)
{
    const core::Vec3 target = player.Position();
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

    const Camera3D view = MakeCamera(player, camera);
    BeginMode3D(view);

    DrawGrid(kGridSlices, kGridSpacing);
    DrawGreyboxBox(kGroundCenter, kGroundSize, kGroundColor);
    DrawGreyboxBox(core::Vec3{5.0f, 0.75f, 0.0f}, core::Vec3{4.0f, 0.5f, 3.0f}, kPlatformColor);
    DrawGreyboxBox(core::Vec3{-4.5f, 1.5f, 0.0f}, core::Vec3{3.0f, 0.5f, 2.5f}, kPlatformAccentColor);
    DrawGreyboxBox(player.Position(), player.Size(), kPlayerColor);

    EndMode3D();
    EndDrawing();
}
}
