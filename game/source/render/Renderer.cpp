#include "render/Renderer.h"

#include "core/RunTimeFormat.h"
#include "core/Vec3.h"
#include "editor/EditorViewportGrid.h"
#include "gameplay/GameFlowState.h"
#include "gameplay/Player.h"
#include "gameplay/PlayerPresentation.h"
#include "gameplay/ItemPickupCollectionFeedback.h"
#include "gameplay/GameplayObjectiveHud.h"
#include "gameplay/PlayerHealth.h"
#include "gameplay/PlayerDeath.h"
#include "gameplay/ItemPickupCollectionHud.h"
#include "platform/RuntimePaths.h"
#include "assets/StaticGlb.h"
#include "render/ItemPickupTargetHighlight.h"
#include "render/ItemPickupCollectionFeedbackDraw.h"
#include "render/LevelGoalVisualization.h"
#include "render/LoadedModelMaterials.h"
#include "render/StaticModelScene.h"
#include "render/TerrainMesh.h"
#include "render/WorldLighting.h"
#include "world/CollectibleWorld.h"
#include "world/GreyboxWorld.h"
#include "world/HazardWorld.h"
#include "world/ItemPickup.h"
#include "world/LevelDefinition.h"
#include "world/LevelGoal.h"
#include "world/DirectionalLightActivation.h"
#include "world/LocalLightActivation.h"
#include "world/RespawnWorld.h"
#include "world/Slope.h"
#include "world/StaticProp.h"
#include "world/TerrainGeometry.h"

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <system_error>
#include <utility>
#include <vector>

