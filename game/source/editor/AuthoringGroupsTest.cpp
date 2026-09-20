#include "editor/AuthoringGroups.h"
#include "editor/AuthoredObjectLifecycle.h"
#include "editor/EditorGizmo.h"
#include "editor/EditorGroupRotate.h"
#include "editor/EditorGroupTranslate.h"
#include "editor/EditorHierarchy.h"
#include "editor/EditorSelection.h"
#include "editor/EditorSelectionSet.h"
#include "editor/ItemIdInspectorEdit.h"
#include "world/ItemPickup.h"
#include "world/LevelDefinition.h"
#include "world/LevelFile.h"
#include "world/LevelWriter.h"
#include "world/LocalLight.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

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

world::StaticPropSpec MakeProp(core::Vec3 position)
{
    world::StaticPropSpec prop{};
    prop.modelIdentity = "models/test_static.glb";
    prop.position = position;
    prop.rotationDegrees = {0.0f, 0.0f, 0.0f};
    prop.scale = {1.0f, 1.0f, 1.0f};
    return prop;
}

world::ItemPickupSpec MakePickup(core::Vec3 position)
{
    world::ItemPickupSpec pickup{};
    pickup.position = position;
    pickup.itemId = "key";
    pickup.quantity = 1;
    pickup.visualScale = {1.0f, 1.0f, 1.0f};
    pickup.showInteractionBounds = true;
    pickup.targetHighlightIntensity = world::kDefaultItemPickupTargetHighlightIntensity;
    pickup.targetHighlightGoldAmount = world::kDefaultItemPickupTargetHighlightGoldAmount;
    return pickup;
}

