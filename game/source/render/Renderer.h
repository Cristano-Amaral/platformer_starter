#pragma once

#include "core/Vec3.h"
#include "gameplay/Inventory.h"
#include "gameplay/ItemPickupCollectionFeedback.h"
#include "gameplay/ItemPickupCollectionHud.h"
#include "gameplay/PlayerPresentation.h"
#include "render/CameraView.h"
#include "world/CollectibleWorld.h"
#include "world/LevelDefinition.h"
#include "world/RespawnWorld.h"

#include <cstdint>
#include <cstddef>
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
    // Editor-only: draw the authored Level Goal AABB as a translucent volume.
    // Gameplay/Release leave this false and draw the marker only.
    bool drawLevelGoalAuthoredVolume = false;
    core::Vec3 spawnCenter{};
    core::Vec3 spawnSize{};
    bool drawHighlight = false;
    core::Vec3 highlightCenter{};
    core::Vec3 highlightSize{};
    float highlightRotationZDegrees = 0.0f;
    // M78 secondary selection wires. Primary stays drawHighlight.
    struct SecondaryHighlightOverlay
    {
        core::Vec3 center{};
        core::Vec3 size{};
        float rotationZDegrees = 0.0f;
    };
    std::vector<SecondaryHighlightOverlay> secondaryHighlights;
    // M58.2 editor-only selected-model ghost. workingCopy visual transform.
    bool drawSelectedModelGhost = false;
    world::StaticPropSpec selectedModelGhost{};
    bool drawSelectedModelBounds = false;
    core::Vec3 selectedModelBoundsCorners[8]{};
    // Editor-only Checkpoint respawn marker. Distinct from the trigger AABB.
    bool drawCheckpointRespawnMarker = false;
    core::Vec3 checkpointRespawnMarker{};
    core::Vec3 checkpointTriggerCenter{};
    bool drawCheckpointRespawnConnector = false;
    // Persistent pending Add/Modify ghosts. selected=true uses stronger cyan.
    struct PendingAuthoringOverlayItem
    {
        int kind = 0; // 0 Platform, 1 Checkpoint, 2 Hazard, 3 Collectible, 4 Dynamic Box, 5 Static Prop, 6 Pressure Plate, 7 Door, 8 Item Pickup, 9 Level Goal
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
    std::vector<int> pendingDeleteLevelGoalIndices;
    std::vector<world::LevelGoalSpec> pendingDeleteLevelGoals;
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
    // World-axis rotation rings. Static Prop rotation or Item Pickup visualRotationDegrees.
    bool drawRotateGizmo = false;
    int gizmoHoveredSign = 1;
    int gizmoActiveSign = 1;
    // M77 editor-only XZ viewport grid. Visualization only: not pickable, not
    // authored, not a Jolt body, and never a Gameplay/Release world grid.
    int editorViewportGridLineCount = 0;
    struct EditorViewportGridDrawLine
    {
        core::Vec3 start{};
        core::Vec3 end{};
        int kind = 0; // 0 minor, 1 major, 2 axisX, 3 axisZ
    };
    EditorViewportGridDrawLine editorViewportGridLines[192]{};
};

// DrawWorld no longer calls raylib DrawGrid. Gameplay and Release overlays
// stay default-empty, so neither a legacy grid nor the M77 authoring grid
// is drawn outside the Development editor.
inline bool DrawWorldDrawsLegacyRaylibGrid()
{
    return false;
}

inline bool DrawWorldDrawsEditorViewportGrid(const DebugWorldOverlay& overlay)
{
    return overlay.editorViewportGridLineCount > 0;
}

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

// Compact Gameplay Level/objective HUD (Milestone 67). Presentation-only;
// Renderer does not derive, own, or persist objective state.
struct ObjectiveHudView
{
    bool visible = false;
    const char* levelLabel = nullptr;
    const char* objectiveLine = nullptr;
};

struct HealthHudView
{
    bool visible = false;
    const char* text = nullptr;
};

struct DamageVignetteView
{
    float opacity = 0.0f;
};

struct DeathHudView
{
    bool visible = false;
    const char* title = nullptr;
};

class StaticModelSceneStore;
class WorldLightingResources;

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
        std::string_view extraIdentity = {},
        const world::LevelDefinition* extraLevel = nullptr);
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

    bool IsPlayerModelLoaded() const;
    std::size_t PlayerModelLoadCount() const;

    void BeginFrame();
    void DrawWorld(
        const gameplay::Player& player,
        const gameplay::PlayerPresentationState& playerPresentation,
        const CameraView& cameraView,
        const world::LevelDefinition& level,
        const std::vector<DynamicBoxDrawState>& dynamicBoxes,
        const std::vector<PressurePlateDrawState>& pressurePlates,
        const std::vector<DoorDrawState>& doors,
        core::Vec3 movingPlatformPosition,
        core::Vec3 movingPlatformSize,
        const std::vector<world::CheckpointVisualState>& checkpointVisuals,
        bool levelCompleted,
        bool destinationContinueHint,
        const std::vector<std::uint8_t>& collectibleCollected,
        int collectedCount,
        const std::vector<std::uint8_t>& itemPickupCollected,
        int itemPickupTargetIndex,
        const gameplay::ItemPickupCollectionFeedbackState& itemPickupCollectionFeedback,
        const gameplay::ItemPickupCollectionHudState& itemPickupCollectionHud,
        int lockedDoorTargetIndex,
        const char* lockedDoorPrompt,
        double elapsedSeconds,
        bool hasBestTime,
        double bestSeconds,
        bool runComplete = false,
        double runCompleteFinalSeconds = 0.0,
        InventoryPanelView inventoryPanel = {},
        ObjectiveHudView objectiveHud = {},
        HealthHudView healthHud = {},
        const DebugWorldOverlay& overlay = {},
        WorldViewRect viewRect = {},
        bool drawGameplayHud = true,
        bool hideInteractionPrompts = false,
        DamageVignetteView damageVignette = {},
        DeathHudView deathHud = {});
    void DrawMainMenu(bool playSelected);
    void DrawPauseMenu(bool resumeSelected);
    void DrawOrientationWidget(const OrientationWidgetOverlay& overlay);
    void DrawEditorPlacementHud(
        bool visible,
        const char* category,
        bool fallback,
        float topInset);
    void EndFrame();

private:
    struct PlayerModelGpuState;
    std::unique_ptr<StaticModelSceneStore> staticPropModels;
    std::unique_ptr<PlayerModelGpuState> playerModelGpu;
    std::unique_ptr<WorldLightingResources> worldLighting;
};
}