namespace render
{
namespace
{
constexpr Color kBackgroundColor{32, 36, 48, 255};
constexpr Color kGroundColor{78, 84, 96, 255};
constexpr Color kTerrainColor{118, 140, 96, 255};
constexpr Color kTerrainSculptBrushColor{255, 214, 70, 255};
constexpr Color kTerrainSculptBrushMuted{255, 214, 70, 120};
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
constexpr Color kPressurePlateHiddenEditorFill{86, 98, 124, 56};
constexpr Color kPressurePlateHiddenEditorWire{140, 176, 214, 220};
constexpr Color kDoorColor{136, 96, 68, 255};
constexpr Color kDoorTargetFill{198, 188, 96, 255};
constexpr Color kDoorTargetWire{236, 214, 72, 255};
constexpr Color kItemPickupFill{92, 176, 214, 255};
constexpr Color kItemPickupTargetGold{255, 220, 72, 255};
constexpr Color kItemPickupTargetWire{236, 214, 72, 255};
constexpr Color kStaticPropFallbackColor{120, 72, 88, 255};
constexpr Color kWireColor{24, 26, 32, 255};
constexpr Color kCheckpointFuturePost{86, 94, 112, 255};
constexpr Color kCheckpointFutureBeacon{140, 148, 168, 255};
constexpr Color kCheckpointCurrentPost{48, 140, 88, 255};
constexpr Color kCheckpointCurrentBeacon{88, 220, 124, 255};
constexpr Color kCheckpointPreviousPost{36, 88, 56, 255};
constexpr Color kCheckpointPreviousBeacon{64, 148, 88, 255};

enum class WorldSolidMode
{
    Combined,
    Solid,
    Wires
};

WorldSolidMode gWorldSolidMode = WorldSolidMode::Combined;
WorldLightingResources* gWorldLighting = nullptr;
const ModelDrawOverride* gWorldModelOverride = nullptr;
constexpr Color kGoalVolumeIncomplete{64, 140, 92, 255};
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
constexpr Color kSecondarySelectionHighlightColor{255, 168, 64, 220};
constexpr Color kSelectedModelBoundsWire{200, 188, 72, 140};
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
constexpr Color kEditorGridMinor{78, 84, 96, 255};
constexpr Color kEditorGridMajor{132, 140, 154, 255};
constexpr Color kEditorGridAxisX{210, 72, 72, 255};
constexpr Color kEditorGridAxisZ{72, 118, 220, 255};

// Inventory path string for Metrics. The texture is not drawn in Level 01.
constexpr const char* kTestTextureRuntimeRelativePath = "assets/textures/test_checker.png";

Vector3 ToRaylib(core::Vec3 value)
{
    return Vector3{value.x, value.y, value.z};
}

Color MixRgb(Color from, Color to, float amount)
{
    const float t = amount < 0.0f ? 0.0f : (amount > 1.0f ? 1.0f : amount);
    const auto mix = [t](unsigned char a, unsigned char b) {
        return static_cast<unsigned char>(std::lround(
            static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * t));
    };
    return Color{mix(from.r, to.r), mix(from.g, to.g), mix(from.b, to.b), 255};
}

void DrawGreyboxBox(core::Vec3 center, core::Vec3 size, Color fill)
{
    if (gWorldSolidMode == WorldSolidMode::Solid && gWorldLighting != nullptr)
    {
        gWorldLighting->DrawSolidBox(center, size, fill);
        return;
    }
    const Vector3 position = ToRaylib(center);
    if (gWorldSolidMode == WorldSolidMode::Wires)
    {
        DrawCubeWires(position, size.x, size.y, size.z, kWireColor);
        return;
    }
    DrawCube(position, size.x, size.y, size.z, fill);
    DrawCubeWires(position, size.x, size.y, size.z, kWireColor);
}

void DrawAuthoredTerrain(const TerrainGpuResources* terrainGpu, Color fill)
{
    if (terrainGpu == nullptr || !terrainGpu->HasMesh() || terrainGpu->GetMesh() == nullptr)
    {
        return;
    }
    const Mesh& mesh = *terrainGpu->GetMesh();
    if (gWorldSolidMode == WorldSolidMode::Wires)
    {
        return;
    }
    const Color surface = terrainGpu->HasTexture() ? WHITE : fill;
    if (gWorldSolidMode == WorldSolidMode::Solid && gWorldLighting != nullptr)
    {
        TerrainLayerDrawRequest request{};
        request.layerCount = terrainGpu->LayerCount();
        const world::TerrainSpec* spec = terrainGpu->LastSpec();
        if (spec != nullptr)
        {
            request.originX = spec->origin.x;
            request.originZ = spec->origin.z;
            for (int layer = 0; layer < world::kMaxTerrainMaterialLayers; ++layer)
            {
                request.tiling[layer] = world::TerrainLayerTextureTiling(*spec, layer);
                const Texture2D* loaded = terrainGpu->GetLayerTexture(layer);
                if (loaded != nullptr)
                {
                    request.layers[layer] = loaded;
                }
                else if (layer > 0 && layer < request.layerCount)
                {
                    request.layers[layer] = terrainGpu->GetMissingLayerTexture();
                }
            }
        }
        gWorldLighting->DrawWorldTerrain(mesh, surface, request);
        return;
    }
    if (mesh.vertices == nullptr || mesh.indices == nullptr)
    {
        return;
    }
    for (int triangle = 0; triangle < mesh.triangleCount; ++triangle)
    {
        const int i0 = mesh.indices[triangle * 3];
        const int i1 = mesh.indices[triangle * 3 + 1];
        const int i2 = mesh.indices[triangle * 3 + 2];
        const Vector3 a{
            mesh.vertices[i0 * 3], mesh.vertices[i0 * 3 + 1], mesh.vertices[i0 * 3 + 2]};
        const Vector3 b{
            mesh.vertices[i1 * 3], mesh.vertices[i1 * 3 + 1], mesh.vertices[i1 * 3 + 2]};
        const Vector3 c{
            mesh.vertices[i2 * 3], mesh.vertices[i2 * 3 + 1], mesh.vertices[i2 * 3 + 2]};
        DrawTriangle3D(a, b, c, surface);
    }
}

bool PlayerModelHasRenderableMesh(const Model& model)
{
    if (model.meshCount <= 0 || model.meshes == nullptr)
    {
        return false;
    }
    for (int i = 0; i < model.meshCount; ++i)
    {
        if (model.meshes[i].vertexCount > 0)
        {
            return true;
        }
    }
    return false;
}

void LogPlayerModelLoadFailureOnce(bool& logged)
{
    if (logged)
    {
        return;
    }
    logged = true;
    std::fprintf(
        stderr,
        "PlayerPresentation: missing or invalid staged model: %s\n",
        gameplay::kPlayerModelLogicalId);
}

void DrawPlayerPresentationModel(
    const Model& model,
    const gameplay::PlayerVisualTransform& visual)
{
    rlDrawRenderBatchActive();
    rlPushMatrix();
    rlTranslatef(visual.position.x, visual.position.y, visual.position.z);
    rlRotatef(visual.yawDegrees, 0.0f, 1.0f, 0.0f);
    rlScalef(visual.scale.x, visual.scale.y, visual.scale.z);
    DrawModelPreservingMaterials(
        model, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE, gWorldModelOverride);
    rlPopMatrix();
    RestoreGreyboxImmediateState();
}

void DrawEditorViewportGrid(const DebugWorldOverlay& overlay)
{
    static_assert(192 == editor::kEditorViewportGridMaxLines);
    const int count = overlay.editorViewportGridLineCount;
    if (count <= 0)
    {
        return;
    }
    const int limited = count > 192 ? 192 : count;
    const float yOffset = editor::kEditorViewportGridRenderYOffset;
    for (int index = 0; index < limited; ++index)
    {
        const DebugWorldOverlay::EditorViewportGridDrawLine& line =
            overlay.editorViewportGridLines[index];
        Color color = kEditorGridMinor;
        const auto kind = static_cast<editor::EditorViewportGridLineKind>(line.kind);
        if (kind == editor::EditorViewportGridLineKind::Major)
        {
            color = kEditorGridMajor;
        }
        else if (kind == editor::EditorViewportGridLineKind::AxisX)
        {
            color = kEditorGridAxisX;
        }
        else if (kind == editor::EditorViewportGridLineKind::AxisZ)
        {
            color = kEditorGridAxisZ;
        }
        DrawLine3D(
            Vector3{line.start.x, line.start.y + yOffset, line.start.z},
            Vector3{line.end.x, line.end.y + yOffset, line.end.z},
            color);
    }
}

void DrawGhostBox(core::Vec3 center, core::Vec3 size, Color fill, Color wire)
{
    if (gWorldSolidMode == WorldSolidMode::Solid || gWorldSolidMode == WorldSolidMode::Wires)
    {
        return;
    }
    const Vector3 position = ToRaylib(center);
    DrawCube(position, size.x, size.y, size.z, fill);
    DrawCubeWires(position, size.x, size.y, size.z, wire);
}

void DrawItemPickupFallbackCube(
    const world::StaticPropSpec& visual,
    Color fill,
    Color wire,
    bool drawWire)
{
    if (gWorldSolidMode == WorldSolidMode::Solid && gWorldLighting != nullptr)
    {
        gWorldLighting->DrawSolidBoxEulerXYZ(visual.position, {
            world::kItemPickupVisualSize,
            world::kItemPickupVisualSize,
            world::kItemPickupVisualSize}, visual.rotationDegrees, fill);
        return;
    }
    if (gWorldSolidMode == WorldSolidMode::Wires)
    {
        if (!drawWire)
        {
            return;
        }
        rlPushMatrix();
        rlTranslatef(visual.position.x, visual.position.y, visual.position.z);
        rlRotatef(visual.rotationDegrees.z, 0.0f, 0.0f, 1.0f);
        rlRotatef(visual.rotationDegrees.y, 0.0f, 1.0f, 0.0f);
        rlRotatef(visual.rotationDegrees.x, 1.0f, 0.0f, 0.0f);
        DrawCubeWires(
            Vector3{0.0f, 0.0f, 0.0f},
            world::kItemPickupVisualSize,
            world::kItemPickupVisualSize,
            world::kItemPickupVisualSize,
            wire);
        rlPopMatrix();
        return;
    }
    rlPushMatrix();
    rlTranslatef(visual.position.x, visual.position.y, visual.position.z);
    rlRotatef(visual.rotationDegrees.z, 0.0f, 0.0f, 1.0f);
    rlRotatef(visual.rotationDegrees.y, 0.0f, 1.0f, 0.0f);
    rlRotatef(visual.rotationDegrees.x, 1.0f, 0.0f, 0.0f);
    DrawCube(
        Vector3{0.0f, 0.0f, 0.0f},
        world::kItemPickupVisualSize,
        world::kItemPickupVisualSize,
        world::kItemPickupVisualSize,
        fill);
    if (drawWire)
    {
        DrawCubeWires(
            Vector3{0.0f, 0.0f, 0.0f},
            world::kItemPickupVisualSize,
            world::kItemPickupVisualSize,
            world::kItemPickupVisualSize,
            wire);
    }
    rlPopMatrix();
}

void DrawTransformedBoundsWires(const core::Vec3 corners[8], Color wire)
{
    const int edges[12][2] = {
        {0, 1},
        {2, 3},
        {4, 5},
        {6, 7},
        {0, 2},
        {1, 3},
        {4, 6},
        {5, 7},
        {0, 4},
        {1, 5},
        {2, 6},
        {3, 7}};
    for (int index = 0; index < 12; ++index)
    {
        DrawLine3D(
            ToRaylib(corners[edges[index][0]]),
            ToRaylib(corners[edges[index][1]]),
            wire);
    }
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

void DrawGameplayObjectiveHud(const ObjectiveHudView& view)
{
    if (!view.visible)
    {
        return;
    }

    if (view.levelLabel != nullptr && view.levelLabel[0] != '\0')
    {
        DrawText(
            view.levelLabel,
            gameplay::kGameplayObjectiveHudMarginX,
            gameplay::kGameplayObjectiveHudLevelY,
            gameplay::kGameplayObjectiveHudLevelFontSize,
            kGrabHudText);
    }
    if (view.objectiveLine != nullptr && view.objectiveLine[0] != '\0')
    {
        DrawText(
            view.objectiveLine,
            gameplay::kGameplayObjectiveHudMarginX,
            gameplay::kGameplayObjectiveHudObjectiveY,
            gameplay::kGameplayObjectiveHudObjectiveFontSize,
            kGrabHudMuted);
    }
}

void DrawHealthHud(const HealthHudView& view)
{
    if (!view.visible || view.text == nullptr || view.text[0] == '\0')
    {
        return;
    }

    DrawText(
        view.text,
        gameplay::kHealthHudMarginX,
        gameplay::kHealthHudY,
        gameplay::kHealthHudFontSize,
        kGrabHudText);
}

void DrawDamageVignette(const DamageVignetteView& view)
{
    if (view.opacity <= 0.0f)
    {
        return;
    }

    const float clamped =
        view.opacity > gameplay::kDamageVignettePeakOpacity
            ? gameplay::kDamageVignettePeakOpacity
            : view.opacity;
    const unsigned char alpha = static_cast<unsigned char>(clamped * 255.0f + 0.5f);
    if (alpha == 0)
    {
        return;
    }

    const Color edge{
        gameplay::kDamageVignetteRed,
        gameplay::kDamageVignetteGreen,
        gameplay::kDamageVignetteBlue,
        alpha};
    const Color clear{
        gameplay::kDamageVignetteRed,
        gameplay::kDamageVignetteGreen,
        gameplay::kDamageVignetteBlue,
        0};
    const int width = GetScreenWidth();
    const int height = GetScreenHeight();
    const int edgeX = static_cast<int>(static_cast<float>(width) * gameplay::kDamageVignetteEdgeFraction);
    const int edgeY = static_cast<int>(static_cast<float>(height) * gameplay::kDamageVignetteEdgeFraction);
    if (edgeX <= 0 || edgeY <= 0)
    {
        return;
    }

    DrawRectangleGradientV(0, 0, width, edgeY, edge, clear);
    DrawRectangleGradientV(0, height - edgeY, width, edgeY, clear, edge);
    DrawRectangleGradientH(0, 0, edgeX, height, edge, clear);
    DrawRectangleGradientH(width - edgeX, 0, edgeX, height, clear, edge);
}

void DrawDeathHud(const DeathHudView& view)
{
    if (!view.visible)
    {
        return;
    }

    const unsigned char overlayAlpha = static_cast<unsigned char>(
        gameplay::kPlayerDeathOverlayOpacity * 255.0f + 0.5f);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, overlayAlpha});

