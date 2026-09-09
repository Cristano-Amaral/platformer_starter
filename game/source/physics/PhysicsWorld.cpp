#include "physics/PhysicsWorld.h"

#include "physics/PhysicsCapacity.h"
#include "physics/PhysicsWorldTestAccess.h"
#include "world/LevelDefinition.h"

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/ShapeFilter.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <vector>

JPH_SUPPRESS_WARNINGS

namespace physics
{
namespace
{
constexpr float kMaxPhysicsDeltaSeconds = 1.0f / 60.0f;
constexpr unsigned int kNumBodyMutexes = 0;
constexpr unsigned int kMaxBodyPairs = 64;
constexpr unsigned int kMaxContactConstraints = 64;
static_assert(
    kMaxPhysicsElevatedPlatformCount == world::kMaxElevatedPlatformCount,
    "world platform budget must match the Jolt body allocator leftover");
constexpr unsigned int kTempAllocatorBytes = 1 * 1024 * 1024;
constexpr unsigned int kMaxPhysicsJobs = 256;

// Capsule matching the 0.8 x 1.6 x 0.8 visual cube:
// radius 0.4, cylinder height 0.8 => total height 1.6, diameter 0.8.
constexpr float kCapsuleRadius = 0.4f;
constexpr float kCapsuleCylinderHeight = 0.8f;
constexpr float kCapsuleHalfCylinder = 0.4f;
constexpr float kCapsuleTotalHeight = kCapsuleCylinderHeight + 2.0f * kCapsuleRadius;
static_assert(kCapsuleTotalHeight == 1.6f, "Capsule total height must match visual Player height");

// Must match gameplay::Player::kGravity. Applied as CharacterVirtual::Update gravity
// (downward force onto supporting bodies), not as Jolt world default 9.81.
constexpr float kCharacterGravityY = -20.0f;

constexpr float kMaxSlopeAngleRadians = JPH::DegreesToRadians(50.0f);
constexpr float kCharacterPadding = 0.02f;
constexpr float kCollisionTolerance = 1.0e-3f;
constexpr float kPredictiveContactDistance = 0.1f;
constexpr float kPenetrationRecoverySpeed = 1.0f;
constexpr float kCharacterMass = 70.0f;
constexpr float kMaxStrength = 100.0f;
// Official Jolt CharacterVirtual sample uses 0.9 so the kinematic inner body sits
// inside the padded CharacterVirtual volume and does not rest on the same surfaces.
constexpr float kInnerShapeFraction = 0.9f;

// M53: constant-speed +Y open/close. Not serialized. Ordinary activation
// never teleports between endpoints; Initialize/Restart snap to derived pose.
constexpr float kDoorOpenSpeedMetersPerSecond = 2.5f;

int gJoltRegistrationUsers = 0;

class IgnoreBodyFilter final : public JPH::BodyFilter
{
public:
    explicit IgnoreBodyFilter(JPH::BodyID id)
        : ignore(id)
    {
    }

    bool ShouldCollide(const JPH::BodyID& inBodyID) const override
    {
        return ignore.IsInvalid() || inBodyID != ignore;
    }

    bool ShouldCollideLocked(const JPH::Body& inBody) const override
    {
        return ignore.IsInvalid() || inBody.GetID() != ignore;
    }

private:
    JPH::BodyID ignore;
};

class WorldSolidBodyFilter final : public JPH::BodyFilter
{
public:
    WorldSolidBodyFilter(
        const std::vector<JPH::BodyID>* dynamicIds,
        JPH::BodyID inner,
        JPH::BodyID extraIgnore)
        : dynamicIds(dynamicIds)
        , inner(inner)
        , extraIgnore(extraIgnore)
    {
    }

    bool ShouldCollide(const JPH::BodyID& inBodyID) const override
    {
        if (inBodyID.IsInvalid())
        {
            return false;
        }
        if (!inner.IsInvalid() && inBodyID == inner)
        {
            return false;
        }
        if (!extraIgnore.IsInvalid() && inBodyID == extraIgnore)
        {
            return false;
        }
        if (dynamicIds != nullptr)
        {
            for (const JPH::BodyID id : *dynamicIds)
            {
                if (inBodyID == id)
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool ShouldCollideLocked(const JPH::Body& inBody) const override
    {
        return ShouldCollide(inBody.GetID());
    }

private:
    const std::vector<JPH::BodyID>* dynamicIds;
    JPH::BodyID inner;
    JPH::BodyID extraIgnore;
};

// Closing-obstruction query: only the player inner body and Dynamic Boxes
// can pause a closing step. Opening is never paused by this filter.
class DoorCloseBlockerFilter final : public JPH::BodyFilter
{
public:
    DoorCloseBlockerFilter(
        JPH::BodyID door,
        JPH::BodyID inner,
        const std::vector<JPH::BodyID>* dynamicIds)
        : door(door)
        , inner(inner)
        , dynamicIds(dynamicIds)
    {
    }

    bool ShouldCollide(const JPH::BodyID& inBodyID) const override
    {
        if (inBodyID.IsInvalid() || inBodyID == door)
        {
            return false;
        }
        if (!inner.IsInvalid() && inBodyID == inner)
        {
            return true;
        }
        if (dynamicIds != nullptr)
        {
            for (const JPH::BodyID id : *dynamicIds)
            {
                if (!id.IsInvalid() && inBodyID == id)
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool ShouldCollideLocked(const JPH::Body& inBody) const override
    {
        return ShouldCollide(inBody.GetID());
    }

private:
    JPH::BodyID door;
    JPH::BodyID inner;
    const std::vector<JPH::BodyID>* dynamicIds;
};

float Vec3Length(core::Vec3 value)
{
    return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}

int FacingSign(float facingX)
{
    return facingX < 0.0f ? -1 : 1;
}

namespace ObjectLayers
{
constexpr JPH::ObjectLayer NonMoving = 0;
constexpr JPH::ObjectLayer Moving = 1;
constexpr JPH::ObjectLayer Count = 2;
}

namespace BroadPhaseLayers
{
constexpr JPH::BroadPhaseLayer NonMoving{0};
constexpr JPH::BroadPhaseLayer Moving{1};
constexpr unsigned int Count = 2;
}

void TraceImpl(const char* format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    std::fprintf(stderr, "%s\n", buffer);
}

#ifdef JPH_ENABLE_ASSERTS
bool AssertFailedImpl(const char* expression, const char* message, const char* file, unsigned int line)
{
    std::fprintf(
        stderr,
        "%s:%u: Jolt assert (%s) %s\n",
        file,
        line,
        expression,
        message != nullptr ? message : "");
    return true;
}
#endif

class BroadPhaseLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    BroadPhaseLayerInterfaceImpl()
    {
        objectToBroadPhase[ObjectLayers::NonMoving] = BroadPhaseLayers::NonMoving;
        objectToBroadPhase[ObjectLayers::Moving] = BroadPhaseLayers::Moving;
    }

    JPH::uint GetNumBroadPhaseLayers() const override
    {
        return BroadPhaseLayers::Count;
    }

    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
    {
        return objectToBroadPhase[layer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override
    {
        switch (static_cast<JPH::BroadPhaseLayer::Type>(layer))
        {
        case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::NonMoving):
            return "NON_MOVING";
        case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::Moving):
            return "MOVING";
        default:
            return "INVALID";
        }
    }
#endif

private:
    JPH::BroadPhaseLayer objectToBroadPhase[ObjectLayers::Count];
};

class ObjectVsBroadPhaseLayerFilterImpl final : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer objectLayer, JPH::BroadPhaseLayer broadPhaseLayer) const override
    {
        switch (objectLayer)
        {
        case ObjectLayers::NonMoving:
            return broadPhaseLayer == BroadPhaseLayers::Moving;
        case ObjectLayers::Moving:
            return true;
        default:
            return false;
        }
    }
};

class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer object1, JPH::ObjectLayer object2) const override
    {
        switch (object1)
        {
        case ObjectLayers::NonMoving:
            return object2 == ObjectLayers::Moving;
        case ObjectLayers::Moving:
            return true;
        default:
            return false;
        }
    }
};

JPH::RVec3 ToRVec3(core::Vec3 value)
{
    return JPH::RVec3(value.x, value.y, value.z);
}

JPH::Vec3 ToHalfExtent(core::Vec3 size)
{
    return JPH::Vec3(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f);
}

core::Vec3 ToVec3(JPH::RVec3Arg value)
{
    return {
        static_cast<float>(value.GetX()),
        static_cast<float>(value.GetY()),
        static_cast<float>(value.GetZ())};
}

float ClampUnit(float value)
{
    if (value < -1.0f)
    {
        return -1.0f;
    }
    if (value > 1.0f)
    {
        return 1.0f;
    }
    return value;
}

// Diagnostic only. CharacterVirtual classification remains authoritative.
float GroundSlopeAngleDegrees(JPH::Vec3Arg normal)
{
    const float lengthSq = normal.LengthSq();
    if (lengthSq <= 1.0e-8f)
    {
        return 0.0f;
    }

    const float upDot = ClampUnit(normal.GetY() / std::sqrt(lengthSq));
    return std::acos(upDot) * 180.0f / 3.14159265358979323846f;
}

void ReportError(const char* message)
{
    std::fprintf(stderr, "PhysicsWorld: %s\n", message);
}

PlayerGroundSupport ToGroundSupport(JPH::CharacterBase::EGroundState state)
{
    switch (state)
    {
    case JPH::CharacterBase::EGroundState::OnGround:
        return PlayerGroundSupport::OnGround;
    case JPH::CharacterBase::EGroundState::OnSteepGround:
        return PlayerGroundSupport::OnSteepGround;
    case JPH::CharacterBase::EGroundState::NotSupported:
        return PlayerGroundSupport::NotSupported;
    case JPH::CharacterBase::EGroundState::InAir:
        return PlayerGroundSupport::InAir;
    default:
        return PlayerGroundSupport::InAir;
    }
}

const char* MotionTypeName(JPH::EMotionType motionType)
{
    switch (motionType)
    {
    case JPH::EMotionType::Static:
        return "Static";
    case JPH::EMotionType::Kinematic:
        return "Kinematic";
    case JPH::EMotionType::Dynamic:
        return "Dynamic";
    default:
        return "None";
    }
}

float ClampDeltaSeconds(float deltaSeconds)
{
    if (deltaSeconds <= 0.0f)
    {
        return 0.0f;
    }
    if (deltaSeconds > kMaxPhysicsDeltaSeconds)
    {
        return kMaxPhysicsDeltaSeconds;
    }
    return deltaSeconds;
}
}

struct PhysicsWorld::Impl
{
    struct CarryContactListener final : public JPH::ContactListener
    {
        Impl* owner = nullptr;

