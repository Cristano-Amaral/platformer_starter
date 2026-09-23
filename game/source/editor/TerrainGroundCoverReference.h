#pragma once

// Loaded Terrain ground-cover texture delete protection. Inspects only the
// currently loaded working/active/saved authored Level.

#include "world/LevelDefinition.h"

#include <string>
#include <string_view>

namespace editor
{
inline bool AuthoredLevelsProtectTerrainGroundCoverIdentity(
    const world::LevelDefinition& workingCopy,
    const world::LevelDefinition& active,
    const world::LevelDefinition& savedSourceBaseline,
    std::string_view identity)
{
    return world::LevelReferencesTerrainGroundCoverTexture(workingCopy, identity)
        || world::LevelReferencesTerrainGroundCoverTexture(active, identity)
        || world::LevelReferencesTerrainGroundCoverTexture(savedSourceBaseline, identity);
}

inline std::string TerrainGroundCoverReferencedDeleteMessage(std::string_view identity)
{
    std::string message = "Cannot delete ";
    message.append(identity);
    message += ": it is referenced by Terrain ground cover in the current authored level "
               "(working copy, applied world, or last saved source).";
    return message;
}
}
