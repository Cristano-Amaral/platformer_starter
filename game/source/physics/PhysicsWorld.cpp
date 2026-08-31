#include "physics/PhysicsWorld.h"

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <cstdarg>
#include <cstdio>
#include <memory>

JPH_SUPPRESS_WARNINGS

namespace physics
{
namespace
{
constexpr float kMaxPhysicsDeltaSeconds = 1.0f / 60.0f;
constexpr unsigned int kMaxBodies = 16;
constexpr unsigned int kNumBodyMutexes = 0;
constexpr unsigned int kMaxBodyPairs = 16;
constexpr unsigned int kMaxContactConstraints = 16;
constexpr unsigned int kTempAllocatorBytes = 1 * 1024 * 1024;
constexpr unsigned int kMaxPhysicsJobs = 256;

// Intentionally duplicates greybox ground for this isolated Jolt experiment.
constexpr core::Vec3 kFloorCenter{0.0f, -0.25f, 0.0f};
constexpr core::Vec3 kFloorSize{24.0f, 0.5f, 8.0f};
constexpr core::Vec3 kDynamicBoxCenter{0.0f, 5.0f, 0.0f};
constexpr core::Vec3 kDynamicBoxSize{1.0f, 1.0f, 1.0f};

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

void ReportError(const char* message)
{
    std::fprintf(stderr, "PhysicsWorld: %s\n", message);
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
    JPH::BodyID floorBodyId;
    JPH::BodyID dynamicBodyId;
    bool typesRegistered = false;
    bool initialized = false;
};

PhysicsWorld::PhysicsWorld()
    : impl(std::make_unique<Impl>())
{
}

PhysicsWorld::~PhysicsWorld()
{
    Shutdown();
}

bool PhysicsWorld::Initialize()
{
    if (impl->initialized)
    {
        return true;
    }

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

    JPH::BodyInterface& bodyInterface = impl->physicsSystem->GetBodyInterface();

    JPH::BoxShapeSettings floorShapeSettings(ToHalfExtent(kFloorSize));
    floorShapeSettings.SetEmbedded();
    const JPH::ShapeSettings::ShapeResult floorShapeResult = floorShapeSettings.Create();
    if (floorShapeResult.HasError())
    {
        ReportError(floorShapeResult.GetError().c_str());
        Shutdown();
        return false;
    }

    JPH::BodyCreationSettings floorSettings(
        floorShapeResult.Get(),
        ToRVec3(kFloorCenter),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Static,
        ObjectLayers::NonMoving);
    JPH::Body* floorBody = bodyInterface.CreateBody(floorSettings);
    if (floorBody == nullptr)
    {
        ReportError("failed to create static floor body.");
        Shutdown();
        return false;
    }

    impl->floorBodyId = floorBody->GetID();
    bodyInterface.AddBody(impl->floorBodyId, JPH::EActivation::DontActivate);

    JPH::BodyCreationSettings boxSettings(
        new JPH::BoxShape(ToHalfExtent(kDynamicBoxSize)),
        ToRVec3(kDynamicBoxCenter),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        ObjectLayers::Moving);
    impl->dynamicBodyId = bodyInterface.CreateAndAddBody(boxSettings, JPH::EActivation::Activate);
    if (impl->dynamicBodyId.IsInvalid())
    {
        ReportError("failed to create dynamic test box.");
        Shutdown();
        return false;
    }

    impl->physicsSystem->OptimizeBroadPhase();
    impl->initialized = true;
    return true;
}

void PhysicsWorld::Update(float deltaSeconds)
{
    if (!impl->initialized)
    {
        return;
    }

    float stepSeconds = deltaSeconds;
    if (stepSeconds <= 0.0f)
    {
        return;
    }
    if (stepSeconds > kMaxPhysicsDeltaSeconds)
    {
        stepSeconds = kMaxPhysicsDeltaSeconds;
    }

    impl->physicsSystem->Update(
        stepSeconds,
        1,
        impl->tempAllocator.get(),
        impl->jobSystem.get());
}

void PhysicsWorld::Shutdown()
{
    if (impl->physicsSystem)
    {
        JPH::BodyInterface& bodyInterface = impl->physicsSystem->GetBodyInterface();
        if (!impl->dynamicBodyId.IsInvalid())
        {
            bodyInterface.RemoveBody(impl->dynamicBodyId);
            bodyInterface.DestroyBody(impl->dynamicBodyId);
            impl->dynamicBodyId = {};
        }
        if (!impl->floorBodyId.IsInvalid())
        {
            bodyInterface.RemoveBody(impl->floorBodyId);
            bodyInterface.DestroyBody(impl->floorBodyId);
            impl->floorBodyId = {};
        }
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

DynamicTestBox PhysicsWorld::GetDynamicTestBox() const
{
    DynamicTestBox box;
    box.size = kDynamicBoxSize;
    if (!impl->initialized)
    {
        return box;
    }

    const JPH::BodyInterface& bodyInterface = impl->physicsSystem->GetBodyInterface();
    box.position = ToVec3(bodyInterface.GetPosition(impl->dynamicBodyId));
    box.active = bodyInterface.IsActive(impl->dynamicBodyId);
    return box;
}
}
