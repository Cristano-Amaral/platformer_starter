#include "render/Renderer.h"

#include "core/RunTimeFormat.h"
#include "core/Vec3.h"
#include "gameplay/Player.h"
#include "gameplay/DoorLockRuntime.h"
#include "platform/RuntimePaths.h"
#include "render/StaticModelScene.h"
#include "world/CollectibleWorld.h"
#include "world/GreyboxWorld.h"
#include "world/HazardWorld.h"
#include "world/ItemPickup.h"
#include "world/LevelDefinition.h"
#include "world/LevelGoal.h"
#include "world/RespawnWorld.h"
#include "world/Slope.h"
#include "world/StaticProp.h"

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <utility>
#include <vector>

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
constexpr Color kDynamicBoxColor{158, 162, 170, 255};
constexpr Color kDynamicBoxTargetFill{198, 188, 96, 255};
constexpr Color kDynamicBoxTargetWire{236, 214, 72, 255};
constexpr Color kDynamicBoxCarryFill{96, 168, 214, 255};
constexpr Color kDynamicBoxCarryWire{72, 214, 236, 255};
constexpr Color kPressurePlateInactive{86, 98, 124, 255};
constexpr Color kPressurePlateActive{56, 188, 92, 255};
constexpr Color kDoorColor{136, 96, 68, 255};
constexpr Color kDoorTargetFill{198, 188, 96, 255};
constexpr Color kDoorTargetWire{236, 214, 72, 255};
constexpr Color kItemPickupFill{92, 176, 214, 255};
constexpr Color kItemPickupTargetFill{198, 188, 96, 255};
constexpr Color kItemPickupTargetWire{236, 214, 72, 255};
constexpr Color kStaticPropFallbackColor{120, 72, 88, 255};
constexpr Color kWireColor{24, 26, 32, 255};
constexpr Color kCheckpointFuturePost{86, 94, 112, 255};
constexpr Color kCheckpointFutureBeacon{140, 148, 168, 255};
constexpr Color kCheckpointCurrentPost{48, 140, 88, 255};
constexpr Color kCheckpointCurrentBeacon{88, 220, 124, 255};
constexpr Color kCheckpointPreviousPost{36, 88, 56, 255};
constexpr Color kCheckpointPreviousBeacon{64, 148, 88, 255};
constexpr Color kGoalIncompletePost{156, 116, 52, 255};
constexpr Color kGoalIncompleteBar{188, 148, 64, 255};
constexpr Color kGoalCompletedPost{212, 168, 48, 255};
constexpr Color kGoalCompletedBar{244, 212, 84, 255};
constexpr Color kHazardBarColor{196, 48, 36, 255};
constexpr Color kHazardToothColor{232, 96, 40, 255};
constexpr Color kCollectibleFill{255, 212, 64, 255};
constexpr Color kCollectibleHudText{255, 212, 64, 255};
constexpr Color kLevelCompleteText{244, 212, 84, 255};
constexpr int kLevelCompleteFontSize = 42;
constexpr int kRestartHintFontSize = 22;
constexpr int kRestartHintGap = 16;
constexpr int kCollectedHudFontSize = 22;
constexpr int kCollectedHudMargin = 20;
constexpr int kTimerHudFontSize = 22;
constexpr int kTimerHudMargin = 20;
constexpr int kBestHudGap = 4;
constexpr Color kTimerHudText{240, 240, 244, 255};
constexpr Color kGrabHudText{236, 214, 72, 255};
constexpr Color kGrabHudMuted{200, 208, 220, 255};
constexpr Color kSpawnMarkerFill{240, 200, 64, 255};
constexpr Color kSelectionHighlightColor{255, 236, 64, 255};
constexpr Color kPendingPreviewWire{72, 220, 236, 255};
constexpr Color kPendingPreviewWireUnselected{72, 220, 236, 130};
constexpr Color kCheckpointRespawnMarkerWire{220, 64, 196, 255};
constexpr Color kCheckpointRespawnConnector{196, 96, 180, 255};
constexpr float kCheckpointRespawnMarkerSize = 0.28f;
constexpr Color kPendingCheckpointPost{64, 188, 204, 96};
constexpr Color kPendingCheckpointBeacon{96, 228, 240, 140};
constexpr Color kPendingCheckpointPostUnselected{64, 188, 204, 48};
constexpr Color kPendingCheckpointBeaconUnselected{96, 228, 240, 70};
constexpr Color kPendingHazardTooth{255, 168, 72, 140};
constexpr Color kPendingHazardBar{196, 48, 36, 72};
constexpr Color kPendingHazardToothUnselected{255, 168, 72, 72};
constexpr Color kPendingHazardBarUnselected{196, 48, 36, 40};
constexpr Color kPendingCollectibleFill{255, 212, 64, 72};
constexpr Color kPendingCollectibleWire{72, 220, 236, 255};
constexpr Color kPendingCollectibleFillUnselected{255, 212, 64, 40};
constexpr Color kPendingCollectibleWireUnselected{72, 220, 236, 130};
constexpr Color kPlacementCandidateWire{196, 255, 255, 255};
constexpr Color kPlacementCandidatePost{96, 232, 244, 140};
constexpr Color kPlacementCandidateBeacon{160, 255, 255, 200};
constexpr Color kPlacementCandidateHazardBar{220, 72, 56, 96};
constexpr Color kPlacementCandidateHazardTooth{255, 196, 96, 180};
constexpr Color kPlacementCandidateCollectibleFill{255, 228, 96, 96};
constexpr Color kPlacementFallbackWire{196, 255, 255, 150};
constexpr Color kPlacementHudText{210, 255, 255, 230};
constexpr Color kPlacementHudMuted{196, 220, 228, 200};
constexpr Color kEditorCollectedCollectibleWire{220, 188, 72, 255};
constexpr Color kPendingDeleteWire{176, 70, 82, 200};
constexpr unsigned char kPendingDeleteFillAlpha = 118;
constexpr Color kGizmoAxisX{220, 72, 72, 255};
constexpr Color kGizmoAxisY{72, 196, 88, 255};
constexpr Color kGizmoAxisZ{72, 128, 232, 255};
constexpr Color kGizmoAxisActive{255, 255, 255, 255};

constexpr int kGridSlices = 20;
constexpr float kGridSpacing = 1.0f;

// Inventory path string for Metrics. The texture is not drawn in Level 01.
constexpr const char* kTestTextureRuntimeRelativePath = "assets/textures/test_checker.png";

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

void DrawGhostBox(core::Vec3 center, core::Vec3 size, Color fill, Color wire)
{
    const Vector3 position = ToRaylib(center);
    DrawCube(position, size.x, size.y, size.z, fill);
    DrawCubeWires(position, size.x, size.y, size.z, wire);
}

void DrawPendingDeleteWires(core::Vec3 center, core::Vec3 size)
{
    if (size.x <= 0.0f || size.y <= 0.0f || size.z <= 0.0f)
    {
        return;
    }
    DrawCubeWires(ToRaylib(center), size.x, size.y, size.z, kPendingDeleteWire);
}

Color FadePendingDelete(Color source, unsigned char alpha)
{
    return Color{
        static_cast<unsigned char>((static_cast<int>(source.r) + 210) / 2),
        static_cast<unsigned char>((static_cast<int>(source.g) + 208) / 2),
        static_cast<unsigned char>((static_cast<int>(source.b) + 206) / 2),
        alpha};
}

