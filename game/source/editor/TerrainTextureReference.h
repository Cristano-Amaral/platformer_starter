#pragma once

// Loaded Terrain texture delete protection. Inspects only the currently
// loaded working/active/saved authored Level. Not a project-wide graph.

#include "world/LevelDefinition.h"

#include <string>
#include <string_view>

namespace editor
{
inline bool AuthoredLevelsProtectTerrainTextureIdentity(
    const world::LevelDefinition& workingCopy,
    const world::LevelDefinition& active,
    const world::LevelDefinition& savedSourceBaseline,
    std::string_view identity)
{
    return world::LevelReferencesTerrainTextureIdentity(workingCopy, identity)
        || world::LevelReferencesTerrainTextureIdentity(active, identity)
        || world::LevelReferencesTerrainTextureIdentity(savedSourceBaseline, identity);
}

inline std::string TerrainTextureReferencedDeleteMessage(std::string_view identity)
{
    std::string message = "Cannot delete ";
    message.append(identity);
    message += ": it is referenced by a currently loaded Terrain material layer "
               "(working copy, applied world, or last saved source). Other Levels are not scanned.";
    return message;
}
}
