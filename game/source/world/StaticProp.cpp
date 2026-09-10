#include "world/StaticProp.h"

#include "assets/StaticGlb.h"
#include "world/LevelDefinition.h"

namespace world
{
bool StaticPropIdentityIsValid(std::string_view identity)
{
    std::string fileName;
    return assets::TryParseStaticModelIdentity(identity, fileName, nullptr);
}

bool StaticPropSpecIsValid(const StaticPropSpec& spec)
{
    return StaticPropIdentityIsValid(spec.modelIdentity) && StaticPropTransformIsValid(spec);
}

bool LevelReferencesStaticPropIdentity(
    const LevelDefinition& level,
    std::string_view identity)
{
    if (identity.empty())
    {
        return false;
    }
    for (const StaticPropSpec& prop : level.staticProps)
    {
        if (prop.modelIdentity == identity)
        {
            return true;
        }
    }
    for (const ItemPickupSpec& pickup : level.itemPickups)
    {
        if (pickup.modelIdentity == identity)
        {
            return true;
        }
    }
    return false;
}
}
