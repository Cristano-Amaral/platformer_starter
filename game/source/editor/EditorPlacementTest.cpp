#include "editor/EditorPlacement.h"

#include "editor/AuthoredObjectLifecycle.h"
#include "editor/EditorPicking.h"
#include "editor/EditorWorkspace.h"
#include "editor/LevelEditor.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace
{
int gFailures = 0;

void Expect(bool condition, const std::string& name)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL %s\n", name.c_str());
        ++gFailures;
    }
}

bool NearlyEqual(float a, float b)
{
    return std::fabs(a - b) <= 0.0001f;
}

bool Vec3Near(core::Vec3 a, core::Vec3 b)
{
    return NearlyEqual(a.x, b.x) && NearlyEqual(a.y, b.y) && NearlyEqual(a.z, b.z);
}

world::LevelDefinition MakeActiveLevel()
{
    world::LevelDefinition level{};
    level.id = "level_01";
    level.initialSpawnVisualCenter = {0.0f, 0.8f, 0.0f};
    level.ground = {{0.0f, -0.25f, 0.0f}, {20.0f, 0.5f, 8.0f}};
    level.elevatedPlatforms.push_back({{4.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
    level.checkpoints.push_back({{16.5f, 1.8f, 0.0f}, {2.4f, 1.6f, 2.0f}, {16.5f, 1.8f, 0.0f}});
    level.hazards.push_back({{11.5f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
    level.collectibles.push_back({{5.0f, 2.5f, 0.0f}, {1.0f, 1.2f, 1.0f}});
    level.camera = {{2.0f, 3.5f, 12.0f}, 40.0f};
    return level;
}
}

int main()
{
    using editor::EditorObjectKind;
    using editor::PlacementMode;

    {
        editor::EditorWorkspaceState workspace{};
        Expect(workspace.showObjectPalette, "Object Palette default visible");
        Expect(editor::AllEditorPanelsVisible(workspace), "defaults include Object Palette");
        workspace.showObjectPalette = false;
        Expect(!editor::AllEditorPanelsVisible(workspace), "hiding Object Palette is independent");
        editor::ResetEditorWorkspaceVisibility(workspace);
        Expect(workspace.showObjectPalette, "Reset restores Object Palette");
        Expect(editor::AllEditorPanelsVisible(workspace), "Reset restores all default panels");
    }

    {
        PlacementMode mode = PlacementMode::None;
        Expect(!editor::PlacementModeIsActive(mode), "idle is not placing");
        editor::ApplyPaletteCategoryClick(mode, PlacementMode::Collectible);
        Expect(mode == PlacementMode::Collectible, "click Collectible enters placement");
        editor::ApplyPaletteCategoryClick(mode, PlacementMode::Collectible);
        Expect(mode == PlacementMode::None, "click Collectible again exits placement");
        Expect(!editor::PaletteCategoryIsActive(mode, PlacementMode::Collectible),
            "None has no active category");
        editor::ApplyPaletteCategoryClick(mode, PlacementMode::Platform);
        Expect(mode == PlacementMode::Platform, "click Platform enters placement");
        Expect(
            editor::PaletteCategoryIsActive(mode, PlacementMode::Platform),
            "active category is Platform");
        Expect(
            !editor::PaletteCategoryIsActive(mode, PlacementMode::Hazard),
            "other categories are not active");
        editor::ApplyPaletteCategoryClick(mode, PlacementMode::Hazard);
        Expect(mode == PlacementMode::Hazard, "click Hazard switches from Platform");
        Expect(
            editor::KindFromPlacementMode(mode) == EditorObjectKind::Hazard,
            "Hazard kind from mode");
        Expect(
            std::strcmp(editor::PlacementStopHintText(), "Esc or click again to stop") == 0,
            "stop hint text");
        Expect(
            std::strcmp(editor::PlacementViewportActionHintText(), "LMB place | Esc cancel") == 0,
            "viewport action hint");
        editor::SetPlacementMode(mode, PlacementMode::Collectible);
        Expect(mode == PlacementMode::Collectible, "forced set still available");
        editor::ClearPlacementMode(mode);
        Expect(mode == PlacementMode::None, "Esc/clear exits placement");
        Expect(!editor::PlacementModeIsActive(mode), "cleared mode is idle");
    }

    {
        Expect(
            editor::ShouldCancelPlacementMode(PlacementMode::Hazard, true, false),
            "Esc cancels placement");
        Expect(
            !editor::ShouldCancelPlacementMode(PlacementMode::Hazard, true, true),
            "Esc ignored while ImGui wants keyboard");
        Expect(
            !editor::ShouldCancelPlacementMode(PlacementMode::None, true, false),
            "Esc does nothing when idle");
        Expect(
            !editor::ShouldCancelPlacementMode(PlacementMode::Platform, false, false),
            "no Esc keeps mode");
    }

    {
        PlacementMode mode = PlacementMode::Checkpoint;
        editor::ClearPlacementMode(mode);
        Expect(mode == PlacementMode::None, "F2/editor-close cancellation helper");
    }

    {
        editor::EditorPickingSet surfaces{};
        surfaces.proxies.push_back(
            {{EditorObjectKind::Ground, 0}, {0.0f, -0.25f, 0.0f}, {20.0f, 0.5f, 8.0f}, 0.0f});
        const editor::Ray3 ray{{0.0f, 10.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
        const editor::PlacementSurfaceHit hit = editor::FindPlacementSurfaceHit(ray, surfaces);
        Expect(hit.hit, "ground is an eligible surface");
        Expect(NearlyEqual(hit.point.y, 0.0f), "hit is the top of the ground AABB");
        const editor::PlacementCandidate candidate = editor::ResolvePlacementCandidate(
            PlacementMode::Platform, ray, surfaces, {99.0f, 99.0f, 99.0f});
        Expect(
            candidate.visible && candidate.source == editor::PlacementCandidateSource::SurfaceHit,
            "surface-hit candidate");
        Expect(
            Vec3Near(
                candidate.center,
                {hit.point.x,
                 hit.point.y + editor::kDefaultAddedPlatformSize.y * 0.5f,
                 hit.point.z}),
            "Platform sits on the hit with authored-center offset");
        Expect(Vec3Near(candidate.size, editor::kDefaultAddedPlatformSize), "Platform default size");
    }

    {
        editor::EditorPickingSet empty{};
        const editor::Ray3 miss{{0.0f, 10.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
        const core::Vec3 fallback{7.0f, 4.0f, -3.0f};
        const editor::PlacementCandidate candidate = editor::ResolvePlacementCandidate(
            PlacementMode::Collectible, miss, empty, fallback);
        Expect(
            candidate.visible && candidate.source == editor::PlacementCandidateSource::CameraFallback,
            "fallback candidate when no surface");
        Expect(Vec3Near(candidate.center, fallback), "fallback uses camera-region point, not Spawn");
        Expect(
            Vec3Near(candidate.size, editor::kDefaultAddedCollectibleSize),
            "fallback Collectible uses M41 size");
    }

    {
        editor::EditorPickingSet mixed{};
        mixed.proxies.push_back(
            {{EditorObjectKind::Checkpoint, 0}, {0.0f, 1.8f, 0.0f}, {2.4f, 1.6f, 2.0f}, 0.0f});
        mixed.proxies.push_back(
            {{EditorObjectKind::ElevatedPlatform, 0}, {0.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}, 0.0f});
        const editor::Ray3 ray{{0.0f, 10.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
        Expect(
            !editor::IsEligiblePlacementSurface(EditorObjectKind::Checkpoint),
            "checkpoints are not placement surfaces");
        Expect(
            editor::IsEligiblePlacementSurface(EditorObjectKind::ElevatedPlatform),
            "elevated platforms are placement surfaces");
        Expect(
            editor::IsEligiblePlacementSurface(EditorObjectKind::Slope),
            "slopes are placement surfaces");
        const editor::PlacementSurfaceHit hit = editor::FindPlacementSurfaceHit(ray, mixed);
        Expect(hit.hit && NearlyEqual(hit.point.x, 0.0f), "nearest eligible surface is the platform");
        Expect(hit.point.y > 0.5f, "hit uses platform top, not checkpoint");
    }

    {
        const core::Vec3 center{3.0f, 2.0f, -1.0f};
        const editor::PlacementCandidate platform =
            editor::MakePlacementCandidate(PlacementMode::Platform, center);
        Expect(platform.kind == EditorObjectKind::ElevatedPlatform, "Platform candidate kind");
        Expect(Vec3Near(platform.center, center), "Platform candidate center");

        const editor::PlacementCandidate checkpoint =
            editor::MakePlacementCandidate(PlacementMode::Checkpoint, center);
        Expect(checkpoint.kind == EditorObjectKind::Checkpoint, "Checkpoint candidate kind");
        Expect(Vec3Near(checkpoint.checkpoint.center, center), "Checkpoint trigger center");
        Expect(
            Vec3Near(
                checkpoint.checkpoint.respawnPosition,
                {center.x + editor::kDefaultAddedCheckpointRespawnOffset.x,
                 center.y + editor::kDefaultAddedCheckpointRespawnOffset.y,
                 center.z + editor::kDefaultAddedCheckpointRespawnOffset.z}),
            "Checkpoint respawn follows default assembly offset");
        Expect(
            Vec3Near(checkpoint.checkpoint.size, editor::kDefaultAddedCheckpointSize),
            "Checkpoint default size");

        const editor::PlacementCandidate hazard =
            editor::MakePlacementCandidate(PlacementMode::Hazard, center);
        Expect(hazard.kind == EditorObjectKind::Hazard, "Hazard candidate kind");
        Expect(Vec3Near(hazard.hazard.center, center), "Hazard candidate center");
        Expect(Vec3Near(hazard.hazard.size, editor::kDefaultAddedHazardSize), "Hazard default size");

        const editor::PlacementCandidate collectible =
            editor::MakePlacementCandidate(PlacementMode::Collectible, center);
        Expect(collectible.kind == EditorObjectKind::Collectible, "Collectible candidate kind");
        Expect(Vec3Near(collectible.collectible.center, center), "Collectible candidate center");
        Expect(
            Vec3Near(collectible.collectible.size, editor::kDefaultAddedCollectibleSize),
            "Collectible default collection bounds");
        Expect(
            !editor::MakePlacementCandidate(PlacementMode::None, center).visible,
            "idle mode has no candidate");
    }

    {
        world::LevelDefinition working = MakeActiveLevel();
        const core::Vec3 world{8.0f, 3.5f, 2.0f};
        Expect(editor::AddPlatformAt(working, world).succeeded, "Platform At world center");
        Expect(Vec3Near(working.elevatedPlatforms.back().center, world), "Platform At ignores spawn.z");
        Expect(
            Vec3Near(working.elevatedPlatforms.back().size, editor::kDefaultAddedPlatformSize),
            "Platform At reuses M41 size");

        Expect(editor::AddCheckpointAt(working, world).succeeded, "Checkpoint At");
        Expect(Vec3Near(working.checkpoints.back().center, world), "Checkpoint At trigger");
        Expect(
            Vec3Near(working.checkpoints.back().respawnPosition, world),
            "Checkpoint At respawn assembly");

        Expect(editor::AddHazardAt(working, world).succeeded, "Hazard At");
        Expect(Vec3Near(working.hazards.back().center, world), "Hazard At center");

        Expect(editor::AddCollectibleAt(working, world).succeeded, "Collectible At");
        Expect(Vec3Near(working.collectibles.back().center, world), "Collectible At center");
        Expect(
            Vec3Near(working.collectibles.back().size, editor::kDefaultAddedCollectibleSize),
            "Collectible At preserves collection bounds size");
    }

    {
        world::LevelDefinition working = MakeActiveLevel();
        const core::Vec3 first{1.0f, 2.0f, 3.0f};
        const core::Vec3 second{4.0f, 5.0f, 6.0f};
        const core::Vec3 third{7.0f, 8.0f, 9.0f};
        Expect(editor::AddCollectibleAt(working, first).succeeded, "repeat 1");
        Expect(editor::AddCollectibleAt(working, second).succeeded, "repeat 2");
        const editor::LifecycleEditResult last = editor::AddCollectibleAt(working, third);
        Expect(last.succeeded && working.collectibles.size() == 4, "one click one object, three adds");
        Expect(
            last.selection.kind == EditorObjectKind::Collectible && last.selection.index == 3,
            "latest Collectible is selected");
        Expect(Vec3Near(working.collectibles[1].center, first), "first pending Collectible remains");
        Expect(Vec3Near(working.collectibles[2].center, second), "second pending Collectible remains");
        Expect(Vec3Near(working.collectibles[3].center, third), "third pending Collectible is latest");
    }

    {
        Expect(
            editor::ShouldConfirmPlacement(
                PlacementMode::Platform, true, false, false, false, false, false, true),
            "viewport click confirms when eligible");
        Expect(
            !editor::ShouldConfirmPlacement(
                PlacementMode::None, true, false, false, false, false, false, true),
            "idle click does not confirm");
        Expect(
            !editor::ShouldConfirmPlacement(
                PlacementMode::Platform, true, true, false, false, false, false, true),
            "ImGui mouse capture blocks placement");
        Expect(
            !editor::ShouldConfirmPlacement(
                PlacementMode::Platform, true, false, true, false, false, false, true),
            "RMB look blocks placement");
        Expect(
            !editor::ShouldConfirmPlacement(
                PlacementMode::Platform, true, false, false, true, false, false, true),
            "orientation widget blocks placement");
        Expect(
            !editor::ShouldConfirmPlacement(
                PlacementMode::Platform, true, false, false, false, true, false, true),
            "gizmo has priority over placement");
        Expect(
            !editor::ShouldConfirmPlacement(
                PlacementMode::Platform, true, false, false, false, false, false, false),
            "capacity rejection blocks confirm");
        Expect(
            !editor::ShouldConfirmPlacement(
                PlacementMode::Platform, false, false, false, false, false, false, true),
            "no click does not place");
    }

    {
        world::LevelDefinition working = MakeActiveLevel();
        working.elevatedPlatforms.resize(static_cast<std::size_t>(world::kMaxElevatedPlatformCount));
        Expect(
            editor::CategoryAtCountLimit(working, EditorObjectKind::ElevatedPlatform),
            "platform physics capacity exhausted");
        Expect(
            std::strcmp(
                editor::CategoryCapacityReason(EditorObjectKind::ElevatedPlatform),
                "Physics body capacity reached.")
                == 0,
            "Platform capacity reason");
        Expect(
            std::strcmp(
                editor::CategoryCapacityReason(EditorObjectKind::Collectible),
                "Level file record limit reached.")
                == 0,
            "Collectible capacity reason");
        Expect(!editor::AddPlatformAt(working, {0.0f, 1.0f, 0.0f}).succeeded, "AddAt rejected at cap");
    }

    {
        editor::LevelEditorState state{};
        state.placementMode = PlacementMode::Collectible;
        editor::ClearPlacementMode(state.placementMode);
        Expect(state.placementMode == PlacementMode::None, "Apply/Revert/Reload cleanup helper");
    }

    {
        world::LevelDefinition working = MakeActiveLevel();
        const std::size_t before = working.elevatedPlatforms.size();
        const core::Vec3 camera{12.0f, 6.0f, 9.0f};
        Expect(editor::AddPlatform(working, camera).succeeded, "Edit > Add still succeeds");
        Expect(
            NearlyEqual(working.elevatedPlatforms.back().center.z, working.initialSpawnVisualCenter.z),
            "Edit > Add still snaps to spawn.z lane");
        Expect(
            NearlyEqual(working.elevatedPlatforms.back().center.x, camera.x),
            "Edit > Add still uses camera X");
        Expect(working.elevatedPlatforms.size() == before + 1, "Edit > Add appends one");
    }

    {
        Expect(
            editor::PlacementAddRequest(PlacementMode::Platform)
                == editor::LevelEditorRequest::AddPlatform,
            "palette confirm reuses AddPlatform request");
        Expect(
            editor::PlacementAddRequest(PlacementMode::None) == editor::LevelEditorRequest::None,
            "idle has no add request");
        Expect(
            std::strcmp(editor::PlacementModeName(PlacementMode::Hazard), "Hazard") == 0,
            "Hazard label");
    }

    {
        Expect(
            editor::StyleForCandidateSource(editor::PlacementCandidateSource::SurfaceHit)
                == editor::PlacementPreviewStyle::Surface,
            "surface source uses bright surface style");
        Expect(
            editor::StyleForCandidateSource(editor::PlacementCandidateSource::CameraFallback)
                == editor::PlacementPreviewStyle::Fallback,
            "fallback source uses distinct fallback style");
        Expect(
            editor::StyleForCandidateSource(editor::PlacementCandidateSource::None)
                == editor::PlacementPreviewStyle::None,
            "idle source has no preview style");
        editor::EditorPickingSet ground{};
        ground.proxies.push_back(
            {{EditorObjectKind::Ground, 0}, {0.0f, -0.25f, 0.0f}, {20.0f, 0.5f, 8.0f}, 0.0f});
        const editor::PlacementCandidate surface = editor::ResolvePlacementCandidate(
            PlacementMode::Platform,
            editor::Ray3{{0.0f, 10.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
            ground,
            {1.0f, 1.0f, 1.0f});
        Expect(
            editor::PlacementCandidateFromSurface(surface)
                && editor::StyleForCandidateSource(surface.source) == editor::PlacementPreviewStyle::Surface,
            "surface-hit helper");
        editor::EditorPickingSet empty{};
        const editor::PlacementCandidate fallback = editor::ResolvePlacementCandidate(
            PlacementMode::Collectible,
            editor::Ray3{{0.0f, 10.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
            empty,
            {7.0f, 4.0f, -3.0f});
        Expect(
            fallback.source == editor::PlacementCandidateSource::CameraFallback
                && editor::StyleForCandidateSource(fallback.source)
                    == editor::PlacementPreviewStyle::Fallback,
            "camera-fallback helper");
    }

    {
        Expect(
            editor::PlacementInteractionClaimsPointer(true, false, false, false, false, false),
            "ImGui claims pointer");
        Expect(
            editor::PlacementInteractionClaimsPointer(false, true, false, false, false, false),
            "RMB look claims pointer");
        Expect(
            editor::PlacementInteractionClaimsPointer(false, false, true, false, false, false),
            "widget claims pointer");
        Expect(
            editor::PlacementInteractionClaimsPointer(false, false, false, true, false, false),
            "gizmo consume claims pointer");
        Expect(
            editor::PlacementInteractionClaimsPointer(false, false, false, false, true, false),
            "gizmo drag claims pointer");
        Expect(
            editor::PlacementInteractionClaimsPointer(false, false, false, false, false, true),
            "gizmo hover on press claims pointer");
        Expect(
            !editor::PlacementInteractionClaimsPointer(false, false, false, false, false, false),
            "clean viewport does not claim pointer");
    }

    {
        bool blocked = false;
        editor::UpdatePlacementPointerBlock(blocked, true, true, false, true);
        Expect(blocked, "gizmo press starts pointer block");
        Expect(
            !editor::ShouldConfirmPlacement(
                PlacementMode::Platform, true, false, false, false, true, blocked, true),
            "gizmo press does not confirm placement");
        editor::UpdatePlacementPointerBlock(blocked, false, true, false, true);
        Expect(blocked, "gizmo drag keeps pointer block");
        Expect(
            !editor::ShouldConfirmPlacement(
                PlacementMode::Platform, false, false, false, false, true, blocked, true),
            "gizmo drag does not confirm placement");
        editor::UpdatePlacementPointerBlock(blocked, false, false, true, true);
        Expect(blocked, "gizmo release keeps block for this frame");
        Expect(
            !editor::ShouldConfirmPlacement(
                PlacementMode::Platform, true, false, false, false, false, blocked, true),
            "gizmo release over viewport does not confirm");
        editor::ClearPlacementPointerBlock(blocked);
        Expect(!blocked, "block clears after the claimed gesture");
        Expect(
            editor::ShouldConfirmPlacement(
                PlacementMode::Platform, true, false, false, false, false, blocked, true),
            "next clean click confirms while mode stays active");
    }

    {
        PlacementMode mode = PlacementMode::Collectible;
        const std::size_t before = MakeActiveLevel().collectibles.size();
        world::LevelDefinition working = MakeActiveLevel();
        editor::ApplyPaletteCategoryClick(mode, PlacementMode::Collectible);
        Expect(mode == PlacementMode::None, "palette toggle off does not add");
        Expect(working.collectibles.size() == before, "palette click creates no object");
        editor::ApplyPaletteCategoryClick(mode, PlacementMode::Hazard);
        Expect(mode == PlacementMode::Hazard, "palette switch only changes mode");
        Expect(working.collectibles.size() == before, "palette switch creates no object");
        Expect(working.hazards.size() == MakeActiveLevel().hazards.size(), "palette switch adds no Hazard");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d editor placement test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Editor placement tests passed.\n");
    return 0;
}