void DrawPendingDeleteSolid(
    core::Vec3 center,
    core::Vec3 size,
    Color sourceFill,
    unsigned char alpha = kPendingDeleteFillAlpha)
{
    if (size.x <= 0.0f || size.y <= 0.0f || size.z <= 0.0f)
    {
        return;
    }
    DrawCube(ToRaylib(center), size.x, size.y, size.z, FadePendingDelete(sourceFill, alpha));
    DrawPendingDeleteWires(center, size);
}

bool OverlayMarksPendingDelete(const std::vector<int>& indices, std::size_t query)
{
    const int queryIndex = static_cast<int>(query);
    for (int index : indices)
    {
        if (index == queryIndex)
        {
            return true;
        }
    }
    return false;
}

void DrawCheckpointMarkerGeometry(
    const world::CheckpointSpec& spec,
    Color postColor,
    Color beaconColor,
    bool ghost,
    Color ghostWire);

// Visual-only: lethal AABB is the bar. Three teeth sit on the top face, inside
// the XZ footprint, so the drawn volume is slightly taller than the lethal box.
void DrawHazardTeeth(
    const world::HazardSpec& spec,
    Color toothColor,
    bool ghost,
    Color ghostWire)
{
    constexpr int kToothCount = 3;
    constexpr float kToothHeight = 0.35f;
    const float toothSizeX = spec.size.x * 0.22f;
    const float toothSizeZ = spec.size.z * 0.40f;
    const float toothCenterY = spec.center.y + spec.size.y * 0.5f + kToothHeight * 0.5f;
    const float xSpan = spec.size.x * 0.32f;
    const core::Vec3 toothSize{toothSizeX, kToothHeight, toothSizeZ};
    for (int toothIndex = 0; toothIndex < kToothCount; ++toothIndex)
    {
        const float xOffset = -xSpan + static_cast<float>(toothIndex) * xSpan;
        const core::Vec3 toothCenter{spec.center.x + xOffset, toothCenterY, spec.center.z};
        if (ghost)
        {
            DrawGhostBox(toothCenter, toothSize, toothColor, ghostWire);
        }
        else
        {
            DrawGreyboxBox(toothCenter, toothSize, toothColor);
        }
    }
}

void DrawHazard(const world::HazardSpec& spec)
{
    DrawGreyboxBox(spec.center, spec.size, kHazardBarColor);
    DrawHazardTeeth(spec, kHazardToothColor, false, kPendingPreviewWire);
}

void DrawPendingDeleteHazard(const world::HazardSpec& spec)
{
    DrawPendingDeleteSolid(spec.center, spec.size, kHazardBarColor);
    DrawHazardTeeth(
        spec,
        FadePendingDelete(kHazardToothColor, kPendingDeleteFillAlpha),
        true,
        kPendingDeleteWire);
}

void DrawPendingDeleteCheckpoint(const world::CheckpointSpec& spec)
{
    DrawPendingDeleteWires(spec.center, spec.size);
    const world::CheckpointMarkerLayout layout = world::MakeCheckpointMarkerLayout(spec);
    DrawPendingDeleteSolid(layout.postCenter, layout.postSize, kCheckpointFuturePost);
    DrawPendingDeleteSolid(layout.beaconCenter, layout.beaconSize, kCheckpointFutureBeacon);
}

void DrawCollectible(const world::CollectibleSpec& spec)
{
    const core::Vec3 visualSize{
        world::kCollectibleVisualSize,
        world::kCollectibleVisualSize,
        world::kCollectibleVisualSize};
    DrawGreyboxBox(spec.center, visualSize, kCollectibleFill);
}

void DrawCollectedCounter(int collectedCount, int collectibleTotal)
{
    const char* text = TextFormat("COLLECTED %d / %d", collectedCount, collectibleTotal);
    const int width = MeasureText(text, kCollectedHudFontSize);
    const int x = GetScreenWidth() - width - kCollectedHudMargin;
    DrawText(text, x, kCollectedHudMargin, kCollectedHudFontSize, kCollectibleHudText);
}

void DrawRunTimer(double elapsedSeconds)
{
    char formatted[32]{};
    core::FormatRunTime(formatted, sizeof(formatted), elapsedSeconds);
    const char* text = TextFormat("TIME %s", formatted);
    DrawText(text, kTimerHudMargin, kTimerHudMargin, kTimerHudFontSize, kTimerHudText);
}

void DrawSessionBest(bool hasBestTime, double bestSeconds)
{
    char formatted[32]{};
    core::FormatSessionBestTime(formatted, sizeof(formatted), hasBestTime, bestSeconds);
    const char* text = TextFormat("BEST %s", formatted);
    const int y = kTimerHudMargin + kTimerHudFontSize + kBestHudGap;
    DrawText(text, kTimerHudMargin, y, kTimerHudFontSize, kTimerHudText);
}

void DrawGrabCarryHud(bool carrying, bool hasTarget)
{
    if (!carrying && !hasTarget)
    {
        return;
    }

    const char* text = carrying ? "E Drop" : "E Grab";
    const int font = kTimerHudFontSize;
    const int width = MeasureText(text, font);
    const int x = (GetScreenWidth() - width) / 2;
    const int y = GetScreenHeight() - font - kTimerHudMargin;
    DrawText(text, x, y, font, carrying ? kGrabHudMuted : kGrabHudText);
}

void DrawPickupHud(const char* text)
{
    if (text == nullptr || text[0] == '\0')
    {
        return;
    }
    const int font = kTimerHudFontSize;
    const int width = MeasureText(text, font);
    const int x = (GetScreenWidth() - width) / 2;
    const int y = GetScreenHeight() - font - kTimerHudMargin;
    DrawText(text, x, y, font, kGrabHudText);
}