        JPH::ValidateResult OnContactValidate(
            const JPH::Body& inBody1,
            const JPH::Body& inBody2,
            JPH::RVec3Arg,
            const JPH::CollideShapeResult&) override
        {
            if (owner == nullptr)
            {
                return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
            }

            const JPH::BodyID carried = owner->CarriedBodyId();
            const JPH::BodyID inner =
                owner->character != nullptr ? owner->character->GetInnerBodyID() : JPH::BodyID();
            if (carried.IsInvalid() || inner.IsInvalid())
            {
                return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
            }

            const JPH::BodyID a = inBody1.GetID();
            const JPH::BodyID b = inBody2.GetID();
            if ((a == carried && b == inner) || (a == inner && b == carried))
            {
                return JPH::ValidateResult::RejectAllContactsForThisBodyPair;
            }
            return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
        }
    };

    BroadPhaseLayerInterfaceImpl broadPhaseLayerInterface;
    ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseLayerFilter;
    ObjectLayerPairFilterImpl objectLayerPairFilter;
    std::unique_ptr<JPH::TempAllocatorImpl> tempAllocator;
    std::unique_ptr<JPH::JobSystemSingleThreaded> jobSystem;
    std::unique_ptr<JPH::PhysicsSystem> physicsSystem;
    std::vector<JPH::BodyID> staticBodyIds;
    std::vector<JPH::BodyID> dynamicBodyIds;
    JPH::BodyID movingPlatformId;
    JPH::Ref<JPH::CharacterVirtual> character;
    core::Vec3 playerVisualSize{0.8f, 1.6f, 0.8f};
    float gameplayZ = 0.0f;
    world::MovingPlatformSpec movingPlatformSpec{};
    std::vector<world::DynamicBoxSpec> dynamicBoxSpecs{};
    std::vector<world::PressurePlateSpec> pressurePlateSpecs{};
    std::vector<world::DoorSpec> doorSpecs{};
    std::vector<JPH::BodyID> doorBodyIds{};
    std::vector<float> doorOpenFraction{};
    std::vector<bool> doorBlockedClosing{};
    float killPlaneY = 0.0f;
    float movingPlatformDirection = 1.0f;
    float carriedGroundVelocityX = 0.0f;
    bool holdsJoltRegistration = false;
    bool initialized = false;
    CarryContactListener contactListener;
    int grabHeldIndex = kNoDynamicBoxGrabIndex;
    int grabTargetIndex = kNoDynamicBoxGrabIndex;
    core::Vec3 grabAimCenter{};
    float grabFacingX = 1.0f;
    bool grabAimValid = false;
    core::Vec3 grabCarryTarget{};
    bool grabCarryTargetValid = false;

    JPH::BodyID CarriedBodyId() const
    {
        if (grabHeldIndex < 0
            || static_cast<std::size_t>(grabHeldIndex) >= dynamicBodyIds.size())
        {
            return {};
        }
        return dynamicBodyIds[static_cast<std::size_t>(grabHeldIndex)];
    }

    bool HeldIndexIsValid() const
    {
        if (grabHeldIndex < 0
            || static_cast<std::size_t>(grabHeldIndex) >= dynamicBodyIds.size())
        {
            return false;
        }
        return !dynamicBodyIds[static_cast<std::size_t>(grabHeldIndex)].IsInvalid();
    }

    bool TargetIndexIsValid() const
    {
        if (grabTargetIndex < 0
            || static_cast<std::size_t>(grabTargetIndex) >= dynamicBodyIds.size())
        {
            return false;
        }
        return !dynamicBodyIds[static_cast<std::size_t>(grabTargetIndex)].IsInvalid();
    }

    IgnoreBodyFilter MakeCharacterBodyFilter() const
    {
        return IgnoreBodyFilter(CarriedBodyId());
    }

    core::Vec3 CharacterVisualCenter() const
    {
        if (character == nullptr)
        {
            return grabAimCenter;
        }
        const JPH::RVec3 feet = character->GetPosition();
        return {
            static_cast<float>(feet.GetX()),
            static_cast<float>(feet.GetY()) + playerVisualSize.y * 0.5f,
            gameplayZ};
    }

    void EnsureGrabAim()
    {
        if (grabAimValid)
        {
            return;
        }
        if (character == nullptr)
        {
            return;
        }
        grabAimCenter = CharacterVisualCenter();
        grabAimValid = true;
    }

    bool CastWorldRay(JPH::RVec3 origin, JPH::Vec3 offset, float& outFraction) const
    {
        if (physicsSystem == nullptr)
        {
            return false;
        }
        if (offset.LengthSq() <= 1.0e-8f)
        {
            return false;
        }

        JPH::RRayCast ray{origin, offset};
        JPH::RayCastSettings settings;
        JPH::ClosestHitCollisionCollector<JPH::CastRayCollector> collector;
        const JPH::BodyID inner =
            character != nullptr ? character->GetInnerBodyID() : JPH::BodyID();
        const WorldSolidBodyFilter bodyFilter(&dynamicBodyIds, inner, {});
        physicsSystem->GetNarrowPhaseQuery().CastRay(
            ray,
            settings,
            collector,
            physicsSystem->GetDefaultBroadPhaseLayerFilter(ObjectLayers::Moving),
            physicsSystem->GetDefaultLayerFilter(ObjectLayers::Moving),
            bodyFilter);
        if (!collector.HadHit())
        {
            return false;
        }
        outFraction = collector.mHit.mFraction;
        return true;
    }

    void ClearCarryState()
    {
        if (HeldIndexIsValid() && physicsSystem != nullptr)
        {
            physicsSystem->GetBodyInterface().SetGravityFactor(CarriedBodyId(), 1.0f);
        }
        grabHeldIndex = kNoDynamicBoxGrabIndex;
        grabCarryTarget = {};
        grabCarryTargetValid = false;
    }

