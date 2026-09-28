#pragma once

#if defined(GAME_DEVELOPMENT)
#include "render/CharacterInstance.h"

#include <array>
#include <memory>
#include <span>

namespace ui
{
// Fixed Development demo, never a level record or authored spawn system.
class CharacterInstanceHarness
{
public:
    void Tick(const gameplay::GameplayDefinitionRegistry& registry, float deltaSeconds,
        core::Vec3 anchor);
    void DrawControls();
    void Shutdown();
    std::span<render::CharacterInstance* const> Instances() const { return drawInstances; }
private:
    bool enabled = false;
    bool initialized = false;
    const gameplay::GameplayDefinitionRegistry* registry = nullptr;
    core::Vec3 anchor{};
    std::array<std::unique_ptr<render::CharacterInstance>, 3> instances{};
    std::array<render::CharacterInstance*, 3> drawInstances{};
    void Create(std::size_t index);
};
}
#endif
