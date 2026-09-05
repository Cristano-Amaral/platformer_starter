#include "editor/EditorNudge.h"

namespace editor
{
float NudgeStep(bool precision)
{
    return precision ? kNudgePrecisionStep : kNudgeStep;
}

core::Vec3 NudgeWorldDelta(EditorAxis axis, float sign, bool precision)
{
    const float magnitude = NudgeStep(precision);
    const float applied = (sign < 0.0f) ? -magnitude : magnitude;
    switch (axis)
    {
    case EditorAxis::X:
        return {applied, 0.0f, 0.0f};
    case EditorAxis::Y:
        return {0.0f, applied, 0.0f};
    case EditorAxis::Z:
        return {0.0f, 0.0f, applied};
    case EditorAxis::None:
        break;
    }
    return {};
}

bool NudgeAllowed(EditorTransformMode mode, bool keyboardCaptured, bool dragging)
{
    return mode == EditorTransformMode::Translate && !keyboardCaptured && !dragging;
}

bool ApplyNudge(
    world::LevelDefinition& workingCopy,
    EditorSelection selection,
    EditorAxis axis,
    float sign,
    bool precision,
    EditorTransformMode mode)
{
    if (mode != EditorTransformMode::Translate || axis == EditorAxis::None || sign == 0.0f
        || !IsGizmoSelection(selection))
    {
        return false;
    }

    core::Vec3* position = GetEditablePosition(workingCopy, selection);
    if (position == nullptr)
    {
        return false;
    }

    const core::Vec3 delta = NudgeWorldDelta(axis, sign, precision);
    const core::Vec3 next{
        position->x + delta.x, position->y + delta.y, position->z + delta.z};
    if (!IsFiniteVec3(next))
    {
        return false;
    }

    *position = next;
    return true;
}
}
