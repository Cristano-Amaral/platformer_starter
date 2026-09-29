#include "render/CharacterInstance.h"
#include "render/LoadedModelMaterials.h"
#include "gameplay/GameplayDefinitionFile.h"
#include "gameplay/RuntimeHealth.h"

#include <raylib.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

namespace
{
int failures = 0;
void Expect(bool condition, const char* message)
{
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); ++failures; }
}

struct Pixels
{
    std::uint64_t left = 14695981039346656037ull;
    std::uint64_t right = 14695981039346656037ull;
    int leftVisible = 0;
    int rightVisible = 0;
};

Pixels Render(RenderTexture2D target, const render::CharacterInstance* a,
    const render::CharacterInstance* b, const render::ModelDrawOverride* override = nullptr)
{
    constexpr Color background{17, 23, 31, 255};
    BeginTextureMode(target);
    ClearBackground(background);
    Camera3D camera{{0.0f, 1.0f, 10.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
        5.0f, CAMERA_ORTHOGRAPHIC};
    BeginMode3D(camera);
    if (a) a->Draw(override);
    if (b) b->Draw(override);
    EndMode3D();
    EndTextureMode();
    Image image = LoadImageFromTexture(target.texture);
    Color* colors = LoadImageColors(image);
    Pixels result;
    for (int y = 0; colors != nullptr && y < image.height; ++y)
        for (int x = 0; x < image.width; ++x)
        {
            const Color color = colors[y * image.width + x];
            auto& hash = x < image.width / 2 ? result.left : result.right;
            auto& visible = x < image.width / 2 ? result.leftVisible : result.rightVisible;
            if (color.r != background.r || color.g != background.g || color.b != background.b) ++visible;
            for (unsigned char channel : {color.r, color.g, color.b, color.a})
            { hash ^= channel; hash *= 1099511628211ull; }
        }
    if (colors) UnloadImageColors(colors);
    UnloadImage(image);
    return result;
}

std::vector<Transform> Pose(const render::CharacterInstance& instance)
{
    const auto* begin = static_cast<const Transform*>(instance.PoseAddress());
    return begin == nullptr ? std::vector<Transform>{}
        : std::vector<Transform>(begin, begin + instance.BoneCount());
}

bool SamePose(const std::vector<Transform>& saved, const render::CharacterInstance& instance)
{
    const auto current = Pose(instance);
    return saved.size() == current.size() && (saved.empty()
        || std::memcmp(saved.data(), current.data(), saved.size() * sizeof(Transform)) == 0);
}
}

