#include "editor/AuthoringGroups.h"
#include "editor/AuthoredObjectLifecycle.h"
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

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d AuthoringGroupsTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("AuthoringGroupsTest passed\n");
    return 0;
}