    void RefreshGrabTarget()
    {
        grabTargetIndex = kNoDynamicBoxGrabIndex;
        if (HeldIndexIsValid())
        {
            return;
        }
        EnsureGrabAim();
        if (!grabAimValid || physicsSystem == nullptr)
        {
            return;
        }

        const float facing = static_cast<float>(FacingSign(grabFacingX));
        int bestIndex = kNoDynamicBoxGrabIndex;
        float bestDistance = kDynamicBoxMaxGrabDistance + 1.0f;
        const std::size_t count = DynamicBoxCount();
        for (std::size_t index = 0; index < count; ++index)
        {
            const JPH::BodyID id = dynamicBodyIds[index];
            if (id.IsInvalid())
            {
                continue;
            }

            const core::Vec3 center = ToVec3(physicsSystem->GetBodyInterface().GetPosition(id));
            const core::Vec3 delta{
                center.x - grabAimCenter.x,
                center.y - grabAimCenter.y,
                center.z - grabAimCenter.z};
            const float distance = Vec3Length(delta);
            if (!(distance <= kDynamicBoxMaxGrabDistance) || distance < 0.05f)
            {
                continue;
            }

            const float facingDelta = delta.x * facing;
            if (facingDelta <= 0.0f)
            {
                continue;
            }
            if (facingDelta < distance * kDynamicBoxMinFacingDot)
            {
                continue;
            }

            float fraction = 1.0f;
            const JPH::Vec3 offset(delta.x, delta.y, delta.z);
            if (CastWorldRay(ToRVec3(grabAimCenter), offset, fraction) && fraction < 0.98f)
            {
                continue;
            }

            if (distance + 1.0e-5f < bestDistance)
            {
                bestDistance = distance;
                bestIndex = static_cast<int>(index);
            }
        }
        grabTargetIndex = bestIndex;
    }

    core::Vec3 ComputeCarryTarget(core::Vec3 boxSize) const
    {
        const float facing = static_cast<float>(FacingSign(grabFacingX));
        core::Vec3 desired{
            grabAimCenter.x + facing * kDynamicBoxCarryDistance,
            grabAimCenter.y + kDynamicBoxCarryHeightOffset,
            grabAimCenter.z};

        const core::Vec3 delta{
            desired.x - grabAimCenter.x,
            desired.y - grabAimCenter.y,
            desired.z - grabAimCenter.z};
        const float desiredLength = Vec3Length(delta);
        if (desiredLength <= 1.0e-4f)
        {
            return desired;
        }

        float fraction = 1.0f;
        if (CastWorldRay(ToRVec3(grabAimCenter), JPH::Vec3(delta.x, delta.y, delta.z), fraction)
            && fraction < 1.0f)
        {
            const float halfExtent =
                0.5f * std::max(boxSize.x, std::max(boxSize.y, boxSize.z));
            float allowed = desiredLength * fraction - halfExtent - kDynamicBoxCarrySafetyMargin;
            if (allowed < kDynamicBoxCarryMinDistance)
            {
                allowed = kDynamicBoxCarryMinDistance;
            }
            if (allowed > desiredLength)
            {
                allowed = desiredLength;
            }
            const float scale = allowed / desiredLength;
            desired.x = grabAimCenter.x + delta.x * scale;
            desired.y = grabAimCenter.y + delta.y * scale;
            desired.z = grabAimCenter.z + delta.z * scale;
        }
        return desired;
    }

    void DriveCarriedBox(float deltaSeconds)
    {
        (void)deltaSeconds;
        if (!HeldIndexIsValid() || physicsSystem == nullptr)
        {
            ClearCarryState();
            return;
        }

        const std::size_t index = static_cast<std::size_t>(grabHeldIndex);
        grabCarryTarget = ComputeCarryTarget(dynamicBoxSpecs[index].size);
        grabCarryTargetValid = true;

        const JPH::BodyID id = dynamicBodyIds[index];
        JPH::BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();
        const JPH::RVec3 position = bodyInterface.GetPosition(id);
        JPH::Vec3 offset(
            grabCarryTarget.x - static_cast<float>(position.GetX()),
            grabCarryTarget.y - static_cast<float>(position.GetY()),
            grabCarryTarget.z - static_cast<float>(position.GetZ()));
        const float distance = offset.Length();
        JPH::Vec3 velocity = JPH::Vec3::sZero();
        if (distance > 1.0e-4f)
        {
            const float speed = std::min(kDynamicBoxCarryMaxSpeed, distance * kDynamicBoxCarryGain);
            velocity = offset * (speed / distance);
        }
        bodyInterface.SetLinearVelocity(id, velocity);
        bodyInterface.SetAngularVelocity(id, JPH::Vec3::sZero());
        bodyInterface.SetGravityFactor(id, 0.0f);
        bodyInterface.ActivateBody(id);
    }

    void TryGrabCurrentTarget()
    {
        if (HeldIndexIsValid() || !TargetIndexIsValid())
        {
            return;
        }
        grabHeldIndex = grabTargetIndex;
        grabTargetIndex = kNoDynamicBoxGrabIndex;
        if (physicsSystem != nullptr)
        {
            physicsSystem->GetBodyInterface().SetGravityFactor(CarriedBodyId(), 0.0f);
            physicsSystem->GetBodyInterface().ActivateBody(CarriedBodyId());
        }
    }

    bool AabbOverlap(
        core::Vec3 aCenter,
        core::Vec3 aSize,
        core::Vec3 bCenter,
        core::Vec3 bSize) const
    {
        return std::fabs(aCenter.x - bCenter.x) < (aSize.x + bSize.x) * 0.5f
            && std::fabs(aCenter.y - bCenter.y) < (aSize.y + bSize.y) * 0.5f
            && std::fabs(aCenter.z - bCenter.z) < (aSize.z + bSize.z) * 0.5f;
    }

    void DropCarriedBox()
    {
        if (!HeldIndexIsValid() || physicsSystem == nullptr)
        {
            ClearCarryState();
            return;
        }

        const std::size_t index = static_cast<std::size_t>(grabHeldIndex);
        const JPH::BodyID id = dynamicBodyIds[index];
        JPH::BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();
        bodyInterface.SetGravityFactor(id, 1.0f);

        const core::Vec3 boxCenter = ToVec3(bodyInterface.GetPosition(id));
        const core::Vec3 boxSize = dynamicBoxSpecs[index].size;
        const core::Vec3 playerCenter = CharacterVisualCenter();
        if (AabbOverlap(playerCenter, playerVisualSize, boxCenter, boxSize))
        {
            const float facing = static_cast<float>(FacingSign(grabFacingX));
            const float needed =
                0.5f * (playerVisualSize.x + boxSize.x) + kDynamicBoxCarrySafetyMargin;
            core::Vec3 nudged = boxCenter;
            nudged.x = playerCenter.x + facing * needed;
            float fraction = 1.0f;
            const JPH::Vec3 offset(
                nudged.x - playerCenter.x,
                nudged.y - playerCenter.y,
                nudged.z - playerCenter.z);
            if (CastWorldRay(ToRVec3(playerCenter), offset, fraction) && fraction < 1.0f)
            {
                const float halfExtent = 0.5f * boxSize.x + kDynamicBoxCarrySafetyMargin;
                float allowed = Vec3Length({offset.GetX(), offset.GetY(), offset.GetZ()}) * fraction
                    - halfExtent;
                if (allowed < kDynamicBoxCarryMinDistance)
                {
                    allowed = kDynamicBoxCarryMinDistance;
                }
                nudged.x = playerCenter.x + facing * allowed;
            }
            bodyInterface.SetPosition(id, ToRVec3(nudged), JPH::EActivation::Activate);
        }

        grabHeldIndex = kNoDynamicBoxGrabIndex;
        grabCarryTarget = {};
        grabCarryTargetValid = false;
    }

    bool AddStaticBox(const world::Box& box, const char* name)
    {
        JPH::BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();
        JPH::BodyCreationSettings settings(
            new JPH::BoxShape(ToHalfExtent(box.size)),
            ToRVec3(box.center),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Static,
            ObjectLayers::NonMoving);
        const JPH::BodyID id =
            bodyInterface.CreateAndAddBody(settings, JPH::EActivation::DontActivate);
        if (id.IsInvalid())
        {
            std::fprintf(
                stderr,
                "PhysicsWorld: failed to create static body '%s' "
                "center=(%.3f, %.3f, %.3f) size=(%.3f, %.3f, %.3f)\n",
                name,
                box.center.x,
                box.center.y,
                box.center.z,
                box.size.x,
                box.size.y,
                box.size.z);
            return false;
        }

        staticBodyIds.push_back(id);
        return true;
    }

