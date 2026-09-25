#include "animation/SkeletalAnimation.h"
#include "gameplay/PlayerAnimation.h"

#include <cmath>
#include <cstdio>

namespace
{
int failures = 0;
void Expect(bool condition, const char* name)
{
    if (!condition) { std::fprintf(stderr, "FAIL %s\n", name); ++failures; }
}
bool Near(float a, float b) { return std::fabs(a-b) < 0.001f; }
}

int main()
{
    animation::Skeleton skeleton;
    animation::SkeletonJoint root; root.name="root"; root.inverseBind=animation::IdentityMatrix();
    animation::SkeletonJoint child; child.name="child"; child.parent=0;
    child.bindLocal.translation={0,1,0};
    child.inverseBind=animation::IdentityMatrix();
    skeleton.joints={root,child};
    Expect(animation::ValidateSkeleton(skeleton), "valid parent-before-child skeleton");
    animation::Skeleton invalid=skeleton; invalid.joints[0].parent=1;
    Expect(!animation::ValidateSkeleton(invalid), "invalid hierarchy rejected");

    animation::AnimationClip clip; clip.name="Move"; clip.durationSeconds=1.0f;
    clip.joints.resize(2);
    clip.joints[0].translations={{{0.0f},{0,0,0}},{{1.0f},{2,0,0}}};
    clip.joints[0].rotations={{{0.0f},{0,0,0,1}},{{1.0f},{0,0,1,0}}};
    clip.joints[0].scales={{{0.0f},{1,1,1}},{{1.0f},{2,2,2}}};
    Expect(animation::ValidateClip(clip,2), "valid typed TRS clip");
    std::vector<animation::JointTransform> pose;
    Expect(animation::SampleClip(skeleton,clip,0.5f,animation::PlaybackMode::Clamp,pose),
        "sample deterministic clip");
    Expect(Near(pose[0].translation.x,1.0f) && Near(pose[0].scale.x,1.5f),
        "translation and scale linear interpolation");
    Expect(Near(std::fabs(pose[0].rotation.z),0.7071f), "rotation slerp");
    Expect(Near(animation::ResolvePlaybackTime(1.25f,1.0f,animation::PlaybackMode::Loop),0.25f),
        "loop wraps");
    Expect(Near(animation::ResolvePlaybackTime(1.25f,1.0f,animation::PlaybackMode::Clamp),1.0f),
        "clamp stops");
    Expect(animation::FindClip({clip},"Move") != nullptr
        && animation::FindClip({clip},"Missing") == nullptr, "clip lookup");
    std::vector<animation::Matrix4> skin;
    Expect(animation::EvaluateSkinMatrices(skeleton,pose,skin), "skin matrices evaluated");
    Expect(Near(skin[1].values[12],-0.5f) && Near(skin[1].values[13],0.0f),
        "child inherits evaluated parent hierarchy");

    Expect(gameplay::SelectPlayerAnimationState(true,0.0f,0.01f)==gameplay::PlayerAnimationState::Idle,
        "grounded stationary is Idle");
    Expect(gameplay::SelectPlayerAnimationState(true,0.02f,0.01f)==gameplay::PlayerAnimationState::Move,
        "grounded moving is Move");
    Expect(gameplay::SelectPlayerAnimationState(false,4.0f,0.01f)==gameplay::PlayerAnimationState::Jump,
        "airborne is Jump");
    gameplay::PlayerAnimationPlayback playback;
    gameplay::UpdatePlayerAnimation(playback,true,1.0f,0.01f,0.02f);
    Expect(playback.state==gameplay::PlayerAnimationState::Move
        && Near(playback.playbackTimeSeconds,0.02f), "switch resets then advances playback");
    gameplay::UpdatePlayerAnimation(playback,false,1.0f,0.01f,0.03f);
    Expect(playback.previousState==gameplay::PlayerAnimationState::Move
        && playback.state==gameplay::PlayerAnimationState::Jump
        && gameplay::PlayerAnimationBlendAmount(playback)>0.0f, "bounded cross-fade state");
    gameplay::ResetPlayerAnimation(playback);
    Expect(playback.state==gameplay::PlayerAnimationState::Idle
        && Near(playback.playbackTimeSeconds,0.0f), "lifecycle reset");

    if (failures) return 1;
    std::printf("Skeletal animation tests passed.\n");
    return 0;
}
