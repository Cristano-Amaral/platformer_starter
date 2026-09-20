#include "render/LightingEnvironment.h"
#include "render/LocalLights.h"
#include "world/LocalLight.h"
#include "world/LocalLightActivation.h"
#include "world/PressurePlate.h"

#include <cmath>
#include <cstdio>
#include <cstdint>
#include <vector>

namespace
{
int gFailures = 0;

void Expect(bool condition, const char* what)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++gFailures;
    }
}

bool NearlyEqual(float a, float b, float epsilon = 0.001f)
{
    return std::fabs(a - b) <= epsilon;
}

bool VecNear(core::Vec3 a, core::Vec3 b, float epsilon = 0.001f)
{
    return NearlyEqual(a.x, b.x, epsilon) && NearlyEqual(a.y, b.y, epsilon)
        && NearlyEqual(a.z, b.z, epsilon);
}

bool FiniteVec(core::Vec3 value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}
}

int main()
{
    Expect(render::kMaxActiveLocalLights == 8, "active local-light cap is 8");

    const world::PointLightSpec defaultPoint = world::MakeDefaultPointLight({0.0f, 2.0f, 0.0f});
    Expect(defaultPoint.enabled, "default point is enabled");
    Expect(NearlyEqual(defaultPoint.intensity, 1.5f), "default point intensity");
    Expect(NearlyEqual(defaultPoint.range, 8.0f), "default point range");
    Expect(world::PointLightIsValid(defaultPoint), "default point is valid");

    const world::SpotLightSpec defaultSpot = world::MakeDefaultSpotLight({1.0f, 4.0f, 0.0f});
    Expect(defaultSpot.enabled, "default spot is enabled");
    Expect(VecNear(defaultSpot.direction, {0.0f, -1.0f, 0.0f}), "default spot aims down");
    Expect(NearlyEqual(defaultSpot.innerConeDegrees, 20.0f), "default inner cone");
    Expect(NearlyEqual(defaultSpot.outerConeDegrees, 35.0f), "default outer cone");
    Expect(world::SpotLightIsValid(defaultSpot), "default spot is valid");

    Expect(NearlyEqual(render::LocalLightDistanceAttenuation(0.0f, 8.0f), 1.0f), "at origin att=1");
    Expect(NearlyEqual(render::LocalLightDistanceAttenuation(4.0f, 8.0f), 0.25f), "(1-d/r)^2 at half range");
    Expect(NearlyEqual(render::LocalLightDistanceAttenuation(8.0f, 8.0f), 0.0f), "at range att=0");
    Expect(NearlyEqual(render::LocalLightDistanceAttenuation(9.0f, 8.0f), 0.0f), "beyond range att=0");
    Expect(std::isfinite(render::LocalLightDistanceAttenuation(3.0f, 8.0f)), "distance att is finite");

    const core::Vec3 down{0.0f, -1.0f, 0.0f};
    const float innerCos = render::CosineFromConeDegrees(20.0f);
    const float outerCos = render::CosineFromConeDegrees(35.0f);
    Expect(
        NearlyEqual(
            render::LocalLightSpotAngularAttenuation(down, down, innerCos, outerCos), 1.0f),
        "on-axis spot is full");
    Expect(
        render::LocalLightSpotAngularAttenuation({1.0f, 0.0f, 0.0f}, down, innerCos, outerCos)
            == 0.0f,
        "outside outer cone is zero");
    const float midDegrees = 27.5f;
    const float midRadians = midDegrees * (3.14159265f / 180.0f);
    const core::Vec3 midDir{std::sin(midRadians), -std::cos(midRadians), 0.0f};
    const float midAtt =
        render::LocalLightSpotAngularAttenuation(midDir, down, innerCos, outerCos);
    Expect(midAtt > 0.0f && midAtt < 1.0f, "between inner and outer is a transition");
    Expect(std::isfinite(midAtt), "angular att is finite");

    world::PointLightSpec disabledPoint = defaultPoint;
    disabledPoint.enabled = false;
    std::vector<world::PointLightSpec> points{disabledPoint};
    std::vector<world::SpotLightSpec> spots{};
    render::PackedLocalLights packed = render::PackAuthoredLocalLights(points, spots);
    Expect(packed.count == 0, "disabled point does not occupy a slot");

    world::PointLightSpec enabledPoint = defaultPoint;
    packed = render::PackAuthoredLocalLights({enabledPoint}, {});
    Expect(packed.count == 1 && packed.lights[0].type == render::kLocalLightTypePoint,
        "enabled point packs");

    const core::Vec3 albedo{1.0f, 1.0f, 1.0f};
    const core::Vec3 normal{0.0f, 1.0f, 0.0f};
    const core::Vec3 litPoint = render::LocalLightDiffuseContribution(
        albedo, normal, {0.0f, 0.0f, 0.0f}, packed.lights[0]);
    Expect(FiniteVec(litPoint) && litPoint.y > 0.0f, "enabled point contributes finite diffuse");

    packed.lights[0].intensity = 0.0f;
    const core::Vec3 zeroIntensity = render::LocalLightDiffuseContribution(
        albedo, normal, {0.0f, 0.0f, 0.0f}, packed.lights[0]);
    Expect(VecNear(zeroIntensity, {}), "zero intensity contributes nothing");

    packed = render::PackAuthoredLocalLights({enabledPoint}, {});
    const core::Vec3 outside = render::LocalLightDiffuseContribution(
        albedo, normal, {0.0f, 20.0f, 0.0f}, packed.lights[0]);
    Expect(VecNear(outside, {}), "outside range contributes nothing");

    world::SpotLightSpec enabledSpot = defaultSpot;
    packed = render::PackAuthoredLocalLights({}, {enabledSpot});
    Expect(packed.count == 1 && packed.lights[0].type == render::kLocalLightTypeSpot,
        "enabled spot packs");
    Expect(NearlyEqual(std::sqrt(
                           packed.lights[0].direction.x * packed.lights[0].direction.x
                           + packed.lights[0].direction.y * packed.lights[0].direction.y
                           + packed.lights[0].direction.z * packed.lights[0].direction.z),
               1.0f),
        "packed spot direction is normalized");
    const core::Vec3 onAxis = render::LocalLightDiffuseContribution(
        albedo, {0.0f, 1.0f, 0.0f}, {1.0f, 2.0f, 0.0f}, packed.lights[0]);
    Expect(FiniteVec(onAxis) && onAxis.y > 0.0f, "on-axis spot contributes");
    const core::Vec3 offAxis = render::LocalLightDiffuseContribution(
        albedo, {0.0f, 1.0f, 0.0f}, {4.0f, 4.0f, 0.0f}, packed.lights[0]);
    Expect(VecNear(offAxis, {}), "inside range but outside outer cone contributes nothing");
    const float innerEdgeAtt = render::LocalLightSpotAngularAttenuation(
        down, down, innerCos, outerCos);
    Expect(NearlyEqual(innerEdgeAtt, 1.0f), "inner cone and on-axis stay fully lit");

    world::SpotLightSpec disabledSpot = defaultSpot;
    disabledSpot.enabled = false;
    packed = render::PackAuthoredLocalLights({}, {disabledSpot});
    Expect(packed.count == 0, "disabled spot does not occupy a slot");

    std::vector<world::PointLightSpec> manyPoints(10, defaultPoint);
    packed = render::PackAuthoredLocalLights(manyPoints, {});
    Expect(packed.count == render::kMaxActiveLocalLights, "overflow keeps the first 8 enabled points");
    Expect(packed.lights[7].type == render::kLocalLightTypePoint, "eighth packed light is a point");

    std::vector<world::PointLightSpec> sixPoints(6, defaultPoint);
    std::vector<world::SpotLightSpec> fourSpots(4, defaultSpot);
    packed = render::PackAuthoredLocalLights(sixPoints, fourSpots);
    Expect(packed.count == 8, "points then spots fill the cap");
    Expect(packed.lights[0].type == render::kLocalLightTypePoint, "ordering starts with points");
    Expect(packed.lights[5].type == render::kLocalLightTypePoint, "sixth is still a point");
    Expect(packed.lights[6].type == render::kLocalLightTypeSpot, "overflow continues with spots");
    Expect(packed.lights[7].type == render::kLocalLightTypeSpot, "eighth is the second spot");

    render::LightingEnvironment environment = render::MakeDefaultLightingEnvironment();
    render::ApplyAuthoredLocalLights(environment, manyPoints, fourSpots);
    Expect(environment.localLights.count == 8, "LightingEnvironment packing uses the same cap");
    Expect(
        environment.directional.authoredEnabled && environment.directional.shadowsEnabled,
        "local-light packing does not change Directional Light");

    {
        world::PointLightSpec linked = defaultPoint;
        world::PointLightSpec unlinked = defaultPoint;
        unlinked.position = {4.0f, 2.0f, 0.0f};
        std::vector<world::PointLightSpec> points{linked, unlinked};
        world::PressurePlateSpec plate{};
        plate.controlledLocalLights.push_back({world::LocalLightKind::Point, 0});
        const std::uint8_t inactive = 0;
        std::vector<std::uint8_t> pointEff;
        std::vector<std::uint8_t> spotEff;
        world::FillEffectiveLocalLightEnabled(
            points, {}, {plate}, &inactive, 1, pointEff, spotEff);
        packed = render::PackAuthoredLocalLights(
            points, {}, pointEff.data(), pointEff.size(), spotEff.data(), spotEff.size());
        Expect(packed.count == 1 && packed.lights[0].position.x == 4.0f,
            "effectively OFF Point does not occupy a renderer slot");
        const std::uint8_t active = 1;
        world::FillEffectiveLocalLightEnabled(
            points, {}, {plate}, &active, 1, pointEff, spotEff);
        packed = render::PackAuthoredLocalLights(
            points, {}, pointEff.data(), pointEff.size(), spotEff.data(), spotEff.size());
        Expect(packed.count == 2, "effectively ON Point occupies a slot in authored order");
    }

    Expect(render::kMaxActiveLocalLights == 8, "M85.4 does not raise the local-light cap");

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d LocalLightsTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("LocalLightsTest passed\n");
    return 0;
}
