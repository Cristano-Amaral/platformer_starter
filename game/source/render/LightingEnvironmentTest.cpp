#include "render/LightingEnvironment.h"

#include <cmath>
#include <cstdio>
#include <limits>

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

float Length(core::Vec3 value)
{
    return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}
}

int main()
{
    const render::LightingEnvironment defaults = render::MakeDefaultLightingEnvironment();
    Expect(VecNear(defaults.ambient.color, {1.0f, 1.0f, 1.0f}), "ambient default color is white");
    Expect(defaults.ambient.intensity > 0.2f && defaults.ambient.intensity < 0.5f,
        "ambient default intensity keeps backfaces readable");
    Expect(NearlyEqual(Length(defaults.directional.rayDirection), 1.0f),
        "default directional ray is normalized");
    Expect(defaults.directional.rayDirection.y < 0.0f, "default rays travel downward");
    Expect(defaults.directional.intensity > 0.5f, "directional default intensity is usable");
    Expect(defaults.shadows.mapResolution == 2048, "default shadow map resolution");
    Expect(NearlyEqual(defaults.shadows.coverage, 72.0f), "default ortho coverage");

    const core::Vec3 toward = render::DirectionalLightTowardSurface(defaults.directional.rayDirection);
    Expect(NearlyEqual(toward.x, -defaults.directional.rayDirection.x), "toward-light is opposite X");
    Expect(NearlyEqual(toward.y, -defaults.directional.rayDirection.y), "toward-light is opposite Y");
    Expect(toward.y > 0.0f, "toward-light points up for the default sun");

    Expect(VecNear(render::NormalizeDirectionalLight({0.0f, 0.0f, 0.0f}),
             render::kFallbackDirectionalRayDirection),
        "zero direction falls back to world-down rays");
    Expect(VecNear(render::NormalizeDirectionalLight(
                 {std::numeric_limits<float>::quiet_NaN(), 1.0f, 0.0f}),
             render::kFallbackDirectionalRayDirection),
        "non-finite direction falls back");
    const core::Vec3 scaled = render::NormalizeDirectionalLight({0.0f, -4.0f, 0.0f});
    Expect(VecNear(scaled, {0.0f, -1.0f, 0.0f}), "non-unit down direction normalizes");

    render::LightingEnvironment invalid{};
    invalid.ambient.color = {2.0f, -1.0f, 0.5f};
    invalid.ambient.intensity = 99.0f;
    invalid.directional.color = {std::numeric_limits<float>::quiet_NaN(), 0.2f, 0.2f};
    invalid.directional.intensity = -3.0f;
    invalid.directional.rayDirection = {};
    invalid.shadows.mapResolution = 8;
    invalid.shadows.bias = std::numeric_limits<float>::quiet_NaN();
    invalid.shadows.coverage = std::numeric_limits<float>::quiet_NaN();
    const render::LightingEnvironment clamped = render::ValidateLightingEnvironment(invalid);
    Expect(clamped.ambient.color.x <= 1.0f && clamped.ambient.color.y >= 0.0f,
        "ambient color is clamped");
    Expect(clamped.ambient.intensity <= render::kMaxAmbientIntensity, "ambient intensity clamped");
    Expect(clamped.directional.color.x == 1.0f, "invalid light color falls back");
    Expect(clamped.directional.intensity == 0.0f, "negative intensity clamps to 0");
    Expect(VecNear(clamped.directional.rayDirection, render::kFallbackDirectionalRayDirection),
        "validated zero ray uses fallback");
    Expect(clamped.shadows.mapResolution >= render::kMinShadowMapResolution,
        "tiny shadow map is raised");
    Expect(clamped.shadows.bias > 0.0f, "invalid bias is replaced");

    const render::DirectionalLightView view =
        render::BuildDirectionalLightView(defaults.directional, defaults.shadows);
    const core::Vec3 toTarget{
        view.target.x - view.eye.x, view.target.y - view.eye.y, view.target.z - view.eye.z};
    const float toTargetLen = Length(toTarget);
    Expect(toTargetLen > 1.0f, "light view sits a usable distance from focus");
    Expect(
        NearlyEqual(toTarget.x / toTargetLen, defaults.directional.rayDirection.x, 0.02f)
            && NearlyEqual(toTarget.y / toTargetLen, defaults.directional.rayDirection.y, 0.02f),
        "light camera looks along ray travel");
    Expect(view.up.y >= 0.0f, "light view up is not inverted");

    const render::ShadowProjection projection = render::BuildShadowProjection(defaults.shadows);
    Expect(NearlyEqual(projection.right, 36.0f), "ortho half-extent matches coverage/2");
    Expect(NearlyEqual(projection.left, -projection.right), "ortho is centered");
    Expect(projection.nearPlane < projection.farPlane, "shadow near is before far");
    Expect(NearlyEqual(render::NormalizedShadowBias(0.0025f), 0.0025f), "default bias is kept");
    Expect(render::NormalizedShadowBias(-1.0f) > 0.0f, "negative bias is raised");

    Expect(render::ShouldCastDirectionalShadow(render::ShadowParticipant::GreyboxWorld),
        "greybox casts");
    Expect(render::ShouldCastDirectionalShadow(render::ShadowParticipant::StaticProp),
        "static props cast");
    Expect(render::ShouldCastDirectionalShadow(render::ShadowParticipant::ItemPickup),
        "item pickups cast");
    Expect(render::ShouldCastDirectionalShadow(render::ShadowParticipant::Player), "player casts");
    Expect(render::ShouldCastDirectionalShadow(render::ShadowParticipant::DynamicBox),
        "dynamic boxes cast");
    Expect(render::ShouldCastDirectionalShadow(render::ShadowParticipant::Door), "doors cast");
    Expect(
        render::ShouldCastDirectionalShadow(render::ShadowParticipant::PressurePlateGameplay),
        "gameplay pressure plates cast");
    Expect(render::ShouldCastDirectionalShadow(render::ShadowParticipant::LevelGoalMarker),
        "goal marker casts");
    Expect(!render::ShouldCastDirectionalShadow(render::ShadowParticipant::EditorGrid),
        "editor grid does not cast");
    Expect(!render::ShouldCastDirectionalShadow(render::ShadowParticipant::EditorGizmo),
        "gizmos do not cast");
    Expect(!render::ShouldCastDirectionalShadow(render::ShadowParticipant::EditorGhost),
        "editor ghosts do not cast");
    Expect(!render::ShouldCastDirectionalShadow(render::ShadowParticipant::Thumbnail),
        "thumbnails do not cast");
    Expect(!render::ShouldCastDirectionalShadow(render::ShadowParticipant::Preview),
        "preview does not cast");
    Expect(!render::ShouldCastDirectionalShadow(render::ShadowParticipant::LevelGoalEditorVolume),
        "goal editor volume does not cast");
    Expect(
        !render::ShouldCastDirectionalShadow(render::ShadowParticipant::PressurePlateEditorGhost),
        "hidden plate ghosts do not cast");
    Expect(render::ShouldReceiveDirectionalShadow(render::ShadowParticipant::GreyboxWorld),
        "greybox receives");
    Expect(!render::ShouldReceiveDirectionalShadow(render::ShadowParticipant::EditorGrid),
        "grid does not receive");

    Expect(defaults.directional.authoredEnabled, "default directional is authored enabled");
    Expect(defaults.directional.shadowsEnabled, "default shadows are enabled");
    Expect(render::EffectiveDirectionalEnabled(defaults), "effective enabled follows authored");
    Expect(render::DirectionalShadowsAreActive(defaults), "shadows active by default");

    const world::LevelEnvironment authored = world::MakeDefaultLevelEnvironment();
    const render::LightingEnvironment fromLevel =
        render::MakeLightingEnvironmentFromAuthored(authored);
    Expect(VecNear(fromLevel.ambient.color, authored.ambientColor), "authored ambient color maps");
    Expect(NearlyEqual(fromLevel.ambient.intensity, authored.ambientIntensity),
        "authored ambient intensity maps");
    Expect(fromLevel.directional.authoredEnabled == authored.directionalEnabled,
        "authored enabled maps");
    Expect(fromLevel.directional.shadowsEnabled == authored.directionalShadowsEnabled,
        "authored shadows map");

    world::LevelEnvironment disabled = authored;
    disabled.directionalEnabled = false;
    disabled.directionalShadowsEnabled = true;
    const render::LightingEnvironment disabledLight =
        render::MakeLightingEnvironmentFromAuthored(disabled);
    Expect(!render::EffectiveDirectionalEnabled(disabledLight),
        "authored disabled is effective disabled");
    Expect(!render::DirectionalShadowsAreActive(disabledLight),
        "disabled light also disables shadows");
    Expect(NearlyEqual(disabledLight.ambient.intensity, authored.ambientIntensity),
        "disabled directional keeps ambient");

    world::LevelEnvironment noShadows = authored;
    noShadows.directionalShadowsEnabled = false;
    const render::LightingEnvironment litNoShadow =
        render::MakeLightingEnvironmentFromAuthored(noShadows);
    Expect(render::EffectiveDirectionalEnabled(litNoShadow), "shadows-off keeps directional");
    Expect(!render::DirectionalShadowsAreActive(litNoShadow), "shadows-off disables shadow pass");

    const core::Vec3 rotated = render::RotateDirectionalRay(
        {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 90.0f);
    Expect(NearlyEqual(Length(rotated), 1.0f), "rotated ray stays normalized");
    Expect(NearlyEqual(rotated.y, 0.0f, 0.05f), "90 deg around X moves off -Y");

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d LightingEnvironmentTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("LightingEnvironmentTest passed\n");
    return 0;
}