int main()
{
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(64, 64, "CharacterInstanceTest");
    if (!IsWindowReady()) return 1;
    auto parsed = gameplay::LoadGameplayDefinitionsFile(PLATFORMER_GAMEPLAY_DEFINITIONS_SOURCE_PATH);
    auto& registry = parsed.registry;
    const auto* definition = registry.Find("characters/player");
    Expect(definition != nullptr, "canonical character exists");
    if (definition == nullptr) { CloseWindow(); return 1; }
    const auto original = definition->character;
    RenderTexture2D target = LoadRenderTexture(512, 256);
    const std::filesystem::path assetRoot = PLATFORMER_SOURCE_ASSET_ROOT;
    const auto vertexShader = assetRoot / "shaders/world_lit.vs";
    const auto fragmentShader = assetRoot / "shaders/world_lit.fs";
    Shader sharedShader = LoadShader(vertexShader.string().c_str(), fragmentShader.string().c_str());
    Expect(GetShaderLocation(sharedShader, "skinningEnabled") >= 0, "production shared shader loaded");
    const float white[3] = {1.0f, 1.0f, 1.0f};
    const float intensity = 1.0f;
    SetShaderValue(sharedShader, GetShaderLocation(sharedShader, "ambientColor"), white, SHADER_UNIFORM_VEC3);
    SetShaderValue(sharedShader, GetShaderLocation(sharedShader, "ambientIntensity"), &intensity, SHADER_UNIFORM_FLOAT);
    render::ModelDrawOverride sharedOverride{};
    sharedOverride.shader = sharedShader;
    {
        auto a = std::make_unique<render::CharacterInstance>("characters/player", registry, PLATFORMER_SOURCE_ASSET_ROOT);
        render::CharacterInstance b("characters/player", registry, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(a->HasModel() && b.HasModel(), "two production models loaded concurrently");
        Expect(a->Handle() != b.Handle() && a->Handle() != 0, "unique transient handles");
        Expect(a->DefinitionIdentity() == b.DefinitionIdentity(), "same authored identity shared");
        Expect(a->Mode() == render::CharacterInstanceMode::Exact && b.Mode() == render::CharacterInstanceMode::Exact,
            "canonical exact preferred");
        Expect(a->PoseAddress() != nullptr && b.PoseAddress() != nullptr
            && a->PoseAddress() != b.PoseAddress(), "pose allocations independent");
        Expect(a->BoneMatricesAddress() != nullptr && b.BoneMatricesAddress() != nullptr
            && a->BoneMatricesAddress() != b.BoneMatricesAddress(), "bone matrix allocations independent");
        render::CharacterInstanceTransform left;
        left.position = {-2.0f, 0.0f, 0.0f};
        auto right = left;
        right.position.x = 2.0f;
        a->SetWorldTransform(left);
        b.SetWorldTransform(right);
        Expect(a->WorldTransform().position.x != b.WorldTransform().position.x, "world transforms independent");
        const Pixels both = Render(target, a.get(), &b, &sharedOverride);
        const Pixels onlyA = Render(target, a.get(), nullptr);
        const Pixels onlyB = Render(target, nullptr, &b);
        Expect(both.leftVisible > 30 && both.rightVisible > 30, "both visible simultaneously");
        Expect(both.left == onlyA.left && both.right == onlyB.right,
            "pixels consume each distinct world transform without leakage");
        Expect(onlyA.rightVisible == 0 && onlyB.leftVisible == 0, "instances confined to independent screen regions");
        a->SetLocomotion(render::CharacterLocomotionState::Move);
        b.SetLocomotion(render::CharacterLocomotionState::Move);
        b.Advance(0.1f);
        const auto savedPose = Pose(b);
        const double savedBones = b.BoneMatrixChecksum();
        const float savedTime = b.PlaybackTime();
        const Pixels before = Render(target, a.get(), &b, &sharedOverride);
        const auto aPose = Pose(*a);
        a->Advance(0.35f);
        const Pixels after = Render(target, a.get(), &b, &sharedOverride);
        Expect(!SamePose(aPose, *a), "advancing A changes real Raylib pose");
        Expect(SamePose(savedPose, b) && b.BoneMatrixChecksum() == savedBones, "advancing A preserves B pose and bones");
        Expect(b.PlaybackTime() == savedTime && a->PlaybackTime() != savedTime, "playback clocks independent");
        Expect(before.left != after.left && before.right == after.right,
            "offscreen pixels prove independent animated rendering");
        a->SetLocomotion(render::CharacterLocomotionState::Jump);
        Expect(b.Locomotion() == render::CharacterLocomotionState::Move && b.PlaybackTime() == savedTime,
            "A state change preserves B state and playback");
        Expect(a->Mode() == render::CharacterInstanceMode::Exact, "canonical Jump remains Exact");
        a->Advance(0.2f);
        const float pausedTime = a->PlaybackTime();
        a->SetPlaying(false);
        a->Advance(1.0f);
        Expect(a->PlaybackTime() == pausedTime && b.PlaybackTime() == savedTime,
            "pausing A affects neither clock B nor its own clock");
        a->Restart();
        Expect(a->PlaybackTime() == 0.0f && b.PlaybackTime() == savedTime && SamePose(savedPose, b),
            "restart A does not restart or mutate B");
        a.reset();
        const Pixels surviving = Render(target, nullptr, &b, &sharedOverride);
        Expect(surviving.right == after.right && b.HasModel(), "destroying A preserves B rendered pixels and lifetime");
        b.Advance(0.2f);
        Expect(b.Mode() == render::CharacterInstanceMode::Exact, "survivor still updates");
        Expect(gameplay::CharacterDefinitionsEqual(original, registry.Find("characters/player")->character),
            "runtime state never mutates authored character");
    }
    {
        render::CharacterInstance retarget("characters/retarget_target", registry, PLATFORMER_SOURCE_ASSET_ROOT);
        render::CharacterInstance retargetB("characters/retarget_target", registry, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(retarget.Mode() == render::CharacterInstanceMode::Retargeted, "M109 fixture reuses retargeted path");
        render::CharacterInstanceTransform left;
        left.position = {-2.0f, 0.0f, 0.0f};
        auto right = left;
        right.position.x = 2.0f;
        retarget.SetWorldTransform(left);
        retargetB.SetWorldTransform(right);
        const auto before = Pose(retarget);
        const auto bBefore = Pose(retargetB);
        const auto pixelsBefore = Render(target, &retarget, &retargetB, &sharedOverride);
        retarget.Advance(0.3f);
        const auto pixelsAfter = Render(target, &retarget, &retargetB, &sharedOverride);
        Expect(!SamePose(before, retarget), "retargeted production pose advances");
        Expect(SamePose(bBefore, retargetB) && pixelsBefore.right == pixelsAfter.right
            && pixelsBefore.left != pixelsAfter.left, "retargeted simultaneous pixels and poses independent");
        const auto* character = registry.Find("characters/retarget_target");
        const auto validation = animation::ValidateCharacterAssets(registry, character->character, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(validation.idle.status == animation::CharacterAnimationCompatibilityStatus::SkeletonIncompatible,
            "retargetable remains exact incompatible");
        render::CharacterInstance missing("characters/missing", registry, PLATFORMER_SOURCE_ASSET_ROOT);
        render::CharacterInstance noModel("characters/guard", registry, PLATFORMER_SOURCE_ASSET_ROOT);
        missing.Advance(1.0f); noModel.Advance(1.0f);
        Expect(!missing.HasModel() && missing.Mode() == render::CharacterInstanceMode::Unavailable,
            "missing definition safe");
        Expect(!noModel.HasModel() && noModel.Mode() == render::CharacterInstanceMode::Unavailable,
            "missing model safe");
        Expect(Render(target, &missing, &retarget).leftVisible + Render(target, &noModel, &retarget).rightVisible > 0,
            "invalid instances do not prevent valid rendering");
    }
    for (int iteration = 0; iteration < 4; ++iteration)
    {
        render::CharacterInstance instance("characters/player", registry, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(instance.HasModel() && instance.Mode() == render::CharacterInstanceMode::Exact,
            "repeated create/destroy safe");
        for (auto state : {render::CharacterLocomotionState::Idle, render::CharacterLocomotionState::Move,
                render::CharacterLocomotionState::Jump})
        {
            instance.SetLocomotion(state);
            instance.Advance(0.1f);
            Expect(instance.Mode() == render::CharacterInstanceMode::Exact, "canonical Idle Move Jump Exact");
        }
    }
    {
        // Test-only authored definitions stay in memory; canonical catalog is untouched.
        auto testRegistry = registry;
        gameplay::GameplayDefinition fixture;
        fixture.identity = "characters/static_test";
        fixture.category = gameplay::GameplayDefinitionCategory::Character;
        fixture.character = gameplay::MakeDefaultCharacterDefinition(fixture.identity);
        fixture.character.worldModelIdentity = "models/test_static.glb";
        Expect(testRegistry.Register(fixture).status == gameplay::RegisterGameplayDefinitionStatus::Registered,
            "static test definition registered");
        render::CharacterInstance staticInstance(fixture.identity, testRegistry, PLATFORMER_SOURCE_ASSET_ROOT);
        staticInstance.Advance(1.0f);
        Expect(staticInstance.HasModel() && staticInstance.IsStatic()
            && staticInstance.Mode() == render::CharacterInstanceMode::Unavailable, "static character safe and unavailable animation");
        const auto staticPixels = Render(target, &staticInstance, nullptr);
        Expect(staticPixels.leftVisible + staticPixels.rightVisible > 30, "static instance renders safely");
        fixture.identity = "characters/bad_model_test";
        fixture.character.worldModelIdentity = "models/missing.glb";
        testRegistry.Register(fixture);
        render::CharacterInstance badModel(fixture.identity, testRegistry, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(!badModel.HasModel(), "nonexistent World Model safe");
        fixture.identity = "characters/bad_assignment_test";
        fixture.character = original;
        fixture.character.animations.idleAsset = "animations/missing";
        testRegistry.Register(fixture);
        render::CharacterInstance badAssignment(fixture.identity, testRegistry, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(badAssignment.HasModel() && badAssignment.Mode() == render::CharacterInstanceMode::Unavailable,
            "missing animation definition does not use embedded fallback");
        badAssignment.SetLocomotion(render::CharacterLocomotionState::Move);
        Expect(badAssignment.Mode() == render::CharacterInstanceMode::Exact, "valid slot independent of unavailable slot");
        badAssignment.SetLocomotion(render::CharacterLocomotionState::Idle);
        Expect(badAssignment.Mode() == render::CharacterInstanceMode::Unavailable, "unavailable slot restores rest matrices");
        render::CharacterInstance unmapped("characters/humanoid_mapping_fixture", testRegistry, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(unmapped.HasModel() && unmapped.Mode() == render::CharacterInstanceMode::Unavailable,
            "unassigned skeletal animation safe");
        fixture.identity = "characters/exact_mapped_test";
        fixture.character = gameplay::MakeDefaultCharacterDefinition(fixture.identity);
        fixture.character.worldModelIdentity = "models/retarget_source.glb";
        fixture.character.animations.idleAsset = "animations/retarget_move";
        fixture.character.humanoidMapping = registry.Find("animations/retarget_move")->animation.sourceHumanoidMapping;
        testRegistry.Register(fixture);
        render::CharacterInstance mappedExact(fixture.identity, testRegistry, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(mappedExact.Mode() == render::CharacterInstanceMode::Exact, "Exact preferred even when humanoid mappings available");
        fixture.identity = "characters/embedded_test";
        fixture.character = original;
        fixture.character.animations.idleAsset.clear();
        testRegistry.Register(fixture);
        render::CharacterInstance embedded(fixture.identity, testRegistry, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(embedded.Mode() == render::CharacterInstanceMode::Exact, "None assignment reuses embedded Exact clip");
        fixture.identity = "characters/missing_embedded_test";
        fixture.character.animations.idle = "MissingClip";
        testRegistry.Register(fixture);
        render::CharacterInstance missingEmbedded(fixture.identity, testRegistry, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(missingEmbedded.Mode() == render::CharacterInstanceMode::Unavailable, "missing embedded clip safe");
        const auto* retargetDefinition = registry.Find("characters/retarget_target");
        fixture.identity = "characters/incompatible_jump_reaction_test";
        fixture.character = retargetDefinition->character;
        fixture.character.animations.hitReactionAsset = "animations/humanoid_jump";
        testRegistry.Register(fixture);
        render::CharacterInstance incompatibleReaction(fixture.identity, testRegistry, PLATFORMER_SOURCE_ASSET_ROOT);
        gameplay::RuntimeHealth incompatibleHealth;
        incompatibleReaction.SetRuntimeHealth(&incompatibleHealth);
        incompatibleHealth.ApplyDamage({1});
        incompatibleReaction.Advance(1.0f / 60.0f);
        Expect(incompatibleReaction.HitReactionDiagnostic().find("Source mapping:") != std::string::npos
            && incompatibleReaction.HitReactionDuration() == 0 && !incompatibleReaction.HitReactionActive()
            && incompatibleReaction.DamageFeedbackActive() && incompatibleHealth.Current() == 99
            && incompatibleReaction.Mode() == render::CharacterInstanceMode::Retargeted,
            "Jump from an unmapped different skeleton safely leaves retargeted locomotion and red feedback functioning");
        fixture.identity = "characters/invalid_mapping_test";
        fixture.character = retargetDefinition->character;
        fixture.character.humanoidMapping.joints[0] = "MissingHips";
        testRegistry.Register(fixture);
        render::CharacterInstance invalidMapping(fixture.identity, testRegistry, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(invalidMapping.HasModel() && invalidMapping.Mode() == render::CharacterInstanceMode::Unavailable,
            "invalid humanoid mapping blocks unavailable retargeting safely");
        fixture.identity = "characters/missing_mapping_test";
        fixture.character.humanoidMapping = {};
        testRegistry.Register(fixture);
        render::CharacterInstance missingMapping(fixture.identity, testRegistry, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(missingMapping.Mode() == render::CharacterInstanceMode::Unavailable,
            "missing humanoid mapping prevents retargeted playback safely");
        for (bool missingSource : {false, true})
        {
            gameplay::GameplayDefinition clip;
            clip.identity = missingSource ? "animations/missing_source_test" : "animations/missing_clip_test";
            clip.category = gameplay::GameplayDefinitionCategory::Animation;
            clip.animation.sourceAssetIdentity = missingSource ? "models/missing.glb" : "models/humanoid_animations.glb";
            clip.animation.sourceClipName = "MissingClip";
            testRegistry.Register(clip);
            fixture.identity = missingSource ? "characters/missing_source_test" : "characters/missing_clip_test";
            fixture.character = original;
            fixture.character.animations.idleAsset = clip.identity;
            testRegistry.Register(fixture);
            render::CharacterInstance invalid(fixture.identity, testRegistry, PLATFORMER_SOURCE_ASSET_ROOT);
            Expect(invalid.HasModel() && invalid.Mode() == render::CharacterInstanceMode::Unavailable,
                "missing source or clip safe without embedded fallback");
        }
    }
    {
        auto definitions = registry;
        auto* character = definitions.FindMutable("characters/player");
        for (const char* asset : {"", "animations/missing", "animations/humanoid_jump"})
        {
            character->character.animations.hitReactionAsset = asset;
            render::CharacterInstance instance("characters/player", definitions, PLATFORMER_SOURCE_ASSET_ROOT);
            gameplay::RuntimeHealth health;
            instance.SetRuntimeHealth(&health);
            health.ApplyDamage({1});
            const bool valid = std::strcmp(asset, "animations/humanoid_jump") == 0;
            Expect(instance.HitReactionActive() == valid && health.Current() == 99 && instance.DamageFeedbackActive(),
                "missing/invalid optional reaction preserves health/flash/locomotion");
            instance.Advance(0);
            const double start = instance.BoneMatrixChecksum();
            health.AdvanceHitReaction(0.15f);
            instance.Advance(0.15f);
            if (valid) Expect(instance.BoneMatrixChecksum() != start, "production reaction changes sampled skeletal pose");
            health.Reset(); instance.Advance(0);
            Expect(!instance.HitReactionActive() && instance.Mode() == render::CharacterInstanceMode::Exact,
                "reset restores normal Exact animation selection");
        }
        auto* targetCharacter = definitions.FindMutable("characters/retarget_target");
        targetCharacter->character.animations.hitReactionAsset = "animations/retarget_move";
        render::CharacterInstance instance("characters/retarget_target", definitions, PLATFORMER_SOURCE_ASSET_ROOT);
        gameplay::RuntimeHealth health;
        instance.SetRuntimeHealth(&health); health.ApplyDamage({1}); instance.Advance(0);
        Expect(instance.HitReactionActive() && instance.Mode() == render::CharacterInstanceMode::Retargeted,
            "optional reaction reuses humanoid mapping and retargeting");
        health.ApplyDamage({100}); instance.Advance(0);
        Expect(!instance.HitReactionActive() && health.Defeated(), "death wins over an active retargeted reaction");
    }
    {
        auto definitions = registry;
        auto& character = definitions.FindMutable("characters/player")->character;
        for (const char* asset : {"", "animations/missing", "animations/humanoid_jump"}) {
            character.animations.attack.clear(); character.animations.attackAsset = asset;
            render::CharacterInstance instance("characters/player", definitions, PLATFORMER_SOURCE_ASSET_ROOT);
            gameplay::RuntimeHealth health;
            instance.SetRuntimeHealth(&health);
            const bool valid = std::string_view(asset) == "animations/humanoid_jump";
            Expect(instance.RequestAttack() == valid && health.Current() == 100
                && instance.Mode() == render::CharacterInstanceMode::Exact, "Attack resolution preserves actual locomotion presentation and never damages");
            if (valid) {
                const double pose = instance.BoneMatrixChecksum();
                health.AdvanceAttack(0.15f); instance.Advance(0.15f);
                Expect(pose != instance.BoneMatrixChecksum(), "Attack samples the actual pose");
                Expect(!instance.RequestAttack(), "repeated Attack is ignored");
                health.AdvanceAttack(10); instance.Advance(0);
                Expect(!instance.AttackActive() && instance.Mode() == render::CharacterInstanceMode::Exact, "Attack completes to locomotion");
                instance.RequestAttack(); health.Reset(); instance.Advance(0);
                Expect(!instance.AttackActive(), "reset clears Attack");
            }
        }
        auto& targetCharacter = definitions.FindMutable("characters/retarget_target")->character;
        targetCharacter.animations.attackAsset = "animations/humanoid_jump";
        render::CharacterInstance incompatible("characters/retarget_target", definitions, PLATFORMER_SOURCE_ASSET_ROOT);
        gameplay::RuntimeHealth health; incompatible.SetRuntimeHealth(&health);
        const double locomotionPose = incompatible.BoneMatrixChecksum();
        Expect(!incompatible.RequestAttack() && incompatible.Mode() == render::CharacterInstanceMode::Retargeted
            && incompatible.BoneMatrixChecksum() == locomotionPose && incompatible.AttackDiagnostic().find("Source mapping:") != std::string::npos,
            "incompatible Attack retains its fallback reason");
        targetCharacter.animations.attackAsset = "animations/retarget_move";
        render::CharacterInstance retargeted("characters/retarget_target", definitions, PLATFORMER_SOURCE_ASSET_ROOT);
        retargeted.SetRuntimeHealth(&health);
        Expect(retargeted.RequestAttack() && retargeted.Mode() == render::CharacterInstanceMode::Retargeted,
            "Attack reuses humanoid retargeting");
    }
    UnloadRenderTexture(target);
    UnloadShader(sharedShader);
    CloseWindow();
    std::printf("CharacterInstanceTest: %s (%d failures)\n", failures == 0 ? "PASS" : "FAIL", failures);
    return failures == 0 ? 0 : 1;
}