    bool AddStaticSlope(const world::SlopeSpec& slope, const char* name)
    {
        JPH::BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();
        const JPH::Quat rotation =
            JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), JPH::DegreesToRadians(slope.rotationZDegrees));
        JPH::BodyCreationSettings settings(
            new JPH::BoxShape(ToHalfExtent(slope.size)),
            ToRVec3(slope.center),
            rotation,
            JPH::EMotionType::Static,
            ObjectLayers::NonMoving);
        const JPH::BodyID id =
            bodyInterface.CreateAndAddBody(settings, JPH::EActivation::DontActivate);
        if (id.IsInvalid())
        {
            std::fprintf(
                stderr,
                "PhysicsWorld: failed to create static slope '%s' "
                "center=(%.3f, %.3f, %.3f) angleZ=%.1f\n",
                name,
                slope.center.x,
                slope.center.y,
                slope.center.z,
                slope.rotationZDegrees);
            return false;
        }

        staticBodyIds.push_back(id);
        return true;
    }

    bool AddGreyboxStaticBodies(const world::LevelDefinition& level)
    {
        if (!AddStaticBox(level.ground, "ground"))
        {
            return false;
        }

        int index = 0;
        for (const world::Box& platform : level.elevatedPlatforms)
        {
            char name[64];
            std::snprintf(name, sizeof(name), "elevated platform %d", index);
            if (!AddStaticBox(platform, name))
            {
                return false;
            }
            ++index;
        }

        if (!AddStaticSlope(
                level.slopes[static_cast<std::size_t>(world::kLevel01WalkableSlopeIndex)],
                "walkable slope"))
        {
            return false;
        }
        if (!AddStaticSlope(
                level.slopes[static_cast<std::size_t>(world::kLevel01SteepSlopeIndex)],
                "steep slope"))
        {
            return false;
        }

        return true;
    }

    bool CreateDynamicBoxes(const world::LevelDefinition& level)
    {
        dynamicBoxSpecs = level.dynamicBoxes;
        dynamicBodyIds.clear();
        dynamicBodyIds.reserve(level.dynamicBoxes.size());
        if (level.dynamicBoxes.empty())
        {
            return true;
        }

        JPH::BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();
        for (std::size_t index = 0; index < level.dynamicBoxes.size(); ++index)
        {
            const world::DynamicBoxSpec& spec = level.dynamicBoxes[index];
            JPH::BodyCreationSettings boxSettings(
                new JPH::BoxShape(ToHalfExtent(spec.size)),
                ToRVec3(spec.center),
                JPH::Quat::sIdentity(),
                JPH::EMotionType::Dynamic,
                ObjectLayers::Moving);
            boxSettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
            boxSettings.mMassPropertiesOverride.mMass = spec.massKg;
            const JPH::BodyID id =
                bodyInterface.CreateAndAddBody(boxSettings, JPH::EActivation::Activate);
            if (id.IsInvalid())
            {
                ReportError("failed to create dynamic box.");
                return false;
            }
            dynamicBodyIds.push_back(id);

            {
                JPH::BodyLockRead lock(physicsSystem->GetBodyLockInterface(), id);
                if (!lock.SucceededAndIsInBroadPhase())
                {
                    ReportError("failed to inspect dynamic box mass.");
                    return false;
                }

                const float inverseMass = lock.GetBody().GetMotionProperties()->GetInverseMass();
                const float mass = inverseMass > 0.0f ? (1.0f / inverseMass) : 0.0f;
                if (std::fabs(mass - spec.massKg) > 0.01f)
                {
                    std::fprintf(
                        stderr,
                        "PhysicsWorld: dynamic box %zu mass is %.3f kg, expected %.3f kg.\n",
                        index,
                        mass,
                        spec.massKg);
                    return false;
                }
            }
        }

        return true;
    }

    bool CreateMovingPlatform()
    {
        const world::MovingPlatformSpec& spec = movingPlatformSpec;
        JPH::BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();
        JPH::BodyCreationSettings settings(
            new JPH::BoxShape(ToHalfExtent(spec.size)),
            JPH::RVec3(spec.startX, spec.centerY, spec.centerZ),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Kinematic,
            ObjectLayers::Moving);
        settings.mAllowSleeping = false;
        movingPlatformId = bodyInterface.CreateAndAddBody(settings, JPH::EActivation::Activate);
        if (movingPlatformId.IsInvalid())
        {
            ReportError("failed to create kinematic moving platform.");
            return false;
        }

        movingPlatformDirection = 1.0f;
        return true;
    }

    bool CreateDoors()
    {
        doorBodyIds.clear();
        doorOpenFraction.assign(doorSpecs.size(), 0.0f);
        doorBlockedClosing.assign(doorSpecs.size(), false);
        JPH::BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();
        for (std::size_t index = 0; index < doorSpecs.size(); ++index)
        {
            const world::DoorSpec& spec = doorSpecs[index];
            JPH::BodyCreationSettings settings(
                new JPH::BoxShape(ToHalfExtent(spec.size)),
                ToRVec3(spec.center),
                JPH::Quat::sIdentity(),
                JPH::EMotionType::Kinematic,
                ObjectLayers::Moving);
            settings.mAllowSleeping = false;
            const JPH::BodyID id =
                bodyInterface.CreateAndAddBody(settings, JPH::EActivation::Activate);
            if (id.IsInvalid())
            {
                ReportError("failed to create kinematic door.");
                return false;
            }
            doorBodyIds.push_back(id);
        }
        return true;
    }

    bool PressurePlateIsActive(std::size_t plateIndex) const
    {
        if (plateIndex >= pressurePlateSpecs.size() || physicsSystem == nullptr)
        {
            return false;
        }

        const world::PressurePlateSpec& spec = pressurePlateSpecs[plateIndex];
        const JPH::BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();
        const std::size_t count = DynamicBoxCount();
        for (std::size_t boxIndex = 0; boxIndex < count; ++boxIndex)
        {
            const JPH::BodyID id = dynamicBodyIds[boxIndex];
            if (id.IsInvalid())
            {
                continue;
            }
            const core::Vec3 center = ToVec3(bodyInterface.GetPosition(id));
            if (world::PressurePlateOverlapsBox(spec, center, dynamicBoxSpecs[boxIndex].size))
            {
                return true;
            }
        }
        return false;
    }

    bool DoorDesiredOpen(int doorIndex) const
    {
        for (std::size_t plateIndex = 0; plateIndex < pressurePlateSpecs.size(); ++plateIndex)
        {
            if (pressurePlateSpecs[plateIndex].linkedDoorIndex != doorIndex)
            {
                continue;
            }
            if (PressurePlateIsActive(plateIndex))
            {
                return true;
            }
        }
        return false;
    }

    bool ClosingBlocked(std::size_t index, JPH::RVec3 proposedCenter) const
    {
        if (index >= doorBodyIds.size() || doorBodyIds[index].IsInvalid() || physicsSystem == nullptr)
        {
            return false;
        }

        JPH::RefConst<JPH::Shape> shape;
        {
            JPH::BodyLockRead lock(physicsSystem->GetBodyLockInterface(), doorBodyIds[index]);
            if (!lock.SucceededAndIsInBroadPhase())
            {
                return true;
            }
            shape = lock.GetBody().GetShape();
        }
        if (shape == nullptr)
        {
            return true;
        }

        JPH::CollideShapeSettings settings{};
        JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;
        const JPH::BodyID inner =
            character != nullptr ? character->GetInnerBodyID() : JPH::BodyID{};
        const DoorCloseBlockerFilter filter(doorBodyIds[index], inner, &dynamicBodyIds);
        physicsSystem->GetNarrowPhaseQuery().CollideShape(
            shape,
            JPH::Vec3::sReplicate(1.0f),
            JPH::RMat44::sTranslation(proposedCenter),
            settings,
            JPH::RVec3::sZero(),
            collector,
            physicsSystem->GetDefaultBroadPhaseLayerFilter(ObjectLayers::Moving),
            physicsSystem->GetDefaultLayerFilter(ObjectLayers::Moving),
            filter);
        return collector.HadHit();
    }

