#pragma once

#include "render/CharacterInstance.h"
#include "world/CharacterPlacement.h"
#include <span>
#include <vector>

namespace render
{
inline CharacterInstanceTransform CharacterPlacementTransform(const world::CharacterPlacementSpec& placement)
{
    return {placement.position, placement.rotationDegrees, placement.scale};
}

// Application-owned realization of ACTIVE authored placements. The same bounded
// presentation path also serves the editor's separate, transient working preview.
// Registry and graphics context must outlive this owner. Type activates no behavior.
class LevelCharacters final
{
public:
    void Clear() { borrowed.clear(); owned.clear(); authored.clear(); }
    void Rebuild(std::span<const world::CharacterPlacementSpec> placements,
        const gameplay::GameplayDefinitionRegistry& registry, const std::filesystem::path& assetRoot)
    {
        Clear();
        authored.assign(placements.begin(), placements.end());
        owned.reserve(placements.size());
        borrowed.reserve(placements.size());
        for (const auto& placement : placements)
        {
            std::unique_ptr<CharacterInstance> instance;
            if (world::CharacterPlacementSpecIsValid(placement)
                && registry.Resolve({placement.definitionIdentity},
                    gameplay::GameplayDefinitionCategory::Character).status == gameplay::GameplayReferenceStatus::Resolved)
            {
                instance = std::make_unique<CharacterInstance>(placement.definitionIdentity, registry, assetRoot);
                instance->SetWorldTransform(CharacterPlacementTransform(placement));
            }
            borrowed.push_back(instance.get());
            owned.push_back(std::move(instance));
        }
    }
    void Sync(std::span<const world::CharacterPlacementSpec> placements,
        const gameplay::GameplayDefinitionRegistry& registry, const std::filesystem::path& assetRoot)
    {
        if (!Matches(placements)) Rebuild(placements, registry, assetRoot);
    }
    void Advance(float deltaSeconds)
    {
        for (auto& instance : owned) if (instance) instance->Advance(deltaSeconds);
    }
    std::span<CharacterInstance* const> Instances() const { return borrowed; }
    std::span<const world::CharacterPlacementSpec> Placements() const { return authored; }
private:
    static bool SameAxis(float a, float b)
    {
        // Invalid working transforms must not cause a preview rebuild every frame.
        return a == b || (std::isnan(a) && std::isnan(b));
    }
    static bool SameVector(core::Vec3 a, core::Vec3 b)
    {
        return SameAxis(a.x, b.x) && SameAxis(a.y, b.y) && SameAxis(a.z, b.z);
    }
    bool Matches(std::span<const world::CharacterPlacementSpec> placements) const
    {
        if (authored.size() != placements.size()) return false;
        for (std::size_t index = 0; index < authored.size(); ++index)
        {
            const auto& a = authored[index]; const auto& b = placements[index];
            if (a.definitionIdentity != b.definitionIdentity
                || !SameVector(a.position, b.position)
                || !SameVector(a.rotationDegrees, b.rotationDegrees)
                || !SameVector(a.scale, b.scale)) return false;
        }
        return true;
    }
    std::vector<world::CharacterPlacementSpec> authored;
    std::vector<std::unique_ptr<CharacterInstance>> owned;
    std::vector<CharacterInstance*> borrowed;
};
}
