#include "editor/AuthoringGroups.h"
#include "editor/AuthoredObjectLifecycle.h"
#include "editor/EditorGroupRotate.h"
#include "editor/EditorGroupTranslate.h"
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

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d AuthoringGroupsTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("AuthoringGroupsTest passed\n");
    return 0;
}
