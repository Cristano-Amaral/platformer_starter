#include "ui/debug/CharacterInstanceHarness.h"

#if defined(GAME_DEVELOPMENT)
#include "platform/RuntimePaths.h"
#include <imgui.h>

namespace ui
{
void CharacterInstanceHarness::Create(std::size_t index)
{
    if (registry == nullptr) return;
    instances[index] = std::make_unique<render::CharacterInstance>(index == 2
        ? "characters/retarget_target" : "characters/player", *registry, platform::RuntimeAssetRoot());
    render::CharacterInstanceTransform transform;
    transform.position = {anchor.x + 2.5f + static_cast<float>(index) * 2.5f,
        anchor.y, anchor.z + 2.5f};
    instances[index]->SetWorldTransform(transform);
    instances[index]->Advance(static_cast<float>(index) * 0.25f);
    drawInstances[index] = instances[index].get();
}

void CharacterInstanceHarness::Tick(const gameplay::GameplayDefinitionRegistry& definitions,
    float deltaSeconds, core::Vec3 position)
{
    registry = &definitions;
    if (!enabled) return;
    if (!initialized)
    {
        anchor = position;
        for (std::size_t index = 0; index < instances.size(); ++index) Create(index);
        initialized = true;
    }
    for (auto& instance : instances)
        if (instance) instance->Advance(deltaSeconds);
}

void CharacterInstanceHarness::Shutdown()
{
    initialized = false;
    drawInstances.fill(nullptr);
    for (auto& instance : instances) instance.reset();
}

void CharacterInstanceHarness::DrawControls()
{
    if (!ImGui::Begin("M110 Runtime Character Instances")) { ImGui::End(); return; }
    ImGui::TextUnformatted("Transient Development demo. No placement or persistence.");
    if (ImGui::Checkbox("Enable simultaneous instances", &enabled) && !enabled) Shutdown();
    if (enabled)
    {
        for (std::size_t index = 0; index < instances.size(); ++index)
        {
            ImGui::PushID(static_cast<int>(index));
            auto& instance = instances[index];
            ImGui::Separator();
            if (!instance)
            {
                if (ImGui::Button("Recreate")) Create(index);
                ImGui::PopID();
                continue;
            }
            ImGui::Text("Handle %llu | %s", static_cast<unsigned long long>(instance->Handle()),
                instance->DefinitionIdentity().c_str());
            const char* mode = instance->Mode() == render::CharacterInstanceMode::Exact ? "Exact"
                : instance->Mode() == render::CharacterInstanceMode::Retargeted ? "Retargeted" : "Unavailable";
            ImGui::Text("%s | time %.3f | bones %d", mode, instance->PlaybackTime(), instance->BoneCount());
            ImGui::TextWrapped("%s", instance->Diagnostic().c_str());
            int state = static_cast<int>(instance->Locomotion());
            if (ImGui::Combo("Locomotion", &state, "Idle\0Move\0Jump\0"))
                instance->SetLocomotion(static_cast<render::CharacterLocomotionState>(state));
            bool playing = instance->IsPlaying();
            if (ImGui::Checkbox("Playing", &playing)) instance->SetPlaying(playing);
            ImGui::SameLine();
            if (ImGui::Button("Restart")) instance->Restart();
            auto transform = instance->WorldTransform();
            float position[3] = {transform.position.x, transform.position.y, transform.position.z};
            if (ImGui::DragFloat3("World position", position, 0.05f))
            {
                transform.position = {position[0], position[1], position[2]};
                instance->SetWorldTransform(transform);
            }
            if (ImGui::Button("Remove")) { drawInstances[index] = nullptr; instance.reset(); }
            ImGui::PopID();
        }
    }
    ImGui::End();
}
}
#endif
