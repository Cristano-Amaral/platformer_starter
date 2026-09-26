#pragma once

#include "animation/SkeletalAnimation.h"
#include "gameplay/GameplayDefinition.h"

#include <filesystem>
#include <string>
#include <vector>

namespace animation
{
enum class AnimationAssetStatus
{
    Resolved,
    MissingSourceAsset,
    MissingSourceClip,
    IncompatibleSkeleton,
    MalformedAuthoring,
};

const char* AnimationAssetStatusName(AnimationAssetStatus status);

struct AnimationCatalogEntry
{
    std::string identity;
    gameplay::AnimationDefinition definition{};
    AnimationAssetStatus status = AnimationAssetStatus::MalformedAuthoring;
};

class AnimationCatalog
{
public:
    void Refresh(const gameplay::GameplayDefinitionRegistry& registry,
        const std::filesystem::path& assetRoot);
    const std::vector<AnimationCatalogEntry>& Entries() const { return entries; }
    const AnimationCatalogEntry* Find(std::string_view identity) const;
    std::size_t Count() const { return entries.size(); }
private:
    std::vector<AnimationCatalogEntry> entries;
};

bool SkeletonsExactlyCompatible(const Skeleton& character, const Skeleton& source,
    std::string* error = nullptr);

struct ResolvedAnimationAsset
{
    AnimationAssetStatus status = AnimationAssetStatus::MalformedAuthoring;
    const gameplay::AnimationDefinition* definition = nullptr;
    const AnimationClip* clip = nullptr;
};

ResolvedAnimationAsset ResolveAnimationAsset(
    const gameplay::GameplayDefinitionRegistry& registry,
    std::string_view identity,
    const Skeleton& characterSkeleton,
    const Skeleton& sourceSkeleton,
    const std::vector<AnimationClip>& sourceClips,
    bool sourceAssetExists = true);
}
