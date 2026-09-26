#include "animation/AnimationLibrary.h"

#include <algorithm>
#include <system_error>

namespace animation
{
const char* AnimationAssetStatusName(AnimationAssetStatus status)
{
    switch (status)
    {
    case AnimationAssetStatus::Resolved: return "Resolved";
    case AnimationAssetStatus::MissingSourceAsset: return "Missing source asset";
    case AnimationAssetStatus::MissingSourceClip: return "Missing source clip";
    case AnimationAssetStatus::IncompatibleSkeleton: return "Incompatible skeleton";
    case AnimationAssetStatus::MalformedAuthoring: return "Malformed authoring";
    }
    return "Malformed authoring";
}

void AnimationCatalog::Refresh(const gameplay::GameplayDefinitionRegistry& registry,
    const std::filesystem::path& assetRoot)
{
    entries.clear();
    for (const auto& authored : registry.Definitions())
    {
        if (authored.category != gameplay::GameplayDefinitionCategory::Animation) continue;
        AnimationCatalogEntry entry{authored.identity, authored.animation};
        if (!gameplay::IsValidAnimationIdentity(authored.identity)
            || !gameplay::ValidateAnimationDefinition(authored.animation))
            entry.status = AnimationAssetStatus::MalformedAuthoring;
        else
        {
            std::error_code error;
            const auto source = (assetRoot / authored.animation.sourceAssetIdentity).lexically_normal();
            entry.status = std::filesystem::is_regular_file(source, error) && !error
                ? AnimationAssetStatus::Resolved : AnimationAssetStatus::MissingSourceAsset;
        }
        entries.push_back(std::move(entry));
    }
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        return a.identity < b.identity;
    });
}

const AnimationCatalogEntry* AnimationCatalog::Find(std::string_view identity) const
{
    const auto found = std::lower_bound(entries.begin(), entries.end(), identity,
        [](const AnimationCatalogEntry& entry, std::string_view value) { return entry.identity < value; });
    return found != entries.end() && found->identity == identity ? &*found : nullptr;
}

bool SkeletonsExactlyCompatible(const Skeleton& character, const Skeleton& source, std::string* error)
{
    if (!ValidateSkeleton(character, error) || !ValidateSkeleton(source, error)) return false;
    if (character.joints.size() != source.joints.size())
    { if (error) *error = "joint count differs"; return false; }
    for (std::size_t index = 0; index < character.joints.size(); ++index)
    {
        if (character.joints[index].name != source.joints[index].name
            || character.joints[index].parent != source.joints[index].parent)
        { if (error) *error = "joint name/order/hierarchy differs"; return false; }
    }
    return true;
}

ResolvedAnimationAsset ResolveAnimationAsset(const gameplay::GameplayDefinitionRegistry& registry,
    std::string_view identity, const Skeleton& characterSkeleton, const Skeleton& sourceSkeleton,
    const std::vector<AnimationClip>& sourceClips, bool sourceAssetExists)
{
    ResolvedAnimationAsset result;
    gameplay::GameplayDefinitionReference reference{std::string(identity)};
    const auto resolution = registry.Resolve(reference, gameplay::GameplayDefinitionCategory::Animation);
    if (resolution.status != gameplay::GameplayReferenceStatus::Resolved || resolution.definition == nullptr
        || !gameplay::ValidateAnimationDefinition(resolution.definition->animation))
        return result;
    result.definition = &resolution.definition->animation;
    if (!sourceAssetExists) { result.status = AnimationAssetStatus::MissingSourceAsset; return result; }
    result.clip = FindClip(sourceClips, result.definition->sourceClipName);
    if (result.clip == nullptr) { result.status = AnimationAssetStatus::MissingSourceClip; return result; }
    if (!SkeletonsExactlyCompatible(characterSkeleton, sourceSkeleton))
    { result.clip = nullptr; result.status = AnimationAssetStatus::IncompatibleSkeleton; return result; }
    result.status = AnimationAssetStatus::Resolved;
    return result;
}
}
