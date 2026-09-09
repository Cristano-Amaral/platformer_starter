#pragma once

// Focused editor selection identity for Level Format v1 categories.
// Type + index remains the identity: M41 keeps it and reconciles after
// structural working-copy edits. No UUID, registry, or entity system.

#include "world/LevelDefinition.h"

#include <cstddef>

namespace editor
{
enum class EditorObjectKind
{
    None,
    Spawn,
    Camera,
    Ground,
    ElevatedPlatform,
    Slope,
    MovingPlatform,
    Checkpoint,
    Hazard,
    Collectible,
    Goal,
    DynamicBox,
    StaticProp,
    PressurePlate,
    Door,
};

struct EditorSelection
{
    EditorObjectKind kind = EditorObjectKind::None;
    std::size_t index = 0;
};

inline bool operator==(EditorSelection a, EditorSelection b)
{
    return a.kind == b.kind && a.index == b.index;
}

inline bool operator!=(EditorSelection a, EditorSelection b)
{
    return !(a == b);
}

inline EditorSelection ClearSelection()
{
    return {};
}

const char* EditorObjectKindName(EditorObjectKind kind);
void FormatSelectionDisplayName(
    EditorSelection selection,
    char* buffer,
    std::size_t bufferSize);
const char* SelectionDisplayName(EditorSelection selection);

// Resolves against the supplied definition's authored counts. Camera and Spawn
// are always valid identities: they are unique v1 records, not world objects
// with a physics body.
bool IsValidSelection(const world::LevelDefinition& level, EditorSelection selection);

// True for Inspector-editable kinds. Selection is not permission to edit; this
// only answers Inspector routing. M41 adds Checkpoint, Hazard, and Collectible.
bool IsEditableSelection(EditorSelection selection);
}
