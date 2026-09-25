#include "animation/SkeletalAnimation.h"
#include "gameplay/EquipmentAttachment.h"

#include <cmath>
#include <cstdio>

namespace { int failures=0; void Expect(bool value,const char* message){if(!value){std::fprintf(stderr,"FAIL: %s\n",message);++failures;}}
gameplay::GameplayDefinition Item(const char* id, const char* model, const char* joint)
{
    gameplay::GameplayDefinition d{}; d.identity=id; d.category=gameplay::GameplayDefinitionCategory::Item;
    d.item=gameplay::MakeDefaultItemDefinition(id); d.item.type=gameplay::ItemType::Equipment;
    d.item.equipmentSlot=gameplay::EquipmentSlot::MainHand; d.item.worldModelIdentity=model;
    if(joint) { gameplay::EquipmentAttachmentDefinition a{}; a.jointName=joint; d.item.equipmentAttachment=a; }
    return d;
}}

int main()
{
    animation::Skeleton skeleton; animation::SkeletonJoint root{}; root.name="Root"; root.inverseBind=animation::IdentityMatrix(); skeleton.joints.push_back(root);
    Expect(gameplay::ResolveSkeletonJoint(skeleton,"Root")==0,"joint resolves");
    Expect(gameplay::ResolveSkeletonJoint(skeleton,"Missing")==-1,"missing joint safe");

    gameplay::GameplayDefinitionRegistry registry;
    Expect(registry.Register(Item("items/sword","models/test_static.glb","Root")).status==gameplay::RegisterGameplayDefinitionStatus::Registered,"register sword");
    Expect(registry.Register(Item("items/axe","models/test_authored.glb","Root")).status==gameplay::RegisterGameplayDefinitionStatus::Registered,"register axe");
    Expect(registry.Register(Item("items/invisible","", "Root")).status==gameplay::RegisterGameplayDefinitionStatus::Registered,"register missing model");
    Expect(registry.Register(Item("items/bad_joint","models/test_static.glb","Nope")).status==gameplay::RegisterGameplayDefinitionStatus::Registered,"register missing joint item");
    gameplay::Inventory inventory; gameplay::Equipment equipment;
    Expect(inventory.TryAdd("items/sword",1,registry)==gameplay::InventoryMutationStatus::Ok,"add sword");
    Expect(equipment.Equip(inventory,"items/sword",registry)==gameplay::EquipmentTransactionStatus::Ok,"equip sword");
    Expect(gameplay::ResolveVisibleEquipmentAttachment(gameplay::EquipmentSlot::MainHand,equipment,registry,skeleton).status==gameplay::EquipmentAttachmentStatus::Ready,"equip presentation ready");
    Expect(inventory.TryAdd("items/axe",1,registry)==gameplay::InventoryMutationStatus::Ok,"add axe");
    Expect(equipment.Equip(inventory,"items/axe",registry)==gameplay::EquipmentTransactionStatus::Ok,"swap axe");
    Expect(gameplay::ResolveVisibleEquipmentAttachment(gameplay::EquipmentSlot::MainHand,equipment,registry,skeleton).itemIdentity=="items/axe","swap presentation immediate");
    Expect(equipment.Unequip(inventory,gameplay::EquipmentSlot::MainHand,registry)==gameplay::EquipmentTransactionStatus::Ok,"unequip");
    Expect(gameplay::ResolveVisibleEquipmentAttachment(gameplay::EquipmentSlot::MainHand,equipment,registry,skeleton).status==gameplay::EquipmentAttachmentStatus::Empty,"unequip presentation immediate");

    gameplay::EquipmentAttachmentDefinition attachment{}; attachment.jointName="Root"; attachment.translation={1,2,3}; attachment.scale={2,2,2};
    animation::JointTransform world{}; world.translation={10,0,0};
    animation::JointTransform joint{}; joint.translation={0,5,0};
    const auto composed=gameplay::ComposeEquipmentAttachmentTransform(animation::TransformMatrix(world),animation::TransformMatrix(joint),attachment);
    Expect(std::fabs(composed.values[12]-11)<0.0001f && std::fabs(composed.values[13]-7)<0.0001f && std::fabs(composed.values[14]-3)<0.0001f,"world * joint * local composition");
    Expect(std::fabs(composed.values[0]-2)<0.0001f,"local scale composed");
    return failures==0 ? (std::puts("EquipmentAttachmentTest passed"),0) : 1;
}