    void MoveDoorKinematic(std::size_t index, float fraction, float deltaSeconds)
    {
        if (index >= doorBodyIds.size() || doorBodyIds[index].IsInvalid())
        {
            return;
        }
        const core::Vec3 center = world::DoorCenterAtFraction(doorSpecs[index], fraction);
        physicsSystem->GetBodyInterface().MoveKinematic(
            doorBodyIds[index],
            ToRVec3(center),
            JPH::Quat::sIdentity(),
            deltaSeconds);
    }

    void SnapDoorsToDesired()
    {
        if (!initialized || physicsSystem == nullptr)
        {
            return;
        }

        JPH::BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();
        for (std::size_t index = 0; index < doorSpecs.size(); ++index)
        {
            const bool desired = DoorDesiredOpen(static_cast<int>(index));
            const float fraction = desired ? 1.0f : 0.0f;
            if (index < doorOpenFraction.size())
            {
                doorOpenFraction[index] = fraction;
            }
            if (index < doorBlockedClosing.size())
            {
                doorBlockedClosing[index] = false;
            }
            if (index >= doorBodyIds.size() || doorBodyIds[index].IsInvalid())
            {
                continue;
            }
            const JPH::BodyID id = doorBodyIds[index];
            bodyInterface.SetPositionAndRotation(
                id,
                ToRVec3(world::DoorCenterAtFraction(doorSpecs[index], fraction)),
                JPH::Quat::sIdentity(),
                JPH::EActivation::Activate);
            bodyInterface.SetLinearVelocity(id, JPH::Vec3::sZero());
            bodyInterface.SetAngularVelocity(id, JPH::Vec3::sZero());
        }
    }

    void StepDoors(float deltaSeconds)
    {
        if (!initialized || physicsSystem == nullptr || deltaSeconds <= 0.0f)
        {
            return;
        }

        for (std::size_t index = 0; index < doorSpecs.size(); ++index)
        {
            if (index >= doorBodyIds.size() || doorBodyIds[index].IsInvalid())
            {
                continue;
            }

            const world::DoorSpec& spec = doorSpecs[index];
            const bool desired = DoorDesiredOpen(static_cast<int>(index));
            const float target = desired ? 1.0f : 0.0f;
            float fraction = index < doorOpenFraction.size() ? doorOpenFraction[index] : 0.0f;
            const float distance = spec.openDistance > 0.0f ? spec.openDistance : 1.0f;
            const float step = (kDoorOpenSpeedMetersPerSecond * deltaSeconds) / distance;
            float next = fraction;
            if (target > fraction)
            {
                next = fraction + step;
                if (next > 1.0f)
                {
                    next = 1.0f;
                }
            }
            else if (target < fraction)
            {
                next = fraction - step;
                if (next < 0.0f)
                {
                    next = 0.0f;
                }
            }

            bool blocked = false;
            if (next < fraction)
            {
                const core::Vec3 proposed = world::DoorCenterAtFraction(spec, next);
                if (ClosingBlocked(index, ToRVec3(proposed)))
                {
                    blocked = true;
                    next = fraction;
                }
            }
            if (index < doorBlockedClosing.size())
            {
                doorBlockedClosing[index] = blocked;
            }
            if (index < doorOpenFraction.size())
            {
                doorOpenFraction[index] = next;
            }
            MoveDoorKinematic(index, next, deltaSeconds);
        }
    }

    void StepMovingPlatform(float deltaSeconds)
    {
        if (movingPlatformId.IsInvalid())
        {
            return;
        }

        const world::MovingPlatformSpec& spec = movingPlatformSpec;
        JPH::BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();
        const JPH::RVec3 current = bodyInterface.GetPosition(movingPlatformId);
        float x = static_cast<float>(current.GetX());
        x += movingPlatformDirection * spec.speed * deltaSeconds;
        if (x >= spec.pathMaxX)
        {
            x = spec.pathMaxX;
            movingPlatformDirection = -1.0f;
        }
        else if (x <= spec.pathMinX)
        {
            x = spec.pathMinX;
            movingPlatformDirection = 1.0f;
        }

        bodyInterface.MoveKinematic(
            movingPlatformId,
            JPH::RVec3(x, spec.centerY, spec.centerZ),
            JPH::Quat::sIdentity(),
            deltaSeconds);
    }

    void EnforceFixedZ()
    {
        if (character == nullptr)
        {
            return;
        }

        JPH::RVec3 position = character->GetPosition();
        position.SetZ(gameplayZ);
        character->SetPosition(position);

        JPH::Vec3 velocity = character->GetLinearVelocity();
        velocity.SetZ(0.0f);
        character->SetLinearVelocity(velocity);

        const JPH::BodyID innerBodyId = character->GetInnerBodyID();
        if (!innerBodyId.IsInvalid())
        {
            JPH::BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();
            JPH::Vec3 innerVelocity = bodyInterface.GetLinearVelocity(innerBodyId);
            innerVelocity.SetZ(0.0f);
            bodyInterface.SetLinearVelocity(innerBodyId, innerVelocity);
        }
    }

    // CharacterVirtual::Update slides the shape but does not write the
    // collision-resolved velocity back to mLinearVelocity. Clear downward
    // speed when standing so it cannot accumulate; keep positive Y so a
    // residual OnGround frame cannot cancel jump takeoff.
    void CancelSupportedDownwardVelocity()
    {
        if (character == nullptr)
        {
            return;
        }

        if (character->GetGroundState() != JPH::CharacterBase::EGroundState::OnGround)
        {
            return;
        }

        JPH::Vec3 velocity = character->GetLinearVelocity();
        if (velocity.GetY() <= 0.0f)
        {
            velocity.SetY(0.0f);
            character->SetLinearVelocity(velocity);
        }
    }

    int CountContacts() const
    {
        if (character == nullptr)
        {
            return 0;
        }

        int count = 0;
        for (const JPH::CharacterContact& contact : character->GetActiveContacts())
        {
            if (contact.mHadCollision)
            {
                ++count;
            }
        }
        return count;
    }

    std::size_t DynamicBoxCount() const
    {
        return dynamicBodyIds.size() < dynamicBoxSpecs.size()
            ? dynamicBodyIds.size()
            : dynamicBoxSpecs.size();
    }

    void ResetDynamicBoxBody(std::size_t index)
    {
        if (index >= DynamicBoxCount())
        {
            return;
        }

        const JPH::BodyID id = dynamicBodyIds[index];
        if (id.IsInvalid())
        {
            return;
        }

        JPH::BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();
        bodyInterface.SetGravityFactor(id, 1.0f);
        bodyInterface.SetPositionAndRotation(
            id,
            ToRVec3(dynamicBoxSpecs[index].center),
            JPH::Quat::sIdentity(),
            JPH::EActivation::Activate);
        bodyInterface.SetLinearVelocity(id, JPH::Vec3::sZero());
        bodyInterface.SetAngularVelocity(id, JPH::Vec3::sZero());
    }

    void RecoverFallenDynamicBoxes()
    {
        if (!initialized || physicsSystem == nullptr)
        {
            return;
        }

        JPH::BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();
        const std::size_t count = DynamicBoxCount();
        for (std::size_t index = 0; index < count; ++index)
        {
            const JPH::BodyID id = dynamicBodyIds[index];
            if (id.IsInvalid())
            {
                continue;
            }

            const float runtimeBodyCenterY = bodyInterface.GetPosition(id).GetY();
            if (runtimeBodyCenterY < killPlaneY)
            {
                if (grabHeldIndex == static_cast<int>(index))
                {
                    ClearCarryState();
                }
                ResetDynamicBoxBody(index);
            }
        }
    }
};

PhysicsWorld::PhysicsWorld()
    : impl(std::make_unique<Impl>())
{
}

PhysicsWorld::~PhysicsWorld()
{
    Shutdown();
}

