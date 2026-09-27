#include "editor/CharacterPreview.h"
#include "animation/HumanoidSkeletonMapping.h"
#include "editor/ItemDatabaseEditor.h"
#include "gameplay/GameplayDefinitionFile.h"
#include "render/CharacterPreviewRenderer.h"

#include <raylib.h>
#include <raymath.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string_view>

namespace
{
int failures = 0;
void Expect(bool condition, const char* name)
{
    if (!condition) { std::fprintf(stderr, "FAIL %s\n", name); ++failures; }
}

bool TransformDiffers(const Transform& a, const Transform& b)
{
    constexpr float epsilon = 0.00001f;
    return std::fabs(a.translation.x - b.translation.x) > epsilon
        || std::fabs(a.translation.y - b.translation.y) > epsilon
        || std::fabs(a.translation.z - b.translation.z) > epsilon
        || std::fabs(a.rotation.x - b.rotation.x) > epsilon
        || std::fabs(a.rotation.y - b.rotation.y) > epsilon
        || std::fabs(a.rotation.z - b.rotation.z) > epsilon
        || std::fabs(a.rotation.w - b.rotation.w) > epsilon
        || std::fabs(a.scale.x - b.scale.x) > epsilon
        || std::fabs(a.scale.y - b.scale.y) > epsilon
        || std::fabs(a.scale.z - b.scale.z) > epsilon;
}
}