void DrawInventoryPanel(const InventoryPanelView& panel)
{
    if (!panel.visible)
    {
        return;
    }

    constexpr int kTitleSize = 28;
    constexpr int kRowSize = 20;
    constexpr int kDetailSize = 20;
    constexpr int kHintSize = 16;
    constexpr int kPad = 20;
    constexpr int kPanelWidth = 420;
    constexpr Color kPanelFill{18, 20, 28, 220};
    constexpr Color kPanelEdge{210, 214, 224, 180};
    constexpr Color kTitle{244, 212, 84, 255};
    constexpr Color kRow{240, 240, 244, 255};
    constexpr Color kSelectedFill{198, 188, 96, 70};
    constexpr Color kSelectedText{236, 214, 72, 255};
    constexpr Color kMuted{200, 208, 220, 255};

    const int entryCount = static_cast<int>(panel.entries.size());
    const int listHeight = entryCount == 0 ? kRowSize : entryCount * (kRowSize + 4);
    const int panelHeight = kPad + kTitleSize + 12 + listHeight + 16 + kDetailSize * 3 + 12
        + kHintSize * 2 + kPad;
    const int panelX = (GetScreenWidth() - kPanelWidth) / 2;
    const int panelY = (GetScreenHeight() - panelHeight) / 2;

    DrawRectangle(panelX, panelY, kPanelWidth, panelHeight, kPanelFill);
    DrawRectangleLines(panelX, panelY, kPanelWidth, panelHeight, kPanelEdge);

    const char* title = "INVENTORY";
    const int titleWidth = MeasureText(title, kTitleSize);
    DrawText(title, panelX + (kPanelWidth - titleWidth) / 2, panelY + kPad, kTitleSize, kTitle);

    int y = panelY + kPad + kTitleSize + 12;
    if (entryCount == 0)
    {
        const char* emptyText = "Inventory is empty";
        const int emptyWidth = MeasureText(emptyText, kRowSize);
        DrawText(
            emptyText, panelX + (kPanelWidth - emptyWidth) / 2, y, kRowSize, kMuted);
        y += kRowSize + 16;
        y += kDetailSize * 3;
    }
    else
    {
        int selectedQuantity = 0;
        std::string_view selectedId = panel.selectedItemId;
        for (const gameplay::InventoryEntry& entry : panel.entries)
        {
            const bool selected = entry.itemId == panel.selectedItemId;
            if (selected)
            {
                selectedQuantity = entry.quantity;
                selectedId = entry.itemId;
                DrawRectangle(panelX + 12, y - 2, kPanelWidth - 24, kRowSize + 4, kSelectedFill);
            }
            const char* marker = selected ? ">" : " ";
            const char* line = TextFormat("%s %s", marker, entry.itemId.c_str());
            DrawText(line, panelX + kPad, y, kRowSize, selected ? kSelectedText : kRow);
            const char* qty = TextFormat("x%d", entry.quantity);
            const int qtyWidth = MeasureText(qty, kRowSize);
            DrawText(qty, panelX + kPanelWidth - kPad - qtyWidth, y, kRowSize,
                selected ? kSelectedText : kRow);
            y += kRowSize + 4;
        }
        y += 12;
        DrawText("Selected:", panelX + kPad, y, kDetailSize, kMuted);
        y += kDetailSize + 2;
        char selectedBuf[48]{};
        if (selectedId.empty())
        {
            std::snprintf(selectedBuf, sizeof(selectedBuf), "-");
        }
        else
        {
            std::snprintf(
                selectedBuf,
                sizeof(selectedBuf),
                "%.*s",
                static_cast<int>(selectedId.size()),
                selectedId.data());
        }
        DrawText(selectedBuf, panelX + kPad, y, kDetailSize, kSelectedText);
        y += kDetailSize + 2;
        DrawText(
            TextFormat("Quantity: %d", selectedQuantity),
            panelX + kPad,
            y,
            kDetailSize,
            kRow);
        y += kDetailSize + 12;
    }

    DrawText("Arrows: Select", panelX + kPad, y, kHintSize, kMuted);
    y += kHintSize + 2;
    DrawText("Tab/Esc: Close", panelX + kPad, y, kHintSize, kMuted);
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

void DrawRuntimeDynamicBox(const DynamicBoxDrawState& box, Color fill, Color wire)
{
    if (!(box.size.x > 0.0f) || !(box.size.y > 0.0f) || !(box.size.z > 0.0f))
    {
        return;
    }

    float axisX = 0.0f;
    float axisY = 1.0f;
    float axisZ = 0.0f;
    float degrees = 0.0f;
    const float w = box.rotationW < -1.0f ? -1.0f : (box.rotationW > 1.0f ? 1.0f : box.rotationW);
    const float sine = std::sqrt(std::max(0.0f, 1.0f - w * w));
    if (sine > 1.0e-6f)
    {
        axisX = box.rotationX / sine;
        axisY = box.rotationY / sine;
        axisZ = box.rotationZ / sine;
        degrees = 2.0f * std::acos(w) * (180.0f / 3.14159265358979323846f);
    }

    rlPushMatrix();
    rlTranslatef(box.center.x, box.center.y, box.center.z);
    if (degrees != 0.0f)
    {
        rlRotatef(degrees, axisX, axisY, axisZ);
    }
    DrawCube(Vector3{0.0f, 0.0f, 0.0f}, box.size.x, box.size.y, box.size.z, fill);
    DrawCubeWires(Vector3{0.0f, 0.0f, 0.0f}, box.size.x, box.size.y, box.size.z, wire);
    rlPopMatrix();
}

Camera3D MakeCamera(const CameraView& view)
{
    Camera3D camera{};
    camera.position = ToRaylib(view.position);
    camera.target = ToRaylib(view.target);
    camera.up = ToRaylib(view.up);
    camera.fovy = view.fieldOfViewY;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}

void BeginMode3DInRect(const Camera3D& camera, const WorldViewRect& rect)
{
    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();
    int width = rect.width;
    int height = rect.height;
    if (width < 1)
    {
        width = 1;
    }
    if (height < 1)
    {
        height = 1;
    }
    int x = rect.x;
    int y = rect.y;
    if (x < 0)
    {
        x = 0;
    }
    if (y < 0)
    {
        y = 0;
    }

    rlDrawRenderBatchActive();
    rlViewport(x, screenH - (y + height), width, height);

    rlMatrixMode(RL_PROJECTION);
    rlPushMatrix();
    rlLoadIdentity();

    const double aspect = static_cast<double>(width) / static_cast<double>(height);
    const double top = RL_CULL_DISTANCE_NEAR * std::tan(static_cast<double>(camera.fovy) * 0.5 * DEG2RAD);
    const double right = top * aspect;
    rlFrustum(-right, right, -top, top, RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);

    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
    const Matrix matView = MatrixLookAt(camera.position, camera.target, camera.up);
    rlMultMatrixf(MatrixToFloat(matView));
    rlEnableDepthTest();

    (void)screenW;
}

void EndMode3DRestoreViewport()
{
    EndMode3D();
    rlViewport(0, 0, GetScreenWidth(), GetScreenHeight());
}

void DrawOrientedWires(core::Vec3 center, core::Vec3 size, float rotationZDegrees, Color color)
{
    rlPushMatrix();
    rlTranslatef(center.x, center.y, center.z);
    rlRotatef(rotationZDegrees, 0.0f, 0.0f, 1.0f);
    DrawCubeWires(Vector3{0.0f, 0.0f, 0.0f}, size.x, size.y, size.z, color);
    rlPopMatrix();
}

void DrawGizmoAxis(
    Vector3 origin,
    Vector3 direction,
    float length,
    float shaftRadius,
    Color color)
{
    const float headLength = length * 0.22f;
    const float shaftLength = length - headLength;
    const Vector3 shaftEnd{
        origin.x + direction.x * shaftLength,
        origin.y + direction.y * shaftLength,
        origin.z + direction.z * shaftLength};
    const Vector3 tip{
        origin.x + direction.x * length,
        origin.y + direction.y * length,
        origin.z + direction.z * length};
    DrawCylinderEx(origin, shaftEnd, shaftRadius, shaftRadius, 8, color);
    DrawCylinderEx(shaftEnd, tip, shaftRadius * 2.4f, 0.0f, 8, color);
}

Color ScaleGizmoColor(Color color, unsigned char alpha)
{
    color.a = alpha;
    return color;
}

void DrawTranslationGizmo(const DebugWorldOverlay& overlay)
{
    if (!overlay.drawTranslationGizmo || !(overlay.gizmoAxisLength > 0.0f))
    {
        return;
    }

    const float length = overlay.gizmoAxisLength;
    // Visual only. Hit testing stays on EditorGizmo world-space shafts.
    const float visualRadius = std::max(length * 0.038f, 0.045f);
    const core::Vec3 origin = overlay.gizmoOrigin;
    const Vector3 originRl = ToRaylib(origin);

    const struct AxisDraw
    {
        int id;
        Vector3 direction;
        Color color;
    } axes[] = {
        {1, {1.0f, 0.0f, 0.0f}, kGizmoAxisX},
        {2, {0.0f, 1.0f, 0.0f}, kGizmoAxisY},
        {3, {0.0f, 0.0f, 1.0f}, kGizmoAxisZ},
    };

    const auto drawPass = [&](unsigned char alpha, float radiusScale)
    {
        DrawSphere(originRl, visualRadius * 0.85f * radiusScale, ScaleGizmoColor({220, 220, 228, 255}, alpha));
        for (const AxisDraw& axis : axes)
        {
            const bool active = overlay.gizmoActiveAxis == axis.id;
            const bool hovered = overlay.gizmoHoveredAxis == axis.id;
            Color color = axis.color;
            float radius = visualRadius * radiusScale;
            if (active)
            {
                color = kGizmoAxisActive;
                radius *= 1.35f;
            }
            else if (hovered)
            {
                color = {
                    static_cast<unsigned char>(std::min(255, axis.color.r + 48)),
                    static_cast<unsigned char>(std::min(255, axis.color.g + 48)),
                    static_cast<unsigned char>(std::min(255, axis.color.b + 48)),
                    255};
                radius *= 1.2f;
            }

            DrawGizmoAxis(
                originRl,
                axis.direction,
                length,
                radius,
                ScaleGizmoColor(color, alpha));
        }
    };

    // Depth-tested pass: faint cue where a handle is not buried in the mesh.
    drawPass(70, 0.85f);

    // Editor overlay: X/Y/Z stay readable through Ground/Platform/Spawn.
    // Flush before and after the GL state change; restore immediately so later
    // 2D HUD / ImGui keep the default raylib depth policy.
    rlDrawRenderBatchActive();
    rlDisableDepthTest();
    rlDisableDepthMask();
    rlDisableBackfaceCulling();
    drawPass(255, 1.0f);
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
    rlEnableDepthTest();
}

void DrawResizeGizmo(const DebugWorldOverlay& overlay)
{
    if ((!overlay.drawResizeGizmo && !overlay.drawScaleGizmo)
        || !(overlay.gizmoAxisLength > 0.0f))
    {
        return;
    }

    const float length = overlay.gizmoAxisLength;
    const float visualRadius = std::max(length * 0.038f, 0.045f);
    const float cubeSize = std::max(length * 0.14f, 0.16f);
    const core::Vec3 origin = overlay.gizmoOrigin;
    const Vector3 originRl = ToRaylib(origin);

    const struct AxisDraw
    {
        int id;
        Vector3 direction;
        Color color;
    } axes[] = {
        {1, {1.0f, 0.0f, 0.0f}, kGizmoAxisX},
        {2, {0.0f, 1.0f, 0.0f}, kGizmoAxisY},
        {3, {0.0f, 0.0f, 1.0f}, kGizmoAxisZ},
    };

    const auto drawPass = [&](unsigned char alpha, float radiusScale)
    {
        DrawSphere(originRl, visualRadius * 0.7f * radiusScale, ScaleGizmoColor({220, 220, 228, 255}, alpha));
        for (const AxisDraw& axis : axes)
        {
            const Vector3 negative{
                originRl.x - axis.direction.x * length,
                originRl.y - axis.direction.y * length,
                originRl.z - axis.direction.z * length};
            const Vector3 positive{
                originRl.x + axis.direction.x * length,
                originRl.y + axis.direction.y * length,
                originRl.z + axis.direction.z * length};
            DrawCylinderEx(
                negative,
                positive,
                visualRadius * 0.55f * radiusScale,
                visualRadius * 0.55f * radiusScale,
                8,
                ScaleGizmoColor(axis.color, static_cast<unsigned char>(alpha / 2 + 40)));

            const int signs[] = {1, -1};
            for (int sign : signs)
            {
                const bool active =
                    overlay.gizmoActiveAxis == axis.id && overlay.gizmoActiveSign == sign;
                const bool hovered =
                    overlay.gizmoHoveredAxis == axis.id && overlay.gizmoHoveredSign == sign;
                Color color = axis.color;
                float size = cubeSize * radiusScale;
                if (active)
                {
                    color = kGizmoAxisActive;
                    size *= 1.28f;
                }
                else if (hovered)
                {
                    color = {
                        static_cast<unsigned char>(std::min(255, axis.color.r + 48)),
                        static_cast<unsigned char>(std::min(255, axis.color.g + 48)),
                        static_cast<unsigned char>(std::min(255, axis.color.b + 48)),
                        255};
                    size *= 1.16f;
                }

                const Vector3 handle{
                    originRl.x + axis.direction.x * length * static_cast<float>(sign),
                    originRl.y + axis.direction.y * length * static_cast<float>(sign),
                    originRl.z + axis.direction.z * length * static_cast<float>(sign)};
                DrawCube(handle, size, size, size, ScaleGizmoColor(color, alpha));
            }
        }
    };

    drawPass(70, 0.85f);

    rlDrawRenderBatchActive();
    rlDisableDepthTest();
    rlDisableDepthMask();
    rlDisableBackfaceCulling();
    drawPass(255, 1.0f);
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
    rlEnableDepthTest();
}

void DrawWorldOverlay(const DebugWorldOverlay& overlay)
{
    if (overlay.drawSpawnMarker
        && overlay.spawnSize.x > 0.0f && overlay.spawnSize.y > 0.0f && overlay.spawnSize.z > 0.0f)
    {
        DrawGreyboxBox(overlay.spawnCenter, overlay.spawnSize, kSpawnMarkerFill);
    }
    if (overlay.drawHighlight
        && overlay.highlightSize.x > 0.0f && overlay.highlightSize.y > 0.0f
        && overlay.highlightSize.z > 0.0f)
    {
        if (overlay.highlightRotationZDegrees != 0.0f)
        {
            DrawOrientedWires(
                overlay.highlightCenter,
                overlay.highlightSize,
                overlay.highlightRotationZDegrees,
                kSelectionHighlightColor);
        }
        else
        {
            DrawCubeWires(
                ToRaylib(overlay.highlightCenter),
                overlay.highlightSize.x,
                overlay.highlightSize.y,
                overlay.highlightSize.z,
                kSelectionHighlightColor);
        }
    }
    if (!overlay.collectedAuthoredCollectibleCenters.empty())
    {
        const float size = world::kCollectibleVisualSize;
        for (const core::Vec3& center : overlay.collectedAuthoredCollectibleCenters)
        {
            DrawCubeWires(ToRaylib(center), size, size, size, kEditorCollectedCollectibleWire);
        }
    }
    // Pending-delete faded solids use the same 3D depth test as the world.
    // They replace the skipped opaque DrawWorld geometry. Gizmo overlay later
    // disables depth; this pass does not.
    {
        const Color platformColors[] = {kPlatformColor, kPlatformAccentColor};
        for (std::size_t index = 0; index < overlay.pendingDeletePlatformCenters.size(); ++index)
        {
            const int platformIndex = overlay.pendingDeletePlatformIndices[index];
            const Color source = platformColors[(platformIndex < 0 ? 0 : platformIndex) % 2];
            DrawPendingDeleteSolid(
                overlay.pendingDeletePlatformCenters[index],
                overlay.pendingDeletePlatformSizes[index],
                source);
        }
    }
    for (const world::CheckpointSpec& checkpoint : overlay.pendingDeleteCheckpoints)
    {
        DrawPendingDeleteCheckpoint(checkpoint);
    }
    for (const world::HazardSpec& hazard : overlay.pendingDeleteHazards)
    {
        DrawPendingDeleteHazard(hazard);
    }
    {
        const core::Vec3 visualSize{
            world::kCollectibleVisualSize,
            world::kCollectibleVisualSize,
            world::kCollectibleVisualSize};
        for (const core::Vec3& center : overlay.pendingDeleteCollectibleCenters)
        {
            DrawPendingDeleteSolid(center, visualSize, kCollectibleFill);
        }
    }
    for (std::size_t index = 0; index < overlay.pendingDeleteDynamicBoxCenters.size(); ++index)
    {
        if (index >= overlay.pendingDeleteDynamicBoxSizes.size())
        {
            break;
        }
        DrawPendingDeleteSolid(
            overlay.pendingDeleteDynamicBoxCenters[index],
            overlay.pendingDeleteDynamicBoxSizes[index],
            kDynamicBoxColor);
    }
    for (std::size_t index = 0; index < overlay.pendingDeletePressurePlateCenters.size(); ++index)
    {
        if (index >= overlay.pendingDeletePressurePlateSizes.size())
        {
            break;
        }
        DrawPendingDeleteSolid(
            overlay.pendingDeletePressurePlateCenters[index],
            overlay.pendingDeletePressurePlateSizes[index],
            kPressurePlateInactive);
    }
    for (std::size_t index = 0; index < overlay.pendingDeleteDoorCenters.size(); ++index)
    {
        if (index >= overlay.pendingDeleteDoorSizes.size())
        {
            break;
        }
        DrawPendingDeleteSolid(
            overlay.pendingDeleteDoorCenters[index],
            overlay.pendingDeleteDoorSizes[index],
            kDoorColor);
    }
    for (std::size_t index = 0; index < overlay.pendingDeleteItemPickupCenters.size(); ++index)
    {
        if (index >= overlay.pendingDeleteItemPickupSizes.size())
        {
            break;
        }
        DrawPendingDeleteSolid(
            overlay.pendingDeleteItemPickupCenters[index],
            overlay.pendingDeleteItemPickupSizes[index],
            kItemPickupFill);
    }
    for (std::size_t index = 0; index < overlay.pendingDeleteStaticPropCenters.size(); ++index)
    {
        if (index >= overlay.pendingDeleteStaticPropSizes.size())
        {
            break;
        }
        DrawPendingDeleteSolid(
            overlay.pendingDeleteStaticPropCenters[index],
            overlay.pendingDeleteStaticPropSizes[index],
            kStaticPropFallbackColor);
    }
    const auto drawPendingAuthoring = [&](bool selectedPass) {
        for (const DebugWorldOverlay::PendingAuthoringOverlayItem& item : overlay.pendingAuthoring)
        {
            if (item.selected != selectedPass)
            {
                continue;
            }
            const Color boundsWire =
                selectedPass ? kPendingPreviewWire : kPendingPreviewWireUnselected;
            if (item.drawObjectVisual)
            {
                if (item.kind == 1)
                {
                    DrawCheckpointMarkerGeometry(
                        item.checkpoint,
                        selectedPass ? kPendingCheckpointPost : kPendingCheckpointPostUnselected,
                        selectedPass ? kPendingCheckpointBeacon
                                     : kPendingCheckpointBeaconUnselected,
                        true,
                        boundsWire);
                }
                else if (item.kind == 2)
                {
                    DrawGhostBox(
                        item.hazard.center,
                        item.hazard.size,
                        selectedPass ? kPendingHazardBar : kPendingHazardBarUnselected,
                        boundsWire);
                    DrawHazardTeeth(
                        item.hazard,
                        selectedPass ? kPendingHazardTooth : kPendingHazardToothUnselected,
                        true,
                        boundsWire);
                }
                else if (item.kind == 3)
                {
                    const core::Vec3 visualSize{
                        world::kCollectibleVisualSize,
                        world::kCollectibleVisualSize,
                        world::kCollectibleVisualSize};
                    DrawGhostBox(
                        item.collectible.center,
                        visualSize,
                        selectedPass ? kPendingCollectibleFill : kPendingCollectibleFillUnselected,
                        selectedPass ? kPendingCollectibleWire
                                     : kPendingCollectibleWireUnselected);
                }
            }
            if (item.boundsSize.x > 0.0f && item.boundsSize.y > 0.0f && item.boundsSize.z > 0.0f)
            {
                DrawCubeWires(
                    ToRaylib(item.boundsCenter),
                    item.boundsSize.x,
                    item.boundsSize.y,
                    item.boundsSize.z,
                    boundsWire);
            }
            if (item.kind == 1)
            {
                DrawCubeWires(
                    ToRaylib(item.checkpoint.respawnPosition),
                    kCheckpointRespawnMarkerSize,
                    kCheckpointRespawnMarkerSize,
                    kCheckpointRespawnMarkerSize,
                    boundsWire);
            }
        }
    };
    drawPendingAuthoring(false);
    drawPendingAuthoring(true);
    if (overlay.drawPlacementCandidate)
    {
        const bool fallback = overlay.placementCandidateFallback;
        const Color boundsWire = fallback ? kPlacementFallbackWire : kPlacementCandidateWire;
        if (!fallback)
        {
            if (overlay.placementCandidateKind == 1)
            {
                DrawCheckpointMarkerGeometry(
                    overlay.placementCandidateCheckpoint,
                    kPlacementCandidatePost,
                    kPlacementCandidateBeacon,
                    true,
                    boundsWire);
                DrawCubeWires(
                    ToRaylib(overlay.placementCandidateCheckpoint.respawnPosition),
                    kCheckpointRespawnMarkerSize,
                    kCheckpointRespawnMarkerSize,
                    kCheckpointRespawnMarkerSize,
                    boundsWire);
            }
            else if (overlay.placementCandidateKind == 2)
            {
                DrawGhostBox(
                    overlay.placementCandidateHazard.center,
                    overlay.placementCandidateHazard.size,
                    kPlacementCandidateHazardBar,
                    boundsWire);
                DrawHazardTeeth(
                    overlay.placementCandidateHazard,
                    kPlacementCandidateHazardTooth,
                    true,
                    boundsWire);
            }
            else if (overlay.placementCandidateKind == 3)
            {
                const core::Vec3 visualSize{
                    world::kCollectibleVisualSize,
                    world::kCollectibleVisualSize,
                    world::kCollectibleVisualSize};
                DrawGhostBox(
                    overlay.placementCandidateCollectible.center,
                    visualSize,
                    kPlacementCandidateCollectibleFill,
                    boundsWire);
            }
        }
        else if (overlay.placementCandidateKind == 1)
        {
            DrawCubeWires(
                ToRaylib(overlay.placementCandidateCheckpoint.respawnPosition),
                kCheckpointRespawnMarkerSize,
                kCheckpointRespawnMarkerSize,
                kCheckpointRespawnMarkerSize,
                boundsWire);
        }
        if (overlay.placementCandidateSize.x > 0.0f && overlay.placementCandidateSize.y > 0.0f
            && overlay.placementCandidateSize.z > 0.0f)
        {
            DrawCubeWires(
                ToRaylib(overlay.placementCandidateCenter),
                overlay.placementCandidateSize.x,
                overlay.placementCandidateSize.y,
                overlay.placementCandidateSize.z,
                boundsWire);
            const Vector3 sit{
                overlay.placementCandidateCenter.x,
                overlay.placementCandidateCenter.y - overlay.placementCandidateSize.y * 0.5f,
                overlay.placementCandidateCenter.z};
            DrawCubeWires(sit, fallback ? 0.36f : 0.28f, 0.06f, fallback ? 0.36f : 0.28f, boundsWire);
            if (fallback)
            {
                DrawCubeWires(
                    ToRaylib(overlay.placementCandidateCenter),
                    overlay.placementCandidateSize.x * 0.92f,
                    overlay.placementCandidateSize.y * 0.92f,
                    overlay.placementCandidateSize.z * 0.92f,
                    boundsWire);
            }
        }
    }
    if (overlay.drawStaticPropPlacementPreview
        && overlay.staticPropPlacementBoundsSize.x > 0.0f
        && overlay.staticPropPlacementBoundsSize.y > 0.0f
        && overlay.staticPropPlacementBoundsSize.z > 0.0f)
    {
        DrawCubeWires(
            ToRaylib(overlay.staticPropPlacementBoundsCenter),
            overlay.staticPropPlacementBoundsSize.x,
            overlay.staticPropPlacementBoundsSize.y,
            overlay.staticPropPlacementBoundsSize.z,
            kPlacementCandidateWire);
    }
    if (overlay.drawCheckpointRespawnMarker)
    {
        const Vector3 respawn = ToRaylib(overlay.checkpointRespawnMarker);
        DrawCubeWires(
            respawn,
            kCheckpointRespawnMarkerSize,
            kCheckpointRespawnMarkerSize,
            kCheckpointRespawnMarkerSize,
            kCheckpointRespawnMarkerWire);
        DrawCubeWires(
            respawn,
            kCheckpointRespawnMarkerSize * 0.45f,
            kCheckpointRespawnMarkerSize * 1.8f,
            kCheckpointRespawnMarkerSize * 0.45f,
            kCheckpointRespawnMarkerWire);
        if (overlay.drawCheckpointRespawnConnector)
        {
            DrawLine3D(
                ToRaylib(overlay.checkpointTriggerCenter),
                respawn,
                kCheckpointRespawnConnector);
        }
    }
    DrawTranslationGizmo(overlay);
    DrawResizeGizmo(overlay);
}

void DrawCheckpointMarkerGeometry(
    const world::CheckpointSpec& spec,
    Color postColor,
    Color beaconColor,
    bool ghost,
    Color ghostWire)
{
    const world::CheckpointMarkerLayout layout = world::MakeCheckpointMarkerLayout(spec);
    if (ghost)
    {
        DrawGhostBox(layout.postCenter, layout.postSize, postColor, ghostWire);
        DrawGhostBox(layout.beaconCenter, layout.beaconSize, beaconColor, ghostWire);
        return;
    }

    DrawGreyboxBox(layout.postCenter, layout.postSize, postColor);
    DrawGreyboxBox(layout.beaconCenter, layout.beaconSize, beaconColor);
}

void DrawCheckpointMarker(
    const world::CheckpointSpec& spec,
    world::CheckpointVisualState visualState)
{
    Color postColor = kCheckpointFuturePost;
    Color beaconColor = kCheckpointFutureBeacon;
    switch (visualState)
    {
    case world::CheckpointVisualState::Current:
        postColor = kCheckpointCurrentPost;
        beaconColor = kCheckpointCurrentBeacon;
        break;
    case world::CheckpointVisualState::PreviouslyActivated:
        postColor = kCheckpointPreviousPost;
        beaconColor = kCheckpointPreviousBeacon;
        break;
    default:
        break;
    }

    DrawCheckpointMarkerGeometry(spec, postColor, beaconColor, false, kPendingPreviewWire);
}

void DrawLevelGoalMarker(const world::LevelGoalSpec& goal, bool levelCompleted)
{
    constexpr float postWidth = 0.16f;
    constexpr float postHeight = 1.6f;
    constexpr float barHeight = 0.16f;
    constexpr float barDepth = 0.16f;
    constexpr float postSpread = 0.70f;
    constexpr float zOffset = -0.90f;

    const float platformTopY =
        goal.center.y - world::kPlayerVisualSize.y * 0.5f;
    const float postCenterY = platformTopY + postHeight * 0.5f;
    const float z = goal.center.z + zOffset;
    const core::Vec3 postSize{postWidth, postHeight, postWidth};
    const core::Vec3 leftPost{
        goal.center.x - postSpread,
        postCenterY,
        z};
    const core::Vec3 rightPost{
        goal.center.x + postSpread,
        postCenterY,
        z};
    const core::Vec3 barCenter{
        goal.center.x,
        platformTopY + postHeight + barHeight * 0.5f,
        z};
    const core::Vec3 barSize{postSpread * 2.0f + postWidth, barHeight, barDepth};

    const Color postColor = levelCompleted ? kGoalCompletedPost : kGoalIncompletePost;
    const Color barColor = levelCompleted ? kGoalCompletedBar : kGoalIncompleteBar;
    DrawGreyboxBox(leftPost, postSize, postColor);
    DrawGreyboxBox(rightPost, postSize, postColor);
    DrawGreyboxBox(barCenter, barSize, barColor);
}

void DrawLevelCompleteMessage()
{
    const char* completeText = "LEVEL COMPLETE";
    const int completeWidth = MeasureText(completeText, kLevelCompleteFontSize);
    const int completeX = (GetScreenWidth() - completeWidth) / 2;
    const int completeY = GetScreenHeight() / 10;
    DrawText(completeText, completeX, completeY, kLevelCompleteFontSize, kLevelCompleteText);

    const char* hintText = "PRESS ENTER TO RESTART";
    const int hintWidth = MeasureText(hintText, kRestartHintFontSize);
    const int hintX = (GetScreenWidth() - hintWidth) / 2;
    const int hintY = completeY + kLevelCompleteFontSize + kRestartHintGap;
    DrawText(hintText, hintX, hintY, kRestartHintFontSize, kLevelCompleteText);
}

}

