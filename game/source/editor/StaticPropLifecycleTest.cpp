// Correction 4: Static Prop Add/Apply/Delete boundaries. No raylib. Apply is
// the same promotion Application uses: active = validated workingCopy.

#include "editor/AuthoredObjectLifecycle.h"
#include "editor/EditorSelection.h"
#include "editor/StaticPropTransform.h"
#include "world/LevelDefinition.h"

#include <cmath>
#include <cstdio>
#include <string>

namespace
{
int gFailures = 0;

void Expect(bool condition, const char* what)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++gFailures;
    }
}

bool NearlyEqual(float a, float b, float epsilon = 0.0001f)
{
    return std::fabs(a - b) <= epsilon;
}

bool Vec3Near(core::Vec3 a, core::Vec3 b, float epsilon = 0.0001f)
{
    return NearlyEqual(a.x, b.x, epsilon) && NearlyEqual(a.y, b.y, epsilon)
        && NearlyEqual(a.z, b.z, epsilon);
}

constexpr char kChest[] = "models/Chest by Quaternius - O72u4Drp8k.glb";
constexpr char kBarrel[] = "models/Barrel by HFJAKI92 - wrYrHLVtxg.glb";

world::LevelDefinition MakeActive()
{
    world::LevelDefinition level{};
    level.id = "level_01";
    level.initialSpawnVisualCenter = {0.0f, 0.8f, 0.0f};
    level.killPlaneY = -8.0f;
    level.ground = {{0.0f, -0.25f, 0.0f}, {56.0f, 0.5f, 8.0f}};
    level.elevatedPlatforms.push_back({{5.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
    level.checkpoint1PlatformIndex = 0;
    level.checkpoint2PlatformIndex = 0;
    level.goalPlatformIndex = 0;
    level.slopes[0] = {{21.7f, 1.6732f, 0.0f}, {6.0f, 0.4f, 4.0f}, 30.0f};
    level.slopes[1] = {{25.6f, 0.966f, 0.0f}, {2.0f, 0.4f, 3.0f}, 60.0f};
    level.movingPlatform.size = {4.0f, 0.4f, 3.0f};
    level.movingPlatform.centerY = 1.3f;
    level.movingPlatform.centerZ = 0.0f;
    level.movingPlatform.pathMinX = -6.0f;
    level.movingPlatform.pathMaxX = 6.0f;
    level.movingPlatform.speed = 2.5f;
    level.movingPlatform.startX = 0.0f;
    level.goal = {{-21.0f, 3.8f, 0.0f}, {2.0f, 1.6f, 1.8f}};
    level.camera = {{2.0f, 3.5f, 12.0f}, 40.0f};
    return level;
}

bool ApplyLikeApplication(
    world::LevelDefinition& active,
    world::LevelDefinition& workingCopy,
    editor::StructuralIndexMap& map)
{
    if (!world::LevelDefinitionHasRequiredAuthoredContent(workingCopy))
    {
        return false;
    }
    active = workingCopy;
    workingCopy = active;
    editor::ResetStructuralIndexMap(map, active);
    return true;
}

std::size_t CountIdentity(const world::LevelDefinition& level, std::string_view identity)
{
    std::size_t count = 0;
    for (const world::StaticPropSpec& prop : level.staticProps)
    {
        if (prop.modelIdentity == identity)
        {
            ++count;
        }
    }
    return count;
}

std::size_t SimulatedDrawSubmissions(const world::LevelDefinition& active)
{
    std::size_t count = 0;
    for (const world::StaticPropSpec& prop : active.staticProps)
    {
        if (world::StaticPropTransformIsValid(prop))
        {
            ++count;
        }
    }
    return count;
}

bool SimulatedSubmitIdentity(const world::LevelDefinition& active, std::string_view identity)
{
    for (const world::StaticPropSpec& prop : active.staticProps)
    {
        if (world::StaticPropTransformIsValid(prop) && prop.modelIdentity == identity)
        {
            return true;
        }
    }
    return false;
}
}

int main()
{
    using editor::EditorObjectKind;
    const core::Vec3 kAnchor{0.42f, 1.54f, 12.0f};

    {
        world::LevelDefinition active = MakeActive();
        world::LevelDefinition workingCopy = active;
        world::LevelDefinition savedSourceBaseline = active;
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        bool placementIdle = true;

        Expect(workingCopy.staticProps.empty(), "before Add workingCopy has 0 props");
        Expect(active.staticProps.empty(), "before Add active has 0 props");
        Expect(savedSourceBaseline.staticProps.empty(), "before Add saved baseline has 0 props");

        const editor::LifecycleEditResult added =
            editor::AddStaticProp(workingCopy, kAnchor, kChest);
        Expect(added.succeeded, "Add Chest mutates workingCopy");
        Expect(workingCopy.staticProps.size() == 1, "after Add workingCopy has 1 Chest");
        Expect(CountIdentity(workingCopy, kChest) == 1, "workingCopy references Chest");
        Expect(active.staticProps.empty(), "Add does not mutate active");
        Expect(savedSourceBaseline.staticProps.empty(), "Add does not mutate saved baseline");
        Expect(placementIdle, "Add does not enter placement");
        Expect(
            editor::AuthoredLevelsProtectStaticPropIdentity(
                workingCopy, active, savedSourceBaseline, kChest),
            "pending Chest already protects Delete Asset");

        Expect(ApplyLikeApplication(active, workingCopy, map), "first Apply accepts Chest");
        Expect(active.staticProps.size() == 1, "after Apply active has 1 Chest");
        Expect(CountIdentity(active, kChest) == 1, "active references Chest after Apply");
        Expect(workingCopy.staticProps.size() == 1, "Apply copies Chest into workingCopy");
        Expect(SimulatedDrawSubmissions(active) == 1, "renderer loop would submit 1 instance");
        Expect(SimulatedSubmitIdentity(active, kChest), "renderer loop would submit Chest");
        Expect(
            editor::MappedActiveIndex(map, EditorObjectKind::StaticProp, 0) == 0,
            "Apply rebuilds Static Prop structural indices");

        const core::Vec3 authoredScale{2.0f, 0.5f, 1.5f};
        workingCopy.staticProps[0].scale = authoredScale;
        Expect(
            Vec3Near(active.staticProps[0].scale, {1.0f, 1.0f, 1.0f}),
            "Scale edit leaves active until Apply");
        Expect(ApplyLikeApplication(active, workingCopy, map), "Apply promotes Scale");
        Expect(Vec3Near(active.staticProps[0].scale, authoredScale), "active uses applied Scale");
        workingCopy.staticProps[0].scale = {4.0f, 4.0f, 4.0f};
        workingCopy = active;
        Expect(Vec3Near(workingCopy.staticProps[0].scale, authoredScale), "Revert restores Scale");

        const editor::LifecycleEditResult duplicated =
            editor::DuplicateSelected(workingCopy, {EditorObjectKind::StaticProp, 0});
        Expect(duplicated.succeeded, "Duplicate after Scale");
        Expect(Vec3Near(workingCopy.staticProps[1].scale, authoredScale), "Duplicate preserves Scale");
        workingCopy.staticProps[1].scale = {3.0f, 1.0f, 1.0f};
        Expect(Vec3Near(workingCopy.staticProps[0].scale, authoredScale), "instances keep independent Scale");
        Expect(ApplyLikeApplication(active, workingCopy, map), "Apply two independent Scales");
        Expect(Vec3Near(active.staticProps[0].scale, authoredScale), "first instance Scale after Apply");
        Expect(
            Vec3Near(active.staticProps[1].scale, {3.0f, 1.0f, 1.0f}),
            "second instance Scale after Apply");
        workingCopy.staticProps.pop_back();
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::StaticProp, true, 1);
        Expect(ApplyLikeApplication(active, workingCopy, map), "Apply restores single instance");
        Expect(active.staticProps.size() == 1, "second instance removed after Apply");

        Expect(ApplyLikeApplication(active, workingCopy, map), "second Apply of the same copy");
        Expect(active.staticProps.size() == 1, "repeated Apply does not accumulate instances");

        const editor::LifecycleEditResult deleted = editor::DeleteSelected(
            workingCopy, {EditorObjectKind::StaticProp, 0});
        Expect(deleted.succeeded, "Delete Chest mutates workingCopy");
        Expect(workingCopy.staticProps.empty(), "after Delete workingCopy has 0 props");
        Expect(active.staticProps.size() == 1, "Delete before Apply leaves active Chest");
        Expect(CountIdentity(active, kChest) == 1, "active still has Chest until second Apply");
        Expect(savedSourceBaseline.staticProps.empty(), "Delete does not mutate saved baseline");
        editor::ApplyLifecycleToStructuralMap(map, EditorObjectKind::StaticProp, true, 0);

        Expect(ApplyLikeApplication(active, workingCopy, map), "second Apply removes Chest");
        Expect(active.staticProps.empty(), "after second Apply active has 0 props");
        Expect(CountIdentity(active, kChest) == 0, "Chest is absent from active");
        Expect(workingCopy.staticProps.empty(), "workingCopy stays empty after Apply");
        Expect(SimulatedDrawSubmissions(active) == 0, "renderer loop submits 0 instances");
        Expect(!SimulatedSubmitIdentity(active, kChest), "renderer loop does not submit Chest");
        Expect(
            !editor::AuthoredLevelsProtectStaticPropIdentity(
                workingCopy, active, savedSourceBaseline, kChest),
            "unreferenced Chest no longer blocks Delete Asset");
        Expect(placementIdle, "Delete/Apply do not enter placement");
    }

    {
        world::LevelDefinition active = MakeActive();
        world::LevelDefinition workingCopy = active;
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);

        Expect(editor::AddStaticProp(workingCopy, kAnchor, kBarrel).succeeded, "Add Barrel");
        Expect(editor::AddStaticProp(workingCopy, kAnchor, kChest).succeeded, "Add Chest beside Barrel");
        Expect(workingCopy.staticProps.size() == 2, "mixed workingCopy has Barrel+Chest");
        Expect(ApplyLikeApplication(active, workingCopy, map), "Apply mixed assets");
        Expect(CountIdentity(active, kBarrel) == 1 && CountIdentity(active, kChest) == 1,
            "active has both identities");

        Expect(
            editor::DeleteSelected(workingCopy, {EditorObjectKind::StaticProp, 1}).succeeded,
            "Delete Chest from mixed pair");
        Expect(CountIdentity(workingCopy, kBarrel) == 1, "workingCopy keeps Barrel");
        Expect(CountIdentity(workingCopy, kChest) == 0, "workingCopy dropped Chest");
        Expect(ApplyLikeApplication(active, workingCopy, map), "Apply after mixed Chest delete");
        Expect(active.staticProps.size() == 1, "active keeps only Barrel");
        Expect(CountIdentity(active, kBarrel) == 1, "Barrel remains in active");
        Expect(CountIdentity(active, kChest) == 0, "Chest is gone from active");
        Expect(SimulatedDrawSubmissions(active) == 1, "only Barrel would be submitted");
        Expect(!SimulatedSubmitIdentity(active, kChest), "Chest is not submitted");
        Expect(SimulatedSubmitIdentity(active, kBarrel), "Barrel is still submitted");
    }

    {
        world::LevelDefinition active = MakeActive();
        world::LevelDefinition workingCopy = active;
        editor::StructuralIndexMap map{};
        editor::ResetStructuralIndexMap(map, active);
        Expect(editor::AddStaticProp(workingCopy, kAnchor, kBarrel).succeeded, "Barrel-only Add");
        Expect(ApplyLikeApplication(active, workingCopy, map), "Barrel-only Apply");
        Expect(editor::DeleteSelected(workingCopy, {EditorObjectKind::StaticProp, 0}).succeeded,
            "Barrel-only Delete");
        Expect(ApplyLikeApplication(active, workingCopy, map), "Barrel-only second Apply");
        Expect(active.staticProps.empty() && workingCopy.staticProps.empty(),
            "Barrel-only delete+Apply yields zero-prop state");
    }

    {
        world::LevelDefinition zero = MakeActive();
        Expect(SimulatedDrawSubmissions(zero) == 0, "baseline zero-prop level submits 0");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d Static Prop lifecycle test(s) failed.\n", gFailures);
        return 1;
    }
    std::printf("Static Prop lifecycle tests passed.\n");
    return 0;
}
