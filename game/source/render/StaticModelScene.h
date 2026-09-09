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
    void Sync(const world::LevelDefinition& level);

    bool HasModel(std::string_view identity) const;
    bool IsFailed(std::string_view identity) const;
    bool TryGetLoadedLocalBounds(
        std::string_view identity,
        core::Vec3& localMin,
        core::Vec3& localMax) const;
    std::size_t UniqueLoadedCount() const;
    std::size_t CachedIdentityCount() const;

    void ResetDrawStats() const;
    std::size_t DrawSubmissionCount() const;
    bool SubmittedIdentity(std::string_view identity) const;
    std::vector<std::string> SubmittedIdentities() const;

    void DrawProp(const world::StaticPropSpec& spec) const;

private:
    struct GpuState;
    std::unique_ptr<GpuState> gpu;
};

// Restore default shader tint/texture after DrawModel so later rlBegin greybox
// draws in the same 3D pass are not left on the last material. Call at the
// start of each 3D world pass and after each prop. This does not restore
// clip planes; BeginMode3D already consumed those (see DrawWorld).
void RestoreGreyboxImmediateState();
}
