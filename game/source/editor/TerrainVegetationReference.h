#pragma once

// Loaded Terrain vegetation model delete protection. Inspects only the
// currently loaded working/active/saved authored Level.

#include "world/LevelDefinition.h"

#include <string>
#include <string_view>

namespace editor
{
inline bool AuthoredLevelsProtectTerrainVegetationIdentity(
    const world::LevelDefinition& workingCopy,
    const world::LevelDefinition& active,
    const world::LevelDefinition& savedSourceBaseline,
    std::string_view identity)
{
    return world::LevelReferencesTerrainVegetationModel(workingCopy, identity)
        || world::LevelReferencesTerrainVegetationModel(active, identity)
        || world::LevelReferencesTerrainVegetationModel(savedSourceBaseline, identity);
}

inline std::string TerrainVegetationReferencedDeleteMessage(std::string_view identity)
{
    std::string message = "Cannot delete ";
    message.append(identity);
    message += ": it is referenced by Terrain vegetation in the current authored level "
               "(working copy, applied world, or last saved source).";
    return message;
}
}
