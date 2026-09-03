#include "physics/PhysicsWorld.h"

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
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/ShapeFilter.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

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
constexpr unsigned int kMaxBodies = 32;
constexpr unsigned int kNumBodyMutexes = 0;
constexpr unsigned int kMaxBodyPairs = 64;
constexpr unsigned int kMaxContactConstraints = 64;
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
    BroadPhaseLayerInterfaceImpl broadPhaseLayerInterface;
    ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseLayerFilter;
    ObjectLayerPairFilterImpl objectLayerPairFilter;
    std::unique_ptr<JPH::TempAllocatorImpl> tempAllocator;
    std::unique_ptr<JPH::JobSystemSingleThreaded> jobSystem;
    std::unique_ptr<JPH::PhysicsSystem> physicsSystem;
    std::vector<JPH::BodyID> staticBodyIds;
    JPH::BodyID dynamicBodyId;
    JPH::BodyID movingPlatformId;
    JPH::Ref<JPH::CharacterVirtual> character;
    core::Vec3 playerVisualSize{0.8f, 1.6f, 0.8f};
    float gameplayZ = 0.0f;
    world::MovingPlatformSpec movingPlatformSpec{};
    world::DynamicBoxSpec dynamicBoxSpec{};
    float movingPlatformDirection = 1.0f;
    float carriedGroundVelocityX = 0.0f;
    bool typesRegistered = false;
    bool initialized = false;

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
    impl->dynamicBoxSpec = level.dynamicBox;

    JPH::RegisterDefaultAllocator();
    JPH::Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)

    if (JPH::Factory::sInstance == nullptr)
    {
        JPH::Factory::sInstance = new JPH::Factory();
    }

    JPH::RegisterTypes();
    impl->typesRegistered = true;

    impl->tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(kTempAllocatorBytes);
    impl->jobSystem = std::make_unique<JPH::JobSystemSingleThreaded>(kMaxPhysicsJobs);
    impl->physicsSystem = std::make_unique<JPH::PhysicsSystem>();
    impl->physicsSystem->Init(
        kMaxBodies,
        kNumBodyMutexes,
        kMaxBodyPairs,
        kMaxContactConstraints,
        impl->broadPhaseLayerInterface,
        impl->objectVsBroadPhaseLayerFilter,
        impl->objectLayerPairFilter);

    if (!impl->AddGreyboxStaticBodies(level))
    {
        Shutdown();
        return false;
    }

    JPH::BodyInterface& bodyInterface = impl->physicsSystem->GetBodyInterface();
    JPH::BodyCreationSettings boxSettings(
        new JPH::BoxShape(ToHalfExtent(impl->dynamicBoxSpec.size)),
        ToRVec3(impl->dynamicBoxSpec.center),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        ObjectLayers::Moving);
    boxSettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    boxSettings.mMassPropertiesOverride.mMass = impl->dynamicBoxSpec.mass;
    impl->dynamicBodyId = bodyInterface.CreateAndAddBody(boxSettings, JPH::EActivation::Activate);
    if (impl->dynamicBodyId.IsInvalid())
    {
        ReportError("failed to create dynamic test box.");
        Shutdown();
        return false;
    }

    {
        JPH::BodyLockRead lock(impl->physicsSystem->GetBodyLockInterface(), impl->dynamicBodyId);
        if (!lock.SucceededAndIsInBroadPhase())
        {
            ReportError("failed to inspect dynamic test box mass.");
            Shutdown();
            return false;
        }

        const float inverseMass = lock.GetBody().GetMotionProperties()->GetInverseMass();
        const float mass = inverseMass > 0.0f ? (1.0f / inverseMass) : 0.0f;
        if (std::fabs(mass - impl->dynamicBoxSpec.mass) > 0.01f)
        {
            std::fprintf(
                stderr,
                "PhysicsWorld: dynamic test box mass is %.3f kg, expected %.3f kg.\n",
                mass,
                impl->dynamicBoxSpec.mass);
            Shutdown();
            return false;
        }
    }

    if (!impl->CreateMovingPlatform())
    {
        Shutdown();
        return false;
    }

    impl->physicsSystem->OptimizeBroadPhase();
    impl->initialized = true;
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

    impl->character->RefreshContacts(
        impl->physicsSystem->GetDefaultBroadPhaseLayerFilter(ObjectLayers::Moving),
        impl->physicsSystem->GetDefaultLayerFilter(ObjectLayers::Moving),
        {},
        {},
        *impl->tempAllocator);
    impl->EnforceFixedZ();
    return true;
}