bool PhysicsWorld::Initialize(const world::LevelDefinition& level)
{
    if (impl->initialized)
    {
        return true;
    }

    impl->movingPlatformSpec = level.movingPlatform;
    impl->dynamicBoxSpecs = level.dynamicBoxes;
    impl->pressurePlateSpecs = level.pressurePlates;
    impl->doorSpecs = level.doors;
    impl->killPlaneY = level.killPlaneY;

    if (!AuthoredPhysicsBodiesWithinBudget(
            static_cast<int>(level.elevatedPlatforms.size()),
            static_cast<int>(level.dynamicBoxes.size()),
            static_cast<int>(level.doors.size())))
    {
        ReportError("authored physics body capacity exceeded.");
        return false;
    }

    JPH::RegisterDefaultAllocator();
    JPH::Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)

    if (gJoltRegistrationUsers == 0)
    {
        if (JPH::Factory::sInstance == nullptr)
        {
            JPH::Factory::sInstance = new JPH::Factory();
        }
        JPH::RegisterTypes();
    }
    ++gJoltRegistrationUsers;
    impl->holdsJoltRegistration = true;

    impl->tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(kTempAllocatorBytes);
    impl->jobSystem = std::make_unique<JPH::JobSystemSingleThreaded>(kMaxPhysicsJobs);
    impl->physicsSystem = std::make_unique<JPH::PhysicsSystem>();
    impl->physicsSystem->Init(
        kPhysicsMaxBodies,
        kNumBodyMutexes,
        kMaxBodyPairs,
        kMaxContactConstraints,
        impl->broadPhaseLayerInterface,
        impl->objectVsBroadPhaseLayerFilter,
        impl->objectLayerPairFilter);
    impl->contactListener.owner = impl.get();
    impl->physicsSystem->SetContactListener(&impl->contactListener);

    if (!impl->AddGreyboxStaticBodies(level))
    {
        Shutdown();
        return false;
    }

    if (!impl->CreateDynamicBoxes(level))
    {
        Shutdown();
        return false;
    }

    if (!impl->CreateMovingPlatform())
    {
        Shutdown();
        return false;
    }

    if (!impl->CreateDoors())
    {
        Shutdown();
        return false;
    }

    impl->physicsSystem->OptimizeBroadPhase();
    impl->initialized = true;
    impl->SnapDoorsToDesired();
    return true;
}

bool PhysicsWorld::InitializePlayer(core::Vec3 visualCenter, core::Vec3 visualSize)
{
    if (!impl->initialized)
    {
        ReportError("cannot initialize player character before PhysicsWorld.");
        return false;
    }
    if (impl->character != nullptr)
    {
        return true;
    }

    impl->playerVisualSize = visualSize;
    impl->gameplayZ = visualCenter.z;

    const JPH::Vec3 capsuleTranslation(0.0f, kCapsuleHalfCylinder + kCapsuleRadius, 0.0f);
    const JPH::RefConst<JPH::Shape> capsule =
        JPH::RotatedTranslatedShapeSettings(
            capsuleTranslation,
            JPH::Quat::sIdentity(),
            new JPH::CapsuleShape(kCapsuleHalfCylinder, kCapsuleRadius))
            .Create()
            .Get();
    if (capsule == nullptr)
    {
        ReportError("failed to create CharacterVirtual capsule shape.");
        return false;
    }

    const JPH::RefConst<JPH::Shape> innerCapsule =
        JPH::RotatedTranslatedShapeSettings(
            capsuleTranslation,
            JPH::Quat::sIdentity(),
            new JPH::CapsuleShape(
                kInnerShapeFraction * kCapsuleHalfCylinder,
                kInnerShapeFraction * kCapsuleRadius))
            .Create()
            .Get();
    if (innerCapsule == nullptr)
    {
        ReportError("failed to create CharacterVirtual inner body shape.");
        return false;
    }

    JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();
    settings->mShape = capsule;
    settings->mInnerBodyShape = innerCapsule;
    settings->mInnerBodyLayer = ObjectLayers::Moving;
    settings->mUp = JPH::Vec3::sAxisY();
    settings->mMaxSlopeAngle = kMaxSlopeAngleRadians;
    settings->mCharacterPadding = kCharacterPadding;
    settings->mCollisionTolerance = kCollisionTolerance;
    settings->mPredictiveContactDistance = kPredictiveContactDistance;
    settings->mPenetrationRecoverySpeed = kPenetrationRecoverySpeed;
    settings->mMass = kCharacterMass;
    settings->mMaxStrength = kMaxStrength;
    settings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -kCapsuleRadius);

    const float feetY = visualCenter.y - visualSize.y * 0.5f;
    const JPH::RVec3 feetPosition(visualCenter.x, feetY, impl->gameplayZ);
    impl->character = new JPH::CharacterVirtual(
        settings,
        feetPosition,
        JPH::Quat::sIdentity(),
        impl->physicsSystem.get());
    if (impl->character->GetInnerBodyID().IsInvalid())
    {
        ReportError("failed to create CharacterVirtual inner body.");
        impl->character = nullptr;
        return false;
    }

    const IgnoreBodyFilter bodyFilter = impl->MakeCharacterBodyFilter();
    impl->character->RefreshContacts(
        impl->physicsSystem->GetDefaultBroadPhaseLayerFilter(ObjectLayers::Moving),
        impl->physicsSystem->GetDefaultLayerFilter(ObjectLayers::Moving),
        bodyFilter,
        {},
        *impl->tempAllocator);
    impl->EnforceFixedZ();
    return true;
}

bool PhysicsWorld::TryRebuild(
    const world::LevelDefinition& level,
    core::Vec3 playerVisualCenter,
    core::Vec3 playerVisualSize)
{
    PhysicsWorld replacement;
    if (!replacement.Initialize(level))
    {
        return false;
    }
    if (!replacement.InitializePlayer(playerVisualCenter, playerVisualSize))
    {
        return false;
    }
    impl.swap(replacement.impl);
    return true;
}

void PhysicsWorld::ResetCharacter(const core::Vec3& visualCenter, const core::Vec3& velocity)
{
    impl->ClearCarryState();
    if (impl->character == nullptr)
    {
        return;
    }

    const float feetY = visualCenter.y - impl->playerVisualSize.y * 0.5f;
    impl->character->SetPosition(JPH::RVec3(visualCenter.x, feetY, impl->gameplayZ));
    impl->character->SetLinearVelocity(JPH::Vec3(velocity.x, velocity.y, 0.0f));
    impl->carriedGroundVelocityX = 0.0f;

    const JPH::BodyID innerBodyId = impl->character->GetInnerBodyID();
    if (!innerBodyId.IsInvalid())
    {
        JPH::BodyInterface& bodyInterface = impl->physicsSystem->GetBodyInterface();
        bodyInterface.SetLinearVelocity(innerBodyId, JPH::Vec3::sZero());
        bodyInterface.SetAngularVelocity(innerBodyId, JPH::Vec3::sZero());
    }

    const IgnoreBodyFilter bodyFilter = impl->MakeCharacterBodyFilter();
    impl->character->RefreshContacts(
        impl->physicsSystem->GetDefaultBroadPhaseLayerFilter(ObjectLayers::Moving),
        impl->physicsSystem->GetDefaultLayerFilter(ObjectLayers::Moving),
        bodyFilter,
        {},
        *impl->tempAllocator);
    impl->EnforceFixedZ();
}

void PhysicsWorld::ResetMovingPlatform()
{
    if (!impl->initialized || impl->movingPlatformId.IsInvalid())
    {
        return;
    }

    const world::MovingPlatformSpec& spec = impl->movingPlatformSpec;
    JPH::BodyInterface& bodyInterface = impl->physicsSystem->GetBodyInterface();
    bodyInterface.SetPositionAndRotation(
        impl->movingPlatformId,
        JPH::RVec3(spec.startX, spec.centerY, spec.centerZ),
        JPH::Quat::sIdentity(),
        JPH::EActivation::Activate);
    bodyInterface.SetLinearVelocity(impl->movingPlatformId, JPH::Vec3::sZero());
    bodyInterface.SetAngularVelocity(impl->movingPlatformId, JPH::Vec3::sZero());
    impl->movingPlatformDirection = 1.0f;
}

void PhysicsWorld::ResetDynamicBoxes()
{
    if (!impl->initialized)
    {
        return;
    }

    impl->ClearCarryState();
    const std::size_t count = impl->DynamicBoxCount();
    for (std::size_t index = 0; index < count; ++index)
    {
        impl->ResetDynamicBoxBody(index);
    }
    impl->SnapDoorsToDesired();
}