Renderer::Renderer()
    : staticPropModels(std::make_unique<StaticModelSceneStore>())
{
}

Renderer::~Renderer()
{
    if (staticPropModels)
    {
        staticPropModels->Shutdown();
    }
}

void Renderer::SyncStaticPropModels(
    const world::LevelDefinition& level,
    std::string_view extraIdentity)
{
    if (staticPropModels)
    {
        staticPropModels->Sync(level, extraIdentity);
    }
}

StaticModelSceneStore* Renderer::StaticPropModels()
{
    return staticPropModels.get();
}

const StaticModelSceneStore* Renderer::StaticPropModels() const
{
    return staticPropModels.get();
}

void Renderer::LoadRuntimeAssets()
{
}

void Renderer::UnloadRuntimeAssets()
{
    if (staticPropModels)
    {
        staticPropModels->Shutdown();
    }
}

bool Renderer::IsTestTextureLoaded() const
{
    return false;
}

bool Renderer::IsTestTextureFallbackActive() const
{
    return false;
}

const char* Renderer::TestTextureLogicalId() const
{
    return platform::kTestCheckerLogicalId.data();
}

const char* Renderer::TestTextureRuntimeRelativePath() const
{
    return kTestTextureRuntimeRelativePath;
}

bool Renderer::IsTestModelLoaded() const
{
    return false;
}