int main()
{
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(64, 64, "CharacterPreviewTest");
    if (!IsWindowReady()) return 1;

    const auto parsed = gameplay::LoadGameplayDefinitionsFile(
        PLATFORMER_GAMEPLAY_DEFINITIONS_SOURCE_PATH);
    const auto* player = parsed.registry.Find("characters/player");
    Expect(player != nullptr, "canonical Player exists");
    if (player == nullptr) return 1;
    const auto validation = animation::ValidateCharacterAssets(
        parsed.registry, player->character, PLATFORMER_SOURCE_ASSET_ROOT);
    Expect(validation.model.status == animation::CharacterModelValidationStatus::Resolved
            && validation.model.modelLoaded,
        "canonical working-copy World Model is previewable");

    const auto idle = editor::ResolveCharacterPreviewAnimation(parsed.registry,
        player->character, validation, editor::CharacterPreviewSlot::Idle);
    const auto move = editor::ResolveCharacterPreviewAnimation(parsed.registry,
        player->character, validation, editor::CharacterPreviewSlot::Move);
    const auto jump = editor::ResolveCharacterPreviewAnimation(parsed.registry,
        player->character, validation, editor::CharacterPreviewSlot::Jump);
    Expect(idle.status == editor::CharacterPreviewAnimationStatus::Reusable
            && idle.animationIdentity == "animations/humanoid_idle"
            && idle.playbackMode == animation::PlaybackMode::Loop
            && validation.idle.retarget.status == animation::RetargetValidationStatus::NotRequested,
        "canonical Idle resolves compatible reusable Loop");
    Expect(move.status == editor::CharacterPreviewAnimationStatus::Reusable
            && move.animationIdentity == "animations/humanoid_move"
            && move.playbackMode == animation::PlaybackMode::Loop,
        "canonical Move resolves compatible reusable Loop");
    Expect(jump.status == editor::CharacterPreviewAnimationStatus::Reusable
            && jump.animationIdentity == "animations/humanoid_jump"
            && jump.playbackMode == animation::PlaybackMode::Clamp,
        "canonical Jump resolves compatible reusable Clamp");
    auto exactWorking = editor::CloneGameplayDefinitionRegistry(parsed.registry);
    exactWorking.FindMutable("animations/humanoid_idle")
        ->animation.sourceHumanoidMapping.joints[0] = "StaleName";
    const auto exactDespiteSourceMapping = animation::ValidateCharacterAssets(
        exactWorking, player->character, PLATFORMER_SOURCE_ASSET_ROOT);
    Expect(exactDespiteSourceMapping.idle.status
        == animation::CharacterAnimationCompatibilityStatus::Compatible
        && exactDespiteSourceMapping.idle.retarget.status
            == animation::RetargetValidationStatus::NotRequested,
        "exact-compatible Player bypasses source mapping and retargeting");

    gameplay::CharacterDefinition edited = player->character;
    edited.animations.idleAsset.clear();
    const auto embeddedValidation = animation::ValidateCharacterAssets(
        parsed.registry, edited, PLATFORMER_SOURCE_ASSET_ROOT);
    const auto embedded = editor::ResolveCharacterPreviewAnimation(parsed.registry,
        edited, embeddedValidation, editor::CharacterPreviewSlot::Idle);
    Expect(embedded.status == editor::CharacterPreviewAnimationStatus::Embedded
            && embedded.sourceAssetIdentity == "models/player.glb"
            && embedded.sourceClipName == "Idle",
        "genuine None uses authored embedded clip");

    edited = player->character;
    edited.animations.idleAsset = "animations/missing";
    const auto invalidValidation = animation::ValidateCharacterAssets(
        parsed.registry, edited, PLATFORMER_SOURCE_ASSET_ROOT);
    const auto invalid = editor::ResolveCharacterPreviewAnimation(parsed.registry,
        edited, invalidValidation, editor::CharacterPreviewSlot::Idle);
    Expect(invalid.status == editor::CharacterPreviewAnimationStatus::InvalidExplicit
            && !invalid.CanSample() && invalid.sourceClipName.empty(),
        "invalid explicit reusable assignment never falls back to embedded");

    edited = player->character;
    edited.worldModelIdentity = "models/humanoid_mapping_fixture.glb";
    const auto differentSkeleton = animation::ValidateCharacterAssets(
        parsed.registry, edited, PLATFORMER_SOURCE_ASSET_ROOT);
    animation::SuggestHumanoidMapping(edited.humanoidMapping, differentSkeleton.model);
    Expect(animation::ValidateHumanoidMapping(edited.humanoidMapping, differentSkeleton.model).state
        == animation::HumanoidMappingState::Usable, "test character has usable semantic mapping");
    const auto incompatibleMappedPreview = editor::ResolveCharacterPreviewAnimation(
        parsed.registry, edited, differentSkeleton, editor::CharacterPreviewSlot::Idle);
    Expect(differentSkeleton.idle.status
            == animation::CharacterAnimationCompatibilityStatus::SkeletonIncompatible
        && incompatibleMappedPreview.status == editor::CharacterPreviewAnimationStatus::InvalidExplicit
        && !incompatibleMappedPreview.CanSample(),
        "semantic mapping does not enable exact-incompatible reusable preview playback");

    edited = player->character;
    edited.worldModelIdentity = "models/test_static.glb";
    const auto staticValidation = animation::ValidateCharacterAssets(
        parsed.registry, edited, PLATFORMER_SOURCE_ASSET_ROOT);
    const auto staticPreview = editor::ResolveCharacterPreviewAnimation(parsed.registry,
        edited, staticValidation, editor::CharacterPreviewSlot::Idle);
    Expect(staticValidation.model.modelLoaded && !staticValidation.model.skinned
            && staticPreview.status == editor::CharacterPreviewAnimationStatus::Unavailable,
        "static geometry previewable but skeletal animation unavailable");

    edited.worldModelIdentity = "models/missing.glb";
    const auto missingValidation = animation::ValidateCharacterAssets(
        parsed.registry, edited, PLATFORMER_SOURCE_ASSET_ROOT);
    const auto missingPreview = editor::ResolveCharacterPreviewAnimation(parsed.registry,
        edited, missingValidation, editor::CharacterPreviewSlot::Idle);
    Expect(missingValidation.model.status == animation::CharacterModelValidationStatus::Missing
            && !missingPreview.CanSample(), "missing World Model clears animation safely");

    auto active = editor::CloneGameplayDefinitionRegistry(parsed.registry);
    auto working = editor::CloneGameplayDefinitionRegistry(parsed.registry);
    working.FindMutable("characters/player")->character.worldModelIdentity = "models/test_static.glb";
    working.FindMutable("characters/player")->character.animations.idleAsset.clear();
    Expect(active.Find("characters/player")->character.worldModelIdentity == "models/player.glb"
            && active.Find("characters/player")->character.animations.idleAsset
                == "animations/humanoid_idle",
        "working-copy preview edits do not mutate active registry");

    editor::CharacterPreviewPlayback playback;
    editor::AdvanceCharacterPreviewPlayback(playback, 2.5f, 1.0f, animation::PlaybackMode::Loop);
    Expect(std::fabs(playback.timeSeconds - 0.5f) < 0.0001f, "Loop playback wraps deterministically");
    editor::RestartCharacterPreviewPlayback(playback);
    editor::AdvanceCharacterPreviewPlayback(playback, 2.5f, 1.0f, animation::PlaybackMode::Clamp);
    Expect(std::fabs(playback.timeSeconds - 1.0f) < 0.0001f, "Clamp playback ends deterministically");
    playback.playing = false;
    editor::AdvanceCharacterPreviewPlayback(playback, 0.5f, 1.0f, animation::PlaybackMode::Loop);
    Expect(std::fabs(playback.timeSeconds - 1.0f) < 0.0001f, "paused playback remains stable");

    Model previewModel = LoadModel((std::filesystem::path(PLATFORMER_SOURCE_ASSET_ROOT)
        / "models/player.glb").string().c_str());
    int animationCount = 0;
    ModelAnimation* realAnimations = LoadModelAnimations(
        (std::filesystem::path(PLATFORMER_SOURCE_ASSET_ROOT)
            / "models/humanoid_animations.glb").string().c_str(), &animationCount);
    const auto findRealClip = [&](std::string_view name) -> ModelAnimation* {
        for (int index = 0; index < animationCount; ++index)
            if (name == realAnimations[index].name) return &realAnimations[index];
        return nullptr;
    };
    ModelAnimation* realIdle = findRealClip("Idle");
    ModelAnimation* realMove = findRealClip("Move");
    ModelAnimation* realJump = findRealClip("Jump");
    Expect(realIdle != nullptr && realMove != nullptr && realJump != nullptr,
        "canonical preview clips load through real Raylib path");
    if (realIdle != nullptr && previewModel.currentPose != nullptr)
    {
        float sampledFrame = -1.0f;
        animation::ApplyRaylibAnimationPose(previewModel, *realIdle, 0.0f,
            animation::PlaybackMode::Loop, &sampledFrame);
        const Transform idleStart = previewModel.currentPose[0];
        editor::CharacterPreviewPlayback visualPlayback;
        editor::AdvanceCharacterPreviewPlayback(visualPlayback, 0.25f, 1.0f,
            animation::PlaybackMode::Loop);
        animation::ApplyRaylibAnimationPose(previewModel, *realIdle, visualPlayback.timeSeconds,
            animation::PlaybackMode::Loop, &sampledFrame);
        const Transform idleAdvanced = previewModel.currentPose[0];
        Expect(sampledFrame > 0.0f && TransformDiffers(idleStart, idleAdvanced),
            "canonical Idle preview advances frame and observable pose");

        visualPlayback.playing = false;
        const float pausedTime = visualPlayback.timeSeconds;
        editor::AdvanceCharacterPreviewPlayback(visualPlayback, 0.2f, 1.0f,
            animation::PlaybackMode::Loop);
        animation::ApplyRaylibAnimationPose(previewModel, *realIdle, visualPlayback.timeSeconds,
            animation::PlaybackMode::Loop, &sampledFrame);
        Expect(visualPlayback.timeSeconds == pausedTime
                && !TransformDiffers(idleAdvanced, previewModel.currentPose[0]),
            "Pause freezes canonical sampled pose");
        visualPlayback.playing = true;
        editor::AdvanceCharacterPreviewPlayback(visualPlayback, 0.2f, 1.0f,
            animation::PlaybackMode::Loop);
        animation::ApplyRaylibAnimationPose(previewModel, *realIdle, visualPlayback.timeSeconds,
            animation::PlaybackMode::Loop, &sampledFrame);
        Expect(visualPlayback.timeSeconds > pausedTime
                && TransformDiffers(idleAdvanced, previewModel.currentPose[0]),
            "Play resumes canonical sampled pose progression");
        editor::RestartCharacterPreviewPlayback(visualPlayback);
        animation::ApplyRaylibAnimationPose(previewModel, *realIdle, visualPlayback.timeSeconds,
            animation::PlaybackMode::Loop, &sampledFrame);
        Expect(sampledFrame == 0.0f && !TransformDiffers(idleStart, previewModel.currentPose[0]),
            "Restart returns canonical preview to first pose");
        animation::ApplyRaylibAnimationPose(previewModel, *realIdle, 1.25f,
            animation::PlaybackMode::Loop, &sampledFrame);
        Expect(std::fabs(sampledFrame - 15.0f) < 0.001f,
            "real Idle Loop frame wraps deterministically");
    }
    if (realMove != nullptr && previewModel.currentPose != nullptr)
    {
        animation::ApplyRaylibAnimationPose(previewModel, *realMove, 0.0f,
            animation::PlaybackMode::Loop);
        const Transform start = previewModel.currentPose[0];
        animation::ApplyRaylibAnimationPose(previewModel, *realMove, 0.25f,
            animation::PlaybackMode::Loop);
        Expect(TransformDiffers(start, previewModel.currentPose[0]),
            "canonical Move preview advances observable pose");
    }
    if (realJump != nullptr && previewModel.currentPose != nullptr)
    {
        float finalFrame = -1.0f;
        animation::ApplyRaylibAnimationPose(previewModel, *realJump, 5.0f,
            animation::PlaybackMode::Clamp, &finalFrame);
        const Transform finalPose = previewModel.currentPose[0];
        float repeatedFrame = -1.0f;
        animation::ApplyRaylibAnimationPose(previewModel, *realJump, 9.0f,
            animation::PlaybackMode::Clamp, &repeatedFrame);
        Expect(finalFrame == 60.0f && repeatedFrame == finalFrame
                && !TransformDiffers(finalPose, previewModel.currentPose[0]),
            "canonical Jump Clamp remains on final frame and pose");
    }
    if (realAnimations != nullptr) UnloadModelAnimations(realAnimations, animationCount);
    UnloadModel(previewModel);

    render::CharacterPreviewRenderer previewRenderer;
    previewRenderer.SyncModel(player->character.worldModelIdentity,
        std::filesystem::path(PLATFORMER_SOURCE_ASSET_ROOT) / player->character.worldModelIdentity,
        validation.model);
    previewRenderer.SyncAnimation(idle, PLATFORMER_SOURCE_ASSET_ROOT);
    editor::StaticModelPreviewOrbit renderOrbit;
    editor::ResetStaticModelPreviewOrbit(renderOrbit, previewRenderer.Bounds());
    editor::CharacterPreviewPlayback renderPlayback;
    Expect(previewRenderer.Render(192, 192, renderOrbit, renderPlayback),
        "preview renderer draws canonical Idle frame zero");
    const double startSkinning = previewRenderer.RenderBoneMatrixChecksum();
    const std::uint64_t startPixels = previewRenderer.RenderedPixelChecksum();
    editor::AdvanceCharacterPreviewPlayback(renderPlayback, 0.25f,
        previewRenderer.AnimationDurationSeconds(), animation::PlaybackMode::Loop);
    Expect(previewRenderer.Render(192, 192, renderOrbit, renderPlayback),
        "preview renderer draws advanced canonical Idle");
    const double advancedSkinning = previewRenderer.RenderBoneMatrixChecksum();
    const std::uint64_t advancedPixels = previewRenderer.RenderedPixelChecksum();
    Expect(std::fabs(startSkinning - advancedSkinning) > 0.00001,
        "preview-owned rendered model receives changing bone matrices");
    Expect(startPixels != advancedPixels,
        "offscreen preview pixels reflect canonical Idle skinning");
    renderPlayback.playing = false;
    editor::AdvanceCharacterPreviewPlayback(renderPlayback, 0.2f,
        previewRenderer.AnimationDurationSeconds(), animation::PlaybackMode::Loop);
    previewRenderer.Render(192, 192, renderOrbit, renderPlayback);
    Expect(previewRenderer.RenderedPixelChecksum() == advancedPixels,
        "paused preview preserves rendered-model Idle state");
    renderPlayback.playing = true;
    editor::AdvanceCharacterPreviewPlayback(renderPlayback, 0.2f,
        previewRenderer.AnimationDurationSeconds(), animation::PlaybackMode::Loop);
    previewRenderer.Render(192, 192, renderOrbit, renderPlayback);
    Expect(previewRenderer.RenderedPixelChecksum() != advancedPixels,
        "resumed preview changes rendered-model Idle state");
    editor::RestartCharacterPreviewPlayback(renderPlayback);
    previewRenderer.Render(192, 192, renderOrbit, renderPlayback);
    Expect(previewRenderer.RenderedPixelChecksum() == startPixels,
        "Restart restores rendered-model Idle frame zero");

    previewRenderer.SyncAnimation(move, PLATFORMER_SOURCE_ASSET_ROOT);
    renderPlayback = {};
    previewRenderer.Render(192, 192, renderOrbit, renderPlayback);
    const std::uint64_t moveStartPixels = previewRenderer.RenderedPixelChecksum();
    editor::AdvanceCharacterPreviewPlayback(renderPlayback, 0.25f,
        previewRenderer.AnimationDurationSeconds(), animation::PlaybackMode::Loop);
    previewRenderer.Render(192, 192, renderOrbit, renderPlayback);
    Expect(previewRenderer.RenderedPixelChecksum() != moveStartPixels,
        "offscreen preview reflects canonical Move Loop skinning");

    previewRenderer.SyncAnimation(jump, PLATFORMER_SOURCE_ASSET_ROOT);
    renderPlayback = {};
    editor::AdvanceCharacterPreviewPlayback(renderPlayback, 5.0f,
        previewRenderer.AnimationDurationSeconds(), animation::PlaybackMode::Clamp);
    previewRenderer.Render(192, 192, renderOrbit, renderPlayback);
    const std::uint64_t jumpFinalPixels = previewRenderer.RenderedPixelChecksum();
    editor::AdvanceCharacterPreviewPlayback(renderPlayback, 4.0f,
        previewRenderer.AnimationDurationSeconds(), animation::PlaybackMode::Clamp);
    previewRenderer.Render(192, 192, renderOrbit, renderPlayback);
    Expect(previewRenderer.RenderedPixelChecksum() == jumpFinalPixels,
        "offscreen preview holds canonical Jump Clamp final skinning");
    previewRenderer.Shutdown();

    const auto* targetFixture = parsed.registry.Find("characters/retarget_target");
    const auto* sourceFixture = parsed.registry.Find("animations/retarget_move");
    Expect(targetFixture != nullptr && sourceFixture != nullptr, "M109 definitions exist");
    if (targetFixture != nullptr && sourceFixture != nullptr)
    {
        const auto fixtureValidation = animation::ValidateCharacterAssets(parsed.registry,
            targetFixture->character, PLATFORMER_SOURCE_ASSET_ROOT);
        const auto targetMapping = animation::ValidateHumanoidMapping(
            targetFixture->character.humanoidMapping, fixtureValidation.model);
        gameplay::CharacterDefinition sourceCharacter;
        sourceCharacter.worldModelIdentity = sourceFixture->animation.sourceAssetIdentity;
        const auto sourceAssets = animation::ValidateCharacterAssets(parsed.registry,
            sourceCharacter, PLATFORMER_SOURCE_ASSET_ROOT);
        const auto sourceMapping = animation::ValidateHumanoidMapping(
            sourceFixture->animation.sourceHumanoidMapping, sourceAssets.model);
        const auto fixturePreview = editor::ResolveCharacterPreviewAnimation(parsed.registry,
            targetFixture->character, fixtureValidation, editor::CharacterPreviewSlot::Idle);
        Expect(fixtureValidation.idle.status
            == animation::CharacterAnimationCompatibilityStatus::SkeletonIncompatible,
            "M109 pair stays M105 exact-incompatible");
        Expect(sourceMapping.state == animation::HumanoidMappingState::Usable
            && targetMapping.state == animation::HumanoidMappingState::Usable,
            "M109 source and target mappings usable");
        Expect(fixturePreview.status == editor::CharacterPreviewAnimationStatus::Retargeted
            && fixtureValidation.idle.retarget.status == animation::RetargetValidationStatus::Retargetable,
            "M109 pair resolves Retargeted without altering exact result");
        int sourceClipCount = 0;
        const std::filesystem::path sourceFixturePath =
            std::filesystem::path(PLATFORMER_SOURCE_ASSET_ROOT) / sourceFixture->animation.sourceAssetIdentity;
        ModelAnimation* sourceClips = LoadModelAnimations(sourceFixturePath.string().c_str(), &sourceClipCount);
        const ModelAnimation* realMoveClip = nullptr;
        for (int index = 0; sourceClips != nullptr && index < sourceClipCount; ++index)
            if (std::string_view(sourceClips[index].name) == "Move") realMoveClip = &sourceClips[index];
        Expect(realMoveClip != nullptr && realMoveClip->keyframeCount > 1
            && TransformDiffers(realMoveClip->keyframePoses[0][0],
                realMoveClip->keyframePoses[realMoveClip->keyframeCount / 2][0]),
            "production source clip has changing animated pose");
        if (sourceClips != nullptr) UnloadModelAnimations(sourceClips, sourceClipCount);
        if (fixturePreview.CanSample())
        {
            previewRenderer.SyncModel(targetFixture->character.worldModelIdentity,
                std::filesystem::path(PLATFORMER_SOURCE_ASSET_ROOT)
                    / targetFixture->character.worldModelIdentity, fixtureValidation.model);
            Expect(previewRenderer.LoadedIdentity() == targetFixture->character.worldModelIdentity,
                "offscreen renderer owns the skinned target rather than source model");
            previewRenderer.SyncAnimation(fixturePreview, PLATFORMER_SOURCE_ASSET_ROOT);
            editor::ResetStaticModelPreviewOrbit(renderOrbit, previewRenderer.Bounds());
            renderPlayback = {};
            Expect(previewRenderer.Render(192, 192, renderOrbit, renderPlayback),
                "retarget target offscreen frame zero rendered");
            const auto initialPixels = previewRenderer.RenderedPixelChecksum();
            const auto initialSkin = previewRenderer.RenderBoneMatrixChecksum();
            const auto initialHips = previewRenderer.CurrentJointTranslation(0);
            editor::AdvanceCharacterPreviewPlayback(renderPlayback, 0.25f,
                previewRenderer.AnimationDurationSeconds(), fixturePreview.playbackMode);
            Expect(previewRenderer.Render(192, 192, renderOrbit, renderPlayback),
                "retarget target advanced frame rendered");
            const auto animatedPixels = previewRenderer.RenderedPixelChecksum();
            const auto animatedHips = previewRenderer.CurrentJointTranslation(0);
            Expect(previewRenderer.LastSampledFrame() > 0.0f
                && std::fabs(previewRenderer.RenderBoneMatrixChecksum() - initialSkin) > 0.00001,
                "retarget target effective bone matrices advance");
            Expect(std::fabs(animatedHips.y - initialHips.y) > 0.00001f,
                "retargeted mapped target Hips pose advances");
            Expect(initialPixels != animatedPixels, "retargeted target skinned pixels change");
            renderPlayback.playing = false;
            editor::AdvanceCharacterPreviewPlayback(renderPlayback, 0.2f,
                previewRenderer.AnimationDurationSeconds(), fixturePreview.playbackMode);
            previewRenderer.Render(192, 192, renderOrbit, renderPlayback);
            Expect(previewRenderer.RenderedPixelChecksum() == animatedPixels,
                "retarget Pause preserves rendered pixels");
            renderPlayback.playing = true;
            editor::AdvanceCharacterPreviewPlayback(renderPlayback, 0.2f,
                previewRenderer.AnimationDurationSeconds(), fixturePreview.playbackMode);
            previewRenderer.Render(192, 192, renderOrbit, renderPlayback);
            Expect(previewRenderer.RenderedPixelChecksum() != animatedPixels,
                "retarget Play resumes rendered motion");
            editor::RestartCharacterPreviewPlayback(renderPlayback);
            previewRenderer.Render(192, 192, renderOrbit, renderPlayback);
            Expect(previewRenderer.RenderedPixelChecksum() == initialPixels,
                "retarget Restart restores rendered first frame");
            editor::AdvanceCharacterPreviewPlayback(renderPlayback,
                previewRenderer.AnimationDurationSeconds() + 0.25f,
                previewRenderer.AnimationDurationSeconds(), animation::PlaybackMode::Loop);
            previewRenderer.Render(192, 192, renderOrbit, renderPlayback);
            Expect(previewRenderer.RenderedPixelChecksum() == animatedPixels,
                "retarget Loop wraps rendered motion");
            const auto clampPreview = editor::ResolveCharacterPreviewAnimation(parsed.registry,
                targetFixture->character, fixtureValidation, editor::CharacterPreviewSlot::Jump);
            Expect(clampPreview.status == editor::CharacterPreviewAnimationStatus::Retargeted,
                "M109 Clamp clip retargetable");
            previewRenderer.SyncAnimation(clampPreview, PLATFORMER_SOURCE_ASSET_ROOT);
            renderPlayback = {};
            editor::AdvanceCharacterPreviewPlayback(renderPlayback, 5.0f,
                previewRenderer.AnimationDurationSeconds(), animation::PlaybackMode::Clamp);
            previewRenderer.Render(192, 192, renderOrbit, renderPlayback);
            const auto finalPixels = previewRenderer.RenderedPixelChecksum();
            editor::AdvanceCharacterPreviewPlayback(renderPlayback, 3.0f,
                previewRenderer.AnimationDurationSeconds(), animation::PlaybackMode::Clamp);
            previewRenderer.Render(192, 192, renderOrbit, renderPlayback);
            Expect(previewRenderer.RenderedPixelChecksum() == finalPixels,
                "retarget Clamp preserves rendered endpoint");
        }
        gameplay::GameplayDefinitionRegistry fixtureWorking = editor::CloneGameplayDefinitionRegistry(parsed.registry);
        auto* mutableTarget = fixtureWorking.FindMutable("characters/retarget_target");
        mutableTarget->character.humanoidMapping.joints[0].clear();
        const auto missingTarget = animation::ValidateCharacterAssets(fixtureWorking,
            mutableTarget->character, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(missingTarget.idle.retarget.status == animation::RetargetValidationStatus::TargetMappingMissing,
            "required target role invalidates retargeting");
        mutableTarget->character.humanoidMapping.joints[0] = "TargetHips";
        auto* mutableSource = fixtureWorking.FindMutable("animations/retarget_move");
        mutableSource->animation.sourceHumanoidMapping.joints[0].clear();
        const auto missingSource = animation::ValidateCharacterAssets(fixtureWorking,
            mutableTarget->character, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(missingSource.idle.retarget.status == animation::RetargetValidationStatus::SourceMappingMissing,
            "required source role invalidates retargeting");
        mutableSource->animation.sourceHumanoidMapping.joints[0] = "Hips";
        mutableSource->animation.sourceHumanoidMapping.joints[0] = "StaleHips";
        const auto staleSource = animation::ValidateCharacterAssets(fixtureWorking,
            mutableTarget->character, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(staleSource.idle.retarget.status == animation::RetargetValidationStatus::InvalidMapping
            && staleSource.idle.retarget.detail.find("Stale/missing") != std::string::npos,
            "stale source joint blocks retargeting with diagnostic");
        mutableSource->animation.sourceHumanoidMapping.joints[0] = "Hips";
        mutableTarget->character.humanoidMapping.joints[0] = "TargetSpine";
        const auto duplicateTarget = animation::ValidateCharacterAssets(fixtureWorking,
            mutableTarget->character, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(duplicateTarget.idle.retarget.status == animation::RetargetValidationStatus::InvalidMapping,
            "duplicate target assignment blocks retargeting");
        mutableTarget->character.humanoidMapping.joints[0] = "TargetHips";
        const auto recovered = animation::ValidateCharacterAssets(fixtureWorking,
            mutableTarget->character, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(recovered.idle.retarget.status == animation::RetargetValidationStatus::Retargetable,
            "restoring source and target role recovers retargeting");
    }
    previewRenderer.Shutdown();

    // Raylib glTF poses are global. This case has different names, indices,
    // rest orientations and proportions, so absolute quaternion copying fails.
    BoneInfo sourceBones[3]{};
    BoneInfo targetBones[4]{};
    std::memcpy(sourceBones[0].name, "SourceHand", sizeof("SourceHand")); sourceBones[0].parent = 1;
    std::memcpy(sourceBones[1].name, "SourceHips", sizeof("SourceHips")); sourceBones[1].parent = -1;
    std::memcpy(sourceBones[2].name, "SourceChest", sizeof("SourceChest")); sourceBones[2].parent = 1;
    std::memcpy(targetBones[0].name, "TargetHips", sizeof("TargetHips")); targetBones[0].parent = -1;
    std::memcpy(targetBones[1].name, "TargetHand", sizeof("TargetHand")); targetBones[1].parent = 3;
    std::memcpy(targetBones[2].name, "TargetChest", sizeof("TargetChest")); targetBones[2].parent = 0;
    std::memcpy(targetBones[3].name, "TargetArmLink", sizeof("TargetArmLink")); targetBones[3].parent = 0;
    const auto dot = [](::Quaternion a, ::Quaternion b) {
        return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
    };
    const ::Quaternion sourceYaw = QuaternionFromAxisAngle({0, 1, 0}, PI / 4);
    const ::Quaternion targetYaw = QuaternionFromAxisAngle({0, 1, 0}, PI / 2);
    const ::Quaternion handMotion = QuaternionFromAxisAngle({0, 0, 1}, PI / 3);
    const ::Quaternion chestMotion = QuaternionFromAxisAngle({1, 0, 0}, PI / 6);
    Transform sourceRest[3] = {
        {{1, 0, 0}, sourceYaw, {1, 1, 1}},
        {{0, 0, 0}, sourceYaw, {1, 1, 1}},
        {{0, 1, 0}, sourceYaw, {1, 1, 1}}};
    Transform targetRest[4] = {
        {{0, 0, 0}, targetYaw, {1, 1, 1}},
        {{0, 2, 0}, targetYaw, {2, 2, 2}},
        {{0, 3, 0}, targetYaw, {1, 1, 1}},
        {{0, 1, 0}, targetYaw, {1, 1, 1}}};
    Transform sourceFrame0[3] = {sourceRest[0], sourceRest[1], sourceRest[2]};
    Transform sourceFrame1[3] = {
        {{5, 0, 0}, QuaternionMultiply(sourceYaw, handMotion), {3, 3, 3}},
        sourceRest[1],
        {{0, 1, 0}, QuaternionMultiply(sourceYaw, chestMotion), {1, 1, 1}}};
    Transform* frames[2] = {sourceFrame0, sourceFrame1};
    Transform targetCurrent[4]{};
    Matrix targetMatrices[4]{};
    Model sourceMath{};
    sourceMath.skeleton = {3, sourceBones, sourceRest};
    Model targetMath{};
    targetMath.skeleton = {4, targetBones, targetRest};
    targetMath.currentPose = targetCurrent;
    targetMath.boneMatrices = targetMatrices;
    ModelAnimation mathClip{};
    mathClip.boneCount = 3;
    mathClip.keyframeCount = 2;
    mathClip.keyframePoses = frames;
    animation::RetargetValidationResult mathMapping;
    mathMapping.status = animation::RetargetValidationStatus::Retargetable;
    mathMapping.sourceJoints.fill(-1); mathMapping.targetJoints.fill(-1);
    const auto hipsRole = static_cast<std::size_t>(gameplay::HumanoidJointRole::Hips);
    const auto handRole = static_cast<std::size_t>(gameplay::HumanoidJointRole::LeftHand);
    const auto chestRole = static_cast<std::size_t>(gameplay::HumanoidJointRole::Chest);
    mathMapping.sourceJoints[hipsRole] = 1; mathMapping.targetJoints[hipsRole] = 0;
    mathMapping.sourceJoints[handRole] = 0; mathMapping.targetJoints[handRole] = 1;
    std::vector<unsigned char> mathScratch;
    Expect(animation::ApplyRaylibRetargetedPose(targetMath, sourceMath, mathClip, mathMapping,
        1.0f / 60.0f, animation::PlaybackMode::Clamp, mathScratch), "math retarget samples differing indices");
    const ::Quaternion expectedHand = QuaternionMultiply(targetYaw, handMotion);
    const float handDot = std::fabs(dot(targetCurrent[1].rotation, expectedHand));
    Expect(handDot > 0.999f && std::fabs(dot(targetCurrent[1].rotation,
        sourceFrame1[0].rotation)) < 0.999f,
        "rest-relative motion uses target rest orientation rather than source absolute rotation");
    Expect(std::fabs(targetCurrent[1].translation.y - 2.0f) < 0.001f
        && std::fabs(targetCurrent[1].scale.x - 2.0f) < 0.001f,
        "non-root target rest translation and scale survive source changes");
    Expect(std::fabs(dot(targetCurrent[2].rotation, targetYaw)) > 0.999f,
        "unmapped optional target joint stays at target rest");
    Expect(std::fabs(targetCurrent[3].translation.y - 1.0f) < 0.001f
        && std::fabs(dot(targetCurrent[3].rotation, targetYaw)) > 0.999f,
        "unmapped intermediate target joint remains locally at rest");
    mathMapping.sourceJoints[chestRole] = 2; mathMapping.targetJoints[chestRole] = 2;
    Expect(animation::ApplyRaylibRetargetedPose(targetMath, sourceMath, mathClip, mathMapping,
        1.0f / 60.0f, animation::PlaybackMode::Clamp, mathScratch), "mapped optional role samples");
    const ::Quaternion expectedChest = QuaternionMultiply(targetYaw, chestMotion);
    Expect(std::fabs(dot(targetCurrent[2].rotation, expectedChest)) > 0.999f,
        "optional role applies when mapped on both sides");
    mathMapping.sourceJoints[chestRole] = -1;
    animation::ApplyRaylibRetargetedPose(targetMath, sourceMath, mathClip, mathMapping,
        1.0f / 60.0f, animation::PlaybackMode::Clamp, mathScratch);
    Expect(std::fabs(dot(targetCurrent[2].rotation, targetYaw)) > 0.999f,
        "optional role omitted when source assignment is absent");
    sourceFrame1[1].translation = {0, 1, 0};
    animation::ApplyRaylibRetargetedPose(targetMath, sourceMath, mathClip, mathMapping,
        1.0f / 60.0f, animation::PlaybackMode::Clamp, mathScratch);
    Expect(std::fabs(targetCurrent[0].translation.y - 1.0f) < 0.001f
        && std::fabs(targetCurrent[1].translation.y - 3.0f) < 0.001f
        && std::fabs(targetCurrent[2].translation.y - 4.0f) < 0.001f
        && std::fabs(targetCurrent[3].translation.y - 2.0f) < 0.001f,
        "root Hips delta moves descendants while their local rest translations remain intact");

    CloseWindow();
    if (failures != 0) return 1;
    std::printf("Character preview tests passed.\n");
    return 0;
}
