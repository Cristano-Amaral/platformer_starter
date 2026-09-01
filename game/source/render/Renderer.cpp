#include "render/Renderer.h"

#include "core/Vec3.h"
#include "gameplay/PlatformerCamera.h"
#include "gameplay/Player.h"
#include "world/GreyboxWorld.h"
#include "world/Slope.h"

#include "raylib.h"
#include "rlgl.h"

namespace render
{
namespace
{
constexpr Color kBackgroundColor{32, 36, 48, 255};
constexpr Color kGroundColor{78, 84, 96, 255};
constexpr Color kPlatformColor{110, 118, 132, 255};
constexpr Color kPlatformAccentColor{96, 104, 118, 255};
constexpr Color kPlayerColor{216, 96, 72, 255};
constexpr Color kMovingPlatformColor{168, 132, 72, 255};
constexpr Color kWalkableSlopeColor{132, 148, 92, 255};
constexpr Color kSteepSlopeColor{148, 92, 84, 255};
constexpr Color kPhysicsTestBoxColor{64, 176, 196, 255};
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

void DrawOrientedGreyboxBox(const world::SlopeSpec& slope, Color fill)
{
    rlPushMatrix();
    rlTranslatef(slope.center.x, slope.center.y, slope.center.z);
    rlRotatef(slope.rotationZDegrees, 0.0f, 0.0f, 1.0f);
    DrawCube(Vector3{0.0f, 0.0f, 0.0f}, slope.size.x, slope.size.y, slope.size.z, fill);
    DrawCubeWires(Vector3{0.0f, 0.0f, 0.0f}, slope.size.x, slope.size.y, slope.size.z, kWireColor);
    rlPopMatrix();
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

void Renderer::BeginFrame()
{
    BeginDrawing();
    ClearBackground(kBackgroundColor);
}

void Renderer::DrawWorld(
    const gameplay::Player& player,
    const gameplay::PlatformerCamera& camera,
    core::Vec3 physicsTestBoxPosition,
    core::Vec3 physicsTestBoxSize,
    core::Vec3 movingPlatformPosition,
    core::Vec3 movingPlatformSize)
{
    const Camera3D view = MakeCamera(camera);
    BeginMode3D(view);

    DrawGrid(kGridSlices, kGridSpacing);
    DrawGreyboxBox(world::kGround.center, world::kGround.size, kGroundColor);

    const Color platformColors[] = {kPlatformColor, kPlatformAccentColor};
    int platformIndex = 0;
    for (const world::Box& platform : world::kElevatedPlatforms)
    {
        DrawGreyboxBox(
            platform.center,
            platform.size,
            platformColors[platformIndex % 2]);
        ++platformIndex;
    }

    DrawGreyboxBox(movingPlatformPosition, movingPlatformSize, kMovingPlatformColor);
    DrawOrientedGreyboxBox(world::kWalkableSlope, kWalkableSlopeColor);
    DrawOrientedGreyboxBox(world::kSteepSlope, kSteepSlopeColor);
    DrawGreyboxBox(player.Position(), player.Size(), kPlayerColor);
    DrawGreyboxBox(physicsTestBoxPosition, physicsTestBoxSize, kPhysicsTestBoxColor);

    EndMode3D();
}

void Renderer::EndFrame()
{
    EndDrawing();
}
}
