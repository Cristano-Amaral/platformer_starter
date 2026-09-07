#pragma once

#include "core/Vec3.h"
#include "render/CameraView.h"
#include "world/CollectibleWorld.h"
#include "world/LevelDefinition.h"
#include "world/RespawnWorld.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace gameplay
{
class Player;
}

namespace render
{
// Debug/Development overlay drawn in the same 3D pass as the world. Renderer
// does not own selection or editor camera; Application fills this each frame.
struct DebugWorldOverlay
{
    bool drawSpawnMarker = false;
    core::Vec3 spawnCenter{};
    core::Vec3 spawnSize{};
    bool drawHighlight = false;
    core::Vec3 highlightCenter{};
    core::Vec3 highlightSize{};
    float highlightRotationZDegrees = 0.0f;
    // Editor-only Checkpoint respawn marker. Distinct from the trigger AABB.
    bool drawCheckpointRespawnMarker = false;
    core::Vec3 checkpointRespawnMarker{};
    core::Vec3 checkpointTriggerCenter{};
    bool drawCheckpointRespawnConnector = false;
    // Persistent pending Add/Modify ghosts. selected=true uses stronger cyan.
    struct PendingAuthoringOverlayItem
    {
        int kind = 0; // 0 Platform, 1 Checkpoint, 2 Hazard, 3 Collectible
        bool selected = false;
        core::Vec3 boundsCenter{};
        core::Vec3 boundsSize{};
        bool drawObjectVisual = false;
        world::CheckpointSpec checkpoint{};
        world::HazardSpec hazard{};
        world::CollectibleSpec collectible{};
    };
    std::vector<PendingAuthoringOverlayItem> pendingAuthoring;
    bool drawPlacementCandidate = false;
    bool placementCandidateFallback = false;
    int placementCandidateKind = 0;
    core::Vec3 placementCandidateCenter{};
    core::Vec3 placementCandidateSize{};
    world::CheckpointSpec placementCandidateCheckpoint{};
    world::HazardSpec placementCandidateHazard{};
    world::CollectibleSpec placementCandidateCollectible{};
    // Editor-only placeholders for collected authored Collectibles (active).
    std::vector<core::Vec3> collectedAuthoredCollectibleCenters;
    // Development-only pending-delete markers from active objects that
    // workingCopy has already removed. Indices skip the opaque DrawWorld
    // path so the overlay can draw a faded copy with world depth. Not
    // pickable as working selections.
    std::vector<int> pendingDeletePlatformIndices;
    std::vector<core::Vec3> pendingDeletePlatformCenters;
    std::vector<core::Vec3> pendingDeletePlatformSizes;
    std::vector<int> pendingDeleteCheckpointIndices;
    std::vector<world::CheckpointSpec> pendingDeleteCheckpoints;
    std::vector<int> pendingDeleteHazardIndices;
    std::vector<world::HazardSpec> pendingDeleteHazards;
    std::vector<int> pendingDeleteCollectibleIndices;
    std::vector<core::Vec3> pendingDeleteCollectibleCenters;
    // Translation gizmo at the working-copy origin. hovered/active: 0 none,
    // 1 X, 2 Y, 3 Z (matches editor::EditorAxis).
    bool drawTranslationGizmo = false;
    core::Vec3 gizmoOrigin{};
    float gizmoAxisLength = 1.0f;
    int gizmoHoveredAxis = 0;
    int gizmoActiveAxis = 0;
    // Resize cubes at +/- axisLength. Sign is +1 / -1; ignored for translate.
    bool drawResizeGizmo = false;
    int gizmoHoveredSign = 1;
    int gizmoActiveSign = 1;
};

// Screen-space orientation triad. Drawn after EndMode3D, before ImGui.
struct OrientationWidgetOverlay
{
    bool visible = false;
    float originX = 872.0f;
    float originY = 100.0f;
    float radius = 36.0f;
    core::Vec3 x{};
    core::Vec3 y{};
    core::Vec3 z{};
};

// Optional 3D sub-rectangle in window pixels (top-left origin). Width/height
// <= 0 draws into the full framebuffer (gameplay / F2 off).
struct WorldViewRect
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

class Renderer
{
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    void LoadRuntimeAssets();
    void UnloadRuntimeAssets();

    bool IsTestTextureLoaded() const;
    bool IsTestTextureFallbackActive() const;
    const char* TestTextureLogicalId() const;
    const char* TestTextureRuntimeRelativePath() const;

    bool IsTestModelLoaded() const;
    bool IsTestModelFallbackActive() const;
    const char* TestModelLogicalId() const;

    bool IsAuthoredModelLoaded() const;
    bool IsAuthoredModelFallbackActive() const;
    const char* AuthoredModelLogicalId() const;

    bool IsTexturedModelLoaded() const;
    bool IsTexturedModelFallbackActive() const;
    const char* TexturedModelLogicalId() const;
    int TexturedModelMaterialCount() const;
    bool TexturedModelHasAlbedoTexture() const;

    void BeginFrame();
    void DrawWorld(
        const gameplay::Player& player,
        const CameraView& cameraView,
        const world::LevelDefinition& level,
        core::Vec3 physicsTestBoxPosition,
        core::Vec3 physicsTestBoxSize,
        core::Vec3 movingPlatformPosition,
        core::Vec3 movingPlatformSize,
        const std::vector<world::CheckpointVisualState>& checkpointVisuals,
        bool levelCompleted,
        const std::vector<std::uint8_t>& collectibleCollected,
        int collectedCount,
        double elapsedSeconds,
        bool hasBestTime,
        double bestSeconds,
        const DebugWorldOverlay& overlay = {},
        WorldViewRect viewRect = {});
    void DrawOrientationWidget(const OrientationWidgetOverlay& overlay);
    void DrawEditorPlacementHud(
        bool visible,
        const char* category,
        bool fallback,
        float topInset);
    void EndFrame();

private:
    void LoadTestCheckerTexture();
    void LoadTestStaticModel();
    void LoadTestAuthoredModel();
    void LoadTestTexturedModel();

    struct GpuTexture;
    std::unique_ptr<GpuTexture> testTexture;
    bool testTextureLoaded = false;
    bool testTextureFallbackActive = false;

    struct GpuModel;
    std::unique_ptr<GpuModel> testModel;
    bool testModelLoaded = false;
    bool testModelFallbackActive = false;

    std::unique_ptr<GpuModel> authoredModel;
    bool authoredModelLoaded = false;
    bool authoredModelFallbackActive = false;

    std::unique_ptr<GpuModel> texturedModel;
    bool texturedModelLoaded = false;
    bool texturedModelFallbackActive = false;
    int texturedModelMaterialCount = 0;
    bool texturedModelHasAlbedoTexture = false;
};
}
