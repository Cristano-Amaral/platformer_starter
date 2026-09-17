#pragma once

// Milestone 63 presentation-only Level Goal drawing. Does not change overlap
// geometry, LevelGoalSpec, or completion authority. Not a generic trigger
// visualization framework.

#include "core/Vec3.h"
#include "world/LevelGoal.h"

namespace render
{
// Editor: authored AABB is visible. Gameplay and Release share the same rule.
enum class LevelGoalViewKind
{
    Editor,
    Gameplay,
};

struct LevelGoalVisualPlan
{
    bool drawAuthoredVolume = false;
    bool drawMarker = false;
    unsigned char volumeFillAlpha = 0;
    unsigned char volumeWireAlpha = 0;
};

// Same restrained fill alpha as hidden-in-gameplay Pressure Plate editor ghost.
inline constexpr unsigned char kLevelGoalEditorVolumeFillAlpha = 56;
inline constexpr unsigned char kLevelGoalEditorVolumeWireAlpha = 220;

inline LevelGoalVisualPlan MakeLevelGoalVisualPlan(LevelGoalViewKind view)
{
    LevelGoalVisualPlan plan{};
    plan.drawMarker = true;
    if (view == LevelGoalViewKind::Editor)
    {
        plan.drawAuthoredVolume = true;
        plan.volumeFillAlpha = kLevelGoalEditorVolumeFillAlpha;
        plan.volumeWireAlpha = kLevelGoalEditorVolumeWireAlpha;
    }
    return plan;
}

inline LevelGoalViewKind LevelGoalViewKindFromEditor(bool editorView)
{
    return editorView ? LevelGoalViewKind::Editor : LevelGoalViewKind::Gameplay;
}

void DrawLevelGoalPresentation(
    const world::LevelGoalSpec& goal,
    bool levelCompleted,
    LevelGoalViewKind view);
}
