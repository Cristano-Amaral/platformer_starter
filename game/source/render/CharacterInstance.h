#pragma once

#include "core/Vec3.h"
#include "editor/CharacterPreview.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace gameplay { class RuntimeHealth; }

namespace render
{
struct ModelDrawOverride;
enum class CharacterLocomotionState { Idle, Move, Jump };
enum class CharacterInstanceMode { Unavailable, Exact, Retargeted };

struct CharacterInstanceTransform
{
    core::Vec3 position{};
    core::Vec3 rotationDegrees{};
    core::Vec3 scale{1.0f, 1.0f, 1.0f};
};

// Transient presentation only. A separate Raylib Model is owned by every instance:
// Raylib's pose and boneMatrices are mutable allocations inside Model.
class CharacterInstance final
{
public:
    CharacterInstance(std::string_view definitionIdentity,
        const gameplay::GameplayDefinitionRegistry& registry,
        const std::filesystem::path& assetRoot);
    ~CharacterInstance();
    CharacterInstance(const CharacterInstance&) = delete;
    CharacterInstance& operator=(const CharacterInstance&) = delete;

    std::uint64_t Handle() const { return handle; }
    const std::string& DefinitionIdentity() const { return definitionIdentity; }
    const CharacterInstanceTransform& WorldTransform() const { return transform; }
    void SetWorldTransform(const CharacterInstanceTransform& value) { transform = value; }
    CharacterLocomotionState Locomotion() const { return locomotion; }
    void SetLocomotion(CharacterLocomotionState value);
    void SetPlaying(bool value) { playing = value; }
    bool IsPlaying() const { return playing; }
    void Restart();
    void Advance(float deltaSeconds);
    float PlaybackTime() const { return timeSeconds; }
    CharacterInstanceMode Mode() const { return mode; }
    bool HasModel() const;
    bool IsStatic() const;
    const std::string& Diagnostic() const { return diagnostic; }
    int BoneCount() const;
    core::Vec3 JointTranslation(int index) const;
    double BoneMatrixChecksum() const;
    const void* PoseAddress() const;
    const void* BoneMatricesAddress() const;
    // Borrowed from the owning runtime actor; actor storage is stable until rebuild.
    void SetRuntimeHealth(gameplay::RuntimeHealth* value);
    bool RequestAttack();
    bool AttackActive() const;
    const std::string& AttackDiagnostic() const { return attackDiagnostic; }
    bool HitReactionActive() const;
    float HitReactionDuration() const { return hitReactionDuration; }
    const std::string& HitReactionDiagnostic() const { return hitReactionDiagnostic; }
    bool DamageFeedbackActive() const;
    void Draw(const ModelDrawOverride* override = nullptr) const;

private:
    struct GpuState;
    void ResolveSlot();
    void ReleaseAnimation();
    const gameplay::GameplayDefinitionRegistry& registry;
    std::filesystem::path assetRoot;
    std::unique_ptr<GpuState> gpu;
    std::uint64_t handle = 0;
    std::string definitionIdentity;
    std::string modelIdentity;
    CharacterInstanceTransform transform{};
    CharacterLocomotionState locomotion = CharacterLocomotionState::Idle;
    CharacterInstanceMode mode = CharacterInstanceMode::Unavailable;
    editor::CharacterPreviewAnimationResolution resolution{};
    std::string diagnostic;
    float timeSeconds = 0.0f;
    bool playing = true;
    gameplay::RuntimeHealth* runtimeHealth = nullptr;
    bool attackSelected = false;
    float attackDuration = 0.0f;
    std::string attackDiagnostic = "Character presentation unavailable";
    bool reactionSelected = false;
    float hitReactionDuration = 0.0f;
    std::string hitReactionDiagnostic = "Character presentation unavailable";
};
}
