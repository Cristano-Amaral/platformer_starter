#pragma once

// Focused editor selection identity for Level Format v1 categories.
// Type + index is enough: v1 has fixed ordered arrays and M33 does not
// create or delete objects. No UUID, registry, or entity system.

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
const char* SelectionDisplayName(EditorSelection selection);

// Resolves against the supplied definition's authored counts. Camera and Spawn
// are always valid identities: they are unique v1 records, not world objects
// with a physics body.
bool IsValidSelection(const world::LevelDefinition& level, EditorSelection selection);

// True for the M32 editable set. Selection is not permission to edit; this
// only answers Inspector routing.
bool IsEditableSelection(EditorSelection selection);
}