void PhysicsWorld::RecoverFallenDynamicBoxes()
{
    impl->RecoverFallenDynamicBoxes();
}

void PhysicsWorld::SetGrabAim(core::Vec3 playerVisualCenter, float facingX)
{
    if (!std::isfinite(playerVisualCenter.x) || !std::isfinite(playerVisualCenter.y)
        || !std::isfinite(playerVisualCenter.z))
    {
        return;
    }
    impl->grabAimCenter = playerVisualCenter;
    impl->grabFacingX = static_cast<float>(FacingSign(facingX));
    impl->grabAimValid = true;
}

void PhysicsWorld::HandleGrabDrop()
{
    if (!impl->initialized)
    {
        return;
    }
    if (impl->HeldIndexIsValid())
    {
        impl->DropCarriedBox();
        return;
    }
    impl->EnsureGrabAim();
    impl->RefreshGrabTarget();
    impl->TryGrabCurrentTarget();
}

void PhysicsWorld::ClearCarry()
{
    impl->ClearCarryState();
}

DynamicBoxGrabState PhysicsWorld::GetGrabState() const
{
    DynamicBoxGrabState state;
    if (impl->HeldIndexIsValid())
    {
        state.carrying = true;
        state.carriedIndex = impl->grabHeldIndex;
        state.carryTarget = impl->grabCarryTarget;
        state.carryTargetValid = impl->grabCarryTargetValid;
        return state;
    }
    if (impl->TargetIndexIsValid())
    {
        state.hasTarget = true;
        state.targetIndex = impl->grabTargetIndex;
    }
    return state;
}

void PhysicsWorldTestAccess::SetDynamicBoxRuntimeMotion(
    PhysicsWorld& world,
    std::size_t index,
    core::Vec3 center,
    core::Vec3 linearVelocity,
    core::Vec3 angularVelocity,
    float rotationX,
    float rotationY,
    float rotationZ,
    float rotationW)
{
    if (!world.impl->initialized || world.impl->physicsSystem == nullptr
        || index >= world.impl->DynamicBoxCount())
    {
        return;
    }

    const JPH::BodyID id = world.impl->dynamicBodyIds[index];
    if (id.IsInvalid())
    {
        return;
    }

    JPH::Quat rotation(rotationX, rotationY, rotationZ, rotationW);
    if (!rotation.IsNormalized())
    {
        if (rotation.LengthSq() > 0.0f)
        {
            rotation = rotation.Normalized();
        }
        else
        {
            rotation = JPH::Quat::sIdentity();
        }
    }

    JPH::BodyInterface& bodyInterface = world.impl->physicsSystem->GetBodyInterface();
    bodyInterface.SetPositionAndRotation(
        id,
        ToRVec3(center),
        rotation,
        JPH::EActivation::Activate);
    bodyInterface.SetLinearVelocity(
        id, JPH::Vec3(linearVelocity.x, linearVelocity.y, linearVelocity.z));
    bodyInterface.SetAngularVelocity(
        id, JPH::Vec3(angularVelocity.x, angularVelocity.y, angularVelocity.z));
}

void PhysicsWorld::UpdateMovingPlatform(float deltaSeconds)
{
    if (!impl->initialized)
    {
        return;
    }

    const float stepSeconds = ClampDeltaSeconds(deltaSeconds);
    if (stepSeconds <= 0.0f)
    {
        return;
    }

    impl->StepMovingPlatform(stepSeconds);
}

void PhysicsWorld::MovePlayer(const PlayerMoveCommand& command, float deltaSeconds)
{
    if (impl->character == nullptr)
    {
        return;
    }

    const float stepSeconds = ClampDeltaSeconds(deltaSeconds);
    if (stepSeconds <= 0.0f)
    {
        return;
    }

    // MoveKinematic already updated the supporting body's velocity this frame.
    // Refresh the cached ground velocity without another collision query.
    impl->character->UpdateGroundVelocity();

    float groundVelocityX = 0.0f;
    if (impl->character->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround)
    {
        groundVelocityX = impl->character->GetGroundVelocity().GetX();
        impl->carriedGroundVelocityX = groundVelocityX;
    }
    else
    {
        groundVelocityX = impl->carriedGroundVelocityX;
    }

    // command.horizontalVelocity is Player-relative. World X includes moving ground
    // so standing still rides the platform and takeoff keeps platform momentum.
    const float worldHorizontalVelocity = command.horizontalVelocity + groundVelocityX;
    impl->character->SetLinearVelocity(
        JPH::Vec3(worldHorizontalVelocity, command.verticalVelocity, 0.0f));
    const IgnoreBodyFilter bodyFilter = impl->MakeCharacterBodyFilter();
    impl->character->Update(
        stepSeconds,
        JPH::Vec3(0.0f, kCharacterGravityY, 0.0f),
        impl->physicsSystem->GetDefaultBroadPhaseLayerFilter(ObjectLayers::Moving),
        impl->physicsSystem->GetDefaultLayerFilter(ObjectLayers::Moving),
        bodyFilter,
        {},
        *impl->tempAllocator);
    impl->EnforceFixedZ();
    impl->CancelSupportedDownwardVelocity();
}

void PhysicsWorld::Update(float deltaSeconds)
{
    if (!impl->initialized)
    {
        return;
    }

    const float stepSeconds = ClampDeltaSeconds(deltaSeconds);
    if (stepSeconds <= 0.0f)
    {
        return;
    }

    impl->EnsureGrabAim();
    if (impl->HeldIndexIsValid())
    {
        impl->DriveCarriedBox(stepSeconds);
    }
    else
    {
        impl->RefreshGrabTarget();
    }

    impl->StepDoors(stepSeconds);

    impl->physicsSystem->Update(
        stepSeconds,
        1,
        impl->tempAllocator.get(),
        impl->jobSystem.get());

    impl->EnforceFixedZ();
    impl->RecoverFallenDynamicBoxes();
}

void PhysicsWorld::Shutdown()
{
    if (impl->physicsSystem)
    {
        impl->physicsSystem->SetContactListener(nullptr);
    }
    impl->contactListener.owner = nullptr;
    impl->ClearCarryState();
    impl->grabTargetIndex = kNoDynamicBoxGrabIndex;
    impl->grabAimValid = false;

    impl->character = nullptr;
    impl->carriedGroundVelocityX = 0.0f;
    impl->movingPlatformDirection = 1.0f;

    if (impl->physicsSystem)
    {
        JPH::BodyInterface& bodyInterface = impl->physicsSystem->GetBodyInterface();
        for (const JPH::BodyID id : impl->dynamicBodyIds)
        {
            if (!id.IsInvalid())
            {
                bodyInterface.RemoveBody(id);
                bodyInterface.DestroyBody(id);
            }
        }
        impl->dynamicBodyIds.clear();
        impl->dynamicBoxSpecs.clear();
        impl->pressurePlateSpecs.clear();
        impl->killPlaneY = 0.0f;
        for (const JPH::BodyID id : impl->doorBodyIds)
        {
            if (!id.IsInvalid())
            {
                bodyInterface.RemoveBody(id);
                bodyInterface.DestroyBody(id);
            }
        }
        impl->doorBodyIds.clear();
        impl->doorSpecs.clear();
        impl->doorOpenFraction.clear();
        impl->doorBlockedClosing.clear();
        if (!impl->movingPlatformId.IsInvalid())
        {
            bodyInterface.RemoveBody(impl->movingPlatformId);
            bodyInterface.DestroyBody(impl->movingPlatformId);
            impl->movingPlatformId = {};
        }
        for (const JPH::BodyID id : impl->staticBodyIds)
        {
            if (!id.IsInvalid())
            {
                bodyInterface.RemoveBody(id);
                bodyInterface.DestroyBody(id);
            }
        }
        impl->staticBodyIds.clear();
    }

    impl->physicsSystem.reset();
    impl->jobSystem.reset();
    impl->tempAllocator.reset();

    if (impl->holdsJoltRegistration)
    {
        impl->holdsJoltRegistration = false;
        --gJoltRegistrationUsers;
        if (gJoltRegistrationUsers == 0)
        {
            JPH::UnregisterTypes();
            delete JPH::Factory::sInstance;
            JPH::Factory::sInstance = nullptr;
        }
    }

    impl->initialized = false;
}

bool PhysicsWorld::IsInitialized() const
{
    return impl->initialized;
}

