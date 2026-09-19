#include "render/LevelGoalVisualization.h"

#include "render/StaticModelScene.h"
#include "world/RespawnWorld.h"

#include "raylib.h"
#include "rlgl.h"

namespace render
{
namespace
{
Vector3 ToRaylib(core::Vec3 value)
{
    return Vector3{value.x, value.y, value.z};
}

constexpr Color kGoalIncompletePost{156, 116, 52, 255};
constexpr Color kGoalIncompleteBar{188, 148, 64, 255};
constexpr Color kGoalCompletedPost{212, 168, 48, 255};
constexpr Color kGoalCompletedBar{244, 212, 84, 255};
constexpr Color kGoalVolumeIncomplete{64, 140, 92, 255};
constexpr Color kGoalVolumeCompleted{88, 176, 72, 255};
constexpr Color kGoalEditorVolumeWire{160, 220, 176, 255};

Color WithAlpha(Color color, unsigned char alpha)
{
    color.a = alpha;
    return color;
}

void DrawLevelGoalEditorAuthoredVolume(
    const world::LevelGoalSpec& goal,
    bool levelCompleted,
    const LevelGoalVisualPlan& plan)
{
    if (!plan.drawAuthoredVolume || goal.size.x <= 0.0f || goal.size.y <= 0.0f
        || goal.size.z <= 0.0f)
    {
        return;
    }

    const Color fill = WithAlpha(
        levelCompleted ? kGoalVolumeCompleted : kGoalVolumeIncomplete, plan.volumeFillAlpha);
    const Color wire = WithAlpha(kGoalEditorVolumeWire, plan.volumeWireAlpha);
    const Vector3 position = ToRaylib(goal.center);

    rlDrawRenderBatchActive();
    BeginBlendMode(BLEND_ALPHA);
    rlDisableDepthMask();
    DrawCube(position, goal.size.x, goal.size.y, goal.size.z, fill);
    DrawCubeWires(position, goal.size.x, goal.size.y, goal.size.z, wire);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    EndBlendMode();
    RestoreGreyboxImmediateState();
}

void DrawLevelGoalMarkerPosts(
    const world::LevelGoalSpec& goal,
    bool levelCompleted,
    LevelGoalDrawLayer layer)
{
    const LevelGoalMarkerLayout layout = MakeLevelGoalMarkerLayout(goal);
    const LevelGoalMarkerColors colors = MakeLevelGoalMarkerColors(levelCompleted);
    const Color postColor{colors.postR, colors.postG, colors.postB, colors.postA};
    const Color barColor{colors.barR, colors.barG, colors.barB, colors.barA};
    const Vector3 left = ToRaylib(layout.leftPost);
    const Vector3 right = ToRaylib(layout.rightPost);
    const Vector3 bar = ToRaylib(layout.barCenter);
    const Color wire{24, 26, 32, 255};
    const bool drawFill = layer == LevelGoalDrawLayer::All || layer == LevelGoalDrawLayer::Solids;
    const bool drawWires = layer == LevelGoalDrawLayer::All || layer == LevelGoalDrawLayer::Wires;
    if (drawFill)
    {
        DrawCube(left, layout.postSize.x, layout.postSize.y, layout.postSize.z, postColor);
        DrawCube(right, layout.postSize.x, layout.postSize.y, layout.postSize.z, postColor);
        DrawCube(bar, layout.barSize.x, layout.barSize.y, layout.barSize.z, barColor);
    }
    if (drawWires)
    {
        DrawCubeWires(left, layout.postSize.x, layout.postSize.y, layout.postSize.z, wire);
        DrawCubeWires(right, layout.postSize.x, layout.postSize.y, layout.postSize.z, wire);
        DrawCubeWires(bar, layout.barSize.x, layout.barSize.y, layout.barSize.z, wire);
    }
}
}

LevelGoalMarkerLayout MakeLevelGoalMarkerLayout(const world::LevelGoalSpec& goal)
{
    constexpr float postWidth = 0.16f;
    constexpr float postHeight = 1.6f;
    constexpr float barHeight = 0.16f;
    constexpr float barDepth = 0.16f;
    constexpr float postSpread = 0.70f;
    constexpr float zOffset = -0.90f;

    const float platformTopY = goal.center.y - world::kPlayerVisualSize.y * 0.5f;
    const float postCenterY = platformTopY + postHeight * 0.5f;
    const float z = goal.center.z + zOffset;
    LevelGoalMarkerLayout layout{};
    layout.postSize = {postWidth, postHeight, postWidth};
    layout.leftPost = {goal.center.x - postSpread, postCenterY, z};
    layout.rightPost = {goal.center.x + postSpread, postCenterY, z};
    layout.barCenter = {goal.center.x, platformTopY + postHeight + barHeight * 0.5f, z};
    layout.barSize = {postSpread * 2.0f + postWidth, barHeight, barDepth};
    return layout;
}

LevelGoalMarkerColors MakeLevelGoalMarkerColors(bool levelCompleted)
{
    if (levelCompleted)
    {
        return {212, 168, 48, 255, 244, 212, 84, 255};
    }
    return {156, 116, 52, 255, 188, 148, 64, 255};
}

void DrawLevelGoalPresentation(
    const world::LevelGoalSpec& goal,
    bool levelCompleted,
    LevelGoalViewKind view)
{
    DrawLevelGoalPresentation(goal, levelCompleted, view, LevelGoalDrawLayer::All);
}

void DrawLevelGoalPresentation(
    const world::LevelGoalSpec& goal,
    bool levelCompleted,
    LevelGoalViewKind view,
    LevelGoalDrawLayer layer)
{
    const LevelGoalVisualPlan plan = MakeLevelGoalVisualPlan(view);
    if (plan.drawAuthoredVolume
        && (layer == LevelGoalDrawLayer::All || layer == LevelGoalDrawLayer::EditorVolume))
    {
        DrawLevelGoalEditorAuthoredVolume(goal, levelCompleted, plan);
    }
    if (plan.drawMarker && layer != LevelGoalDrawLayer::EditorVolume)
    {
        DrawLevelGoalMarkerPosts(goal, levelCompleted, layer);
    }
}
}
