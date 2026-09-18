#pragma once

// Milestone 76: deterministic authored-transform snapping. Quantization and
// apply helpers are callable without ImGui. This is not a command protocol,
// Agent API, or generalized authoring framework.

#include "core/Vec3.h"
#include "editor/EditorGizmo.h"

#include <filesystem>
#include <string>
#include <string_view>

namespace editor
{
inline constexpr float kDefaultTranslateSnapIncrement = 0.25f;
inline constexpr float kDefaultResizeSnapIncrement = 0.25f;
inline constexpr float kDefaultScaleSnapIncrement = 0.10f;
inline constexpr float kDefaultRotateSnapIncrementDegrees = 15.0f;
inline constexpr float kMinSnapIncrement = 0.001f;
inline constexpr float kMaxSnapIncrement = 1000.0f;

// Custom section inside the existing editor_layout.ini. Not a new file and
// not a layout format version.
inline constexpr const char* kEditorSnapIniTypeName = "Platformer3D.Snap";
inline constexpr const char* kEditorSnapIniEntryName = "Settings";

// Temporary inversion while a gizmo is manipulated. Ctrl+click during
// selection (not during an active drag) is the M78 add/remove modifier.
// Shift is camera speed; Alt is wheel dolly.
inline constexpr const char* kEditorSnapInvertModifierName = "Ctrl";

struct EditorSnapPreferences
{
    bool enabled = true;
    float translateIncrement = kDefaultTranslateSnapIncrement;
    float resizeIncrement = kDefaultResizeSnapIncrement;
    float scaleIncrement = kDefaultScaleSnapIncrement;
    float rotateIncrementDegrees = kDefaultRotateSnapIncrementDegrees;
};

inline EditorSnapPreferences MakeDefaultEditorSnapPreferences()
{
    return {};
}

// XOR: holding the modifier inverts the persisted toggle without writing it.
inline bool EffectiveSnapEnabled(bool persistedEnabled, bool invertHeld)
{
    return invertHeld ? !persistedEnabled : persistedEnabled;
}

inline bool EditorSnapIsActive(const EditorSnapPreferences* preferences, bool invertHeld)
{
    if (preferences == nullptr)
    {
        return false;
    }
    return EffectiveSnapEnabled(preferences->enabled, invertHeld);
}

float SanitizeSnapIncrement(float value, float fallback);
void SanitizeEditorSnapPreferences(EditorSnapPreferences& preferences);

float EditorSnapDefaultIncrement(EditorTransformMode mode);
float EditorSnapIncrementForMode(
    const EditorSnapPreferences& preferences,
    EditorTransformMode mode);
float* EditorSnapIncrementPointer(
    EditorSnapPreferences& preferences,
    EditorTransformMode mode);
const char* EditorSnapIncrementLabel(EditorTransformMode mode);

// Round-half-away-from-zero of (value / increment), then multiply back.
// Exact snap boundaries stay on the grid. std::round is the tie rule.
float QuantizeToIncrement(float value, float increment);
core::Vec3 QuantizeVec3Axis(core::Vec3 value, EditorAxis axis, float increment);

// Authored-transform apply: same intended input + increment + enabled flag
// always yields the same result. Resize/Scale still run current validation
// clamps so snapping cannot weaken minimums.
core::Vec3 ApplyAuthoredTransformSnap(
    core::Vec3 intended,
    EditorTransformMode mode,
    EditorAxis axis,
    bool snapEnabled,
    float increment);

bool ParseEditorSnapPreferenceLine(EditorSnapPreferences& preferences, std::string_view line);
EditorSnapPreferences ParseEditorSnapPreferencesFromLayoutText(std::string_view layoutText);
std::string SerializeEditorSnapPreferencesSection(const EditorSnapPreferences& preferences);
std::string MergeEditorSnapPreferencesIntoLayoutText(
    std::string_view layoutText,
    const EditorSnapPreferences& preferences);

EditorSnapPreferences LoadEditorSnapPreferencesFromLayoutPath(const std::filesystem::path& path);
bool SaveEditorSnapPreferencesToLayoutPath(
    const std::filesystem::path& path,
    const EditorSnapPreferences& preferences);

void BindEditorSnapPreferences(EditorSnapPreferences* preferences);
EditorSnapPreferences* BoundEditorSnapPreferences();
}
