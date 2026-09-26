#include "animation/CharacterAssetValidator.h"
#include "editor/CharacterDatabaseEditor.h"
#include "gameplay/GameplayDefinitionFile.h"

#include <raylib.h>

#include <cstdio>
#include <filesystem>
#include <fstream>

namespace
{
int failures = 0;
void Expect(bool condition, const char* name)
{
    if (!condition) { std::fprintf(stderr, "FAIL %s\n", name); ++failures; }
}
}

int main()
{
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(64, 64, "CharacterAssetValidatorTest");
    if (!IsWindowReady())
    {
        std::fprintf(stderr, "FAIL hidden Raylib validation window\n");
        return 1;
    }
    const auto parsed = gameplay::LoadGameplayDefinitionsFile(
        PLATFORMER_GAMEPLAY_DEFINITIONS_SOURCE_PATH);
    const auto* canonical = parsed.registry.Find("characters/player");
    Expect(canonical != nullptr, "canonical Player exists");
    if (canonical == nullptr) return 1;

    const auto valid = animation::ValidateCharacterAssets(parsed.registry,
        canonical->character, PLATFORMER_SOURCE_ASSET_ROOT);
    Expect(valid.model.status == animation::CharacterModelValidationStatus::Resolved,
        "canonical Player resolves through Raylib");
    Expect(valid.model.modelLoaded && valid.model.skinned && valid.model.hasSkeleton,
        "canonical Player is skinned and skeletal");
    Expect(valid.model.jointCount == 1 && valid.model.joints.size() == 1,
        "canonical skeleton metadata is deterministic and bounded");
    Expect(valid.model.joints.size() == 1 && valid.model.joints[0].name == "Root"
            && valid.model.joints[0].parentIndex == -1,
        "canonical joint names and hierarchy exposed");
    Expect(valid.idle.status == animation::CharacterAnimationCompatibilityStatus::Compatible
            && valid.move.status == animation::CharacterAnimationCompatibilityStatus::Compatible
            && valid.jump.status == animation::CharacterAnimationCompatibilityStatus::Compatible,
        "canonical reusable locomotion assets exactly compatible");

    gameplay::CharacterDefinition probe = canonical->character;
    probe.worldModelIdentity = "models/test_static.glb";
    const auto staticResult = animation::ValidateCharacterAssets(
        parsed.registry, probe, PLATFORMER_SOURCE_ASSET_ROOT);
    Expect(staticResult.model.status == animation::CharacterModelValidationStatus::Resolved
            && staticResult.model.modelLoaded && !staticResult.model.skinned
            && !staticResult.model.hasSkeleton && staticResult.model.jointCount == 0,
        "static model safely classified as non-skinned");
    Expect(staticResult.idle.status
            == animation::CharacterAnimationCompatibilityStatus::SkeletonIncompatible,
        "static model unsuitable for reusable skeletal animation");

    probe.worldModelIdentity = "models/missing.glb";
    const auto missingModel = animation::ValidateCharacterAssets(
        parsed.registry, probe, PLATFORMER_SOURCE_ASSET_ROOT);
    Expect(missingModel.model.status == animation::CharacterModelValidationStatus::Missing,
        "missing World Model distinguished");

    const std::filesystem::path malformedRoot = std::filesystem::temp_directory_path()
        / "platformer_character_asset_validator";
    std::filesystem::create_directories(malformedRoot / "models");
    {
        std::ofstream malformed(malformedRoot / "models/malformed.glb", std::ios::binary);
        malformed << "bad";
    }
    std::filesystem::copy_file(
        std::filesystem::path(PLATFORMER_SOURCE_ASSET_ROOT) / "models/player.glb",
        malformedRoot / "models/player.glb", std::filesystem::copy_options::overwrite_existing);
    probe.animations = {};
    probe.worldModelIdentity = "models/malformed.glb";
    const auto malformedModel = animation::ValidateCharacterAssets(
        parsed.registry, probe, malformedRoot);
    Expect(malformedModel.model.status == animation::CharacterModelValidationStatus::LoadFailure,
        "malformed World Model distinguished from missing");

    gameplay::GameplayDefinitionRegistry malformedSourceRegistry =
        editor::CloneGameplayDefinitionRegistry(parsed.registry);
    auto* idle = malformedSourceRegistry.FindMutable("animations/humanoid_idle");
    idle->animation.sourceAssetIdentity = "models/malformed.glb";
    probe = canonical->character;
    probe.animations.moveAsset.clear();
    probe.animations.jumpAsset.clear();
    const auto malformedSource = animation::ValidateCharacterAssets(
        malformedSourceRegistry, probe, malformedRoot);
    Expect(malformedSource.idle.status
            == animation::CharacterAnimationCompatibilityStatus::LoadFailure,
        "malformed reusable source asset distinguished");
    std::error_code cleanupError;
    std::filesystem::remove_all(malformedRoot, cleanupError);

    probe = canonical->character;
    probe.animations.idleAsset = "animations/does_not_exist";
    const auto missingDefinition = animation::ValidateCharacterAssets(
        parsed.registry, probe, PLATFORMER_SOURCE_ASSET_ROOT);
    Expect(missingDefinition.idle.status
            == animation::CharacterAnimationCompatibilityStatus::MissingDefinition,
        "missing reusable identity distinguished from incompatibility");

    gameplay::GameplayDefinitionRegistry missingSourceRegistry =
        editor::CloneGameplayDefinitionRegistry(parsed.registry);
    idle = missingSourceRegistry.FindMutable("animations/humanoid_idle");
    idle->animation.sourceAssetIdentity = "models/missing.glb";
    const auto missingSource = animation::ValidateCharacterAssets(missingSourceRegistry,
        canonical->character, PLATFORMER_SOURCE_ASSET_ROOT);
    Expect(missingSource.idle.status
            == animation::CharacterAnimationCompatibilityStatus::MissingSourceAsset,
        "missing reusable source asset distinguished");

    gameplay::GameplayDefinitionRegistry missingClipRegistry =
        editor::CloneGameplayDefinitionRegistry(parsed.registry);
    idle = missingClipRegistry.FindMutable("animations/humanoid_idle");
    idle->animation.sourceClipName = "AbsentClip";
    const auto missingClip = animation::ValidateCharacterAssets(missingClipRegistry,
        canonical->character, PLATFORMER_SOURCE_ASSET_ROOT);
    Expect(missingClip.idle.status
            == animation::CharacterAnimationCompatibilityStatus::MissingSourceClip,
        "missing source clip distinguished");

    probe = canonical->character;
    probe.animations.idleAsset.clear();
    const auto embedded = animation::ValidateCharacterAssets(
        parsed.registry, probe, PLATFORMER_SOURCE_ASSET_ROOT);
    Expect(embedded.idle.status
            == animation::CharacterAnimationCompatibilityStatus::NoneEmbedded,
        "cleared reusable assignment reports embedded path");

    BoneInfo characterBones[1]{};
    BoneInfo sourceBones[1]{};
    std::snprintf(characterBones[0].name, sizeof(characterBones[0].name), "Root");
    std::snprintf(sourceBones[0].name, sizeof(sourceBones[0].name), "Other");
    characterBones[0].parent = sourceBones[0].parent = -1;
    Model characterModel{}; characterModel.skeleton.boneCount = 1;
    characterModel.skeleton.bones = characterBones;
    Model sourceModel{}; sourceModel.skeleton.boneCount = 1;
    sourceModel.skeleton.bones = sourceBones;
    ModelAnimation sourceAnimation{}; sourceAnimation.boneCount = 1;
    Expect(!animation::RaylibSkeletonsExactlyCompatible(
        characterModel, sourceModel, sourceAnimation),
        "exact skeleton name mismatch rejected");
    std::snprintf(sourceBones[0].name, sizeof(sourceBones[0].name), "Root");
    sourceBones[0].parent = 0;
    Expect(!animation::RaylibSkeletonsExactlyCompatible(
        characterModel, sourceModel, sourceAnimation),
        "exact skeleton hierarchy mismatch rejected");

    editor::CharacterDatabaseEditorState editorState;
    Expect(editor::ApplyLoadedCharacterDatabase(editorState, parsed), "editor working copy loaded");
    gameplay::GameplayDefinitionRegistry active =
        editor::CloneGameplayDefinitionRegistry(parsed.registry);
    auto* workingPlayer = editorState.working.FindMutable("characters/player");
    editor::RefreshCharacterAssetValidation(
        editorState, workingPlayer->character, PLATFORMER_SOURCE_ASSET_ROOT);
    Expect(editorState.assetValidation.idle.status
            == animation::CharacterAnimationCompatibilityStatus::Compatible,
        "initial working-copy validation compatible");
    editor::TryAssignCharacterWorldModel(*workingPlayer, "models/test_static.glb");
    editor::RefreshCharacterAssetValidation(
        editorState, workingPlayer->character, PLATFORMER_SOURCE_ASSET_ROOT);
    Expect(!editorState.assetValidation.model.skinned
            && editorState.assetValidation.idle.status
                == animation::CharacterAnimationCompatibilityStatus::SkeletonIncompatible,
        "working-copy World Model edit recomputes immediately");
    Expect(active.Find("characters/player")->character.worldModelIdentity == "models/player.glb",
        "working-copy validation does not mutate active registry");
    editor::TryClearCharacterAnimationAsset(*workingPlayer, editor::CharacterAnimationSlot::Idle);
    editor::RefreshCharacterAssetValidation(
        editorState, workingPlayer->character, PLATFORMER_SOURCE_ASSET_ROOT);
    Expect(editorState.assetValidation.idle.status
            == animation::CharacterAnimationCompatibilityStatus::NoneEmbedded,
        "working-copy reusable assignment clear recomputes immediately");
    editor::TryAssignCharacterAnimationAsset(*workingPlayer,
        editor::CharacterAnimationSlot::Idle, "animations/humanoid_idle");
    editor::RefreshCharacterAssetValidation(
        editorState, workingPlayer->character, PLATFORMER_SOURCE_ASSET_ROOT);
    Expect(editorState.assetValidation.idle.status
            == animation::CharacterAnimationCompatibilityStatus::SkeletonIncompatible,
        "working-copy reusable assignment replacement recomputes immediately");

    CloseWindow();
    if (failures != 0) return 1;
    std::printf("Character asset validator tests passed.\n");
    return 0;
}