bool Renderer::IsTestModelFallbackActive() const
{
    return false;
}

const char* Renderer::TestModelLogicalId() const
{
    return platform::kTestStaticModelLogicalId.data();
}

bool Renderer::IsAuthoredModelLoaded() const
{
    return false;
}

bool Renderer::IsAuthoredModelFallbackActive() const
{
    return false;
}

const char* Renderer::AuthoredModelLogicalId() const
{
    return platform::kTestAuthoredModelLogicalId.data();
}

bool Renderer::IsTexturedModelLoaded() const
{
    return false;
}

bool Renderer::IsTexturedModelFallbackActive() const
{
    return false;
}

const char* Renderer::TexturedModelLogicalId() const
{
    return platform::kTestTexturedModelLogicalId.data();
}

int Renderer::TexturedModelMaterialCount() const
{
    return 0;
}

bool Renderer::TexturedModelHasAlbedoTexture() const
{
    return false;
}

void Renderer::BeginFrame()
{
    BeginDrawing();
    ClearBackground(kBackgroundColor);
}

void Renderer::DrawWorld(
    const gameplay::Player& player,
    const CameraView& cameraView,
    const world::LevelDefinition& level,
    const std::vector<DynamicBoxDrawState>& dynamicBoxes,
    const std::vector<PressurePlateDrawState>& pressurePlates,
    const std::vector<DoorDrawState>& doors,
    core::Vec3 movingPlatformPosition,
        core::Vec3 movingPlatformSize,
        const std::vector<world::CheckpointVisualState>& checkpointVisuals,
        bool levelCompleted,
        const std::vector<std::uint8_t>& collectibleCollected,
        int collectedCount,
        const std::vector<std::uint8_t>& itemPickupCollected,
        int itemPickupTargetIndex,
        int lockedDoorTargetIndex,
        bool inventoryHasKey,
        double elapsedSeconds,
        bool hasBestTime,
        double bestSeconds,
        InventoryPanelView inventoryPanel,
        const DebugWorldOverlay& overlay,
        WorldViewRect viewRect)
{
    const Camera3D view = MakeCamera(cameraView);
    const bool subViewport = viewRect.width > 0 && viewRect.height > 0;

    // BeginMode3D reads rlGetCullDistanceNear/Far. Model Preview / thumbnails
    // set those to a tight model frame and must not leak into Gameplay.
    rlSetClipPlanes(RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);

    if (subViewport)
    {
        BeginMode3DInRect(view, viewRect);
    }
    else
    {
        BeginMode3D(view);
    }

    RestoreGreyboxImmediateState();

    DrawGrid(kGridSlices, kGridSpacing);
    DrawGreyboxBox(level.ground.center, level.ground.size, kGroundColor);

    const Color platformColors[] = {kPlatformColor, kPlatformAccentColor};
    int platformIndex = 0;
    for (const world::Box& platform : level.elevatedPlatforms)
    {
        if (!OverlayMarksPendingDelete(
                overlay.pendingDeletePlatformIndices, static_cast<std::size_t>(platformIndex)))
        {
            DrawGreyboxBox(
                platform.center,
                platform.size,
                platformColors[platformIndex % 2]);
        }
        ++platformIndex;
    }

    DrawGreyboxBox(movingPlatformPosition, movingPlatformSize, kMovingPlatformColor);
    bool grabHudCarrying = false;
    bool grabHudTarget = false;
    for (std::size_t index = 0; index < dynamicBoxes.size(); ++index)
    {
        if (OverlayMarksPendingDelete(overlay.pendingDeleteDynamicBoxIndices, index))
        {
            continue;
        }
        Color fill = kDynamicBoxColor;
        Color wire = kWireColor;
        if (dynamicBoxes[index].feedback == DynamicBoxDrawFeedback::Carried)
        {
            fill = kDynamicBoxCarryFill;
            wire = kDynamicBoxCarryWire;
            grabHudCarrying = true;
        }
        else if (dynamicBoxes[index].feedback == DynamicBoxDrawFeedback::Targeted)
        {
            fill = kDynamicBoxTargetFill;
            wire = kDynamicBoxTargetWire;
            grabHudTarget = true;
        }
        DrawRuntimeDynamicBox(dynamicBoxes[index], fill, wire);
    }
    for (std::size_t index = 0; index < pressurePlates.size(); ++index)
    {
        if (OverlayMarksPendingDelete(overlay.pendingDeletePressurePlateIndices, index))
        {
            continue;
        }
        DrawGreyboxBox(
            pressurePlates[index].center,
            pressurePlates[index].size,
            pressurePlates[index].active ? kPressurePlateActive : kPressurePlateInactive);
    }
    bool doorHudTarget = false;
    for (std::size_t index = 0; index < doors.size(); ++index)
    {
        if (OverlayMarksPendingDelete(overlay.pendingDeleteDoorIndices, index))
        {
            continue;
        }
        const bool targeted = lockedDoorTargetIndex == static_cast<int>(index);
        if (targeted)
        {
            doorHudTarget = true;
            DrawGhostBox(doors[index].center, doors[index].size, kDoorTargetFill, kDoorTargetWire);
        }
        else
        {
            DrawGreyboxBox(doors[index].center, doors[index].size, kDoorColor);
        }
    }
    if (staticPropModels)
    {
        staticPropModels->ResetDrawStats();
    }
    for (std::size_t index = 0; index < level.staticProps.size(); ++index)
    {
        if (OverlayMarksPendingDelete(overlay.pendingDeleteStaticPropIndices, index))
        {
            continue;
        }
        if (staticPropModels)
        {
            staticPropModels->DrawProp(level.staticProps[index]);
        }
    }
    bool pickupHudTarget = false;
    char pickupHudText[64]{};
    for (std::size_t index = 0; index < level.itemPickups.size(); ++index)
    {
        if (OverlayMarksPendingDelete(overlay.pendingDeleteItemPickupIndices, index))
        {
            continue;
        }
        if (index < itemPickupCollected.size() && itemPickupCollected[index] != 0)
        {
            continue;
        }
        const world::ItemPickupSpec& pickup = level.itemPickups[index];
        const bool targeted = itemPickupTargetIndex == static_cast<int>(index);
        if (targeted)
        {
            pickupHudTarget = true;
            std::snprintf(
                pickupHudText,
                sizeof(pickupHudText),
                "E Pick Up %s x%d",
                pickup.itemId.c_str(),
                pickup.quantity);
        }
        const Color fill = targeted ? kItemPickupTargetFill : kItemPickupFill;
        const Color wire = targeted ? kItemPickupTargetWire : kWireColor;
        if (!pickup.modelIdentity.empty() && staticPropModels)
        {
            world::StaticPropSpec visual{};
            visual.modelIdentity = pickup.modelIdentity;
            visual.position = pickup.position;
            visual.rotationDegrees = world::kDefaultStaticPropRotationDegrees;
            visual.scale = world::kDefaultStaticPropScale;
            staticPropModels->DrawProp(visual);
            if (targeted)
            {
                DrawCubeWires(
                    ToRaylib(pickup.position),
                    world::kItemPickupVisualSize,
                    world::kItemPickupVisualSize,
                    world::kItemPickupVisualSize,
                    wire);
            }
            continue;
        }
        DrawCube(
            ToRaylib(pickup.position),
            world::kItemPickupVisualSize,
            world::kItemPickupVisualSize,
            world::kItemPickupVisualSize,
            fill);
        DrawCubeWires(
            ToRaylib(pickup.position),
            world::kItemPickupVisualSize,
            world::kItemPickupVisualSize,
            world::kItemPickupVisualSize,
            wire);
    }
    DrawOrientedGreyboxBox(
        level.slopes[static_cast<std::size_t>(world::kLevel01WalkableSlopeIndex)],
        kWalkableSlopeColor);
    DrawOrientedGreyboxBox(
        level.slopes[static_cast<std::size_t>(world::kLevel01SteepSlopeIndex)],
        kSteepSlopeColor);
    for (std::size_t hazardIndex = 0; hazardIndex < level.hazards.size(); ++hazardIndex)
    {
        if (OverlayMarksPendingDelete(overlay.pendingDeleteHazardIndices, hazardIndex))
        {
            continue;
        }
        DrawHazard(level.hazards[hazardIndex]);
    }
    const std::size_t collectibleCount =
        level.collectibles.size() < collectibleCollected.size() ? level.collectibles.size()
                                                                : collectibleCollected.size();
    for (std::size_t collectibleIndex = 0; collectibleIndex < collectibleCount; ++collectibleIndex)
    {
        if (OverlayMarksPendingDelete(
                overlay.pendingDeleteCollectibleIndices, collectibleIndex))
        {
            continue;
        }
        if (collectibleCollected[collectibleIndex] == 0)
        {
            DrawCollectible(level.collectibles[collectibleIndex]);
        }
        // Collected cubes stay hidden here. Editor F2 draws an authored
        // wireframe placeholder from DebugWorldOverlay instead of mutating
        // CollectibleRunState. Pending-delete collectibles use the faded
        // overlay instead of this runtime cube or the collected gold wire.
    }
    DrawGreyboxBox(player.Position(), player.Size(), kPlayerColor);
    const std::size_t checkpointCount =
        level.checkpoints.size() < checkpointVisuals.size() ? level.checkpoints.size()
                                                            : checkpointVisuals.size();
    for (std::size_t checkpointIndex = 0; checkpointIndex < checkpointCount; ++checkpointIndex)
    {
        if (OverlayMarksPendingDelete(overlay.pendingDeleteCheckpointIndices, checkpointIndex))
        {
            continue;
        }
        DrawCheckpointMarker(
            level.checkpoints[checkpointIndex], checkpointVisuals[checkpointIndex]);
    }
    DrawLevelGoalMarker(level.goal, levelCompleted);

    // Editor overlay last in 3D: world, then marker/highlight/faded
    // pending-delete (depth on), cyan pending ghost, then the
    // depth-independent gizmo. ImGui is after EndMode3D.
    if (overlay.drawStaticPropPlacementPreview && staticPropModels)
    {
        staticPropModels->DrawPlacementPreview(overlay.staticPropPlacementPreview);
    }
    DrawWorldOverlay(overlay);

    if (subViewport)
    {
        EndMode3DRestoreViewport();
    }
    else
    {
        EndMode3D();
    }

    DrawRunTimer(elapsedSeconds);
    DrawSessionBest(hasBestTime, bestSeconds);
    DrawCollectedCounter(collectedCount, static_cast<int>(level.collectibles.size()));
    if (!inventoryPanel.visible)
    {
        DrawGrabCarryHud(grabHudCarrying, grabHudTarget);
        if (!grabHudCarrying && !grabHudTarget && pickupHudTarget)
        {
            DrawPickupHud(pickupHudText);
        }
        else if (!grabHudCarrying && !grabHudTarget && !pickupHudTarget && doorHudTarget)
        {
            DrawPickupHud(gameplay::LockedDoorPromptText(inventoryHasKey));
        }
    }
    if (levelCompleted)
    {
        DrawLevelCompleteMessage();
    }
    DrawInventoryPanel(inventoryPanel);
}

