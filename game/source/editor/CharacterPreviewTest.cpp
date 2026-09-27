#include "editor/CharacterPreview.h"
#include "animation/HumanoidSkeletonMapping.h"
#include "editor/ItemDatabaseEditor.h"
#include "gameplay/GameplayDefinitionFile.h"
#include "render/CharacterPreviewRenderer.h"

#include <raylib.h>

#include <cmath>
#include <cstdio>
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
            && idle.playbackMode == animation::PlaybackMode::Loop,
        "canonical Idle resolves compatible reusable Loop");
    Expect(move.status == editor::CharacterPreviewAnimationStatus::Reusable
            && move.animationIdentity == "animations/humanoid_move"
            && move.playbackMode == animation::PlaybackMode::Loop,
        "canonical Move resolves compatible reusable Loop");
    Expect(jump.status == editor::CharacterPreviewAnimationStatus::Reusable
            && jump.animationIdentity == "animations/humanoid_jump"
            && jump.playbackMode == animation::PlaybackMode::Clamp,
        "canonical Jump resolves compatible reusable Clamp");

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

    CloseWindow();
    if (failures != 0) return 1;
    std::printf("Character preview tests passed.\n");
    return 0;
}
