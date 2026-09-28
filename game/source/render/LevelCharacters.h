#pragma once

#include "gameplay/CharacterHealth.h"

#include "render/CharacterInstance.h"
#include "gameplay/NpcRuntime.h"
#include "gameplay/EnemyRuntime.h"
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
// Registry and graphics context must outlive this owner. NPCs and Enemies borrow its instances.
class LevelCharacters final
{
public:
    explicit LevelCharacters(bool enablePatrolRuntime = true) : patrolRuntimeEnabled(enablePatrolRuntime) {}
    ~LevelCharacters() { Clear(); }
    void Clear() { enemies.clear(); npcs.clear(); borrowed.clear(); owned.clear(); authored.clear(); }
    void Rebuild(std::span<const world::CharacterPlacementSpec> placements,
        const gameplay::GameplayDefinitionRegistry& registry, const std::filesystem::path& assetRoot)
    {
        Clear();
        authored.assign(placements.begin(), placements.end());
        npcs.reserve(placements.size());
        enemies.reserve(placements.size());
        owned.reserve(placements.size());
        borrowed.reserve(placements.size());
        for (std::size_t index = 0; index < placements.size(); ++index)
        {
            const auto& placement = placements[index];
            std::unique_ptr<CharacterInstance> instance;
            if (world::CharacterPlacementSpecIsValid(placement)
                && registry.Resolve({placement.definitionIdentity},
                    gameplay::GameplayDefinitionCategory::Character).status == gameplay::GameplayReferenceStatus::Resolved)
            {
                instance = std::make_unique<CharacterInstance>(placement.definitionIdentity, registry, assetRoot);
                instance->SetWorldTransform(CharacterPlacementTransform(placement));
                if (patrolRuntimeEnabled && registry.Find(placement.definitionIdentity)->character.type == gameplay::CharacterType::NPC)
                {
                    npcs.emplace_back(index, placement, instance.get(),
                        gameplay::ResolveCharacterMaxHealth(registry.Find(placement.definitionIdentity)->character));
                    DrivePresentation(npcs.back(), *instance);
                }
                else if (patrolRuntimeEnabled && registry.Find(placement.definitionIdentity)->character.type == gameplay::CharacterType::Enemy)
                {
                    enemies.emplace_back(index, placement, instance.get(),
                        gameplay::ResolveCharacterMaxHealth(registry.Find(placement.definitionIdentity)->character));
                    DrivePresentation(enemies.back(), *instance);
                }
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
        for (auto& npc : npcs) { npc.Advance(deltaSeconds); DrivePresentation(npc, *npc.instance); }
        for (auto& enemy : enemies) { enemy.Advance(deltaSeconds); DrivePresentation(enemy, *enemy.instance); }
        for (auto& instance : owned) if (instance) instance->Advance(deltaSeconds);
    }
    std::span<CharacterInstance* const> Instances() const { return borrowed; }
    std::span<gameplay::NpcRuntimeActor> Npcs() { return npcs; }
    std::span<gameplay::EnemyRuntimeActor> Enemies() { return enemies; }
    std::span<const gameplay::NpcRuntimeActor> Npcs() const { return npcs; }
    std::span<const gameplay::EnemyRuntimeActor> Enemies() const { return enemies; }
    std::span<const world::CharacterPlacementSpec> Placements() const { return authored; }
private:
    static void DrivePresentation(const gameplay::CharacterPatrolState& patrol, CharacterInstance& instance)
    {
        instance.SetWorldTransform({patrol.position, patrol.rotationDegrees, patrol.origin.scale});
        instance.SetLocomotion(patrol.locomotion == gameplay::CharacterPatrolLocomotionState::Move
            ? CharacterLocomotionState::Move : CharacterLocomotionState::Idle);
    }
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
                || !SameVector(a.scale, b.scale)
                || a.patrolEnabled != b.patrolEnabled
                || !SameAxis(a.patrolDistance, b.patrolDistance)
                || !SameAxis(a.patrolSpeed, b.patrolSpeed)) return false;
        }
        return true;
    }
    bool patrolRuntimeEnabled;
    std::vector<gameplay::NpcRuntimeActor> npcs;
    std::vector<gameplay::EnemyRuntimeActor> enemies;
    std::vector<world::CharacterPlacementSpec> authored;
    std::vector<std::unique_ptr<CharacterInstance>> owned;
    std::vector<CharacterInstance*> borrowed;
};
}
