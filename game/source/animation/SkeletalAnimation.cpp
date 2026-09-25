#include "animation/SkeletalAnimation.h"

#include <algorithm>
#include <cmath>

namespace animation
{
namespace
{
core::Vec3 Lerp(core::Vec3 a, core::Vec3 b, float t)
{
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
}

template <typename Key, typename Value, typename Blend>
Value SampleKeys(
    const std::vector<Key>& keys, float time, const Value& fallback,
    Interpolation interpolation, Blend blend)
{
    if (keys.empty()) return fallback;
    if (time <= keys.front().time) return keys.front().value;
    if (time >= keys.back().time) return keys.back().value;
    const auto upper = std::upper_bound(keys.begin(), keys.end(), time,
        [](float value, const Key& key) { return value < key.time; });
    const Key& b = *upper;
    const Key& a = *(upper - 1);
    if (interpolation == Interpolation::Step || !(b.time > a.time)) return a.value;
    return blend(a.value, b.value, (time - a.time) / (b.time - a.time));
}
}

Matrix4 IdentityMatrix()
{
    Matrix4 result{};
    result.values[0] = result.values[5] = result.values[10] = result.values[15] = 1.0f;
    return result;
}

Matrix4 Multiply(const Matrix4& a, const Matrix4& b)
{
    Matrix4 result{};
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row)
            for (int index = 0; index < 4; ++index)
                result.values[column * 4 + row] +=
                    a.values[index * 4 + row] * b.values[column * 4 + index];
    return result;
}

Quaternion Normalize(Quaternion q)
{
    const float length = std::sqrt(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
    if (!(length > 0.0f) || !std::isfinite(length)) return {};
    return {q.x/length, q.y/length, q.z/length, q.w/length};
}

Quaternion Slerp(Quaternion a, Quaternion b, float t)
{
    a = Normalize(a); b = Normalize(b);
    float dot = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
    if (dot < 0.0f) { dot = -dot; b = {-b.x, -b.y, -b.z, -b.w}; }
    if (dot > 0.9995f)
        return Normalize({a.x+(b.x-a.x)*t, a.y+(b.y-a.y)*t,
            a.z+(b.z-a.z)*t, a.w+(b.w-a.w)*t});
    dot = std::clamp(dot, -1.0f, 1.0f);
    const float angle = std::acos(dot);
    const float divisor = std::sin(angle);
    const float left = std::sin((1.0f-t)*angle)/divisor;
    const float right = std::sin(t*angle)/divisor;
    return {a.x*left+b.x*right, a.y*left+b.y*right,
        a.z*left+b.z*right, a.w*left+b.w*right};
}

Matrix4 TransformMatrix(const JointTransform& transform)
{
    const Quaternion q = Normalize(transform.rotation);
    const float xx=q.x*q.x, yy=q.y*q.y, zz=q.z*q.z;
    const float xy=q.x*q.y, xz=q.x*q.z, yz=q.y*q.z;
    const float wx=q.w*q.x, wy=q.w*q.y, wz=q.w*q.z;
    Matrix4 result = IdentityMatrix();
    result.values[0]=(1-2*(yy+zz))*transform.scale.x;
    result.values[1]=(2*(xy+wz))*transform.scale.x;
    result.values[2]=(2*(xz-wy))*transform.scale.x;
    result.values[4]=(2*(xy-wz))*transform.scale.y;
    result.values[5]=(1-2*(xx+zz))*transform.scale.y;
    result.values[6]=(2*(yz+wx))*transform.scale.y;
    result.values[8]=(2*(xz+wy))*transform.scale.z;
    result.values[9]=(2*(yz-wx))*transform.scale.z;
    result.values[10]=(1-2*(xx+yy))*transform.scale.z;
    result.values[12]=transform.translation.x;
    result.values[13]=transform.translation.y;
    result.values[14]=transform.translation.z;
    return result;
}

float ResolvePlaybackTime(float time, float duration, PlaybackMode mode)
{
    if (!(duration > 0.0f) || !std::isfinite(time)) return 0.0f;
    if (mode == PlaybackMode::Clamp) return std::clamp(time, 0.0f, duration);
    float result = std::fmod(time, duration);
    return result < 0.0f ? result + duration : result;
}

const AnimationClip* FindClip(const std::vector<AnimationClip>& clips, std::string_view name)
{
    const auto found = std::find_if(clips.begin(), clips.end(),
        [name](const AnimationClip& clip) { return clip.name == name; });
    return found == clips.end() ? nullptr : &*found;
}

bool ValidateSkeleton(const Skeleton& skeleton, std::string* error)
{
    if (skeleton.joints.empty() || skeleton.joints.size() > kMaxSkinJoints)
    { if (error) *error = "joint count is outside the supported range"; return false; }
    for (std::size_t i=0; i<skeleton.joints.size(); ++i)
    {
        const int parent=skeleton.joints[i].parent;
        if (parent >= static_cast<int>(i) || parent < -1)
        { if (error) *error = "joint hierarchy is not parent-before-child"; return false; }
    }
    return true;
}

bool ValidateClip(const AnimationClip& clip, std::size_t jointCount, std::string* error)
{
    if (clip.name.empty() || !(clip.durationSeconds > 0.0f) || clip.joints.size()!=jointCount)
    { if (error) *error = "clip name, duration, or joint count is invalid"; return false; }
    auto ordered=[](const auto& keys) { for (std::size_t i=1;i<keys.size();++i)
        if (!(keys[i].time > keys[i-1].time)) return false; return true; };
    for (const auto& channel:clip.joints)
        if (!ordered(channel.translations)||!ordered(channel.rotations)||!ordered(channel.scales))
        { if (error) *error="clip key times are not strictly increasing"; return false; }
    return true;
}

bool SampleClip(const Skeleton& skeleton, const AnimationClip& clip, float time,
    PlaybackMode mode, std::vector<JointTransform>& pose)
{
    if (!ValidateSkeleton(skeleton) || !ValidateClip(clip, skeleton.joints.size())) return false;
    const float sampleTime=ResolvePlaybackTime(time, clip.durationSeconds, mode);
    pose.resize(skeleton.joints.size());
    for (std::size_t i=0;i<pose.size();++i)
    {
        const auto& bind=skeleton.joints[i].bindLocal;
        const auto& channel=clip.joints[i];
        pose[i].translation=SampleKeys(channel.translations,sampleTime,bind.translation,
            channel.interpolation,[](auto a,auto b,float t){return Lerp(a,b,t);});
        pose[i].rotation=SampleKeys(channel.rotations,sampleTime,bind.rotation,
            channel.interpolation,[](auto a,auto b,float t){return Slerp(a,b,t);});
        pose[i].scale=SampleKeys(channel.scales,sampleTime,bind.scale,
            channel.interpolation,[](auto a,auto b,float t){return Lerp(a,b,t);});
    }
    return true;
}

bool EvaluateSkinMatrices(const Skeleton& skeleton,
    const std::vector<JointTransform>& pose, std::vector<Matrix4>& skin)
{
    if (!ValidateSkeleton(skeleton) || pose.size()!=skeleton.joints.size()) return false;
    std::vector<Matrix4> global(pose.size()); skin.resize(pose.size());
    for (std::size_t i=0;i<pose.size();++i)
    {
        const Matrix4 local=TransformMatrix(pose[i]);
        const int parent=skeleton.joints[i].parent;
        global[i]=parent < 0 ? local : Multiply(global[static_cast<std::size_t>(parent)],local);
        skin[i]=Multiply(global[i],skeleton.joints[i].inverseBind);
    }
    return true;
}
}