void PhysicsWorld::ResetCharacter(const core::Vec3& visualCenter, const core::Vec3& velocity)
{
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

    impl->character->RefreshContacts(
        impl->physicsSystem->GetDefaultBroadPhaseLayerFilter(ObjectLayers::Moving),
        impl->physicsSystem->GetDefaultLayerFilter(ObjectLayers::Moving),
        {},
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

void PhysicsWorld::ResetDynamicTestBox()
{
    if (!impl->initialized || impl->dynamicBodyId.IsInvalid())
    {
        return;
    }

    JPH::BodyInterface& bodyInterface = impl->physicsSystem->GetBodyInterface();
    bodyInterface.SetPositionAndRotation(
        impl->dynamicBodyId,
        ToRVec3(impl->dynamicBoxSpec.center),
        JPH::Quat::sIdentity(),
        JPH::EActivation::Activate);
    bodyInterface.SetLinearVelocity(impl->dynamicBodyId, JPH::Vec3::sZero());
    bodyInterface.SetAngularVelocity(impl->dynamicBodyId, JPH::Vec3::sZero());
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
    impl->character->Update(
        stepSeconds,
        JPH::Vec3(0.0f, kCharacterGravityY, 0.0f),
        impl->physicsSystem->GetDefaultBroadPhaseLayerFilter(ObjectLayers::Moving),
        impl->physicsSystem->GetDefaultLayerFilter(ObjectLayers::Moving),
        {},
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

    impl->physicsSystem->Update(
        stepSeconds,
        1,
        impl->tempAllocator.get(),
        impl->jobSystem.get());

    impl->EnforceFixedZ();
}

void PhysicsWorld::Shutdown()
{
    impl->character = nullptr;
    impl->carriedGroundVelocityX = 0.0f;
    impl->movingPlatformDirection = 1.0f;

    if (impl->physicsSystem)
    {
        JPH::BodyInterface& bodyInterface = impl->physicsSystem->GetBodyInterface();
        if (!impl->dynamicBodyId.IsInvalid())
        {
            bodyInterface.RemoveBody(impl->dynamicBodyId);
            bodyInterface.DestroyBody(impl->dynamicBodyId);
            impl->dynamicBodyId = {};
        }
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

    if (impl->typesRegistered)
    {
        JPH::UnregisterTypes();
        impl->typesRegistered = false;
    }

    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

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

bool PhysicsWorld::IsDynamicTestBodyValid() const
{
    return impl->initialized && !impl->dynamicBodyId.IsInvalid();
}

DynamicTestBox PhysicsWorld::GetDynamicTestBox() const
{
    DynamicTestBox box;
    box.size = impl->dynamicBoxSpec.size;
    box.valid = IsDynamicTestBodyValid();
    if (!box.valid)
    {
        return box;
    }

    const JPH::BodyInterface& bodyInterface = impl->physicsSystem->GetBodyInterface();
    box.position = ToVec3(bodyInterface.GetPosition(impl->dynamicBodyId));
    const JPH::Vec3 linearVelocity = bodyInterface.GetLinearVelocity(impl->dynamicBodyId);
    box.linearVelocity = {linearVelocity.GetX(), linearVelocity.GetY(), linearVelocity.GetZ()};
    box.active = bodyInterface.IsActive(impl->dynamicBodyId);
    return box;
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