void Renderer::DrawOrientationWidget(const OrientationWidgetOverlay& overlay)
{
    if (!overlay.visible || !(overlay.radius > 0.0f))
    {
        return;
    }

    const float ox = overlay.originX;
    const float oy = overlay.originY;
    const float radius = overlay.radius;
    DrawCircle(static_cast<int>(ox), static_cast<int>(oy), radius + 10.0f, Color{18, 20, 28, 150});
    DrawCircleLines(static_cast<int>(ox), static_cast<int>(oy), radius + 10.0f, Color{210, 214, 224, 180});

    struct AxisTip
    {
        core::Vec3 projected;
        Color color;
        const char* label;
        float depth;
    };

    AxisTip axes[] = {
        {overlay.x, kGizmoAxisX, "X", overlay.x.z},
        {overlay.y, kGizmoAxisY, "Y", overlay.y.z},
        {overlay.z, kGizmoAxisZ, "Z", overlay.z.z},
    };
    if (axes[0].depth > axes[1].depth)
    {
        std::swap(axes[0], axes[1]);
    }
    if (axes[1].depth > axes[2].depth)
    {
        std::swap(axes[1], axes[2]);
    }
    if (axes[0].depth > axes[1].depth)
    {
        std::swap(axes[0], axes[1]);
    }

    for (const AxisTip& axis : axes)
    {
        const float posX = ox + axis.projected.x * radius;
        const float posY = oy - axis.projected.y * radius;
        const float negX = ox - axis.projected.x * radius;
        const float negY = oy + axis.projected.y * radius;
        const Color dim{
            static_cast<unsigned char>(axis.color.r / 2 + 20),
            static_cast<unsigned char>(axis.color.g / 2 + 20),
            static_cast<unsigned char>(axis.color.b / 2 + 20),
            200};
        DrawLineEx({ox, oy}, {negX, negY}, 2.0f, dim);
        DrawCircle(static_cast<int>(negX), static_cast<int>(negY), 4.0f, dim);
        DrawLineEx({ox, oy}, {posX, posY}, 3.0f, axis.color);
        DrawCircle(static_cast<int>(posX), static_cast<int>(posY), 6.0f, axis.color);
        DrawText(
            axis.label,
            static_cast<int>(posX) + 7,
            static_cast<int>(posY) - 6,
            12,
            axis.color);
    }
}

