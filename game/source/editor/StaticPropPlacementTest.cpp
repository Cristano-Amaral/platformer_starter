#include "editor/StaticPropPlacement.h"

#include "editor/AuthoredLifecycleCommands.h"
#include "editor/EditorGizmo.h"
#include "editor/EditorHierarchy.h"
#include "editor/EditorPicking.h"
#include "editor/LevelEditor.h"
#include "editor/StaticPropTransform.h"
#include "world/LevelDefinition.h"

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

constexpr char kBarrel[] = "models/Barrel by HFJAKI92 - wrYrHLVtxg.glb";
constexpr char kChest[] = "models/Chest by Quaternius - O72u4Drp8k.glb";
constexpr char kTestStatic[] = "models/test_static.glb";

world::LevelDefinition MakeActiveLevel()
{
    world::LevelDefinition level{};
    level.id = "level_01";
    level.initialSpawnVisualCenter = {0.0f, 0.8f, 0.0f};
    level.killPlaneY = -8.0f;
    level.ground = {{0.0f, -0.25f, 0.0f}, {20.0f, 0.5f, 8.0f}};
    level.elevatedPlatforms.push_back({{4.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
    level.checkpoint1PlatformIndex = 0;
    level.checkpoint2PlatformIndex = 0;
    level.goalPlatformIndex = 0;
    level.slopes[0] = {{8.0f, 1.0f, 0.0f}, {4.0f, 0.4f, 3.0f}, 30.0f};
    level.slopes[1] = {{11.0f, 0.8f, 0.0f}, {2.0f, 0.4f, 3.0f}, 60.0f};
    level.movingPlatform.size = {4.0f, 0.4f, 3.0f};
    level.movingPlatform.centerY = 1.3f;
    level.movingPlatform.centerZ = 0.0f;
    level.movingPlatform.pathMinX = -6.0f;
    level.movingPlatform.pathMaxX = 6.0f;
    level.movingPlatform.speed = 2.5f;
    level.checkpoints.push_back({{16.5f, 1.8f, 0.0f}, {2.4f, 1.6f, 2.0f}, {16.5f, 1.8f, 0.0f}});
    level.hazards.push_back({{11.5f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
    level.collectibles.push_back({{5.0f, 2.5f, 0.0f}, {1.0f, 1.2f, 1.0f}});
    level.goal = {{-8.0f, 3.8f, 0.0f}, {2.0f, 1.6f, 1.8f}};
    level.camera = {{2.0f, 3.5f, 12.0f}, 40.0f};
    return level;
}

void SeedEditor(editor::LevelEditorState& state, const world::LevelDefinition& active)
{
    state.workingCopy = active;
    state.savedSourceBaseline = active;
    editor::RefreshLevelEditorDerivedFlags(state, active);
}

std::size_t HierarchyStaticPropCount(const world::LevelDefinition& level)
{
    std::size_t count = 0;
    for (const editor::HierarchyEntry& entry : editor::BuildHierarchyEntries(level))
    {
        if (entry.selection.kind == editor::EditorObjectKind::StaticProp)
        {
            ++count;
        }
    }
    return count;
}

editor::EditorPickingSet MakeSurfaceSet()
{
    editor::EditorPickingSet set{};
    set.proxies.push_back(
        {{editor::EditorObjectKind::Ground, 0}, {0.0f, -0.25f, 0.0f}, {20.0f, 0.5f, 8.0f}, 0.0f});
    set.proxies.push_back(
        {{editor::EditorObjectKind::ElevatedPlatform, 0},
         {4.0f, 0.75f, 0.0f},
         {4.0f, 0.5f, 3.0f},
         0.0f});
    set.proxies.push_back(
        {{editor::EditorObjectKind::Slope, 0}, {8.0f, 1.0f, 0.0f}, {4.0f, 0.4f, 3.0f}, 30.0f});
    return set;
}

bool ApplyLikeApplication(
    world::LevelDefinition& active,
    editor::LevelEditorState& state)
{
    if (!world::LevelDefinitionHasRequiredAuthoredContent(state.workingCopy))
    {
        return false;
    }
    active = state.workingCopy;
    state.workingCopy = active;
    editor::ResetStructuralIndexMap(state.structuralMap, active);
    editor::RefreshLevelEditorDerivedFlags(state, active);
    return true;
}
}

int main()
{
    using editor::EditorObjectKind;
    using editor::PlacementMode;

    Expect(
        std::strcmp(
            editor::kContentBrowserAddStaticPropLabel, editor::kContentBrowserPlaceStaticPropLabel)
            != 0,
        "Add and Place labels are distinct");
    Expect(
        editor::ContentBrowserAddStaticPropRequest() == editor::LevelEditorRequest::AddStaticProp,
        "Direct Add remains AddStaticProp");
    Expect(
        editor::PlacementModeFromKind(EditorObjectKind::StaticProp) == PlacementMode::None,
        "Static Prop is still not an Object Palette mode");

    {
        Expect(!editor::CanStartStaticPropPlacement(""), "empty identity cannot start placement");
        Expect(
            !editor::CanStartStaticPropPlacement("barrel.glb"),
            "non-canonical identity cannot start placement");
        Expect(
            editor::CanStartStaticPropPlacement(kTestStatic),
            "valid Content Browser model can start placement");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        state.contentBrowser.selectedIdentity = kTestStatic;
        const world::LevelDefinition workingBefore = state.workingCopy;
        const world::LevelDefinition savedBefore = state.savedSourceBaseline;
        const bool modifiedBefore = state.modified;
        Expect(
            editor::ApplyPlaceStaticPropClick(
                state.staticPropPlacement,
                state.placementMode,
                state.placementPointerBlocked,
                kTestStatic)
                == editor::PlaceStaticPropClickResult::Started,
            "valid model starts placement");
        Expect(editor::StaticPropPlacementIsActive(state.staticPropPlacement), "placement is active");
        Expect(state.staticPropPlacement.modelIdentity == kTestStatic, "placement consumes identity");
        Expect(
            world::AuthoredLevelDataEqual(state.workingCopy, workingBefore),
            "entering placement does not mutate workingCopy");
        Expect(
            world::AuthoredLevelDataEqual(active, MakeActiveLevel()),
            "entering placement does not mutate active");
        Expect(
            world::AuthoredLevelDataEqual(state.savedSourceBaseline, savedBefore),
            "entering placement does not mutate savedSourceBaseline");
        Expect(state.modified == modifiedBefore, "entering placement does not mark Modified");
        Expect(state.workingCopy.staticProps.empty(), "no Static Prop is authored yet");
        Expect(
            HierarchyStaticPropCount(state.workingCopy) == 0,
            "transient preview is not a Hierarchy object");
    }

    {
        editor::StaticPropPlacementState placement{};
        editor::StartStaticPropPlacement(placement, kTestStatic);
        const editor::EditorPickingSet surfaces = MakeSurfaceSet();
        const core::Vec3 localMin{-0.4f, -0.8f, -0.4f};
        const core::Vec3 localMax{0.4f, 0.2f, 0.4f};
        const editor::Ray3 groundRay{{0.0f, 10.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
        const editor::StaticPropPlacementPreview groundPreview =
            editor::ResolveStaticPropPlacementPreview(placement, groundRay, surfaces, localMin, localMax);
        Expect(groundPreview.valid, "Ground produces a valid placement hit");
        Expect(groundPreview.modelIdentity == kTestStatic, "preview identity is the placement asset");
        Expect(
            Vec3Near(groundPreview.rotationDegrees, world::kDefaultStaticPropRotationDegrees),
            "preview rotation is the Static Prop default");
        Expect(
            Vec3Near(groundPreview.scale, world::kDefaultStaticPropScale),
            "preview scale is the Static Prop default");
        const editor::PlacementSurfaceHit groundHit =
            editor::FindPlacementSurfaceHit(groundRay, surfaces);
        Expect(groundHit.hit, "ground ray hits");
        Expect(
            Vec3Near(
                groundPreview.position,
                editor::MakeStaticPropPlacementPosition(groundHit.point, localMin, localMax)),
            "preview position matches bounds-based support offset");
        Expect(
            NearlyEqual(groundPreview.position.y, groundHit.point.y - localMin.y),
            "Y offset sits local min on the surface for default rotation/scale");

        const editor::Ray3 platformRay{{4.0f, 10.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
        const editor::StaticPropPlacementPreview platformPreview =
            editor::ResolveStaticPropPlacementPreview(
                placement, platformRay, surfaces, localMin, localMax);
        Expect(platformPreview.valid, "Platform produces a valid placement hit");
        const editor::PlacementSurfaceHit platformHit =
            editor::FindPlacementSurfaceHit(platformRay, surfaces);
        Expect(
            Vec3Near(
                platformPreview.position,
                editor::MakeStaticPropPlacementPosition(platformHit.point, localMin, localMax)),
            "Platform preview position matches the shown support offset");

        const editor::Ray3 slopeRay{{8.0f, 10.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
        const editor::StaticPropPlacementPreview slopePreview =
            editor::ResolveStaticPropPlacementPreview(placement, slopeRay, surfaces, localMin, localMax);
        Expect(slopePreview.valid, "Slope produces a valid placement hit");
        Expect(
            Vec3Near(slopePreview.rotationDegrees, world::kDefaultStaticPropRotationDegrees),
            "Slope placement does not invent rotation-to-normal");

        const editor::StaticPropPlacementPreview again =
            editor::ResolveStaticPropPlacementPreview(
                placement, groundRay, surfaces, localMin, localMax);
        Expect(
            Vec3Near(again.position, groundPreview.position),
            "bounds-based support offset is deterministic");
    }

    {
        Expect(!editor::IsEligiblePlacementSurface(EditorObjectKind::DynamicBox),
            "Dynamic Box is not a placement surface");
        Expect(!editor::IsEligiblePlacementSurface(EditorObjectKind::StaticProp),
            "Static Prop is not a placement surface");
        Expect(!editor::IsEligiblePlacementSurface(EditorObjectKind::Hazard),
            "Hazard is not a placement surface");
        Expect(!editor::IsEligiblePlacementSurface(EditorObjectKind::Collectible),
            "Collectible is not a placement surface");
        Expect(!editor::IsEligiblePlacementSurface(EditorObjectKind::Checkpoint),
            "Checkpoint is not a placement surface");

        editor::StaticPropPlacementState placement{};
        editor::StartStaticPropPlacement(placement, kTestStatic);
        editor::EditorPickingSet boxes{};
        boxes.proxies.push_back(
            {{EditorObjectKind::DynamicBox, 0}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 0.0f});
        const editor::Ray3 down{{0.0f, 10.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
        Expect(
            !editor::FindPlacementSurfaceHit(down, boxes).hit,
            "Dynamic Box ray does not produce a placement hit");
        Expect(
            !editor::ResolveStaticPropPlacementPreview(
                 placement, down, boxes, editor::kStaticPropDefaultLocalMin, editor::kStaticPropDefaultLocalMax)
                 .valid,
            "Dynamic Box cannot confirm");

        editor::EditorPickingSet props{};
        editor::PickingProxy propProxy{};
        propProxy.selection = {EditorObjectKind::StaticProp, 0};
        propProxy.usesStaticPropTransform = true;
        propProxy.staticProp.modelIdentity = kTestStatic;
        propProxy.staticProp.position = {0.0f, 1.0f, 0.0f};
        propProxy.staticProp.scale = world::kDefaultStaticPropScale;
        propProxy.localMin = editor::kStaticPropDefaultLocalMin;
        propProxy.localMax = editor::kStaticPropDefaultLocalMax;
        editor::StaticPropWorldAabb(
            propProxy.staticProp,
            propProxy.localMin,
            propProxy.localMax,
            propProxy.center,
            propProxy.size);
        props.proxies.push_back(propProxy);
        Expect(
            !editor::FindPlacementSurfaceHit(down, props).hit,
            "Static Prop ray does not produce a placement hit");
        Expect(
            !editor::ResolveStaticPropPlacementPreview(
                 placement,
                 down,
                 props,
                 editor::kStaticPropDefaultLocalMin,
                 editor::kStaticPropDefaultLocalMax)
                 .valid,
            "Static Prop cannot be a placement surface");
    }

    {
        editor::StaticPropPlacementState placement{};
        editor::StartStaticPropPlacement(placement, kTestStatic);
        editor::EditorPickingSet empty{};
        const editor::Ray3 miss{{0.0f, 10.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
        const editor::StaticPropPlacementPreview preview = editor::ResolveStaticPropPlacementPreview(
            placement,
            miss,
            empty,
            editor::kStaticPropDefaultLocalMin,
            editor::kStaticPropDefaultLocalMax);
        Expect(!preview.valid, "invalid/no surface hit is not a valid preview");
        Expect(
            !editor::StaticPropPlacementPreviewShouldDraw(preview),
            "invalid preview is not submitted for draw");
        Expect(
            !editor::ShouldConfirmStaticPropPlacement(
                placement, preview.valid, true, false, false, false, false, false, true),
            "invalid hit cannot confirm");
        Expect(
            !editor::ShouldConfirmStaticPropPlacement(
                placement, true, true, false, false, false, false, false, false),
            "capacity/canAdd false cannot confirm");
        Expect(
            editor::ShouldConfirmStaticPropPlacement(
                placement, true, true, false, false, false, false, false, true),
            "valid LMB confirms when canAdd");
        Expect(
            !editor::ShouldConfirmStaticPropPlacement(
                placement, true, true, true, false, false, false, false, true),
            "ImGui capture does not confirm");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        state.contentBrowser.selectedIdentity = kBarrel;
        editor::ApplyPlaceStaticPropClick(
            state.staticPropPlacement,
            state.placementMode,
            state.placementPointerBlocked,
            kBarrel);
        const editor::EditorPickingSet surfaces = MakeSurfaceSet();
        const editor::Ray3 groundRay{{0.0f, 10.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
        editor::StaticPropPlacementPreview preview = editor::ResolveStaticPropPlacementPreview(
            state.staticPropPlacement,
            groundRay,
            surfaces,
            editor::kStaticPropDefaultLocalMin,
            editor::kStaticPropDefaultLocalMax);
        Expect(preview.valid, "confirm fixture has a valid Ground preview");
        const bool modifiedBeforeMove = state.modified;
        preview = editor::ResolveStaticPropPlacementPreview(
            state.staticPropPlacement,
            editor::Ray3{{1.0f, 10.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
            surfaces,
            editor::kStaticPropDefaultLocalMin,
            editor::kStaticPropDefaultLocalMax);
        editor::RefreshLevelEditorDerivedFlags(state, active);
        Expect(state.modified == modifiedBeforeMove, "moving preview does not mark Modified");
        Expect(state.workingCopy.staticProps.empty(), "moving preview does not author");

        Expect(
            editor::HandleAuthoredLifecycleRequest(
                state,
                active,
                editor::LevelEditorRequest::AddStaticProp,
                true,
                preview.position,
                true,
                state.staticPropPlacement.modelIdentity),
            "valid confirmation mutates workingCopy");
        Expect(state.workingCopy.staticProps.size() == 1, "confirmation creates exactly one Static Prop");
        Expect(active.staticProps.empty(), "created object is added to workingCopy only");
        Expect(
            state.workingCopy.staticProps[0].modelIdentity == kBarrel,
            "created asset identity matches placement asset");
        Expect(
            Vec3Near(state.workingCopy.staticProps[0].position, preview.position),
            "created position matches preview position");
        Expect(
            Vec3Near(
                state.workingCopy.staticProps[0].rotationDegrees,
                world::kDefaultStaticPropRotationDegrees),
            "created rotation equals expected default");
        Expect(
            Vec3Near(state.workingCopy.staticProps[0].scale, world::kDefaultStaticPropScale),
            "created scale equals expected default");
        Expect(active.staticProps.empty(), "active remains unchanged before Apply");
        Expect(
            state.selection.kind == EditorObjectKind::StaticProp && state.selection.index == 0,
            "confirmed prop becomes scene selection");
        Expect(
            state.contentBrowser.selectedIdentity == kBarrel,
            "Content Browser selection remains independent");
        Expect(HierarchyStaticPropCount(state.workingCopy) == 1, "Hierarchy contains the confirmed prop");
        Expect(editor::StaticPropPlacementIsActive(state.staticPropPlacement), "repeated placement stays active");
        Expect(state.modified, "confirmation marks Modified");
        Expect(
            editor::IsGizmoSelection(state.selection),
            "Translate gizmo remains available after placement");
        Expect(editor::IsScaleSelection(state.selection), "Static Prop Scale gizmo remains available");
        Expect(!editor::IsResizeSelection(state.selection), "primitive Resize is still not Static Prop Scale");
        Expect(
            editor::AuthoredLevelsProtectStaticPropIdentity(
                state.workingCopy, active, state.savedSourceBaseline, kBarrel),
            "confirmed prop protects Delete Asset");

        const editor::StaticPropPlacementPreview secondPreview =
            editor::ResolveStaticPropPlacementPreview(
                state.staticPropPlacement,
                editor::Ray3{{2.0f, 10.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
                surfaces,
                editor::kStaticPropDefaultLocalMin,
                editor::kStaticPropDefaultLocalMax);
        Expect(secondPreview.valid, "repeated placement still has a valid preview");
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                state,
                active,
                editor::LevelEditorRequest::AddStaticProp,
                true,
                secondPreview.position,
                true,
                state.staticPropPlacement.modelIdentity),
            "second confirmation succeeds");
        Expect(state.workingCopy.staticProps.size() == 2, "repeated placement creates one object per confirmation");
        Expect(
            state.workingCopy.staticProps[0].position.x != state.workingCopy.staticProps[1].position.x
                || state.workingCopy.staticProps[0].position.y != state.workingCopy.staticProps[1].position.y,
            "repeated instances are independent");
        Expect(state.selection.index == 1, "latest placed prop is scene selection");
        Expect(active.staticProps.empty(), "repeated placement still leaves active unchanged");

        world::LevelDefinition applied = active;
        Expect(ApplyLikeApplication(applied, state), "Apply promotes placed props");
        Expect(applied.staticProps.size() == 2, "Apply copies workingCopy Static Props into active");
        Expect(applied.staticProps[0].modelIdentity == kBarrel, "Apply keeps placement identity");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        state.contentBrowser.selectedIdentity = kChest;
        editor::StartStaticPropPlacement(state.staticPropPlacement, kChest);
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                state,
                active,
                editor::LevelEditorRequest::AddStaticProp,
                true,
                {1.0f, 0.5f, 0.0f},
                true,
                kChest),
            "place-then-revert fixture adds one prop");
        Expect(state.modified, "pending placement is Modified");
        state.workingCopy = active;
        editor::RefreshLevelEditorDerivedFlags(state, active);
        Expect(state.workingCopy.staticProps.empty(), "Revert restores workingCopy");
        Expect(!state.modified, "Revert clears Modified");
        Expect(active.staticProps.empty(), "Revert does not invent active props");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        state.contentBrowser.selectedIdentity = kTestStatic;
        editor::StartStaticPropPlacement(state.staticPropPlacement, kTestStatic);
        const bool modifiedBefore = state.modified;
        Expect(
            editor::ShouldCancelStaticPropPlacement(state.staticPropPlacement, true, false),
            "Esc cancels Static Prop placement");
        editor::CancelStaticPropPlacement(state.staticPropPlacement);
        Expect(!editor::StaticPropPlacementIsActive(state.staticPropPlacement), "cancel clears preview state");
        Expect(state.staticPropPlacement.modelIdentity.empty(), "cancel drops placement identity");
        Expect(state.workingCopy.staticProps.empty(), "cancel creates no pending object");
        Expect(active.staticProps.empty(), "cancel leaves active untouched");
        Expect(
            world::AuthoredLevelDataEqual(state.savedSourceBaseline, active),
            "cancel leaves savedSourceBaseline untouched");
        editor::RefreshLevelEditorDerivedFlags(state, active);
        Expect(state.modified == modifiedBefore, "cancel alone does not mark Modified");
        editor::StaticPropPlacementPreview stale{};
        Expect(
            !editor::StaticPropPlacementPreviewShouldDraw(stale),
            "cancel/switch leaves no stale preview submission");
        Expect(
            !editor::AuthoredLevelsProtectStaticPropIdentity(
                state.workingCopy, active, state.savedSourceBaseline, kTestStatic),
            "transient preview alone is not a persisted authored dependency");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState state{};
        SeedEditor(state, active);
        state.contentBrowser.selectedIdentity = kBarrel;
        editor::StartStaticPropPlacement(state.staticPropPlacement, kBarrel);
        editor::SyncStaticPropPlacementIdentityFromBrowser(state.staticPropPlacement, kChest);
        Expect(
            state.staticPropPlacement.modelIdentity == kChest,
            "valid Content Browser change updates the pending placement asset");
        editor::SyncStaticPropPlacementIdentityFromBrowser(state.staticPropPlacement, "");
        Expect(
            state.staticPropPlacement.modelIdentity == kChest,
            "empty Content Browser selection does not mix identities");
        Expect(
            state.contentBrowser.selectedIdentity == kBarrel,
            "scene/placement sync does not write Content Browser selection");
    }

    {
        const world::LevelDefinition active = MakeActiveLevel();
        editor::LevelEditorState direct{};
        SeedEditor(direct, active);
        direct.contentBrowser.selectedIdentity = kTestStatic;
        const core::Vec3 cameraAnchor{12.0f, 5.0f, -6.0f};
        Expect(
            editor::HandleAuthoredLifecycleRequest(
                direct, active, editor::ContentBrowserAddStaticPropRequest(), true, cameraAnchor),
            "Direct Add remains immediate");
        Expect(direct.workingCopy.staticProps.size() == 1, "Direct Add creates immediately");
        Expect(
            NearlyEqual(direct.workingCopy.staticProps[0].position.x, cameraAnchor.x),
            "Direct Add still uses camera-region X");
        Expect(!editor::StaticPropPlacementIsActive(direct.staticPropPlacement), "Direct Add does not enter placement");
        Expect(direct.placementMode == PlacementMode::None, "Direct Add does not enter Object Palette placement");

        editor::LevelEditorState place{};
        SeedEditor(place, active);
        place.contentBrowser.selectedIdentity = kTestStatic;
        editor::ApplyPlaceStaticPropClick(
            place.staticPropPlacement, place.placementMode, place.placementPointerBlocked, kTestStatic);
        Expect(place.workingCopy.staticProps.empty(), "Place Static Prop is not immediate Direct Add");
        Expect(editor::StaticPropPlacementIsActive(place.staticPropPlacement), "Place enters placement mode");
    }

    {
        editor::StaticPropPlacementState placement{};
        PlacementMode palette = PlacementMode::None;
        bool blocked = false;
        Expect(
            editor::ApplyPlaceStaticPropClick(placement, palette, blocked, "")
                == editor::PlaceStaticPropClickResult::Ignored,
            "Place without a valid asset is ignored");
        editor::ApplyPlaceStaticPropClick(placement, palette, blocked, kBarrel);
        palette = PlacementMode::Collectible;
        Expect(
            editor::ApplyPlaceStaticPropClick(placement, palette, blocked, kChest)
                == editor::PlaceStaticPropClickResult::UpdatedIdentity,
            "Place with another asset updates identity");
        Expect(palette == PlacementMode::None, "starting Static Prop placement cancels Object Palette");
        Expect(
            editor::ApplyPlaceStaticPropClick(placement, palette, blocked, kChest)
                == editor::PlaceStaticPropClickResult::Cancelled,
            "Place of the same asset toggles off");
        Expect(!editor::StaticPropPlacementIsActive(placement), "toggle-off cancels placement");
    }

    {
        editor::StaticPropPlacementState placement{};
        PlacementMode palette = PlacementMode::Platform;
        bool blocked = true;
        editor::StartStaticPropPlacement(placement, kTestStatic);
        editor::CancelAllEditorPlacement(palette, blocked, placement);
        Expect(palette == PlacementMode::None, "CancelAll clears Object Palette");
        Expect(!blocked, "CancelAll clears pointer block");
        Expect(!editor::StaticPropPlacementIsActive(placement), "CancelAll clears Static Prop placement");
        Expect(
            std::strcmp(editor::StaticPropPlacementHudName(), "Static Prop") == 0,
            "HUD name is Static Prop");
    }

    if (gFailures > 0)
    {
        std::fprintf(stderr, "%d static prop placement test(s) failed.\n", gFailures);
        return 1;
    }
    std::printf("Static prop placement tests passed.\n");
    return 0;
}