int PhysicsWorld::StaticBodyCount() const
{
    return static_cast<int>(impl->staticBodyIds.size());
}

int PhysicsWorld::DynamicBodyCount() const
{
    int count = 0;
    for (const JPH::BodyID id : impl->dynamicBodyIds)
    {
        if (!id.IsInvalid())
        {
            ++count;
        }
    }
    return count;
}

std::vector<DynamicBoxRuntimeState> PhysicsWorld::GetDynamicBoxes() const
{
    std::vector<DynamicBoxRuntimeState> boxes(impl->dynamicBoxSpecs.size());
    for (std::size_t index = 0; index < impl->dynamicBoxSpecs.size(); ++index)
    {
        DynamicBoxRuntimeState& box = boxes[index];
        box.size = impl->dynamicBoxSpecs[index].size;
        box.massKg = impl->dynamicBoxSpecs[index].massKg;
        box.valid = impl->initialized && index < impl->dynamicBodyIds.size()
            && !impl->dynamicBodyIds[index].IsInvalid();
        if (!box.valid)
        {
            box.center = impl->dynamicBoxSpecs[index].center;
            box.rotationW = 1.0f;
            continue;
        }

        const JPH::BodyInterface& bodyInterface = impl->physicsSystem->GetBodyInterface();
        const JPH::BodyID id = impl->dynamicBodyIds[index];
        box.center = ToVec3(bodyInterface.GetPosition(id));
        const JPH::Vec3 linearVelocity = bodyInterface.GetLinearVelocity(id);
        box.linearVelocity = {linearVelocity.GetX(), linearVelocity.GetY(), linearVelocity.GetZ()};
        const JPH::Vec3 angularVelocity = bodyInterface.GetAngularVelocity(id);
        box.angularVelocity = {
            angularVelocity.GetX(), angularVelocity.GetY(), angularVelocity.GetZ()};
        const JPH::Quat rotation = bodyInterface.GetRotation(id);
        box.rotationX = rotation.GetX();
        box.rotationY = rotation.GetY();
        box.rotationZ = rotation.GetZ();
        box.rotationW = rotation.GetW();
        box.active = bodyInterface.IsActive(id);
    }
    return boxes;
}

std::vector<PressurePlateRuntimeState> PhysicsWorld::GetPressurePlates() const
{
    std::vector<PressurePlateRuntimeState> plates(impl->pressurePlateSpecs.size());
    const std::vector<DynamicBoxRuntimeState> boxes = GetDynamicBoxes();
    for (std::size_t plateIndex = 0; plateIndex < impl->pressurePlateSpecs.size(); ++plateIndex)
    {
        const world::PressurePlateSpec& spec = impl->pressurePlateSpecs[plateIndex];
        PressurePlateRuntimeState& plate = plates[plateIndex];
        plate.center = spec.center;
        plate.size = spec.size;
        plate.active = false;
        for (const DynamicBoxRuntimeState& box : boxes)
        {
            if (!box.valid)
            {
                continue;
            }
            if (world::PressurePlateOverlapsBox(spec, box.center, box.size))
            {
                plate.active = true;
                break;
            }
        }
    }
    return plates;
}

std::vector<DoorRuntimeState> PhysicsWorld::GetDoors() const
{
    std::vector<DoorRuntimeState> doors(impl->doorSpecs.size());
    for (std::size_t index = 0; index < impl->doorSpecs.size(); ++index)
    {
        const world::DoorSpec& spec = impl->doorSpecs[index];
        DoorRuntimeState& door = doors[index];
        door.closedCenter = spec.center;
        door.size = spec.size;
        door.openDistance = spec.openDistance;
        door.openFraction =
            index < impl->doorOpenFraction.size() ? impl->doorOpenFraction[index] : 0.0f;
        door.desiredOpen = impl->DoorDesiredOpen(static_cast<int>(index));
        door.blockedClosing =
            index < impl->doorBlockedClosing.size() && impl->doorBlockedClosing[index];
        door.valid = impl->initialized && index < impl->doorBodyIds.size()
            && !impl->doorBodyIds[index].IsInvalid();
        if (!door.valid || impl->physicsSystem == nullptr)
        {
            door.center = world::DoorCenterAtFraction(spec, door.openFraction);
            continue;
        }
        door.center = ToVec3(impl->physicsSystem->GetBodyInterface().GetPosition(
            impl->doorBodyIds[index]));
    }
    return doors;
}

int PhysicsWorld::DoorBodyCount() const
{
    int count = 0;
    for (const JPH::BodyID id : impl->doorBodyIds)
    {
        if (!id.IsInvalid())
        {
            ++count;
        }
    }
    return count;
}

MovingPlatformState PhysicsWorld::GetMovingPlatform() const
{
    MovingPlatformState state;
    const world::MovingPlatformSpec& spec = impl->movingPlatformSpec;
    state.size = spec.size;
    state.pathMinX = spec.pathMinX;
    state.pathMaxX = spec.pathMaxX;
    state.speed = spec.speed;
    state.direction = impl->movingPlatformDirection;
    state.valid = impl->initialized && !impl->movingPlatformId.IsInvalid();
    if (!state.valid)
    {
        state.position = {spec.startX, spec.centerY, spec.centerZ};
        return state;
    }

    const JPH::BodyInterface& bodyInterface = impl->physicsSystem->GetBodyInterface();
    state.position = ToVec3(bodyInterface.GetPosition(impl->movingPlatformId));
    const JPH::Vec3 velocity = bodyInterface.GetLinearVelocity(impl->movingPlatformId);
    state.velocity = {velocity.GetX(), velocity.GetY(), velocity.GetZ()};
    return state;
}

PlayerPhysicsState PhysicsWorld::GetPlayerPhysicsState() const
{
    PlayerPhysicsState state;
    state.characterInitialized = impl->character != nullptr;
    if (impl->character == nullptr)
    {
        return state;
    }

    const JPH::RVec3 feet = impl->character->GetPosition();
    const JPH::Vec3 velocity = impl->character->GetLinearVelocity();
    const JPH::Vec3 groundVelocity = impl->character->GetGroundVelocity();
    state.visualCenter = {
        static_cast<float>(feet.GetX()),
        static_cast<float>(feet.GetY()) + impl->playerVisualSize.y * 0.5f,
        impl->gameplayZ};
    state.horizontalVelocity = velocity.GetX();
    state.verticalVelocity = velocity.GetY();
    state.worldVelocity = {velocity.GetX(), velocity.GetY(), velocity.GetZ()};
    state.groundSupport = ToGroundSupport(impl->character->GetGroundState());
    state.supported = state.groundSupport == PlayerGroundSupport::OnGround;
    state.contactCount = impl->CountContacts();
    state.groundVelocity = {groundVelocity.GetX(), groundVelocity.GetY(), groundVelocity.GetZ()};
    state.supportingGroundMoving = state.supported && std::fabs(groundVelocity.GetX()) > 0.01f;
    const JPH::Vec3 groundNormal = impl->character->GetGroundNormal();
    state.groundNormal = {
        groundNormal.GetX(),
        groundNormal.GetY(),
        groundNormal.GetZ()};
    state.groundSlopeAngleDegrees = GroundSlopeAngleDegrees(groundNormal);
    state.currentSupportWalkable = state.supported;

    bool dynamicContact = false;
    for (const JPH::CharacterContact& contact : impl->character->GetActiveContacts())
    {
        if (contact.mHadCollision && contact.mMotionTypeB == JPH::EMotionType::Dynamic)
        {
            dynamicContact = true;
            break;
        }
    }
    state.dynamicContact = dynamicContact;

    state.supportBodyKind = "None";
    const JPH::BodyID groundBodyId = impl->character->GetGroundBodyID();
    if (!groundBodyId.IsInvalid())
    {
        JPH::BodyLockRead lock(impl->physicsSystem->GetBodyLockInterface(), groundBodyId);
        if (lock.SucceededAndIsInBroadPhase())
        {
            state.supportBodyKind = MotionTypeName(lock.GetBody().GetMotionType());
        }
    }

    const JPH::BodyID innerBodyId = impl->character->GetInnerBodyID();
    state.characterInnerBodyActive =
        !innerBodyId.IsInvalid()
        && impl->physicsSystem->GetBodyInterface().IsActive(innerBodyId);

    return state;
}
}
