#pragma once

// Milestone 85: engine-owned world lighting configuration. Presentation only.
// Milestone 85.1 supplies Level-authored environment data into this boundary.
// Not a scene-light list, ECS component, or Lighting Editor panel.

#include "core/Vec3.h"
#include "world/LevelEnvironment.h"

namespace render
{
// Stored directional direction is the direction light *rays travel* through
// the world (from the sun toward surfaces). World-up is +Y. Shader NdotL uses
// the opposite vector (from the surface toward the light).
struct AmbientLight
{
    core::Vec3 color{1.0f, 1.0f, 1.0f};
    float intensity = 0.34f;
};

struct DirectionalLight
{
    core::Vec3 rayDirection{-0.42f, -1.0f, -0.38f};
    core::Vec3 color{1.0f, 0.96f, 0.88f};
    float intensity = 0.88f;
    // Authored enabled. Effective enabled currently equals this value.
    bool authoredEnabled = true;
    bool shadowsEnabled = true;
};

struct DirectionalShadowConfig
{
    int mapResolution = 2048;
    // Full orthographic height consumed by raylib CAMERA_ORTHOGRAPHIC fovy.
    float coverage = 72.0f;
    float nearPlane = 1.0f;
    float farPlane = 120.0f;
    float lightDistance = 48.0f;
    float bias = 0.0025f;
    core::Vec3 focus{0.0f, 4.0f, 0.0f};
};

struct LightingEnvironment
{
    AmbientLight ambient{};
    DirectionalLight directional{};
    DirectionalShadowConfig shadows{};
};

struct DirectionalLightView
{
    core::Vec3 eye{};
    core::Vec3 target{};
    core::Vec3 up{0.0f, 1.0f, 0.0f};
};

struct ShadowProjection
{
    float left = -36.0f;
    float right = 36.0f;
    float bottom = -36.0f;
    float top = 36.0f;
    float nearPlane = 1.0f;
    float farPlane = 120.0f;
};

enum class ShadowParticipant
{
    GreyboxWorld,
    MovingPlatform,
    Slope,
    DynamicBox,
    PressurePlateGameplay,
    PressurePlateEditorGhost,
    Door,
    StaticProp,
    ItemPickup,
    Player,
    Hazard,
    Collectible,
    CheckpointMarker,
    LevelGoalMarker,
    LevelGoalEditorVolume,
    EditorGrid,
    EditorGizmo,
    EditorGhost,
    EditorHighlight,
    OverlayWires,
    Thumbnail,
    Preview
};

inline constexpr core::Vec3 kFallbackDirectionalRayDirection{0.0f, -1.0f, 0.0f};
inline constexpr float kMinDirectionalLightLength = 1.0e-5f;
inline constexpr float kMaxLightIntensity = 4.0f;
inline constexpr float kMaxAmbientIntensity = 2.0f;
inline constexpr int kMinShadowMapResolution = 256;
inline constexpr int kMaxShadowMapResolution = 4096;

LightingEnvironment MakeDefaultLightingEnvironment();
LightingEnvironment ValidateLightingEnvironment(LightingEnvironment environment);
LightingEnvironment MakeLightingEnvironmentFromAuthored(const world::LevelEnvironment& authored);
bool EffectiveDirectionalEnabled(const LightingEnvironment& environment);
bool DirectionalShadowsAreActive(const LightingEnvironment& environment);
core::Vec3 NormalizeDirectionalLight(core::Vec3 rayDirection);
core::Vec3 DirectionalLightTowardSurface(core::Vec3 rayDirection);
core::Vec3 RotateDirectionalRay(core::Vec3 rayDirection, core::Vec3 axis, float degrees);
DirectionalLightView BuildDirectionalLightView(
    const DirectionalLight& light,
    const DirectionalShadowConfig& shadows);
ShadowProjection BuildShadowProjection(const DirectionalShadowConfig& shadows);
float NormalizedShadowBias(float bias);
int NormalizedShadowMapResolution(int resolution);
bool ShouldCastDirectionalShadow(ShadowParticipant participant);
bool ShouldReceiveDirectionalShadow(ShadowParticipant participant);
}