    const char* title =
        view.title != nullptr && view.title[0] != '\0' ? view.title : gameplay::kPlayerDeathTitle;
    const int titleWidth = MeasureText(title, kLevelCompleteFontSize);
    const int titleX = (GetScreenWidth() - titleWidth) / 2;
    const int titleY = GetScreenHeight() / 10;
    DrawText(title, titleX, titleY, kLevelCompleteFontSize, kLevelCompleteText);
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

void DrawItemPickupCollectionHud(const gameplay::ItemPickupCollectionHudState& state)
{
    const int active = gameplay::ActiveItemPickupCollectionHudCount(state);
    if (active <= 0)
    {
        return;
    }

    const int font = kTimerHudFontSize;
    const int lineHeight = font + 8;
    const int promptY = GetScreenHeight() - font - kTimerHudMargin;
    const int newestY = promptY - lineHeight - 8;
    char text[gameplay::kItemPickupCollectionHudTextCapacity]{};
    for (int index = 0; index < active; ++index)
    {
        const gameplay::ItemPickupCollectionHudEntry& entry = state.entries[index];
        gameplay::FormatItemPickupCollectionHudText(
            text, sizeof(text), entry.itemId, entry.quantity);
        const unsigned char alpha = gameplay::ItemPickupCollectionHudAlpha(entry);
        const int width = MeasureText(text, font);
        const int x = (GetScreenWidth() - width) / 2;
        const int y = newestY - (active - 1 - index) * lineHeight;
        const unsigned char backgroundAlpha =
            static_cast<unsigned char>((150 * static_cast<int>(alpha)) / 255);
        DrawRectangle(
            x - 8,
            y - 4,
            width + 16,
            font + 8,
            Color{18, 24, 32, backgroundAlpha});
        DrawText(
            text,
            x,
            y,
            font,
            Color{kGrabHudText.r, kGrabHudText.g, kGrabHudText.b, alpha});
    }
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
    if (gWorldSolidMode == WorldSolidMode::Solid && gWorldLighting != nullptr)
    {
        gWorldLighting->DrawSolidBoxRotatedZ(
            slope.center, slope.size, slope.rotationZDegrees, fill);
        return;
    }
    rlPushMatrix();
    rlTranslatef(slope.center.x, slope.center.y, slope.center.z);
    rlRotatef(slope.rotationZDegrees, 0.0f, 0.0f, 1.0f);
    if (gWorldSolidMode == WorldSolidMode::Wires)
    {
        DrawCubeWires(
            Vector3{0.0f, 0.0f, 0.0f}, slope.size.x, slope.size.y, slope.size.z, kWireColor);
        rlPopMatrix();
        return;
    }
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
    if (gWorldSolidMode == WorldSolidMode::Solid && gWorldLighting != nullptr)
    {
        rlPopMatrix();
        gWorldLighting->DrawSolidBoxAxisAngle(
            box.center, box.size, axisX, axisY, axisZ, degrees, fill);
        return;
    }
    if (gWorldSolidMode == WorldSolidMode::Wires)
    {
        DrawCubeWires(Vector3{0.0f, 0.0f, 0.0f}, box.size.x, box.size.y, box.size.z, wire);
        rlPopMatrix();
        return;
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

void DrawRotateGizmo(const DebugWorldOverlay& overlay)
{
    if (!overlay.drawRotateGizmo || !(overlay.gizmoAxisLength > 0.0f))
    {
        return;
    }

    const float length = overlay.gizmoAxisLength;
    const float visualRadius = std::max(length * 0.038f, 0.045f);
    const core::Vec3 origin = overlay.gizmoOrigin;
    const Vector3 originRl = ToRaylib(origin);
    constexpr int kSegments = 48;

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

    const auto ringPoint = [](int axisId, float radius, float angle) -> Vector3 {
        const float cosine = std::cos(angle);
        const float sine = std::sin(angle);
        if (axisId == 1)
        {
            return {0.0f, cosine * radius, sine * radius};
        }
        if (axisId == 2)
        {
            return {sine * radius, 0.0f, cosine * radius};
        }
        return {cosine * radius, sine * radius, 0.0f};
    };

    const auto drawPass = [&](unsigned char alpha, float radiusScale)
    {
        DrawSphere(originRl, visualRadius * 0.55f * radiusScale, ScaleGizmoColor({220, 220, 228, 255}, alpha));
        const float step = (2.0f * static_cast<float>(PI)) / static_cast<float>(kSegments);
        for (const AxisDraw& axis : axes)
        {
            const bool active = overlay.gizmoActiveAxis == axis.id;
            const bool hovered = overlay.gizmoHoveredAxis == axis.id;
            Color color = axis.color;
            float tube = visualRadius * 0.42f * radiusScale;
            if (active)
            {
                color = kGizmoAxisActive;
                tube *= 1.55f;
            }
            else if (hovered)
            {
                color = {
                    static_cast<unsigned char>(std::min(255, axis.color.r + 48)),
                    static_cast<unsigned char>(std::min(255, axis.color.g + 48)),
                    static_cast<unsigned char>(std::min(255, axis.color.b + 48)),
                    255};
                tube *= 1.28f;
            }

            for (int segment = 0; segment < kSegments; ++segment)
            {
                const Vector3 localA = ringPoint(axis.id, length, step * static_cast<float>(segment));
                const Vector3 localB =
                    ringPoint(axis.id, length, step * static_cast<float>(segment + 1));
                const Vector3 a{
                    originRl.x + localA.x, originRl.y + localA.y, originRl.z + localA.z};
                const Vector3 b{
                    originRl.x + localB.x, originRl.y + localB.y, originRl.z + localB.z};
                DrawCylinderEx(a, b, tube, tube, 6, ScaleGizmoColor(color, alpha));
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

void DrawDirectionalLightAuthoring(const DebugWorldOverlay& overlay)
{
    if (!overlay.drawDirectionalLightAuthoring)
    {
        return;
    }

    const Vector3 origin = ToRaylib(overlay.directionalLightAnchor);
    const core::Vec3 ray = overlay.directionalLightRay;
    float scale = overlay.directionalLightScale;
    if (!std::isfinite(scale) || scale < 0.25f)
    {
        scale = 1.0f;
    }
    if (scale > 8.0f)
    {
        scale = 8.0f;
    }
    const float rayLength = 3.4f * scale;
    const Vector3 tip{
        origin.x + ray.x * rayLength,
        origin.y + ray.y * rayLength,
        origin.z + ray.z * rayLength};
    const Color sunFill = overlay.directionalLightSelected
        ? Color{255, 214, 92, 255}
        : Color{255, 186, 64, 220};
    const Color sunWire = overlay.directionalLightSelected
        ? Color{255, 240, 170, 255}
        : Color{255, 210, 110, 255};
    const Color rayColor = overlay.directionalLightSelected
        ? Color{255, 230, 140, 255}
        : Color{255, 196, 96, 255};
    const float sunRadius = (overlay.directionalLightSelected ? 0.28f : 0.24f) * scale;
    const float tipRadius = 0.09f * scale;
    DrawSphere(origin, sunRadius, sunFill);
    DrawSphereWires(origin, sunRadius, 8, 8, sunWire);
    DrawLine3D(origin, tip, rayColor);
    DrawSphere(tip, tipRadius, rayColor);
}

void DrawLocalLightAuthoring(const DebugWorldOverlay& overlay)
{
    if (!overlay.drawLocalLightAuthoring)
    {
        return;
    }

    auto orthonormal = [](core::Vec3 axis, core::Vec3& right, core::Vec3& up) {
        core::Vec3 unit = axis;
        const float lengthSq = unit.x * unit.x + unit.y * unit.y + unit.z * unit.z;
        if (!(lengthSq > 1.0e-10f))
        {
            unit = {0.0f, -1.0f, 0.0f};
        }
        else
        {
            const float inv = 1.0f / std::sqrt(lengthSq);
            unit = {unit.x * inv, unit.y * inv, unit.z * inv};
        }
        core::Vec3 helper = (std::fabs(unit.y) > 0.9f) ? core::Vec3{1.0f, 0.0f, 0.0f}
                                                       : core::Vec3{0.0f, 1.0f, 0.0f};
        right = {
            helper.y * unit.z - helper.z * unit.y,
            helper.z * unit.x - helper.x * unit.z,
            helper.x * unit.y - helper.y * unit.x};
        const float rightSq = right.x * right.x + right.y * right.y + right.z * right.z;
        const float invRight = 1.0f / std::sqrt(rightSq > 1.0e-10f ? rightSq : 1.0f);
        right = {right.x * invRight, right.y * invRight, right.z * invRight};
        up = {
            unit.y * right.z - unit.z * right.y,
            unit.z * right.x - unit.x * right.z,
            unit.x * right.y - unit.y * right.x};
        return unit;
    };

    constexpr int kConeSegments = 12;
    for (std::size_t index = 0; index < overlay.previewPointLights.size(); ++index)
    {
        const world::PointLightSpec& light = overlay.previewPointLights[index];
        const bool selected = index == overlay.selectedPointLightIndex;
        const Color fill = selected ? Color{255, 210, 96, 255} : Color{255, 176, 64, 220};
        const Color wire = selected ? Color{255, 236, 150, 255} : Color{255, 196, 110, 255};
        const Vector3 origin = ToRaylib(light.position);
        DrawSphere(origin, selected ? 0.22f : 0.18f, fill);
        DrawSphereWires(origin, selected ? 0.22f : 0.18f, 8, 8, wire);
        const float range = world::ClampLocalLightRange(light.range);
        DrawCircle3D(origin, range, Vector3{1.0f, 0.0f, 0.0f}, 90.0f, wire);
        DrawCircle3D(origin, range, Vector3{0.0f, 1.0f, 0.0f}, 0.0f, wire);
        DrawCircle3D(origin, range, Vector3{0.0f, 0.0f, 1.0f}, 90.0f, wire);
    }

    for (std::size_t index = 0; index < overlay.previewSpotLights.size(); ++index)
    {
        const world::SpotLightSpec& light = overlay.previewSpotLights[index];
        const bool selected = index == overlay.selectedSpotLightIndex;
        const Color fill = selected ? Color{120, 196, 255, 255} : Color{80, 160, 230, 220};
        const Color wire = selected ? Color{180, 220, 255, 255} : Color{120, 186, 240, 255};
        const Vector3 origin = ToRaylib(light.position);
        DrawSphere(origin, selected ? 0.22f : 0.18f, fill);
        DrawSphereWires(origin, selected ? 0.22f : 0.18f, 8, 8, wire);
        core::Vec3 right{};
        core::Vec3 up{};
        const core::Vec3 axis = orthonormal(light.direction, right, up);
        const float range = world::ClampLocalLightRange(light.range);
        const Vector3 tip{
            origin.x + axis.x * range,
            origin.y + axis.y * range,
            origin.z + axis.z * range};
        DrawLine3D(origin, tip, wire);
        const float outerRadians = light.outerConeDegrees * (3.14159265f / 180.0f);
        const float innerRadians = light.innerConeDegrees * (3.14159265f / 180.0f);
        const float outerRadius = range * std::tan(outerRadians);
        const float innerRadius = range * std::tan(innerRadians);
        Vector3 prevOuter{};
        Vector3 prevInner{};
        for (int segment = 0; segment <= kConeSegments; ++segment)
        {
            const float angle = (static_cast<float>(segment % kConeSegments) / static_cast<float>(kConeSegments))
                * 6.2831853f;
            const float cosine = std::cos(angle);
            const float sine = std::sin(angle);
            const Vector3 outer{
                tip.x + right.x * cosine * outerRadius + up.x * sine * outerRadius,
                tip.y + right.y * cosine * outerRadius + up.y * sine * outerRadius,
                tip.z + right.z * cosine * outerRadius + up.z * sine * outerRadius};
            const Vector3 inner{
                tip.x + right.x * cosine * innerRadius + up.x * sine * innerRadius,
                tip.y + right.y * cosine * innerRadius + up.y * sine * innerRadius,
                tip.z + right.z * cosine * innerRadius + up.z * sine * innerRadius};
            if (segment == 0 || segment == kConeSegments / 4 || segment == kConeSegments / 2
                || segment == (3 * kConeSegments) / 4)
            {
                DrawLine3D(origin, outer, wire);
            }
            if (segment > 0)
            {
                DrawLine3D(prevOuter, outer, wire);
                DrawLine3D(prevInner, inner, Color{wire.r, wire.g, wire.b, 140});
            }
            prevOuter = outer;
            prevInner = inner;
        }
    }
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
    for (const DebugWorldOverlay::SecondaryHighlightOverlay& secondary : overlay.secondaryHighlights)
    {
        if (!(secondary.size.x > 0.0f) || !(secondary.size.y > 0.0f)
            || !(secondary.size.z > 0.0f))
        {
            continue;
        }
        if (secondary.rotationZDegrees != 0.0f)
        {
            DrawOrientedWires(
                secondary.center,
                secondary.size,
                secondary.rotationZDegrees,
                kSecondarySelectionHighlightColor);
        }
        else
        {
            DrawCubeWires(
                ToRaylib(secondary.center),
                secondary.size.x,
                secondary.size.y,
                secondary.size.z,
                kSecondarySelectionHighlightColor);
        }
    }
    if (overlay.drawSelectedModelBounds)
    {
        DrawTransformedBoundsWires(overlay.selectedModelBoundsCorners, kSelectedModelBoundsWire);
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
    for (const world::LevelGoalSpec& goal : overlay.pendingDeleteLevelGoals)
    {
        DrawPendingDeleteSolid(goal.center, goal.size, kGoalVolumeIncomplete);
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
            const bool skipModelBackedBounds =
                item.selected && overlay.drawSelectedModelGhost
                && (item.kind == 5 || item.kind == 8);
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
            if (!skipModelBackedBounds && item.boundsSize.x > 0.0f && item.boundsSize.y > 0.0f
                && item.boundsSize.z > 0.0f)
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
    DrawDirectionalLightAuthoring(overlay);
    DrawLocalLightAuthoring(overlay);
    if (overlay.drawTerrainSculptBrush && overlay.terrainSculptBrushRadius > 0.0f)
    {
        const Vector3 center = ToRaylib(overlay.terrainSculptBrushCenter);
        const Vector3 lifted{
            center.x, center.y + 0.03f, center.z};
        DrawCircle3D(lifted, overlay.terrainSculptBrushRadius, {1.0f, 0.0f, 0.0f}, 90.0f,
            kTerrainSculptBrushColor);
        DrawCircle3D(
            lifted,
            overlay.terrainSculptBrushRadius * 0.15f,
            {1.0f, 0.0f, 0.0f},
            90.0f,
            kTerrainSculptBrushColor);
        DrawLine3D(
            lifted,
            Vector3{lifted.x, lifted.y + 0.35f, lifted.z},
            kTerrainSculptBrushColor);
        rlDrawRenderBatchActive();
        rlDisableDepthTest();
        DrawCircle3D(lifted, overlay.terrainSculptBrushRadius, {1.0f, 0.0f, 0.0f}, 90.0f,
            kTerrainSculptBrushMuted);
        rlDrawRenderBatchActive();
        rlEnableDepthTest();
    }
    DrawTranslationGizmo(overlay);
    DrawResizeGizmo(overlay);
    DrawRotateGizmo(overlay);
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

void DrawLevelCompleteMessage(bool destinationContinueHint)
{
    const char* completeText = "LEVEL COMPLETE";
    const int completeWidth = MeasureText(completeText, kLevelCompleteFontSize);
    const int completeX = (GetScreenWidth() - completeWidth) / 2;
    const int completeY = GetScreenHeight() / 10;
    DrawText(completeText, completeX, completeY, kLevelCompleteFontSize, kLevelCompleteText);

    const char* hintText = destinationContinueHint
        ? gameplay::kDestinationCompleteHint
        : gameplay::kFailedDestinationRestartHint;
    const int hintWidth = MeasureText(hintText, kRestartHintFontSize);
    const int hintX = (GetScreenWidth() - hintWidth) / 2;
    const int hintY = completeY + kLevelCompleteFontSize + kRestartHintGap;
    DrawText(hintText, hintX, hintY, kRestartHintFontSize, kLevelCompleteText);
}

void DrawRunCompleteMessage(double capturedFinalSeconds)
{
    const int titleWidth = MeasureText(gameplay::kRunCompleteTitle, kLevelCompleteFontSize);
    const int titleX = (GetScreenWidth() - titleWidth) / 2;
    const int titleY = GetScreenHeight() / 10;
    DrawText(
        gameplay::kRunCompleteTitle, titleX, titleY, kLevelCompleteFontSize, kLevelCompleteText);

    char formatted[32]{};
    core::FormatRunTime(formatted, sizeof(formatted), capturedFinalSeconds);
    const char* timeText = TextFormat("TIME  %s", formatted);
    const int timeWidth = MeasureText(timeText, kTimerHudFontSize);
    const int timeX = (GetScreenWidth() - timeWidth) / 2;
    const int timeY = titleY + kLevelCompleteFontSize + kRestartHintGap;
    DrawText(timeText, timeX, timeY, kTimerHudFontSize, kTimerHudText);

    const int playAgainWidth = MeasureText(gameplay::kRunCompletePlayAgainHint, kRestartHintFontSize);
    const int playAgainX = (GetScreenWidth() - playAgainWidth) / 2;
    const int playAgainY = timeY + kTimerHudFontSize + kRestartHintGap;
    DrawText(
        gameplay::kRunCompletePlayAgainHint,
        playAgainX,
        playAgainY,
        kRestartHintFontSize,
        kLevelCompleteText);

    const int restartWidth = MeasureText(gameplay::kRunCompleteRestartHint, kRestartHintFontSize);
    const int restartX = (GetScreenWidth() - restartWidth) / 2;
    const int restartY = playAgainY + kRestartHintFontSize + kBestHudGap;
    DrawText(
        gameplay::kRunCompleteRestartHint,
        restartX,
        restartY,
        kRestartHintFontSize,
        kLevelCompleteText);

    const int menuWidth = MeasureText(gameplay::kRunCompleteMainMenuHint, kRestartHintFontSize);
    const int menuX = (GetScreenWidth() - menuWidth) / 2;
    const int menuY = restartY + kRestartHintFontSize + kBestHudGap;
    DrawText(
        gameplay::kRunCompleteMainMenuHint,
        menuX,
        menuY,
        kRestartHintFontSize,
        kLevelCompleteText);
}

void DrawMainMenuOverlay(bool playSelected)
{
    const int titleWidth = MeasureText(gameplay::kMainMenuTitle, kLevelCompleteFontSize);
    const int titleX = (GetScreenWidth() - titleWidth) / 2;
    const int titleY = GetScreenHeight() / 5;
    DrawText(
        gameplay::kMainMenuTitle, titleX, titleY, kLevelCompleteFontSize, kLevelCompleteText);

    const char* playText = playSelected ? "> PLAY" : "  PLAY";
    const char* quitText = playSelected ? "  QUIT" : "> QUIT";
    const Color playColor = playSelected ? kLevelCompleteText : kGrabHudMuted;
    const Color quitColor = playSelected ? kGrabHudMuted : kLevelCompleteText;

    const int playWidth = MeasureText(playText, kRestartHintFontSize);
    const int playX = (GetScreenWidth() - playWidth) / 2;
    const int playY = titleY + kLevelCompleteFontSize + kRestartHintGap * 2;
    DrawText(playText, playX, playY, kRestartHintFontSize, playColor);

    const int quitWidth = MeasureText(quitText, kRestartHintFontSize);
    const int quitX = (GetScreenWidth() - quitWidth) / 2;
    const int quitY = playY + kRestartHintFontSize + kRestartHintGap;
    DrawText(quitText, quitX, quitY, kRestartHintFontSize, quitColor);
}

void DrawPauseMenuOverlay(bool resumeSelected)
{
    const int titleWidth = MeasureText(gameplay::kPauseMenuTitle, kLevelCompleteFontSize);
    const int titleX = (GetScreenWidth() - titleWidth) / 2;
    const int titleY = GetScreenHeight() / 5;
    DrawText(
        gameplay::kPauseMenuTitle, titleX, titleY, kLevelCompleteFontSize, kLevelCompleteText);

    const char* resumeText = resumeSelected ? "> RESUME" : "  RESUME";
    const char* menuText = resumeSelected ? "  MAIN MENU" : "> MAIN MENU";
    const Color resumeColor = resumeSelected ? kLevelCompleteText : kGrabHudMuted;
    const Color menuColor = resumeSelected ? kGrabHudMuted : kLevelCompleteText;

    const int resumeWidth = MeasureText(resumeText, kRestartHintFontSize);
    const int resumeX = (GetScreenWidth() - resumeWidth) / 2;
    const int resumeY = titleY + kLevelCompleteFontSize + kRestartHintGap * 2;
    DrawText(resumeText, resumeX, resumeY, kRestartHintFontSize, resumeColor);

    const int menuWidth = MeasureText(menuText, kRestartHintFontSize);
    const int menuX = (GetScreenWidth() - menuWidth) / 2;
    const int menuY = resumeY + kRestartHintFontSize + kRestartHintGap;
    DrawText(menuText, menuX, menuY, kRestartHintFontSize, menuColor);
}

}

struct Renderer::PlayerModelGpuState
{
    Model model{};
    gameplay::PlayerPresentationModelLifetime lifetime{};
    bool loaded = false;
    bool failed = false;
    bool missingLogged = false;
};

Renderer::Renderer()
    : staticPropModels(std::make_unique<StaticModelSceneStore>())
    , playerModelGpu(std::make_unique<PlayerModelGpuState>())
    , worldLighting(std::make_unique<WorldLightingResources>())
    , terrainGpu(std::make_unique<TerrainGpuResources>())
{
}

Renderer::~Renderer()
{
    UnloadRuntimeAssets();
}

void Renderer::SyncStaticPropModels(
    const world::LevelDefinition& level,
    std::string_view extraIdentity,
    const world::LevelDefinition* extraLevel)
{
    if (staticPropModels)
    {
        staticPropModels->Sync(level, extraIdentity, extraLevel);
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

void Renderer::SetTerrainAuthoringCookedRoot(const std::filesystem::path& cookedRoot)
{
    if (terrainGpu)
    {
        terrainGpu->SetAuthoringCookedRoot(cookedRoot);
    }
}

void Renderer::SetTerrainAuthoringSourceRoot(const std::filesystem::path& sourceRoot)
{
    if (terrainGpu)
    {
        terrainGpu->SetAuthoringSourceRoot(sourceRoot);
    }
}

void Renderer::LoadRuntimeAssets()
{
    if (worldLighting == nullptr)
    {
        worldLighting = std::make_unique<WorldLightingResources>();
    }
    worldLighting->Load();

    if (playerModelGpu == nullptr)
    {
        playerModelGpu = std::make_unique<PlayerModelGpuState>();
    }
    if (!gameplay::PlayerPresentationShouldAttemptModelLoad(playerModelGpu->lifetime))
    {
        return;
    }
    gameplay::NotePlayerPresentationLoadAttempt(playerModelGpu->lifetime);

    const std::filesystem::path path = platform::RuntimeAssetPath(gameplay::kPlayerModelLogicalId);
    if (path.empty() || !path.is_absolute())
    {
        playerModelGpu->failed = true;
        LogPlayerModelLoadFailureOnce(playerModelGpu->missingLogged);
        return;
    }
    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error))
    {
        playerModelGpu->failed = true;
        LogPlayerModelLoadFailureOnce(playerModelGpu->missingLogged);
        return;
    }
    const assets::StaticGlbValidation validation = assets::ValidateStaticGlbFile(path);
    if (validation.status != assets::StaticGlbStatus::Ok)
    {
        playerModelGpu->failed = true;
        LogPlayerModelLoadFailureOnce(playerModelGpu->missingLogged);
        return;
    }
    Model model = LoadModel(path.string().c_str());
    if (!PlayerModelHasRenderableMesh(model))
    {
        UnloadModel(model);
        playerModelGpu->failed = true;
        LogPlayerModelLoadFailureOnce(playerModelGpu->missingLogged);
        return;
    }
    PrepareLoadedModelMaterials(model);
    playerModelGpu->model = model;
    playerModelGpu->loaded = true;
    playerModelGpu->failed = false;
}

void Renderer::UnloadRuntimeAssets()
{
    if (playerModelGpu != nullptr)
    {
        if (playerModelGpu->loaded)
        {
            UnloadModel(playerModelGpu->model);
            playerModelGpu->model = {};
            playerModelGpu->loaded = false;
        }
        playerModelGpu->failed = false;
        playerModelGpu->missingLogged = false;
        gameplay::ClearPlayerPresentationModelLifetime(playerModelGpu->lifetime);
    }
    if (staticPropModels)
    {
        staticPropModels->Shutdown();
    }
    if (worldLighting)
    {
        worldLighting->Unload();
    }
    if (terrainGpu)
    {
        terrainGpu->Unload();
    }
}

bool Renderer::IsPlayerModelLoaded() const
{
    return playerModelGpu != nullptr && playerModelGpu->loaded;
}

std::size_t Renderer::PlayerModelLoadCount() const
{
    if (playerModelGpu == nullptr)
    {
        return 0;
    }
    return playerModelGpu->lifetime.loadCount;
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
        const gameplay::PlayerPresentationState& playerPresentation,
        const CameraView& cameraView,
    const world::LevelDefinition& level,
    const std::vector<DynamicBoxDrawState>& dynamicBoxes,
    const std::vector<PressurePlateDrawState>& pressurePlates,
    const std::vector<DoorDrawState>& doors,
    core::Vec3 movingPlatformPosition,
        core::Vec3 movingPlatformSize,
        const std::vector<world::CheckpointVisualState>& checkpointVisuals,
        bool levelCompleted,
        bool destinationContinueHint,
        const std::vector<std::uint8_t>& collectibleCollected,
        int collectedCount,
        const std::vector<std::uint8_t>& itemPickupCollected,
        int itemPickupTargetIndex,
        const gameplay::ItemPickupCollectionFeedbackState& itemPickupCollectionFeedback,
        const gameplay::ItemPickupCollectionHudState& itemPickupCollectionHud,
        int lockedDoorTargetIndex,
        const char* lockedDoorPrompt,
        double elapsedSeconds,
        bool hasBestTime,
        double bestSeconds,
        bool runComplete,
        double runCompleteFinalSeconds,
        InventoryPanelView inventoryPanel,
        ObjectiveHudView objectiveHud,
        HealthHudView healthHud,
        const DebugWorldOverlay& overlay,
        WorldViewRect viewRect,
        bool drawGameplayHud,
        bool hideInteractionPrompts,
        DamageVignetteView damageVignette,
        DeathHudView deathHud)
{
    const Camera3D view = MakeCamera(cameraView);
    const bool subViewport = viewRect.width > 0 && viewRect.height > 0;

    // BeginMode3D reads rlGetCullDistanceNear/Far. Model Preview / thumbnails
    // set those to a tight model frame and must not leak into Gameplay.
    rlSetClipPlanes(RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);

    bool grabHudCarrying = false;
    bool grabHudTarget = false;
    bool doorHudTarget = false;
    bool pickupHudTarget = false;
    char pickupHudText[64]{};

    if (terrainGpu)
    {
        const world::TerrainSpec* spec = nullptr;
        if (overlay.usePreviewTerrain)
        {
            spec = overlay.previewHasTerrain ? &overlay.previewTerrain : nullptr;
        }
        else if (level.hasTerrain && level.terrain.enabled)
        {
            spec = &level.terrain;
        }
        terrainGpu->Sync(spec);
    }

    auto drawWorldGeometry = [&]() {
        DrawGreyboxBox(level.ground.center, level.ground.size, kGroundColor);
        DrawAuthoredTerrain(terrainGpu.get(), kTerrainColor);

        const Color platformColors[] = {kPlatformColor, kPlatformAccentColor};
        int platformIndex = 0;
        for (const world::Box& platform : level.elevatedPlatforms)
        {
            if (!OverlayMarksPendingDelete(
                    overlay.pendingDeletePlatformIndices, static_cast<std::size_t>(platformIndex)))
            {
                DrawGreyboxBox(
                    platform.center, platform.size, platformColors[platformIndex % 2]);
            }
            ++platformIndex;
        }

        DrawGreyboxBox(movingPlatformPosition, movingPlatformSize, kMovingPlatformColor);
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
            const PressurePlateDrawState& plate = pressurePlates[index];
            if (!plate.visibleInGameplay && !plate.revealInEditor)
            {
                continue;
            }
            if (!plate.visibleInGameplay && plate.revealInEditor)
            {
                DrawGhostBox(
                    plate.center,
                    plate.size,
                    kPressurePlateHiddenEditorFill,
                    kPressurePlateHiddenEditorWire);
                continue;
            }
            DrawGreyboxBox(
                plate.center,
                plate.size,
                plate.active ? kPressurePlateActive : kPressurePlateInactive);
        }
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
                if (gWorldSolidMode == WorldSolidMode::Combined)
                {
                    DrawGhostBox(
                        doors[index].center, doors[index].size, kDoorTargetFill, kDoorTargetWire);
                }
                else
                {
                    DrawGreyboxBox(doors[index].center, doors[index].size, kDoorTargetFill);
                }
            }
            else
            {
                DrawGreyboxBox(doors[index].center, doors[index].size, kDoorColor);
            }
        }
        if (gWorldSolidMode != WorldSolidMode::Wires)
        {
            for (std::size_t index = 0; index < level.staticProps.size(); ++index)
            {
                if (OverlayMarksPendingDelete(overlay.pendingDeleteStaticPropIndices, index))
                {
                    continue;
                }
                if (staticPropModels)
                {
                    if (gWorldModelOverride != nullptr)
                    {
                        staticPropModels->DrawProp(level.staticProps[index], *gWorldModelOverride);
                    }
                    else
                    {
                        staticPropModels->DrawProp(level.staticProps[index]);
                    }
                }
            }
        }
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
            core::Vec3 loadedMin{};
            core::Vec3 loadedMax{};
            bool haveLoadedBounds = false;
            if (!pickup.modelIdentity.empty() && staticPropModels != nullptr)
            {
                haveLoadedBounds = staticPropModels->TryGetLoadedLocalBounds(
                    pickup.modelIdentity, loadedMin, loadedMax);
            }
            const ItemPickupTargetPresentation presentation = MakeItemPickupTargetPresentation(
                pickup,
                targeted,
                false,
                haveLoadedBounds,
                loadedMin,
                loadedMax,
                elapsedSeconds);
            if (presentation.drawHud)
            {
                pickupHudTarget = true;
                std::snprintf(
                    pickupHudText,
                    sizeof(pickupHudText),
                    "E Pick Up %s x%d",
                    pickup.itemId.c_str(),
                    pickup.quantity);
            }
            Color fill = kItemPickupFill;
            if (presentation.drawFallbackHighlight)
            {
                const Color goldPush = MixRgb(
                    kItemPickupFill, kItemPickupTargetGold, presentation.highlightGoldAmount);
                fill = MixRgb(kItemPickupFill, goldPush, presentation.highlightIntensity);
            }
            const Color wire = targeted ? kItemPickupTargetWire : kWireColor;
            const bool overlayPass = gWorldSolidMode == WorldSolidMode::Combined;
            if (!pickup.modelIdentity.empty() && staticPropModels)
            {
                if (gWorldSolidMode != WorldSolidMode::Wires)
                {
                    const world::StaticPropSpec visual =
                        world::ItemPickupPresentedVisualProp(pickup, elapsedSeconds);
                    if (gWorldModelOverride != nullptr)
                    {
                        staticPropModels->DrawProp(visual, *gWorldModelOverride);
                    }
                    else
                    {
                        staticPropModels->DrawProp(visual);
                    }
                }
                if (overlayPass && presentation.drawModelHighlight)
                {
                    staticPropModels->DrawGameplayTargetHighlight(
                        presentation.visual,
                        presentation.highlightRed,
                        presentation.highlightGreen,
                        presentation.highlightBlue,
                        presentation.highlightAlpha);
                }
                if (overlayPass && presentation.drawInteractionBounds)
                {
                    DrawTransformedBoundsWires(
                        presentation.boundsCorners, kSelectedModelBoundsWire);
                }
                continue;
            }
            DrawItemPickupFallbackCube(
                presentation.visual,
                fill,
                wire,
                !targeted || presentation.drawInteractionBounds);
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
        for (std::size_t collectibleIndex = 0; collectibleIndex < collectibleCount;
             ++collectibleIndex)
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
        }
        const bool playerModelLoaded = IsPlayerModelLoaded();
        if (gWorldSolidMode != WorldSolidMode::Wires
            && gameplay::ShouldDrawPlayerPresentationModel(playerModelLoaded)
            && playerModelGpu != nullptr)
        {
            const gameplay::PlayerVisualTransform visual = gameplay::BuildPlayerVisualTransform(
                player.Position(),
                gameplay::kDefaultPlayerPresentationConfig,
                playerPresentation.facingYawDegrees);
            DrawPlayerPresentationModel(playerModelGpu->model, visual);
        }
        else if (gameplay::ShouldDrawPlayerGameplayPrimitive(playerModelLoaded))
        {
            DrawGreyboxBox(player.Position(), player.Size(), kPlayerColor);
        }
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
        for (std::size_t goalIndex = 0; goalIndex < level.levelGoals.size(); ++goalIndex)
        {
            if (OverlayMarksPendingDelete(overlay.pendingDeleteLevelGoalIndices, goalIndex))
            {
                continue;
            }
            const LevelGoalViewKind goalView =
                LevelGoalViewKindFromEditor(overlay.drawLevelGoalAuthoredVolume);
            if (gWorldSolidMode == WorldSolidMode::Solid)
            {
                const LevelGoalMarkerLayout layout =
                    MakeLevelGoalMarkerLayout(level.levelGoals[goalIndex]);
                const LevelGoalMarkerColors colors = MakeLevelGoalMarkerColors(levelCompleted);
                const Color postColor{colors.postR, colors.postG, colors.postB, colors.postA};
                const Color barColor{colors.barR, colors.barG, colors.barB, colors.barA};
                DrawGreyboxBox(layout.leftPost, layout.postSize, postColor);
                DrawGreyboxBox(layout.rightPost, layout.postSize, postColor);
                DrawGreyboxBox(layout.barCenter, layout.barSize, barColor);
            }
            else if (gWorldSolidMode == WorldSolidMode::Wires)
            {
                DrawLevelGoalPresentation(
                    level.levelGoals[goalIndex],
                    levelCompleted,
                    goalView,
                    LevelGoalDrawLayer::Wires);
            }
            else
            {
                DrawLevelGoalPresentation(level.levelGoals[goalIndex], levelCompleted, goalView);
            }
        }
    };

    LightingEnvironment lightingEnv = MakeLightingEnvironmentFromAuthored(
        overlay.usePreviewLighting ? overlay.previewLighting : level.environment);
    if (overlay.usePreviewLighting)
    {
        ApplyAuthoredLocalLights(
            lightingEnv, overlay.previewPointLights, overlay.previewSpotLights);
    }
    else
    {
        std::vector<std::uint8_t> plateActive(level.pressurePlates.size(), 0);
        const std::size_t plateCount = level.pressurePlates.size() < pressurePlates.size()
            ? level.pressurePlates.size()
            : pressurePlates.size();
        for (std::size_t index = 0; index < plateCount; ++index)
        {
            plateActive[index] = pressurePlates[index].active ? 1 : 0;
        }
        const world::DirectionalLightActivation activation =
            world::ResolveDirectionalLightActivation(
                level.pressurePlates,
                plateActive.empty() ? nullptr : plateActive.data(),
                plateActive.size());
        ApplyDirectionalLightActivation(
            lightingEnv,
            activation.hasLinkedPressurePlates,
            activation.anyLinkedPressurePlateActive);
        std::vector<std::uint8_t> pointEffective;
        std::vector<std::uint8_t> spotEffective;
        world::FillEffectiveLocalLightEnabled(
            level.pointLights,
            level.spotLights,
            level.pressurePlates,
            plateActive.empty() ? nullptr : plateActive.data(),
            plateActive.size(),
            pointEffective,
            spotEffective);
        ApplyEffectiveLocalLights(
            lightingEnv,
            level.pointLights,
            level.spotLights,
            pointEffective.empty() ? nullptr : pointEffective.data(),
            pointEffective.size(),
            spotEffective.empty() ? nullptr : spotEffective.data(),
            spotEffective.size());
    }
    const bool lightingReady = worldLighting != nullptr && worldLighting->IsReady();
    const bool shadowsActive = lightingReady && DirectionalShadowsAreActive(lightingEnv);
    if (shadowsActive)
    {
        gWorldLighting = worldLighting.get();
        gWorldSolidMode = WorldSolidMode::Solid;
        const ModelDrawOverride shadowOverride = worldLighting->ShadowModelOverride();
        gWorldModelOverride = &shadowOverride;
        worldLighting->BeginShadowPass(lightingEnv);
        drawWorldGeometry();
        worldLighting->EndShadowPass();
        gWorldModelOverride = nullptr;
        rlSetClipPlanes(RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
    }

    if (subViewport)
    {
        BeginMode3DInRect(view, viewRect);
    }
    else
    {
        BeginMode3D(view);
    }

    RestoreGreyboxImmediateState();

    if (DrawWorldDrawsEditorViewportGrid(overlay))
    {
        DrawEditorViewportGrid(overlay);
    }

    if (lightingReady)
    {
        const ModelDrawOverride litOverride = worldLighting->LitModelOverride();
        gWorldLighting = worldLighting.get();
        gWorldSolidMode = WorldSolidMode::Solid;
        gWorldModelOverride = &litOverride;
        worldLighting->BindLitPass(lightingEnv);
        if (staticPropModels)
        {
            staticPropModels->ResetDrawStats();
        }
        drawWorldGeometry();
        worldLighting->UnbindLitPass();
        gWorldModelOverride = nullptr;
        gWorldSolidMode = WorldSolidMode::Wires;
        drawWorldGeometry();
    }
    else
    {
        gWorldLighting = nullptr;
        gWorldSolidMode = WorldSolidMode::Combined;
        gWorldModelOverride = nullptr;
        if (staticPropModels)
        {
            staticPropModels->ResetDrawStats();
        }
        drawWorldGeometry();
    }

    gWorldSolidMode = WorldSolidMode::Combined;
    gWorldLighting = nullptr;
    gWorldModelOverride = nullptr;
    RestoreGreyboxImmediateState();

    if (lightingReady)
    {
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
            core::Vec3 loadedMin{};
            core::Vec3 loadedMax{};
            bool haveLoadedBounds = false;
            if (!pickup.modelIdentity.empty() && staticPropModels != nullptr)
            {
                haveLoadedBounds = staticPropModels->TryGetLoadedLocalBounds(
                    pickup.modelIdentity, loadedMin, loadedMax);
            }
            const ItemPickupTargetPresentation presentation = MakeItemPickupTargetPresentation(
                pickup,
                targeted,
                false,
                haveLoadedBounds,
                loadedMin,
                loadedMax,
                elapsedSeconds);
            if (!pickup.modelIdentity.empty() && staticPropModels)
            {
                if (presentation.drawModelHighlight)
                {
                    staticPropModels->DrawGameplayTargetHighlight(
                        presentation.visual,
                        presentation.highlightRed,
                        presentation.highlightGreen,
                        presentation.highlightBlue,
                        presentation.highlightAlpha);
                }
                if (presentation.drawInteractionBounds)
                {
                    DrawTransformedBoundsWires(
                        presentation.boundsCorners, kSelectedModelBoundsWire);
                }
            }
        }
        for (std::size_t goalIndex = 0; goalIndex < level.levelGoals.size(); ++goalIndex)
        {
            if (OverlayMarksPendingDelete(overlay.pendingDeleteLevelGoalIndices, goalIndex))
            {
                continue;
            }
            DrawLevelGoalPresentation(
                level.levelGoals[goalIndex],
                levelCompleted,
                LevelGoalViewKindFromEditor(overlay.drawLevelGoalAuthoredVolume),
                LevelGoalDrawLayer::EditorVolume);
        }
        for (std::size_t index = 0; index < pressurePlates.size(); ++index)
        {
            if (OverlayMarksPendingDelete(overlay.pendingDeletePressurePlateIndices, index))
            {
                continue;
            }
            const PressurePlateDrawState& plate = pressurePlates[index];
            if (!plate.visibleInGameplay && plate.revealInEditor)
            {
                DrawGhostBox(
                    plate.center,
                    plate.size,
                    kPressurePlateHiddenEditorFill,
                    kPressurePlateHiddenEditorWire);
            }
        }
    }

    if (drawGameplayHud)
    {
        DrawItemPickupCollectionFeedback(itemPickupCollectionFeedback);
    }

    // Editor overlay last in 3D: world, then marker/highlight/faded
    // pending-delete (depth on), cyan pending ghost, then the
    // depth-independent gizmo. ImGui is after EndMode3D.
    if (overlay.drawStaticPropPlacementPreview && staticPropModels)
    {
        staticPropModels->DrawPlacementPreview(overlay.staticPropPlacementPreview);
    }
    if (overlay.drawSelectedModelGhost && staticPropModels)
    {
        staticPropModels->DrawSelectionHighlight(overlay.selectedModelGhost);
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

    if (drawGameplayHud)
    {
        DrawRunTimer(elapsedSeconds);
        DrawSessionBest(hasBestTime, bestSeconds);
        DrawCollectedCounter(collectedCount, static_cast<int>(level.collectibles.size()));
        DrawGameplayObjectiveHud(objectiveHud);
        DrawHealthHud(healthHud);
        DrawDamageVignette(damageVignette);
        if (deathHud.visible)
        {
            DrawDeathHud(deathHud);
        }
        if (!inventoryPanel.visible && !runComplete)
        {
            if (!hideInteractionPrompts && !deathHud.visible)
            {
                DrawGrabCarryHud(grabHudCarrying, grabHudTarget);
                if (!grabHudCarrying && !grabHudTarget && pickupHudTarget)
                {
                    DrawPickupHud(pickupHudText);
                }
                else if (!grabHudCarrying && !grabHudTarget && !pickupHudTarget && doorHudTarget)
                {
                    DrawPickupHud(
                        lockedDoorPrompt != nullptr && lockedDoorPrompt[0] != '\0'
                            ? lockedDoorPrompt
                            : "Requires item");
                }
            }
            if (!deathHud.visible)
            {
                DrawItemPickupCollectionHud(itemPickupCollectionHud);
            }
        }
        if (runComplete)
        {
            DrawRunCompleteMessage(runCompleteFinalSeconds);
        }
        else if (levelCompleted)
        {
            DrawLevelCompleteMessage(destinationContinueHint);
        }
        DrawInventoryPanel(inventoryPanel);
    }
}

void Renderer::DrawMainMenu(bool playSelected)
{
    DrawMainMenuOverlay(playSelected);
}

void Renderer::DrawPauseMenu(bool resumeSelected)
{
    DrawPauseMenuOverlay(resumeSelected);
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

void Renderer::DrawEditorTerrainSculptHud(
    bool visible,
    const char* operation,
    bool hasHit,
    float topInset)
{
    if (!visible || operation == nullptr || operation[0] == '\0')
    {
        return;
    }

    const int font = 16;
    const char* line1 = TextFormat("Terrain Sculpt: %s", operation);
    const char* line2 = "LMB sculpt | Esc exit";
    const char* line3 = hasHit ? nullptr : "No Terrain hit";
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

void Renderer::DrawEditorTerrainPaintHud(
    bool visible,
    const char* layerName,
    bool hasHit,
    float topInset)
{
    if (!visible || layerName == nullptr || layerName[0] == '\0')
    {
        return;
    }

    const int font = 16;
    const char* line1 = TextFormat("Terrain Paint: %s", layerName);
    const char* line2 = "LMB paint | Esc exit";
    const char* line3 = hasHit ? nullptr : "No Terrain hit";
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