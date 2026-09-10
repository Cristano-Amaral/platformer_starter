#pragma once

// Runtime/editor scene cache: canonical static-model identity -> one loaded
// Model. Independent of thumbnail PNG cache and Model Preview.

#include "core/Vec3.h"
#include "world/LevelDefinition.h"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace render
{
class StaticModelSceneStore
{
public:
    StaticModelSceneStore();
    ~StaticModelSceneStore();

    StaticModelSceneStore(const StaticModelSceneStore&) = delete;
    StaticModelSceneStore& operator=(const StaticModelSceneStore&) = delete;

    void Shutdown();
    void Sync(
        const world::LevelDefinition& level,
        std::string_view extraIdentity = {},
        const world::LevelDefinition* extraLevel = nullptr);

    bool HasModel(std::string_view identity) const;
    bool IsFailed(std::string_view identity) const;
    bool TryGetLoadedLocalBounds(
        std::string_view identity,
        core::Vec3& localMin,
        core::Vec3& localMax) const;
    std::size_t UniqueLoadedCount() const;
    std::size_t CachedIdentityCount() const;
    std::size_t LoadCount() const;

    void ResetDrawStats() const;
    std::size_t DrawSubmissionCount() const;
    bool SubmittedIdentity(std::string_view identity) const;
    std::vector<std::string> SubmittedIdentities() const;

    void DrawProp(const world::StaticPropSpec& spec) const;
    void DrawPlacementPreview(const world::StaticPropSpec& spec) const;
    // Editor-only second pass of the same cached model. Does not LoadModel,
    // mutate materials, or add a persistent scene object.
    void DrawSelectionHighlight(const world::StaticPropSpec& spec) const;
    // Gameplay/Release Item Pickup target tint. Same cached model as DrawProp.
    // Depth-respecting golden pass only; no editor x-ray and no LoadModel.
    // alpha 0 skips the extra pass without changing renderer state.
    void DrawGameplayTargetHighlight(
        const world::StaticPropSpec& spec,
        unsigned char alpha = 180) const;
    std::size_t HighlightSubmissionCount() const;
    std::size_t GameplayHighlightSubmissionCount() const;

private:
    void DrawPropTinted(
        const world::StaticPropSpec& spec,
        unsigned char red,
        unsigned char green,
        unsigned char blue,
        unsigned char alpha) const;

    struct GpuState;
    std::unique_ptr<GpuState> gpu;
};

// Restore default shader tint/texture after DrawModel so later rlBegin greybox
// draws in the same 3D pass are not left on the last material. Call at the
// start of each 3D world pass and after each prop. This does not restore
// clip planes; BeginMode3D already consumed those (see DrawWorld).
void RestoreGreyboxImmediateState();
// Restore depth/cull/shader after an editor selection-highlight pass.
void RestoreEditorModelHighlightState();
}
