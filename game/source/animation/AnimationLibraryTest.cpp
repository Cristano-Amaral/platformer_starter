#include "animation/AnimationLibrary.h"
#include "gameplay/GameplayDefinitionFile.h"

#include <cstdio>
#include <filesystem>

namespace { int failures = 0; void Expect(bool value, const char* name) { if (!value) { ++failures; std::fprintf(stderr, "FAIL %s\n", name); } } }

int main()
{
    Expect(gameplay::IsValidAnimationIdentity("animations/humanoid_idle"), "valid identity");
    Expect(!gameplay::IsValidAnimationIdentity("characters/humanoid_idle"), "category-bound identity");
    const auto parsed = gameplay::ParseGameplayDefinitionsText(
        "PLATFORMER_GAMEPLAY_DEFINITIONS\n"
        "definition animations/walk\nsource_asset \"models/walk.glb\"\nsource_clip \"Walk\"\nplayback Loop\n"
        "definition characters/test\nanimation_idle_asset \"animations/walk\"\n");
    Expect(parsed.status == gameplay::LoadGameplayDefinitionsStatus::Loaded, "typed parse");
    const auto written = gameplay::WriteGameplayDefinitionsText(parsed.registry);
    const auto roundTrip = gameplay::ParseGameplayDefinitionsText(written.text);
    Expect(written.ok && roundTrip.status == gameplay::LoadGameplayDefinitionsStatus::Loaded, "round trip");
    Expect(gameplay::ParseGameplayDefinitionsText(
        "PLATFORMER_GAMEPLAY_DEFINITIONS\ndefinition animations/x\nsource_asset \"models/x.glb\"\nsource_clip \"X\"\n"
        "definition animations/x\nsource_asset \"models/x.glb\"\nsource_clip \"X\"\n").status
        == gameplay::LoadGameplayDefinitionsStatus::Invalid, "duplicate rejected");
    Expect(gameplay::ParseGameplayDefinitionsText(
        "PLATFORMER_GAMEPLAY_DEFINITIONS\ndefinition animations/x\nsource_asset \"models/x.glb\"\n").status
        == gameplay::LoadGameplayDefinitionsStatus::Invalid, "malformed authoring rejected");

    const auto canonical = gameplay::LoadGameplayDefinitionsFile(
        PLATFORMER_GAMEPLAY_DEFINITIONS_SOURCE_PATH);
    animation::AnimationCatalog catalog;
    catalog.Refresh(canonical.registry, PLATFORMER_SOURCE_ASSET_ROOT);
    Expect(catalog.Count() == 3, "catalog discovers canonical animations");
    const auto* canonicalIdle = catalog.Find("animations/humanoid_idle");
    Expect(canonicalIdle != nullptr
            && canonicalIdle->status == animation::AnimationAssetStatus::Resolved,
        "catalog resolves canonical source asset");
    animation::AnimationCatalog missingCatalog;
    missingCatalog.Refresh(parsed.registry, PLATFORMER_SOURCE_ASSET_ROOT);
    const auto* missingEntry = missingCatalog.Find("animations/walk");
    Expect(missingEntry != nullptr
            && missingEntry->status == animation::AnimationAssetStatus::MissingSourceAsset,
        "catalog diagnoses missing source asset");

    animation::Skeleton skeleton;
    skeleton.joints.push_back({"Root", -1});
    animation::AnimationClip clip; clip.name = "Walk"; clip.durationSeconds = 1.0f; clip.joints.resize(1);
    const std::vector<animation::AnimationClip> clips{clip};
    auto resolved = animation::ResolveAnimationAsset(parsed.registry, "animations/walk", skeleton,
        skeleton, clips, true);
    Expect(resolved.status == animation::AnimationAssetStatus::Resolved, "compatible resolved");
    std::vector<animation::JointTransform> pose;
    Expect(animation::SampleClip(skeleton, *resolved.clip, 1.5f,
        resolved.definition->playbackMode, pose), "reusable sampling");
    auto missingSource = animation::ResolveAnimationAsset(parsed.registry, "animations/walk", skeleton,
        skeleton, {clip}, false);
    Expect(missingSource.status == animation::AnimationAssetStatus::MissingSourceAsset, "missing source");
    auto missingClip = animation::ResolveAnimationAsset(parsed.registry, "animations/walk", skeleton,
        skeleton, {}, true);
    Expect(missingClip.status == animation::AnimationAssetStatus::MissingSourceClip, "missing clip");
    animation::Skeleton incompatible = skeleton; incompatible.joints[0].name = "Other";
    auto badRig = animation::ResolveAnimationAsset(parsed.registry, "animations/walk", skeleton,
        incompatible, {clip}, true);
    Expect(badRig.status == animation::AnimationAssetStatus::IncompatibleSkeleton, "incompatible rejected");
    Expect(animation::ResolveAnimationAsset(parsed.registry, "animations/missing", skeleton,
        skeleton, clips, true).status == animation::AnimationAssetStatus::MalformedAuthoring,
        "missing reusable identity is safe");
    Expect(animation::ResolvePlaybackTime(1.5f, 1.0f, animation::PlaybackMode::Loop) == 0.5f,
        "loop behavior");
    Expect(animation::ResolvePlaybackTime(1.5f, 1.0f, animation::PlaybackMode::Clamp) == 1.0f,
        "clamp behavior");
    if (failures) return 1;
    std::printf("Animation library tests passed.\n"); return 0;
}
