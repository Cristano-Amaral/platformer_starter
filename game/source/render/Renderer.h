#pragma once

#include "core/Vec3.h"
#include "gameplay/Inventory.h"
#include "render/CameraView.h"
#include "world/CollectibleWorld.h"
#include "world/LevelDefinition.h"
#include "world/RespawnWorld.h"

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
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
        int kind = 0; // 0 Platform, 1 Checkpoint, 2 Hazard, 3 Collectible, 4 Dynamic Box, 5 Static Prop, 6 Pressure Plate, 7 Door, 8 Item Pickup
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
    // M50 editor-only Static Prop real-model preview. Not authored, not Gameplay.
    bool drawStaticPropPlacementPreview = false;
    world::StaticPropSpec staticPropPlacementPreview{};
    core::Vec3 staticPropPlacementBoundsCenter{};
    core::Vec3 staticPropPlacementBoundsSize{};
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
    std::vector<int> pendingDeleteDynamicBoxIndices;
    std::vector<core::Vec3> pendingDeleteDynamicBoxCenters;
    std::vector<core::Vec3> pendingDeleteDynamicBoxSizes;
    std::vector<int> pendingDeletePressurePlateIndices;
    std::vector<core::Vec3> pendingDeletePressurePlateCenters;
    std::vector<core::Vec3> pendingDeletePressurePlateSizes;
    std::vector<int> pendingDeleteDoorIndices;
    std::vector<core::Vec3> pendingDeleteDoorCenters;
    std::vector<core::Vec3> pendingDeleteDoorSizes;
    std::vector<int> pendingDeleteItemPickupIndices;
    std::vector<core::Vec3> pendingDeleteItemPickupCenters;
    std::vector<core::Vec3> pendingDeleteItemPickupSizes;
    std::vector<int> pendingDeleteStaticPropIndices;
    std::vector<core::Vec3> pendingDeleteStaticPropCenters;
    std::vector<core::Vec3> pendingDeleteStaticPropSizes;
    // Translation gizmo at the working-copy origin. hovered/active: 0 none,
    // 1 X, 2 Y, 3 Z (matches editor::EditorAxis).
    bool drawTranslationGizmo = false;
    core::Vec3 gizmoOrigin{};
    float gizmoAxisLength = 1.0f;
    int gizmoHoveredAxis = 0;
    int gizmoActiveAxis = 0;
    // Resize cubes at +/- axisLength. Sign is +1 / -1; ignored for translate.
    bool drawResizeGizmo = false;
    // Static Prop Scale uses the same cube handles; edits visual scale, not size.
    bool drawScaleGizmo = false;
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

// M44: cooker probes remain in cook/stage inventory but are not drawn or
// loaded into the canonical Level 01 scene.
inline constexpr bool kCanonicalSceneInstantiatesCookerProbes = false;

enum class DynamicBoxDrawFeedback
{
    None,
    Targeted,
    Carried,
};

struct DynamicBoxDrawState
{
    core::Vec3 center{};
    core::Vec3 size{};
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    float rotationZ = 0.0f;
    float rotationW = 1.0f;
    DynamicBoxDrawFeedback feedback = DynamicBoxDrawFeedback::None;
};

struct PressurePlateDrawState
{
    core::Vec3 center{};
    core::Vec3 size{};
    bool active = false;
    bool visibleInGameplay = true;
    bool revealInEditor = false;
};

struct DoorDrawState
{
    core::Vec3 center{};
    core::Vec3 size{};
};

// Player-facing Inventory panel (Milestone 56). Game HUD, including Release.
// Reads production Inventory entries; Renderer does not own contents.
struct InventoryPanelView
{
    bool visible = false;
    std::span<const gameplay::InventoryEntry> entries{};
    std::string_view selectedItemId{};
};

class StaticModelSceneStore;

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
    void SyncStaticPropModels(
        const world::LevelDefinition& level,
        std::string_view extraIdentity = {});
    StaticModelSceneStore* StaticPropModels();
    const StaticModelSceneStore* StaticPropModels() const;

    // Cooker inventory ids. M44 does not load or draw these in the scene.
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
        const std::vector<DynamicBoxDrawState>& dynamicBoxes,
        const std::vector<PressurePlateDrawState>& pressurePlates,
        const std::vector<DoorDrawState>& doors,
        core::Vec3 movingPlatformPosition,
        core::Vec3 movingPlatformSize,
        const std::vector<world::CheckpointVisualState>& checkpointVisuals,
        bool levelCompleted,
        const std::vector<std::uint8_t>& collectibleCollected,
        int collectedCount,
        const std::vector<std::uint8_t>& itemPickupCollected,
        int itemPickupTargetIndex,
        int lockedDoorTargetIndex,
        const char* lockedDoorPrompt,
        double elapsedSeconds,
        bool hasBestTime,
        double bestSeconds,
        InventoryPanelView inventoryPanel = {},
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
    std::unique_ptr<StaticModelSceneStore> staticPropModels;
};
}
