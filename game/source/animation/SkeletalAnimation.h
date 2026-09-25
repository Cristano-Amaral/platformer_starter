#pragma once

#include "core/Vec3.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace animation
{
inline constexpr std::size_t kMaxSkinJoints = 64;

struct Quaternion
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;
};

struct Matrix4
{
    // Column-major, matching GLSL.
    float values[16]{};
};

struct JointTransform
{
    core::Vec3 translation{};
    Quaternion rotation{};
    core::Vec3 scale{1.0f, 1.0f, 1.0f};
};

struct SkeletonJoint
{
    std::string name;
    int parent = -1;
    JointTransform bindLocal{};
    Matrix4 inverseBind{};
};

struct Skeleton
{
    std::vector<SkeletonJoint> joints;
};

enum class Interpolation { Linear, Step };

struct Vec3Key { float time = 0.0f; core::Vec3 value{}; };
struct QuaternionKey { float time = 0.0f; Quaternion value{}; };

struct JointChannel
{
    std::vector<Vec3Key> translations;
    std::vector<QuaternionKey> rotations;
    std::vector<Vec3Key> scales;
    Interpolation interpolation = Interpolation::Linear;
};

struct AnimationClip
{
    std::string name;
    float durationSeconds = 0.0f;
    std::vector<JointChannel> joints;
};

enum class PlaybackMode { Loop, Clamp };

Matrix4 IdentityMatrix();
Matrix4 Multiply(const Matrix4& a, const Matrix4& b);
Matrix4 TransformMatrix(const JointTransform& transform);
Quaternion Normalize(Quaternion value);
Quaternion Slerp(Quaternion a, Quaternion b, float amount);
float ResolvePlaybackTime(float timeSeconds, float durationSeconds, PlaybackMode mode);
const AnimationClip* FindClip(const std::vector<AnimationClip>& clips, std::string_view name);
bool ValidateSkeleton(const Skeleton& skeleton, std::string* error = nullptr);
bool ValidateClip(const AnimationClip& clip, std::size_t jointCount, std::string* error = nullptr);
bool SampleClip(
    const Skeleton& skeleton,
    const AnimationClip& clip,
    float timeSeconds,
    PlaybackMode mode,
    std::vector<JointTransform>& localPose);
bool EvaluateSkinMatrices(
    const Skeleton& skeleton,
    const std::vector<JointTransform>& localPose,
    std::vector<Matrix4>& skinMatrices);
bool EvaluateJointGlobalMatrices(
    const Skeleton& skeleton,
    const std::vector<JointTransform>& localPose,
    std::vector<Matrix4>& globalMatrices);
}
