#pragma once

// Milestone 35: precise world-space translation of gizmo-supported objects.
// Translate mode only. Not camera-relative, not a grid snap system.

#include "core/Vec3.h"
#include "editor/EditorGizmo.h"
#include "editor/EditorSelection.h"
#include "world/LevelDefinition.h"

namespace editor
{
inline constexpr float kNudgeStep = 0.10f;
inline constexpr float kNudgePrecisionStep = 0.01f;

float NudgeStep(bool precision);
core::Vec3 NudgeWorldDelta(EditorAxis axis, float sign, bool precision);

// Writes workingCopy position only. Returns false for unsupported selections,
// non-finite results, or when mode is not Translate.
bool ApplyNudge(
    world::LevelDefinition& workingCopy,
    EditorSelection selection,
    EditorAxis axis,
    float sign,
    bool precision,
    EditorTransformMode mode = EditorTransformMode::Translate);

// Live editor gate: Translate only, no ImGui keyboard capture, no active drag.
bool NudgeAllowed(EditorTransformMode mode, bool keyboardCaptured, bool dragging);
}