void Renderer::DrawEditorPlacementHud(
    bool visible,
    const char* category,
    bool fallback,
    float topInset)
{
    if (!visible || category == nullptr || category[0] == '\0')
    {
        return;
    }

    const int font = 16;
    const char* line1 = TextFormat("Placing: %s", category);
    const char* line2 = "LMB place | Esc cancel";
    const char* line3 = fallback ? "No surface hit" : nullptr;
    const int width1 = MeasureText(line1, font);
    const int width2 = MeasureText(line2, font);
    const int width3 = line3 != nullptr ? MeasureText(line3, font) : 0;
    const int maxWidth = width1 > width2 ? width1 : width2;
    const int boxWidth = (maxWidth > width3 ? maxWidth : width3) + 16;
    const int lineCount = line3 != nullptr ? 3 : 2;
    const int boxHeight = 8 + lineCount * (font + 2);
    const int x = (GetScreenWidth() - boxWidth) / 2;
    const int y = static_cast<int>(topInset > 0.0f ? topInset : 8.0f) + 6;
    DrawRectangle(x, y, boxWidth, boxHeight, Color{18, 24, 32, 150});
    const int textX1 = (GetScreenWidth() - width1) / 2;
    const int textX2 = (GetScreenWidth() - width2) / 2;
    DrawText(line1, textX1, y + 4, font, kPlacementHudText);
    DrawText(line2, textX2, y + 4 + font + 2, font, kPlacementHudMuted);
    if (line3 != nullptr)
    {
        const int textX3 = (GetScreenWidth() - width3) / 2;
        DrawText(line3, textX3, y + 4 + 2 * (font + 2), font, kPlacementHudMuted);
    }
}

void Renderer::EndFrame()
{
    EndDrawing();
}
}