void MakeWritable(world::LevelDefinition& level)
{
    level.id = "level_group_test";
    level.initialSpawnVisualCenter = {0.0f, 0.8f, 0.0f};
    level.killPlaneY = -8.0f;
    level.ground = {{0.0f, -0.25f, 0.0f}, {16.0f, 0.5f, 8.0f}};
    if (level.elevatedPlatforms.empty())
    {
        level.elevatedPlatforms.push_back({{0.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
    }
    level.checkpoint1PlatformIndex = 0;
    level.checkpoint2PlatformIndex = 0;
    level.goalPlatformIndex = 0;
    level.slopes[0] = {{4.0f, 1.0f, 0.0f}, {4.0f, 0.4f, 2.0f}, 30.0f};
    level.slopes[1] = {{8.0f, 1.0f, 0.0f}, {4.0f, 0.4f, 2.0f}, 60.0f};
    level.movingPlatform.size = {4.0f, 0.4f, 3.0f};
    level.movingPlatform.centerY = 1.3f;
    level.movingPlatform.centerZ = 0.0f;
    level.movingPlatform.pathMinX = -8.0f;
    level.movingPlatform.pathMaxX = 8.0f;
    level.movingPlatform.speed = 2.0f;
    level.movingPlatform.startX = 0.0f;
    level.camera = {{2.0f, 3.5f, 12.0f}, 40.0f};
}

world::LevelDefinition MakeLevelWithProps()
{
    world::LevelDefinition level{};
    MakeWritable(level);
    level.staticProps.push_back(MakeProp({1.0f, 1.0f, 0.0f}));
    level.staticProps.push_back(MakeProp({3.0f, 1.0f, 0.0f}));
    level.staticProps.push_back(MakeProp({5.0f, 1.0f, 0.0f}));
    level.staticProps.push_back(MakeProp({7.0f, 1.0f, 0.0f}));
    level.itemPickups.push_back(MakePickup({2.0f, 1.5f, 0.0f}));
    level.hazards.push_back({{11.5f, 0.5f, 0.0f}, {1.4f, 1.0f, 2.0f}});
    return level;
}

bool Vec3Equal(core::Vec3 a, core::Vec3 b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool NearlyEqual(float a, float b, float epsilon = 0.0001f)
{
    return std::fabs(a - b) <= epsilon;
}

bool NearlyEqualVec(core::Vec3 a, core::Vec3 b, float epsilon = 0.0001f)
{
    return NearlyEqual(a.x, b.x, epsilon) && NearlyEqual(a.y, b.y, epsilon)
        && NearlyEqual(a.z, b.z, epsilon);
}

render::CameraView MakeEditorView(core::Vec3 position, core::Vec3 target)
{
    render::CameraView view{};
    view.position = position;
    view.target = target;
    view.up = {0.0f, 1.0f, 0.0f};
    view.fieldOfViewY = 60.0f;
    return view;
}

editor::Ray3 RayThroughView(const render::CameraView& view, core::Vec3 worldPoint)
{
    editor::Ray3 ray{};
    ray.origin = view.position;
    ray.direction = editor::NormalizeOr(
        {worldPoint.x - view.position.x,
         worldPoint.y - view.position.y,
         worldPoint.z - view.position.z},
        {0.0f, 0.0f, -1.0f});
    return ray;
}

world::SpotLightSpec MakeSpot(core::Vec3 position, core::Vec3 direction)
{
    world::SpotLightSpec light = world::MakeDefaultSpotLight(position);
    light.direction = world::CanonicalSpotLightDirection(direction);
    return light;
}
}

int main()
{
    using editor::EditorObjectKind;
    using editor::EditorSelection;

    {
        world::LevelDefinition working = MakeLevelWithProps();
        const world::LevelDefinition before = working;
        const EditorSelection primary{EditorObjectKind::StaticProp, 0};
        const std::vector<EditorSelection> additional{{EditorObjectKind::StaticProp, 1}};
        Expect(editor::CanCreateAuthoringGroup(working, primary, additional),
            "1. Group Selected enabled for 2+ objects");
        const editor::AuthoringGroupEditResult created =
            editor::CreateAuthoringGroupFromSelection(working, primary, additional);
        Expect(created.succeeded, "1. create group from 2 selected objects");
        Expect(working.authoringGroups.size() == 1, "1. one group created");
        Expect(working.authoringGroups[0].members.size() == 2, "2. every member once");
        Expect(working.authoringGroups[0].members[0].index == 0
                && working.authoringGroups[0].members[1].index == 1,
            "3. member order is PRIMARY then additional");
        Expect(created.selection == primary && created.additionalSelections == additional,
            "4. PRIMARY identity/order preserved");
        Expect(Vec3Equal(working.staticProps[0].position, before.staticProps[0].position)
                && Vec3Equal(working.staticProps[1].position, before.staticProps[1].position)
                && Vec3Equal(
                    working.staticProps[0].rotationDegrees, before.staticProps[0].rotationDegrees)
                && Vec3Equal(
                    working.staticProps[1].rotationDegrees, before.staticProps[1].rotationDegrees)
                && working.staticProps[0].modelIdentity == before.staticProps[0].modelIdentity,
            "5. grouping does not mutate member transforms/properties");
        Expect(!world::AuthoredLevelDataEqual(working, before),
            "6. group metadata makes workingCopy Dirty");
        Expect(working.authoringGroups[0].name == "Group_01", "default name Group_01");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        Expect(
            !editor::CanCreateAuthoringGroup(
                working,
                {EditorObjectKind::Environment, 0},
                {{EditorObjectKind::StaticProp, 0}}),
            "Environment cannot join an Authoring Group");
        Expect(
            !editor::CanCreateAuthoringGroup(
                working,
                {EditorObjectKind::DirectionalLight, 0},
                {{EditorObjectKind::StaticProp, 0}}),
            "Directional Light cannot join an Authoring Group");
        Expect(
            !editor::CanDuplicateSelected(
                true, working, {EditorObjectKind::Environment, 0}, false),
            "Environment cannot Duplicate");
        Expect(
            !editor::CanDeleteSelected(
                true, working, {EditorObjectKind::DirectionalLight, 0}, false),
            "Directional Light cannot Delete");
        Expect(
            !editor::SelectionCanJoinAuthoringGroup(
                working, {EditorObjectKind::Environment, 0}),
            "Environment remains ungroupable");
        Expect(
            !editor::SelectionCanJoinAuthoringGroup(
                working, {EditorObjectKind::DirectionalLight, 0}),
            "Directional Light remains ungroupable");
        working.pointLights.push_back(world::MakeDefaultPointLight({0.0f, 2.0f, 0.0f}));
        working.spotLights.push_back(world::MakeDefaultSpotLight({1.0f, 4.0f, 0.0f}));
        Expect(
            editor::SelectionCanJoinAuthoringGroup(
                working, {EditorObjectKind::PointLight, 0}),
            "1. Point Light is a valid Authoring Group member");
        Expect(
            editor::SelectionCanJoinAuthoringGroup(
                working, {EditorObjectKind::SpotLight, 0}),
            "2. Spot Light is a valid Authoring Group member");
        Expect(
            editor::CanCreateAuthoringGroup(
                working,
                {EditorObjectKind::StaticProp, 0},
                {{EditorObjectKind::PointLight, 0}}),
            "3. Static Prop + Point Light can Group Selected");
        Expect(
            editor::CanCreateAuthoringGroup(
                working,
                {EditorObjectKind::StaticProp, 0},
                {{EditorObjectKind::SpotLight, 0}}),
            "4. Static Prop + Spot Light can Group Selected");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 0},
            {{EditorObjectKind::StaticProp, 1}});
        const std::string text = world::SerializeLevelText(working);
        Expect(!text.empty(), "7. writable grouped level serializes");
        Expect(text.find("authoring_group Group_01 static_prop 0 static_prop 1") != std::string::npos,
            "7. serialized group membership");
        const world::ParseLevelFileResult parsed = world::ParseLevelText(text);
        Expect(parsed.status == world::LoadLevelFileStatus::Loaded, "7. parse roundtrip loads");
        Expect(world::AuthoredLevelDataEqual(working, parsed.level),
            "7. Save/parse roundtrip preserves group metadata");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        working.authoringGroups.clear();
        const std::string text = world::SerializeLevelText(working);
        Expect(text.find("authoring_group") == std::string::npos,
            "8. group-less Level emits no authoring_group");
        const world::ParseLevelFileResult parsed = world::ParseLevelText(text);
        Expect(parsed.status == world::LoadLevelFileStatus::Loaded, "8. old Level without groups loads");
        Expect(parsed.level.authoringGroups.empty(), "8. parsed groups stay empty");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 1},
            {{EditorObjectKind::StaticProp, 0}, {EditorObjectKind::ItemPickup, 0}});
        EditorSelection primary{};
        std::vector<EditorSelection> additional;
        Expect(editor::TrySelectAuthoringGroup(working, 0, primary, additional),
            "9. selecting persisted group succeeds");
        Expect(primary.kind == EditorObjectKind::StaticProp && primary.index == 1,
            "9. reconstructed PRIMARY is stored members[0]");
        Expect(additional.size() == 2, "9. remaining members are secondaries");
        Expect(editor::EditorSelectionSetSupportsGroupTranslate(working, primary, additional),
            "10. selected group uses existing Group Translate eligibility");
        Expect(editor::EditorSelectionSetSupportsGroupRotate(working, primary, additional),
            "11. compatible selected group uses existing Group Rotate eligibility");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 0},
            {{EditorObjectKind::Hazard, 0}});
        EditorSelection primary{};
        std::vector<EditorSelection> additional;
        editor::TrySelectAuthoringGroup(working, 0, primary, additional);
        Expect(!editor::EditorSelectionSetSupportsGroupRotate(working, primary, additional),
            "12. non-rotatable member refuses Group Rotate");
        Expect(editor::GroupRotateDisableReason(working, primary, additional) != nullptr,
            "12. existing atomic Rotate refusal text");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 0},
            {{EditorObjectKind::StaticProp, 1}});
        const world::LevelDefinition before = working;
        const editor::AuthoringGroupEditResult refused = editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 0},
            {{EditorObjectKind::StaticProp, 2}});
        Expect(!refused.succeeded, "13. object cannot belong to two groups");
        Expect(refused.status == editor::AuthoringGroupEditStatus::MemberAlreadyGrouped,
            "14. overlapping grouping is refused");
        Expect(world::AuthoredLevelDataEqual(working, before),
            "14. overlapping Group Selected is atomic");
        Expect(working.authoringGroups.size() == 1, "15. nested grouping is not introduced");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 0},
            {{EditorObjectKind::StaticProp, 1}});
        Expect(editor::RenameAuthoringGroup(working, 0, "Door_Puzzle").succeeded, "16. rename succeeds");
        Expect(working.authoringGroups[0].name == "Door_Puzzle", "16. rename persists in workingCopy");
        Expect(!editor::RenameAuthoringGroup(working, 0, "").succeeded, "17. empty rename refused");
        Expect(!editor::RenameAuthoringGroup(working, 0, "Door Puzzle").succeeded,
            "17. space rename refused");
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 2},
            {{EditorObjectKind::StaticProp, 3}});
        Expect(!editor::RenameAuthoringGroup(working, 1, "Door_Puzzle").succeeded,
            "17. duplicate rename refused");
        Expect(working.authoringGroups[1].name == "Group_01"
                || working.authoringGroups[1].name == "Group_02",
            "second default name is deterministic");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        const world::StaticPropSpec prop0 = working.staticProps[0];
        const world::StaticPropSpec prop1 = working.staticProps[1];
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 0},
            {{EditorObjectKind::StaticProp, 1}});
        const editor::AuthoringGroupEditResult ungrouped = editor::UngroupAuthoringGroup(working, 0);
        Expect(ungrouped.succeeded, "18. Ungroup succeeds");
        Expect(working.authoringGroups.empty(), "18. Ungroup removes metadata only");
        Expect(working.staticProps.size() == 4, "19. Ungroup preserves member objects");
        Expect(Vec3Equal(working.staticProps[0].position, prop0.position)
                && Vec3Equal(working.staticProps[1].position, prop1.position)
                && working.staticProps[0].modelIdentity == prop0.modelIdentity,
            "19. Ungroup preserves properties/transforms");
        Expect(ungrouped.selection.kind == EditorObjectKind::StaticProp
                && ungrouped.selection.index == 0
                && ungrouped.additionalSelections.size() == 1
                && ungrouped.additionalSelections[0].index == 1,
            "20. Ungroup keeps a valid M78 multi-selection");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        const EditorSelection primary{EditorObjectKind::StaticProp, 0};
        const std::vector<EditorSelection> additional{{EditorObjectKind::StaticProp, 1}};
        editor::CreateAuthoringGroupFromSelection(working, primary, additional);
        const std::size_t originalCount = working.staticProps.size();
        const editor::LifecycleEditResult duplicated =
            editor::DuplicateSelectionSet(working, primary, additional);
        Expect(duplicated.succeeded, "21. complete group Duplicate Selected succeeds");
        Expect(working.staticProps.size() == originalCount + 2,
            "21. members duplicated exactly once");
        Expect(working.authoringGroups.size() == 2, "22. copied composition receives a new group");
        Expect(working.authoringGroups[0].name == "Group_01", "23. original group kept");
        Expect(working.authoringGroups[0].members[0].index == 0
                && working.authoringGroups[0].members[1].index == 1,
            "23. originals remain in original group");
        Expect(working.authoringGroups[1].name == "Group_01_Copy", "22. copied group name");
        Expect(working.authoringGroups[1].members[0].index == originalCount
                && working.authoringGroups[1].members[1].index == originalCount + 1,
            "24. copies belong only to copied group");
        Expect(duplicated.selection.kind == EditorObjectKind::StaticProp
                && duplicated.selection.index == originalCount,
            "25. copied PRIMARY remains PRIMARY");
        Expect(editor::FindAuthoringGroupContaining(working, duplicated.selection) == 1,
            "24. copied PRIMARY is in copied group");
        Expect(editor::FindAuthoringGroupContaining(
                   working, {EditorObjectKind::StaticProp, 0})
                == 0,
            "23. original PRIMARY stays in original group");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 0},
            {{EditorObjectKind::StaticProp, 1}});
        const world::LevelDefinition before = working;
        const editor::LifecycleEditResult refused = editor::DuplicateSelectionSet(
            working, {EditorObjectKind::StaticProp, 0}, {});
        Expect(!refused.succeeded, "partial group duplicate refused");
        Expect(refused.status == editor::LifecycleEditStatus::InvalidGroupOperation,
            "partial group duplicate uses InvalidGroupOperation");
        Expect(world::AuthoredLevelDataEqual(working, before),
            "partial group duplicate is atomic");
        const editor::LifecycleEditResult mixed = editor::DuplicateSelectionSet(
            working,
            {EditorObjectKind::StaticProp, 0},
            {{EditorObjectKind::StaticProp, 2}});
        Expect(!mixed.succeeded, "mixed grouped+ungrouped duplicate refused");
        Expect(world::AuthoredLevelDataEqual(working, before),
            "mixed partial duplicate is atomic");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 0},
            {{EditorObjectKind::StaticProp, 1}});
        const std::size_t remaining = working.staticProps.size() - 2;
        const editor::LifecycleEditResult deleted = editor::DeleteSelectionSet(
            working,
            {EditorObjectKind::StaticProp, 0},
            {{EditorObjectKind::StaticProp, 1}});
        Expect(deleted.succeeded, "26. complete group Delete Selected succeeds");
        Expect(working.staticProps.size() == remaining, "26. members removed");
        Expect(working.authoringGroups.empty(), "26. empty group metadata removed");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        working.staticProps.push_back(MakeProp({9.0f, 1.0f, 0.0f}));
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 1},
            {{EditorObjectKind::StaticProp, 4}});
        Expect(editor::DeleteSelected(working, {EditorObjectKind::StaticProp, 2}).succeeded,
            "escaped-bug: delete unrelated same-category object");
        Expect(working.authoringGroups.size() == 1, "30. group survives unrelated delete");
        Expect(working.authoringGroups[0].members[0].index == 1
                && working.authoringGroups[0].members[1].index == 3,
            "30. same-category deletion remaps former index 4 to 3");
        Expect(editor::DeleteSelected(working, {EditorObjectKind::StaticProp, 1}).succeeded,
            "27. individual member deletion");
        Expect(working.authoringGroups.empty(),
            "29. group auto-dissolves below 2 surviving members");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 1},
            {{EditorObjectKind::ItemPickup, 0}, {EditorObjectKind::Hazard, 0}});
        Expect(editor::DeleteSelected(working, {EditorObjectKind::ItemPickup, 0}).succeeded,
            "27. delete one grouped Item Pickup");
        Expect(working.authoringGroups.size() == 1, "28. unrelated group members survive");
        Expect(working.authoringGroups[0].members.size() == 2, "28. two members remain");
        Expect(working.authoringGroups[0].members[0].kind
                == world::AuthoringGroupMemberKind::StaticProp
                && working.authoringGroups[0].members[0].index == 1,
            "32. mixed-category delete keeps surviving membership");
        Expect(working.authoringGroups[0].members[1].kind
                == world::AuthoringGroupMemberKind::Hazard
                && working.authoringGroups[0].members[1].index == 0,
            "32. mixed-category remaining hazard index unchanged");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 0},
            {{EditorObjectKind::StaticProp, 1}});
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 2},
            {{EditorObjectKind::StaticProp, 3}});
        Expect(working.authoringGroups.size() == 2, "two independent groups");
        Expect(editor::DeleteSelected(working, {EditorObjectKind::Hazard, 0}).succeeded,
            "delete unrelated category");
        Expect(working.authoringGroups.size() == 2, "33. unrelated groups survive lifecycle");
        Expect(editor::AddStaticPropAt(working, {11.0f, 1.0f, 0.0f}, "models/test_static.glb")
                .succeeded,
            "34. Add succeeds");
        Expect(working.authoringGroups.size() == 2
                && working.authoringGroups[0].members[0].index == 0
                && working.authoringGroups[1].members[1].index == 3,
            "34. Add append does not corrupt memberships");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        working.doors.push_back({{4.0f, 1.5f, 0.0f}, {1.2f, 3.0f, 2.4f}, 3.2f, ""});
        working.doors.push_back({{8.0f, 1.5f, 0.0f}, {1.2f, 3.0f, 2.4f}, 3.2f, ""});
        world::PressurePlateSpec plate{};
        plate.center = {4.0f, 0.1f, 0.0f};
        plate.size = {2.0f, 0.2f, 2.0f};
        plate.linkedDoorIndex = 1;
        working.pressurePlates.push_back(plate);
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 0},
            {{EditorObjectKind::StaticProp, 1}});
        Expect(editor::DeleteSelected(working, {EditorObjectKind::Door, 0}).succeeded,
            "39. Door delete while groups exist");
        Expect(working.pressurePlates[0].linkedDoorIndex == 0,
            "39. M79 Pressure Plate -> Door remap remains correct");
        Expect(working.authoringGroups.size() == 1, "39. groups survive Door remap");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        working.elevatedPlatforms.push_back({{12.0f, 0.75f, 0.0f}, {4.0f, 0.5f, 3.0f}});
        working.checkpoint1PlatformIndex = 0;
        working.checkpoint2PlatformIndex = 1;
        working.goalPlatformIndex = 1;
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 0},
            {{EditorObjectKind::StaticProp, 1}});
        Expect(editor::DeleteSelected(working, {EditorObjectKind::ElevatedPlatform, 0}).succeeded
                == false,
            "40. referenced first platform still refused");
        working.checkpoint1PlatformIndex = 1;
        Expect(editor::DeleteSelected(working, {EditorObjectKind::ElevatedPlatform, 0}).succeeded,
            "40. unreferenced earlier platform deletes");
        Expect(working.checkpoint2PlatformIndex == 0 && working.goalPlatformIndex == 0,
            "40. M79 platform support-index remapping remains correct");
        Expect(working.authoringGroups[0].members[0].index == 0,
            "40. group membership independent of support remap");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 0},
            {{EditorObjectKind::StaticProp, 1}});
        working.itemPickups[0].itemId = "card";
        editor::ItemIdInspectorFieldState field{};
        editor::CopyItemIdInspectorBuffer(field.buffer, "key");
        field.boundSelection = {EditorObjectKind::ItemPickup, 0};
        std::string dest = working.itemPickups[0].itemId;
        Expect(editor::TryAcceptItemIdInspectorField(dest, field)
                == editor::ItemIdInspectorCommitResult::Accepted,
            "41. Item ID Inspector accept still works");
        Expect(dest == "key", "41. Item ID write is PRIMARY-only");
        Expect(working.authoringGroups.size() == 1, "41. Item ID edit does not touch groups");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 0},
            {{EditorObjectKind::StaticProp, 1}});
        EditorSelection primary{};
        std::vector<EditorSelection> additional;
        editor::TrySelectAuthoringGroup(working, 0, primary, additional);
        const std::vector<EditorSelection> members =
            editor::EditorSelectionSetMembers(primary, additional);
        std::vector<core::Vec3> starts;
        Expect(editor::CaptureGroupTranslateStarts(working, members, starts),
            "10. capture Group Translate starts from selected group");
        Expect(editor::ApplyGroupTranslateFromPrimaryResult(
                working,
                members,
                starts,
                {2.0f, 1.0f, 0.0f},
                editor::EditorAxis::X,
                false,
                0.25f),
            "10. Group Translate mutates reconstructed selection");
        Expect(working.staticProps[0].position.x == 2.0f, "10. PRIMARY translated");
        Expect(working.staticProps[1].position.x == 4.0f, "10. secondary shares delta");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 0},
            {{EditorObjectKind::StaticProp, 1}});
        EditorSelection primary{};
        std::vector<EditorSelection> additional;
        editor::TrySelectAuthoringGroup(working, 0, primary, additional);
        const std::vector<EditorSelection> members =
            editor::EditorSelectionSetMembers(primary, additional);
        std::vector<core::Vec3> startPositions;
        std::vector<core::Vec3> startRotations;
        core::Vec3 pivot{};
        Expect(editor::TryPrimaryRotatePivot(working, primary, pivot), "11. PRIMARY rotate pivot");
        Expect(editor::CaptureGroupRotateStarts(working, members, startPositions, startRotations),
            "11. capture Group Rotate starts");
        Expect(editor::ApplyGroupRotateFromPrimaryResult(
                working,
                members,
                startPositions,
                startRotations,
                pivot,
                {90.0f, 0.0f, 0.0f},
                editor::EditorAxis::X,
                false,
                15.0f),
            "11. Group Rotate reuses M80");
        Expect(working.staticProps[0].rotationDegrees.x == 90.0f, "11. PRIMARY rotation");
        Expect(working.staticProps[0].position.x == 1.0f, "11. PRIMARY position stays");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 0},
            {{EditorObjectKind::StaticProp, 1}});
        const world::LevelDefinition applied = working;
        world::LevelDefinition reverted = applied;
        Expect(world::AuthoredLevelDataEqual(reverted, applied), "35. Apply candidate equals workingCopy");
        reverted.authoringGroups.clear();
        Expect(!world::AuthoredLevelDataEqual(reverted, applied),
            "36. Revert restoring groups is visible to Dirty");
        Expect(world::AuthoredLevelDataEqual(applied, applied), "35. identical Apply comparison");
    }

    {
        const world::ParseLevelFileResult malformed = world::ParseLevelText(
            "PLATFORMER_LEVEL 1\n"
            "id level_group_test\n"
            "authoring_group Group_01 static_prop 0\n");
        Expect(malformed.status == world::LoadLevelFileStatus::Invalid,
            "malformed one-member group is rejected");
        const world::ParseLevelFileResult badKind = world::ParseLevelText(
            "PLATFORMER_LEVEL 1\n"
            "id level_group_test\n"
            "authoring_group Group_01 prefab 0 static_prop 1\n");
        Expect(badKind.status == world::LoadLevelFileStatus::Invalid,
            "unknown member kind is rejected");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 1},
            {{EditorObjectKind::StaticProp, 3}});
        editor::DuplicateSelectionSet(
            working,
            {EditorObjectKind::StaticProp, 1},
            {{EditorObjectKind::StaticProp, 3}});
        Expect(working.authoringGroups.size() == 2, "escaped-bug: duplicate-group membership");
        Expect(editor::DeleteSelected(working, {EditorObjectKind::StaticProp, 0}).succeeded,
            "escaped-bug: delete before both groups");
        Expect(working.authoringGroups[0].members[0].index == 0
                && working.authoringGroups[0].members[1].index == 2,
            "escaped-bug: original group remaps after earlier delete");
        Expect(working.authoringGroups[1].members[0].index == 3
                && working.authoringGroups[1].members[1].index == 4,
            "escaped-bug: copied group remaps after earlier delete");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        const EditorSelection primary{EditorObjectKind::StaticProp, 0};
        const std::vector<EditorSelection> additional{{EditorObjectKind::StaticProp, 1}};
        editor::CreateAuthoringGroupFromSelection(working, primary, additional);
        const world::LevelDefinition beforeClick = working;
        const std::vector<editor::HierarchyRow> rows = editor::BuildHierarchyRows(working);
        Expect(editor::FindHierarchyGroupRow(rows, 0) != nullptr, "1. group row exists");
        Expect(editor::FindHierarchyMemberRow(rows, primary) != nullptr,
            "6. grouped member is identifiable");
        Expect(editor::FindHierarchyMemberRow(rows, additional[0]) != nullptr,
            "6. second grouped member is identifiable");
        Expect(!editor::HierarchyHasDuplicateTopLevelObject(rows, primary)
                && !editor::HierarchyHasDuplicateTopLevelObject(rows, additional[0]),
            "7. grouped members are not duplicate top-level objects");
        Expect(editor::FindHierarchyUngroupedRow(rows, primary) == nullptr,
            "7. grouped PRIMARY is omitted from ungrouped list");
        Expect(editor::FindHierarchyUngroupedRow(rows, {EditorObjectKind::StaticProp, 2}) != nullptr,
            "ungrouped Static Prop remains a top-level object");

        std::size_t groupCount = 0;
        std::size_t memberCount = 0;
        std::size_t firstMemberIndex = 99;
        std::size_t secondMemberIndex = 99;
        for (std::size_t rowIndex = 0; rowIndex < rows.size(); ++rowIndex)
        {
            if (rows[rowIndex].kind == editor::HierarchyRowKind::Group)
            {
                ++groupCount;
                Expect(rows[rowIndex].groupIndex == 0, "8. group display order is authored order");
            }
            if (rows[rowIndex].kind == editor::HierarchyRowKind::GroupMember)
            {
                if (memberCount == 0)
                {
                    firstMemberIndex = rowIndex;
                    Expect(rows[rowIndex].selection == primary, "9. members[0] is first member row");
                }
                if (memberCount == 1)
                {
                    secondMemberIndex = rowIndex;
                    Expect(rows[rowIndex].selection == additional[0],
                        "9. remaining members follow persistent order");
                }
                ++memberCount;
            }
        }
        Expect(groupCount == 1 && memberCount == 2, "8. one group and two member rows");
        Expect(firstMemberIndex + 1 == secondMemberIndex, "9. member rows are consecutive");

        EditorSelection selectedPrimary{};
        std::vector<EditorSelection> selectedAdditional;
        Expect(
            editor::ApplyHierarchyRowClick(
                working, *editor::FindHierarchyGroupRow(rows, 0), selectedPrimary, selectedAdditional, false),
            "1. group row click reconstructs selection");
        Expect(selectedPrimary == primary && selectedAdditional == additional,
            "1. group row reconstructs M81 PRIMARY + secondaries");
        Expect(selectedPrimary.index == 0, "10. members[0] remains PRIMARY");
        Expect(editor::HierarchyGroupRowIsSelected(working, 0, selectedPrimary, selectedAdditional),
            "group row selected visual matches complete group");
        Expect(world::AuthoredLevelDataEqual(working, beforeClick),
            "group row click does not mutate authored data");

        EditorSelection memberPrimary = selectedPrimary;
        std::vector<EditorSelection> memberAdditional = selectedAdditional;
        Expect(
            editor::ApplyHierarchyRowClick(
                working,
                *editor::FindHierarchyMemberRow(rows, additional[0]),
                memberPrimary,
                memberAdditional,
                false),
            "2. member row click selects that object");
        Expect(memberPrimary == additional[0] && memberAdditional.empty(),
            "2. member row selects only that authored object");
        Expect(working.authoringGroups[0].members.size() == 2
                && working.authoringGroups[0].members[0].index == 0
                && working.authoringGroups[0].members[1].index == 1,
            "3. member selection does not alter membership");
        Expect(world::AuthoredLevelDataEqual(working, beforeClick),
            "4. member selection does not mark Dirty");

        Expect(
            editor::ApplyHierarchyRowClick(
                working,
                *editor::FindHierarchyGroupRow(rows, 0),
                memberPrimary,
                memberAdditional,
                false),
            "5. clicking group after member restores full group");
        Expect(memberPrimary == primary && memberAdditional == additional,
            "5. restored selection is the complete group");

        EditorSelection ctrlPrimary = primary;
        std::vector<EditorSelection> ctrlAdditional;
        Expect(
            editor::ApplyHierarchyRowClick(
                working,
                *editor::FindHierarchyMemberRow(rows, additional[0]),
                ctrlPrimary,
                ctrlAdditional,
                true),
            "Ctrl member row uses M78 toggle");
        Expect(ctrlPrimary == additional[0] && ctrlAdditional.size() == 1
                && ctrlAdditional[0] == primary,
            "Ctrl-clicking a grouped member toggles like a normal object row");

        EditorSelection groupCtrlPrimary = additional[0];
        std::vector<EditorSelection> groupCtrlAdditional;
        Expect(
            editor::ApplyHierarchyRowClick(
                working,
                *editor::FindHierarchyGroupRow(rows, 0),
                groupCtrlPrimary,
                groupCtrlAdditional,
                true),
            "Ctrl on a group row still selects the complete group");
        Expect(groupCtrlPrimary == primary && groupCtrlAdditional == additional,
            "group-row Ctrl does not add a second group selection");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        const EditorSelection primary{EditorObjectKind::StaticProp, 0};
        const std::vector<EditorSelection> additional{{EditorObjectKind::StaticProp, 1}};
        const editor::AuthoringGroupEditResult created =
            editor::CreateAuthoringGroupFromSelection(working, primary, additional);
        Expect(created.succeeded, "11. Group Selected succeeds");
        editor::HierarchyExpansionState expansion{};
        editor::RevealHierarchyGroup(expansion, created.groupIndex, working);
        const std::vector<editor::HierarchyRow> rows = editor::BuildHierarchyRows(working);
        Expect(editor::FindHierarchyGroupRow(rows, created.groupIndex) != nullptr,
            "11. Group Selected produces an immediately representable group");
        Expect(editor::HierarchyGroupIsExpanded(expansion, working.authoringGroups[0].name),
            "11. new group is revealed/expanded");
        Expect(!editor::HierarchyHasDuplicateTopLevelObject(rows, primary),
            "11. grouped member is not duplicated after Group Selected");
        Expect(created.selection == primary && created.additionalSelections == additional,
            "11. complete composition remains selected");

        const world::LevelDefinition beforeExpand = working;
        editor::SetHierarchyGroupExpanded(expansion, working.authoringGroups[0].name, false);
        Expect(!editor::HierarchyGroupIsExpanded(expansion, working.authoringGroups[0].name),
            "collapse hides only member rows");
        Expect(world::AuthoredLevelDataEqual(working, beforeExpand),
            "28. expand/collapse does not mutate LevelDefinition");
        Expect(working.authoringGroups.size() == 1 && working.staticProps.size() == 4,
            "28. collapse does not hide/delete world objects");
        editor::SetHierarchyGroupExpanded(expansion, working.authoringGroups[0].name, true);
        Expect(world::AuthoredLevelDataEqual(working, beforeExpand),
            "28. expanding again still does not mark Dirty");

        const world::LevelDefinition beforeRename = working;
        EditorSelection renamePrimary = created.selection;
        std::vector<EditorSelection> renameAdditional = created.additionalSelections;
        const editor::AuthoringGroupEditResult renamed =
            editor::RenameAuthoringGroup(working, 0, "Door_Puzzle");
        Expect(renamed.succeeded, "12. valid rename succeeds");
        Expect(renamed.selection == renamePrimary && renamed.additionalSelections == renameAdditional,
            "12. Rename preserves group selection");
        Expect(working.authoringGroups[0].name == "Door_Puzzle", "12. rename changes group metadata");
        Expect(working.staticProps[0].modelIdentity == beforeRename.staticProps[0].modelIdentity
                && Vec3Equal(working.staticProps[0].position, beforeRename.staticProps[0].position)
                && Vec3Equal(working.staticProps[1].position, beforeRename.staticProps[1].position),
            "12. member objects remain untouched");

        const world::LevelDefinition afterValidRename = working;
        Expect(!editor::RenameAuthoringGroup(working, 0, "").succeeded, "13. invalid rename refused");
        Expect(working.authoringGroups[0].name == "Door_Puzzle"
                && world::AuthoredLevelDataEqual(working, afterValidRename),
            "13. invalid rename does not mutate data");
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 2},
            {{EditorObjectKind::StaticProp, 3}});
        const world::LevelDefinition afterSecondGroup = working;
        Expect(!editor::RenameAuthoringGroup(working, 1, "Door_Puzzle").succeeded,
            "14. duplicate-name rename refused");
        Expect(world::AuthoredLevelDataEqual(working, afterSecondGroup),
            "14. duplicate-name rename does not mutate data");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        const EditorSelection primary{EditorObjectKind::StaticProp, 0};
        const std::vector<EditorSelection> additional{{EditorObjectKind::StaticProp, 1}};
        editor::CreateAuthoringGroupFromSelection(working, primary, additional);
        editor::HierarchyExpansionState expansion{};
        expansion.collapsedGroupNames.push_back("Group_01");
        expansion.renameFromHierarchy = true;
        const world::StaticPropSpec prop0 = working.staticProps[0];
        const world::StaticPropSpec prop1 = working.staticProps[1];
        const editor::AuthoringGroupEditResult ungrouped = editor::UngroupAuthoringGroup(working, 0);
        Expect(ungrouped.succeeded, "15. Ungroup succeeds");
        editor::ReconcileHierarchyExpansion(expansion, working);
        const std::vector<editor::HierarchyRow> rows = editor::BuildHierarchyRows(working);
        Expect(editor::FindHierarchyGroupRow(rows, 0) == nullptr, "15. Ungroup removes group presentation");
        Expect(expansion.collapsedGroupNames.empty(), "15. stale group expansion is dropped");
        Expect(editor::FindHierarchyUngroupedRow(rows, primary) != nullptr
                && editor::FindHierarchyUngroupedRow(rows, additional[0]) != nullptr,
            "16. former members become ungrouped objects");
        Expect(working.staticProps.size() == 4
                && working.staticProps[0].position.x == prop0.position.x
                && working.staticProps[1].position.x == prop1.position.x,
            "16. Ungroup preserves every member object");
        Expect(ungrouped.selection == primary && ungrouped.additionalSelections == additional,
            "17. Ungroup preserves the M78 multi-selection");
        Expect(ungrouped.selection.index == 0, "17. PRIMARY remains deterministic");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        const EditorSelection primary{EditorObjectKind::StaticProp, 0};
        const std::vector<EditorSelection> additional{{EditorObjectKind::StaticProp, 1}};
        editor::CreateAuthoringGroupFromSelection(working, primary, additional);
        const std::size_t originalCount = working.staticProps.size();
        const editor::LifecycleEditResult duplicated =
            editor::DuplicateSelectionSet(working, primary, additional);
        Expect(duplicated.succeeded, "18. complete-group Duplicate succeeds");
        const std::vector<editor::HierarchyRow> rows = editor::BuildHierarchyRows(working);
        Expect(working.authoringGroups.size() == 2, "18. copied group exists");
        Expect(editor::FindHierarchyGroupRow(rows, 1) != nullptr,
            "18. copied group is presented");
        Expect(duplicated.selection.kind == EditorObjectKind::StaticProp
                && duplicated.selection.index == originalCount,
            "19. copied composition PRIMARY is the copied members[0]");
        Expect(editor::FindExactAuthoringGroup(
                   working, duplicated.selection, duplicated.additionalSelections)
                == 1,
            "19. copied group selection corresponds to copied composition");
        Expect(editor::FindHierarchyMemberRow(rows, duplicated.selection) != nullptr
                && editor::FindHierarchyMemberRow(rows, duplicated.additionalSelections[0]) != nullptr,
            "18. copied members are represented under the copied group");
        Expect(!editor::HierarchyHasDuplicateTopLevelObject(rows, duplicated.selection),
            "18. copied member is not also a top-level duplicate");
        Expect(working.authoringGroups[0].name == "Group_01"
                && working.authoringGroups[0].members[0].index == 0,
            "18. original group remains intact");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 0},
            {{EditorObjectKind::StaticProp, 1}});
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 2},
            {{EditorObjectKind::StaticProp, 3}});
        editor::HierarchyExpansionState expansion{};
        expansion.collapsedGroupNames.push_back("Group_01");
        Expect(
            editor::DeleteSelectionSet(
                working,
                {EditorObjectKind::StaticProp, 0},
                {{EditorObjectKind::StaticProp, 1}})
                .succeeded,
            "20. complete-group Delete Selected succeeds");
        editor::ReconcileHierarchyExpansion(expansion, working);
        const std::vector<editor::HierarchyRow> afterDelete = editor::BuildHierarchyRows(working);
        Expect(working.authoringGroups.size() == 1, "20. unrelated group remains");
        Expect(working.staticProps.size() == 2, "20. deleted members are removed from the Level");
        Expect(working.authoringGroups[0].members.size() == 2
                && working.authoringGroups[0].members[0].index == 0
                && working.authoringGroups[0].members[1].index == 1,
            "20. surviving group remaps onto remaining objects");
        Expect(editor::FindHierarchyGroupRow(afterDelete, 0) != nullptr,
            "20. surviving group is still presented");
        Expect(editor::FindHierarchyMemberRow(afterDelete, {EditorObjectKind::StaticProp, 0}) != nullptr
                && editor::FindHierarchyMemberRow(afterDelete, {EditorObjectKind::StaticProp, 1}) != nullptr,
            "20. remapped surviving members remain presented");
        Expect(!editor::HierarchyHasDuplicateTopLevelObject(
                   afterDelete, {EditorObjectKind::StaticProp, 0}),
            "20. surviving members are not duplicated as top-level objects");
        Expect(expansion.collapsedGroupNames.empty(),
            "20. stale expansion for the deleted group is dropped");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        working.staticProps.push_back(MakeProp({9.0f, 1.0f, 0.0f}));
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 0},
            std::vector<EditorSelection>{
                {EditorObjectKind::StaticProp, 1}, {EditorObjectKind::StaticProp, 2}});
        Expect(editor::DeleteSelected(working, {EditorObjectKind::StaticProp, 1}).succeeded,
            "21. individual member delete succeeds");
        const std::vector<editor::HierarchyRow> afterMemberDelete = editor::BuildHierarchyRows(working);
        Expect(working.authoringGroups.size() == 1, "21. group survives with 2+ members");
        Expect(working.authoringGroups[0].members.size() == 2, "21. membership updates");
        Expect(editor::FindHierarchyMemberRow(
                   afterMemberDelete,
                   {EditorObjectKind::StaticProp, working.authoringGroups[0].members[0].index})
                != nullptr,
            "21. surviving member rows remain");
        Expect(
            editor::FindHierarchyMemberRow(afterMemberDelete, {EditorObjectKind::StaticProp, 2}) != nullptr
                || editor::FindHierarchyMemberRow(afterMemberDelete, {EditorObjectKind::StaticProp, 1})
                    != nullptr,
            "23. same-category remap still displays the surviving member");

        Expect(editor::DeleteSelected(working, {EditorObjectKind::StaticProp, 0}).succeeded,
            "22. deleting down to one member auto-dissolves");
        editor::HierarchyExpansionState expansion{};
        expansion.collapsedGroupNames.push_back("Group_01");
        editor::ReconcileHierarchyExpansion(expansion, working);
        const std::vector<editor::HierarchyRow> afterDissolve = editor::BuildHierarchyRows(working);
        Expect(working.authoringGroups.empty(), "22. auto-dissolve removes group metadata");
        Expect(editor::FindHierarchyGroupRow(afterDissolve, 0) == nullptr,
            "22. auto-dissolve removes stale group UI");
        Expect(expansion.collapsedGroupNames.empty(), "22. dissolved group expansion is dropped");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 1},
            {{EditorObjectKind::StaticProp, 3}});
        Expect(editor::DeleteSelected(working, {EditorObjectKind::StaticProp, 0}).succeeded,
            "23. deleting an earlier same-category object remaps");
        const std::vector<editor::HierarchyRow> remapped = editor::BuildHierarchyRows(working);
        Expect(working.authoringGroups[0].members[0].index == 0
                && working.authoringGroups[0].members[1].index == 2,
            "23. membership indices remap");
        Expect(editor::FindHierarchyMemberRow(remapped, {EditorObjectKind::StaticProp, 0}) != nullptr
                && editor::FindHierarchyMemberRow(remapped, {EditorObjectKind::StaticProp, 2}) != nullptr,
            "23. remapped members remain identifiable");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        working.pointLights.push_back(world::MakeDefaultPointLight({1.0f, 3.0f, 0.0f}));
        working.pointLights[0].color = {0.2f, 0.4f, 0.6f};
        working.pointLights[0].intensity = 2.25f;
        working.pointLights[0].range = 12.0f;
        working.pointLights[0].enabled = false;
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 0},
            {{EditorObjectKind::PointLight, 0}});
        Expect(working.authoringGroups[0].members[0].kind
                == world::AuthoringGroupMemberKind::StaticProp,
            "fixture group PRIMARY is Static Prop");
        Expect(working.authoringGroups[0].members[1].kind
                == world::AuthoringGroupMemberKind::PointLight,
            "fixture group stores point_light member kind");
        const std::string text = world::SerializeLevelText(working);
        Expect(text.find("authoring_group Group_01 static_prop 0 point_light 0") != std::string::npos,
            "5. writer emits point_light typed member token");
        const world::ParseLevelFileResult parsed = world::ParseLevelText(text);
        Expect(parsed.status == world::LoadLevelFileStatus::Loaded, "5. point member roundtrip loads");
        Expect(world::AuthoredLevelDataEqual(working, parsed.level),
            "5. group writer/parser roundtrip preserves Point member");
        std::string invalidPoint = text;
        const std::string goodPoint = "authoring_group Group_01 static_prop 0 point_light 0\n";
        const std::string badPoint = "authoring_group Group_01 static_prop 0 point_light 1\n";
        const std::size_t pointLine = invalidPoint.find(goodPoint);
        Expect(pointLine != std::string::npos, "7. serialized point group line exists");
        if (pointLine != std::string::npos)
        {
            invalidPoint.replace(pointLine, goodPoint.size(), badPoint);
        }
        Expect(world::ParseLevelText(invalidPoint).status == world::LoadLevelFileStatus::Invalid,
            "7. invalid Point Light group index is rejected");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        working.spotLights.push_back(MakeSpot({3.0f, 4.0f, 0.0f}, {0.0f, -1.0f, 0.0f}));
        working.spotLights[0].color = {0.9f, 0.1f, 0.2f};
        working.spotLights[0].intensity = 3.0f;
        working.spotLights[0].range = 16.0f;
        working.spotLights[0].innerConeDegrees = 10.0f;
        working.spotLights[0].outerConeDegrees = 25.0f;
        working.spotLights[0].enabled = false;
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 1},
            {{EditorObjectKind::SpotLight, 0}});
        Expect(working.authoringGroups[0].members[1].kind
                == world::AuthoringGroupMemberKind::SpotLight,
            "fixture group stores spot_light member kind");
        const std::string text = world::SerializeLevelText(working);
        Expect(text.find("authoring_group Group_01 static_prop 1 spot_light 0") != std::string::npos,
            "6. writer emits spot_light typed member token");
        const world::ParseLevelFileResult parsed = world::ParseLevelText(text);
        Expect(parsed.status == world::LoadLevelFileStatus::Loaded, "6. spot member roundtrip loads");
        Expect(world::AuthoredLevelDataEqual(working, parsed.level),
            "6. group writer/parser roundtrip preserves Spot member");
        std::string invalidSpot = text;
        const std::string goodSpot = "authoring_group Group_01 static_prop 1 spot_light 0\n";
        const std::string badSpot = "authoring_group Group_01 static_prop 1 spot_light 4\n";
        const std::size_t spotLine = invalidSpot.find(goodSpot);
        Expect(spotLine != std::string::npos, "7. serialized spot group line exists");
        if (spotLine != std::string::npos)
        {
            invalidSpot.replace(spotLine, goodSpot.size(), badSpot);
        }
        Expect(world::ParseLevelText(invalidSpot).status == world::LoadLevelFileStatus::Invalid,
            "7. invalid Spot Light group index is rejected");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        working.pointLights.push_back(world::MakeDefaultPointLight({4.0f, 2.0f, 1.0f}));
        working.pointLights[0].intensity = 1.75f;
        working.pointLights[0].range = 9.0f;
        const EditorSelection primary{EditorObjectKind::StaticProp, 0};
        const std::vector<EditorSelection> additional{{EditorObjectKind::PointLight, 0}};
        editor::CreateAuthoringGroupFromSelection(working, primary, additional);
        const std::vector<editor::HierarchyRow> rows = editor::BuildHierarchyRows(working);
        Expect(editor::FindHierarchyMemberRow(rows, additional[0]) != nullptr,
            "grouped Point Light appears as a group member");
        Expect(editor::FindHierarchyUngroupedRow(rows, additional[0]) == nullptr,
            "grouped Point Light is omitted from Point Lights category");
        Expect(!editor::HierarchyHasDuplicateTopLevelObject(rows, additional[0]),
            "grouped Point Light is not duplicated top-level");
        EditorSelection selectedPrimary{};
        std::vector<EditorSelection> selectedAdditional;
        Expect(
            editor::ApplyHierarchyRowClick(
                working, *editor::FindHierarchyGroupRow(rows, 0), selectedPrimary, selectedAdditional, false),
            "group row selects complete Point fixture");
        Expect(selectedPrimary == primary && selectedAdditional == additional,
            "group row reconstructs Prop + Point");
        Expect(
            editor::EditorSelectionSetSupportsGroupTranslate(working, primary, additional),
            "8. Prop + Point supports Group Translate");
        std::vector<core::Vec3> starts;
        Expect(editor::CaptureGroupTranslateStarts(working, {primary, additional[0]}, starts),
            "8. capture Point Group Translate starts");
        const core::Vec3 pointBefore = working.pointLights[0].position;
        const float intensityBefore = working.pointLights[0].intensity;
        const float rangeBefore = working.pointLights[0].range;
        Expect(
            editor::ApplyGroupTranslateFromPrimaryResult(
                working,
                {primary, additional[0]},
                starts,
                {starts[0].x + 2.0f, starts[0].y, starts[0].z},
                editor::EditorAxis::X,
                false,
                0.25f),
            "8. Group Translate moves Point fixture");
        Expect(NearlyEqual(working.staticProps[0].position.x, starts[0].x + 2.0f),
            "8. Static Prop receives shared delta");
        Expect(NearlyEqualVec(working.pointLights[0].position, {pointBefore.x + 2.0f, pointBefore.y, pointBefore.z}),
            "8. group Translate moves Point position by exact shared delta");
        Expect(working.pointLights[0].intensity == intensityBefore
                && working.pointLights[0].range == rangeBefore,
            "8. Point intensity/range unchanged by Group Translate");
        Expect(
            editor::ApplyGroupTranslateFromPrimaryResult(
                working,
                {primary, additional[0]},
                starts,
                {starts[0].x + 0.13f, starts[0].y, starts[0].z},
                editor::EditorAxis::X,
                true,
                0.25f),
            "11. Group Translate still snaps PRIMARY then shares delta");
        Expect(NearlyEqual(working.pointLights[0].position.x - pointBefore.x,
                   working.staticProps[0].position.x - starts[0].x),
            "11. snapped Point delta matches PRIMARY shared delta");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        working.spotLights.push_back(MakeSpot({5.0f, 3.0f, -1.0f}, {0.0f, -1.0f, 0.0f}));
        const core::Vec3 spotDirectionBefore = working.spotLights[0].direction;
        const EditorSelection primary{EditorObjectKind::StaticProp, 0};
        const std::vector<EditorSelection> additional{{EditorObjectKind::SpotLight, 0}};
        editor::CreateAuthoringGroupFromSelection(working, primary, additional);
        const std::vector<editor::HierarchyRow> rows = editor::BuildHierarchyRows(working);
        Expect(editor::FindHierarchyUngroupedRow(rows, additional[0]) == nullptr,
            "grouped Spot Light is omitted from Spot Lights category");
        Expect(
            editor::EditorSelectionSetSupportsGroupTranslate(working, primary, additional),
            "9. Prop + Spot supports Group Translate");
        std::vector<core::Vec3> starts;
        Expect(editor::CaptureGroupTranslateStarts(working, {primary, additional[0]}, starts),
            "9. capture Spot Group Translate starts");
        const core::Vec3 spotBefore = working.spotLights[0].position;
        Expect(
            editor::ApplyGroupTranslateFromPrimaryResult(
                working,
                {primary, additional[0]},
                starts,
                {starts[0].x, starts[0].y + 1.5f, starts[0].z},
                editor::EditorAxis::Y,
                false,
                0.25f),
            "9. Group Translate moves Spot fixture");
        Expect(NearlyEqualVec(working.spotLights[0].position, {spotBefore.x, spotBefore.y + 1.5f, spotBefore.z}),
            "9. group Translate moves Spot position by exact shared delta");
        Expect(NearlyEqualVec(working.spotLights[0].direction, spotDirectionBefore),
            "9. Spot direction unchanged by Group Translate");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        working.staticProps[0].position = {0.0f, 1.0f, 0.0f};
        working.staticProps[0].rotationDegrees = {0.0f, 0.0f, 0.0f};
        working.pointLights.push_back(world::MakeDefaultPointLight({0.0f, 1.0f, 2.0f}));
        working.pointLights[0].color = {0.1f, 0.2f, 0.3f};
        working.pointLights[0].intensity = 2.0f;
        working.pointLights[0].range = 11.0f;
        working.pointLights[0].enabled = true;
        const EditorSelection primary{EditorObjectKind::StaticProp, 0};
        const std::vector<EditorSelection> additional{{EditorObjectKind::PointLight, 0}};
        editor::CreateAuthoringGroupFromSelection(working, primary, additional);
        Expect(
            editor::EditorSelectionSetSupportsGroupRotate(working, primary, additional),
            "10. Prop + Point supports Group Rotate");
        Expect(!editor::IsRotateSelection(additional[0]),
            "10. Point Light still has no individual orientation");
        std::vector<core::Vec3> startPositions;
        std::vector<core::Vec3> startRotations;
        core::Vec3 pivot{};
        Expect(editor::TryPrimaryRotatePivot(working, primary, pivot), "10. PRIMARY rotate pivot");
        Expect(editor::CaptureGroupRotateStarts(
                   working, {primary, additional[0]}, startPositions, startRotations),
            "10. capture Point Group Rotate starts");
        Expect(
            editor::ApplyGroupRotateFromPrimaryResult(
                working,
                {primary, additional[0]},
                startPositions,
                startRotations,
                pivot,
                {90.0f, 0.0f, 0.0f},
                editor::EditorAxis::X,
                false,
                15.0f),
            "10. Group Rotate pivots Point position");
        Expect(NearlyEqualVec(working.pointLights[0].position, {0.0f, -1.0f, 0.0f}),
            "10. group Rotate orbits Point around PRIMARY pivot");
        Expect(working.pointLights[0].color.x == 0.1f && working.pointLights[0].intensity == 2.0f
                && working.pointLights[0].range == 11.0f && working.pointLights[0].enabled,
            "10. Point Rotate does not invent orientation or mutate lighting fields");
        Expect(!editor::IsScaleSelection(additional[0]),
            "group Scale remains unsupported for Point Light");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        working.staticProps[0].position = {0.0f, 1.0f, 0.0f};
        working.staticProps[0].rotationDegrees = {0.0f, 0.0f, 0.0f};
        working.spotLights.push_back(MakeSpot({0.0f, 1.0f, 2.0f}, {0.0f, -1.0f, 0.0f}));
        working.spotLights[0].color = {0.5f, 0.6f, 0.7f};
        working.spotLights[0].intensity = 1.25f;
        working.spotLights[0].range = 7.0f;
        working.spotLights[0].innerConeDegrees = 12.0f;
        working.spotLights[0].outerConeDegrees = 22.0f;
        const EditorSelection primary{EditorObjectKind::StaticProp, 0};
        const std::vector<EditorSelection> additional{{EditorObjectKind::SpotLight, 0}};
        editor::CreateAuthoringGroupFromSelection(working, primary, additional);
        Expect(
            editor::EditorSelectionSetSupportsGroupRotate(working, primary, additional),
            "11. Prop + Spot supports Group Rotate");
        std::vector<core::Vec3> startPositions;
        std::vector<core::Vec3> startRotations;
        core::Vec3 pivot{};
        Expect(editor::TryPrimaryRotatePivot(working, primary, pivot), "11. Spot PRIMARY pivot");
        Expect(editor::CaptureGroupRotateStarts(
                   working, {primary, additional[0]}, startPositions, startRotations),
            "11. capture Spot Group Rotate starts");
        Expect(
            editor::ApplyGroupRotateFromPrimaryResult(
                working,
                {primary, additional[0]},
                startPositions,
                startRotations,
                pivot,
                {90.0f, 0.0f, 0.0f},
                editor::EditorAxis::X,
                false,
                15.0f),
            "11. Group Rotate pivots Spot position");
        Expect(NearlyEqualVec(working.spotLights[0].position, {0.0f, -1.0f, 0.0f}),
            "11. group Rotate orbits Spot around PRIMARY pivot");
        Expect(NearlyEqualVec(working.spotLights[0].direction, {0.0f, 0.0f, -1.0f}),
            "12. group Rotate rotates Spot direction by the same world-axis delta");
        Expect(world::SpotLightDirectionIsValid(working.spotLights[0].direction),
            "13. Spot direction remains finite/non-zero");
        Expect(NearlyEqualVec(
                   working.spotLights[0].direction,
                   world::CanonicalSpotLightDirection(working.spotLights[0].direction)),
            "13. Spot direction remains normalized");
        Expect(working.spotLights[0].intensity == 1.25f && working.spotLights[0].range == 7.0f
                && working.spotLights[0].innerConeDegrees == 12.0f
                && working.spotLights[0].outerConeDegrees == 22.0f,
            "Spot cone/intensity/range unchanged by Group Rotate");
        Expect(!editor::IsScaleSelection(additional[0]),
            "group Scale remains unsupported for Spot Light");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        working.pointLights.push_back(world::MakeDefaultPointLight({2.0f, 2.0f, 0.0f}));
        working.pointLights[0].color = {0.3f, 0.5f, 0.7f};
        working.pointLights[0].intensity = 2.5f;
        working.pointLights[0].range = 14.0f;
        working.pointLights[0].enabled = false;
        const EditorSelection primary{EditorObjectKind::StaticProp, 0};
        const std::vector<EditorSelection> additional{{EditorObjectKind::PointLight, 0}};
        editor::CreateAuthoringGroupFromSelection(working, primary, additional);
        const editor::LifecycleEditResult duplicated =
            editor::DuplicateSelectionSet(working, primary, additional);
        Expect(duplicated.succeeded, "14. complete-group Duplicate with Point succeeds");
        Expect(working.authoringGroups.size() == 2, "14. copied Point fixture has a new group");
        Expect(working.pointLights.size() == 2, "14. copied Point Light is independent");
        Expect(duplicated.selection.kind == EditorObjectKind::StaticProp
                && duplicated.additionalSelections.size() == 1
                && duplicated.additionalSelections[0].kind == EditorObjectKind::PointLight,
            "14. duplicated selection remaps onto copied Point");
        Expect(working.authoringGroups[1].members[1].kind
                    == world::AuthoringGroupMemberKind::PointLight
                && working.authoringGroups[1].members[1].index
                    == duplicated.additionalSelections[0].index,
            "14. copied group remaps Point Light index");
        Expect(working.pointLights[1].color.x == working.pointLights[0].color.x
                && working.pointLights[1].intensity == 2.5f
                && working.pointLights[1].range == 14.0f
                && working.pointLights[1].enabled == false,
            "14. copied Point preserves authored lighting fields");
        Expect(NearlyEqual(
                   working.pointLights[1].position.x,
                   working.pointLights[0].position.x + editor::kLifecycleDuplicateOffsetX),
            "14. copied Point uses existing duplicate placement offset");
        Expect(
            !editor::DuplicateSelectionSet(working, primary, {}).succeeded,
            "16. partial-group Duplicate remains an atomic refusal");
        Expect(working.pointLights.size() == 2 && working.authoringGroups.size() == 2,
            "16. refused Point partial Duplicate mutates nothing");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        working.spotLights.push_back(MakeSpot({6.0f, 5.0f, 1.0f}, {1.0f, 0.0f, 0.0f}));
        working.spotLights[0].color = {0.8f, 0.2f, 0.1f};
        working.spotLights[0].intensity = 3.5f;
        working.spotLights[0].range = 20.0f;
        working.spotLights[0].innerConeDegrees = 8.0f;
        working.spotLights[0].outerConeDegrees = 18.0f;
        working.spotLights[0].enabled = false;
        const EditorSelection primary{EditorObjectKind::StaticProp, 2};
        const std::vector<EditorSelection> additional{{EditorObjectKind::SpotLight, 0}};
        editor::CreateAuthoringGroupFromSelection(working, primary, additional);
        const editor::LifecycleEditResult duplicated =
            editor::DuplicateSelectionSet(working, primary, additional);
        Expect(duplicated.succeeded, "15. complete-group Duplicate with Spot succeeds");
        Expect(working.spotLights.size() == 2, "15. copied Spot Light is independent");
        Expect(working.authoringGroups[1].members[1].kind
                    == world::AuthoringGroupMemberKind::SpotLight
                && working.authoringGroups[1].members[1].index
                    == duplicated.additionalSelections[0].index,
            "15. copied group remaps Spot Light index");
        Expect(NearlyEqualVec(working.spotLights[1].direction, working.spotLights[0].direction)
                && Vec3Equal(working.spotLights[1].color, working.spotLights[0].color)
                && working.spotLights[1].intensity == 3.5f
                && working.spotLights[1].range == 20.0f
                && working.spotLights[1].innerConeDegrees == 8.0f
                && working.spotLights[1].outerConeDegrees == 18.0f
                && working.spotLights[1].enabled == false,
            "15. copied Spot preserves authored semantic data");
        Expect(NearlyEqual(
                   working.spotLights[1].position.x,
                   working.spotLights[0].position.x + editor::kLifecycleDuplicateOffsetX),
            "15. copied Spot uses existing duplicate placement offset");
        Expect(
            !editor::DuplicateSelectionSet(
                    working, additional[0], std::vector<EditorSelection>{})
                .succeeded,
            "16. partial Spot-member Duplicate remains an atomic refusal");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        working.pointLights.push_back(world::MakeDefaultPointLight({0.0f, 2.0f, 0.0f}));
        working.pointLights.push_back(world::MakeDefaultPointLight({4.0f, 2.0f, 0.0f}));
        working.spotLights.push_back(MakeSpot({0.0f, 5.0f, 0.0f}, {0.0f, -1.0f, 0.0f}));
        working.spotLights.push_back(MakeSpot({4.0f, 5.0f, 0.0f}, {1.0f, 0.0f, 0.0f}));
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 0},
            {{EditorObjectKind::PointLight, 1}});
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 1},
            {{EditorObjectKind::SpotLight, 1}});
        Expect(editor::DeleteSelected(working, {EditorObjectKind::PointLight, 0}).succeeded,
            "17. deleting an earlier Point remaps grouped Point");
        Expect(working.authoringGroups[0].members[1].kind
                    == world::AuthoringGroupMemberKind::PointLight
                && working.authoringGroups[0].members[1].index == 0,
            "17. grouped Point index remaps after earlier Point delete");
        Expect(working.authoringGroups[1].members[1].kind
                    == world::AuthoringGroupMemberKind::SpotLight
                && working.authoringGroups[1].members[1].index == 1,
            "17. Spot membership is independent of Point remap");
        Expect(editor::DeleteSelected(working, {EditorObjectKind::SpotLight, 0}).succeeded,
            "17. deleting an earlier Spot remaps grouped Spot");
        Expect(working.authoringGroups[1].members[1].index == 0,
            "17. grouped Spot index remaps after earlier Spot delete");
        Expect(working.authoringGroups[0].members[1].index == 0,
            "17. Point membership is independent of Spot remap");

        Expect(
            editor::DeleteSelectionSet(
                working,
                {EditorObjectKind::StaticProp, 0},
                {{EditorObjectKind::PointLight, 0}})
                .succeeded,
            "complete Point fixture group Delete succeeds");
        Expect(working.authoringGroups.size() == 1, "unrelated Spot fixture group remains");
        Expect(working.pointLights.empty(), "deleted grouped Point is removed");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        working.pointLights.push_back(world::MakeDefaultPointLight({2.0f, 3.0f, 0.0f}));
        working.pointLights[0].intensity = 2.75f;
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 0},
            {{EditorObjectKind::PointLight, 0}});
        Expect(editor::DeleteSelected(working, {EditorObjectKind::StaticProp, 0}).succeeded,
            "18. deleting down to one local-light member auto-dissolves");
        Expect(working.authoringGroups.empty(), "18. auto-dissolve removes group metadata");
        Expect(working.pointLights.size() == 1 && working.pointLights[0].intensity == 2.75f,
            "18. surviving Point Light remains authored");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        working.spotLights.push_back(MakeSpot({1.0f, 4.0f, 2.0f}, {0.0f, -1.0f, 0.0f}));
        working.spotLights[0].color = {0.4f, 0.5f, 0.6f};
        working.spotLights[0].intensity = 1.8f;
        working.spotLights[0].range = 13.0f;
        working.spotLights[0].innerConeDegrees = 15.0f;
        working.spotLights[0].outerConeDegrees = 30.0f;
        working.spotLights[0].enabled = false;
        editor::CreateAuthoringGroupFromSelection(
            working,
            {EditorObjectKind::StaticProp, 0},
            {{EditorObjectKind::SpotLight, 0}});
        const world::SpotLightSpec beforeUngroup = working.spotLights[0];
        const core::Vec3 propBefore = working.staticProps[0].position;
        Expect(editor::UngroupAuthoringGroup(working, 0).succeeded, "19. Ungroup succeeds");
        Expect(working.authoringGroups.empty(), "19. Ungroup removes only group metadata");
        Expect(world::SpotLightEqual(working.spotLights[0], beforeUngroup),
            "19. Ungroup preserves Spot authored semantic data");
        Expect(Vec3Equal(working.staticProps[0].position, propBefore),
            "19. Ungroup preserves fixture object data");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        const EditorSelection pickupPrimary{EditorObjectKind::StaticProp, 2};
        const std::vector<EditorSelection> pickupAdditional{{EditorObjectKind::ItemPickup, 0}};
        Expect(editor::CanCreateAuthoringGroup(working, pickupPrimary, pickupAdditional),
            "22. existing Static Prop / Item Pickup grouping remains valid");
        Expect(editor::CreateAuthoringGroupFromSelection(working, pickupPrimary, pickupAdditional)
                   .succeeded,
            "22. Static Prop + Item Pickup Group Selected unchanged");
        Expect(working.authoringGroups[0].members[0].kind
                    == world::AuthoringGroupMemberKind::StaticProp
                && working.authoringGroups[0].members[1].kind
                    == world::AuthoringGroupMemberKind::ItemPickup,
            "22. Item Pickup typed membership is unchanged");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        working.spotLights.push_back(MakeSpot({2.0f, 4.0f, 0.0f}, {0.0f, -1.0f, 0.0f}));
        const EditorSelection prop{EditorObjectKind::StaticProp, 0};
        const EditorSelection spot{EditorObjectKind::SpotLight, 0};
        editor::CreateAuthoringGroupFromSelection(working, prop, {{spot}});
        const world::StaticPropSpec propBefore = working.staticProps[0];
        Expect(editor::IsRotateSelection(spot)
                && editor::SelectionHasRotateOrientation(working, spot),
            "7. grouped Spot member remains Rotate-capable");
        const render::CameraView spotView = MakeEditorView({20.0f, 8.0f, 20.0f}, working.spotLights[0].position);
        Expect(editor::MakeRotateGizmoDrawRequest(spot, working, spotView, {}).visible,
            "7. grouped Spot member still shows the Rotate gizmo");
        const core::Vec3 origin = working.spotLights[0].position;
        const float length = editor::GizmoWorldLength(spotView, origin);
        const float unique = 0.70710678f;
        const core::Vec3 xRing{origin.x, origin.y + length * unique, origin.z + length * unique};
        const core::Vec3 xSwept{origin.x, origin.y - length * unique, origin.z + length * unique};
        editor::GizmoInteractionState state{};
        const core::Vec3 startDir = working.spotLights[0].direction;
        Expect(
            editor::UpdateRotateInteraction(
                state, spot, working, spotView, RayThroughView(spotView, xRing), false, false, true, true, false),
            "8. grouped-member Spot Rotate starts");
        Expect(
            editor::UpdateRotateInteraction(
                state, spot, working, spotView, RayThroughView(spotView, xSwept), false, false, false, true, false),
            "8. grouped-member Spot Rotate updates");
        Expect(!NearlyEqualVec(working.spotLights[0].direction, startDir, 0.01f),
            "8. individual grouped-member Rotate changes only that Spot direction");
        Expect(NearlyEqualVec(working.spotLights[0].position, origin),
            "8. grouped-member Rotate leaves Spot position");
        Expect(Vec3Equal(working.staticProps[0].position, propBefore.position)
                && Vec3Equal(working.staticProps[0].rotationDegrees, propBefore.rotationDegrees),
            "8. grouped-member Rotate leaves Static Prop unchanged");
        editor::EndGizmoDrag(state);
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        working.spotLights.push_back(MakeSpot({0.0f, 2.0f, 0.0f}, {0.0f, -1.0f, 0.0f}));
        working.spotLights.push_back(MakeSpot({0.0f, 2.0f, 2.0f}, {0.0f, -1.0f, 0.0f}));
        const EditorSelection primary{EditorObjectKind::SpotLight, 0};
        const std::vector<EditorSelection> additional{{EditorObjectKind::SpotLight, 1}};
        editor::CreateAuthoringGroupFromSelection(working, primary, additional);
        Expect(working.authoringGroups[0].members[0].kind == world::AuthoringGroupMemberKind::SpotLight,
            "9. persisted PRIMARY is Spot Light");
        Expect(editor::EditorSelectionSetSupportsGroupRotate(working, primary, additional),
            "9. complete group with Spot PRIMARY is Group-Rotate-capable");
        Expect(editor::GroupRotateDisableReason(working, primary, additional) == nullptr,
            "9. Spot-primary group has no Rotate disable reason");
        const render::CameraView view = MakeEditorView({20.0f, 8.0f, 20.0f}, working.spotLights[0].position);
        const editor::GizmoDrawRequest draw =
            editor::MakeRotateGizmoDrawRequest(primary, working, view, {});
        Expect(draw.visible, "9. Spot-primary group exposes the Rotate gizmo");
        core::Vec3 pivot{};
        Expect(editor::TryPrimaryRotatePivot(working, primary, pivot), "10. Spot-primary pivot exists");
        Expect(NearlyEqualVec(pivot, working.spotLights[0].position),
            "10. Spot-primary group pivot equals Spot PRIMARY position");
        Expect(NearlyEqualVec(draw.origin, working.spotLights[0].position),
            "10. drawn Rotate gizmo origin matches Spot PRIMARY");
        std::vector<core::Vec3> startPositions;
        std::vector<core::Vec3> startRotations;
        Expect(editor::CaptureGroupRotateStarts(working, {primary, additional[0]}, startPositions, startRotations),
            "11. capture Spot-primary Group Rotate starts");
        const core::Vec3 primaryDirBefore = working.spotLights[0].direction;
        const core::Vec3 primaryPosBefore = working.spotLights[0].position;
        Expect(
            editor::ApplyGroupRotateFromPrimaryResult(
                working,
                {primary, additional[0]},
                startPositions,
                startRotations,
                pivot,
                {90.0f, 0.0f, 0.0f},
                editor::EditorAxis::X,
                false,
                15.0f),
            "11. Spot-primary Group Rotate applies");
        Expect(NearlyEqualVec(working.spotLights[0].position, primaryPosBefore),
            "11. Spot PRIMARY position stays at the pivot");
        Expect(!NearlyEqualVec(working.spotLights[0].direction, primaryDirBefore, 0.01f),
            "11. Spot PRIMARY direction rotates");
        Expect(NearlyEqualVec(working.spotLights[0].direction, {0.0f, 0.0f, -1.0f}),
            "11. Spot PRIMARY direction receives the shared world-axis delta");
        Expect(NearlyEqualVec(working.spotLights[1].position, {0.0f, 0.0f, 0.0f}),
            "12. secondary Spot position orbits pivot");
        Expect(NearlyEqualVec(working.spotLights[1].direction, {0.0f, 0.0f, -1.0f}),
            "13. secondary Spot direction rotates by the same delta");
        Expect(world::SpotLightDirectionIsValid(working.spotLights[0].direction)
                && world::SpotLightDirectionIsValid(working.spotLights[1].direction),
            "Spot-primary directions remain finite/non-zero");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        working.staticProps[0].position = {0.0f, 1.0f, 2.0f};
        working.staticProps[0].rotationDegrees = {};
        working.spotLights.push_back(MakeSpot({0.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}));
        const EditorSelection primary{EditorObjectKind::SpotLight, 0};
        const std::vector<EditorSelection> additional{{EditorObjectKind::StaticProp, 0}};
        editor::CreateAuthoringGroupFromSelection(working, primary, additional);
        Expect(editor::EditorSelectionSetSupportsGroupRotate(working, primary, additional),
            "14. Spot-primary + Static Prop group can start Rotate");
        Expect(editor::MakeRotateGizmoDrawRequest(
                   primary, working, MakeEditorView({20.0f, 8.0f, 20.0f}, working.spotLights[0].position), {})
                   .visible,
            "14. Spot-primary mixed group exposes Rotate gizmo");
        std::vector<core::Vec3> startPositions;
        std::vector<core::Vec3> startRotations;
        core::Vec3 pivot{};
        Expect(editor::TryPrimaryRotatePivot(working, primary, pivot)
                && NearlyEqualVec(pivot, working.spotLights[0].position),
            "14. mixed group pivot remains Spot PRIMARY position");
        Expect(editor::CaptureGroupRotateStarts(working, {primary, additional[0]}, startPositions, startRotations),
            "14. capture mixed Spot-primary starts");
        Expect(
            editor::ApplyGroupRotateFromPrimaryResult(
                working,
                {primary, additional[0]},
                startPositions,
                startRotations,
                pivot,
                {90.0f, 0.0f, 0.0f},
                editor::EditorAxis::X,
                false,
                15.0f),
            "14. mixed Spot-primary Group Rotate applies");
        Expect(NearlyEqual(working.staticProps[0].rotationDegrees.x, 90.0f),
            "14. Static Prop Euler receives the shared delta");
        Expect(NearlyEqualVec(working.staticProps[0].position, {0.0f, -1.0f, 0.0f}),
            "14. Static Prop orbits around Spot PRIMARY");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        working.pointLights.push_back(world::MakeDefaultPointLight({0.0f, 2.0f, 0.0f}));
        working.pointLights.push_back(world::MakeDefaultPointLight({0.0f, 2.0f, 2.0f}));
        working.spotLights.push_back(MakeSpot({0.0f, 2.0f, 0.0f}, {0.0f, -1.0f, 0.0f}));
        const EditorSelection point0{EditorObjectKind::PointLight, 0};
        const EditorSelection point1{EditorObjectKind::PointLight, 1};
        editor::CreateAuthoringGroupFromSelection(working, point0, {{point1}});
        Expect(!editor::IsRotateSelection(point0), "16. Point member remains Rotate-ineligible");
        Expect(!editor::SelectionHasRotateOrientation(working, point0),
            "16. individually selected Point member has no Rotate orientation");
        Expect(!editor::MakeRotateGizmoDrawRequest(
                    point0, working, MakeEditorView({20.0f, 8.0f, 20.0f}, working.pointLights[0].position), {})
                    .visible,
            "16. Point member has no Rotate gizmo");
        Expect(!editor::EditorSelectionSetSupportsGroupRotate(working, point0, {{point1}}),
            "17. Point PRIMARY limitation is retained");
        Expect(editor::GroupRotateDisableReason(working, point0, {{point1}}) != nullptr,
            "17. Point-primary group cannot start Rotate");

        world::LevelDefinition mixed = MakeLevelWithProps();
        mixed.staticProps[0].position = {0.0f, 1.0f, 0.0f};
        mixed.staticProps[0].rotationDegrees = {};
        mixed.pointLights.push_back(world::MakeDefaultPointLight({0.0f, 1.0f, 2.0f}));
        const EditorSelection propPrimary{EditorObjectKind::StaticProp, 0};
        const std::vector<EditorSelection> pointAdditional{{EditorObjectKind::PointLight, 0}};
        editor::CreateAuthoringGroupFromSelection(mixed, propPrimary, pointAdditional);
        Expect(editor::EditorSelectionSetSupportsGroupRotate(mixed, propPrimary, pointAdditional),
            "18. Point member orbits when another valid PRIMARY drives Rotate");
        std::vector<core::Vec3> startPositions;
        std::vector<core::Vec3> startRotations;
        core::Vec3 pivot{};
        Expect(editor::TryPrimaryRotatePivot(mixed, propPrimary, pivot), "18. Prop PRIMARY pivot");
        Expect(editor::CaptureGroupRotateStarts(mixed, {propPrimary, pointAdditional[0]}, startPositions, startRotations),
            "18. capture Point member orbit");
        Expect(
            editor::ApplyGroupRotateFromPrimaryResult(
                mixed,
                {propPrimary, pointAdditional[0]},
                startPositions,
                startRotations,
                pivot,
                {90.0f, 0.0f, 0.0f},
                editor::EditorAxis::X,
                false,
                15.0f),
            "18. Point member Group Rotate from Prop PRIMARY");
        Expect(NearlyEqualVec(mixed.pointLights[0].position, {0.0f, -1.0f, 0.0f}),
            "18. Point member still orbits the valid PRIMARY pivot");
    }

    {
        world::LevelDefinition working = MakeLevelWithProps();
        working.staticProps[0].position = {0.0f, 1.0f, 0.0f};
        working.staticProps[0].rotationDegrees = {};
        working.staticProps[1].position = {2.0f, 1.0f, 0.0f};
        working.staticProps[1].rotationDegrees = {15.0f, 0.0f, 0.0f};
        const EditorSelection prop0{EditorObjectKind::StaticProp, 0};
        const std::vector<EditorSelection> prop1{{EditorObjectKind::StaticProp, 1}};
        Expect(editor::EditorSelectionSetSupportsGroupRotate(working, prop0, prop1),
            "19. existing Static Prop group Rotate unchanged");
        working.itemPickups[0].position = {4.0f, 1.0f, 0.0f};
        working.itemPickups[0].visualRotationDegrees = {};
        Expect(
            editor::EditorSelectionSetSupportsGroupRotate(
                working, prop0, std::vector<EditorSelection>{{EditorObjectKind::ItemPickup, 0}}),
            "20. existing Item Pickup group Rotate unchanged");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d AuthoringGroupsTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("AuthoringGroupsTest passed\n");
    return 0;
}
