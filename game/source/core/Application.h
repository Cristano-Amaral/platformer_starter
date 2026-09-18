#pragma once

#include "gameplay/CollectibleRunState.h"
#include "gameplay/Inventory.h"
#include "gameplay/InventoryUi.h"
#include "gameplay/ItemPickupRuntime.h"
#include "gameplay/ItemPickupCollectionFeedback.h"
#include "gameplay/ItemPickupCollectionHud.h"
#include "gameplay/DoorLockRuntime.h"
#include "gameplay/GameFlowState.h"
#include "gameplay/GameplayAudio.h"
#include "gameplay/PlayerHealth.h"
#include "gameplay/PlayerDeath.h"
#include "gameplay/LevelCompletionState.h"
#include "gameplay/LevelTransition.h"
#include "gameplay/PlatformerCamera.h"
#include "gameplay/Player.h"
#include "gameplay/RespawnState.h"
#include "gameplay/RunTimerState.h"
#include "gameplay/SessionBestTimeState.h"
#include "physics/PhysicsWorld.h"
#include "persistence/BestTimeSave.h"
#include "platform/Window.h"
#include "platform/GameplayAudio.h"
#include "render/Renderer.h"
#include "world/LevelDefinition.h"
#include "world/LevelFile.h"
#include "world/RespawnWorld.h"

#if defined(PLATFORMER_ENABLE_DEBUG_UI)
#include "editor/CookStageReloadWorkflow.h"
#include "editor/LevelEditor.h"
#include "ui/debug/DebugUi.h"
#endif
#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
#include "render/StaticModelThumbnail.h"
#include "render/StaticModelPreview.h"
#endif

#include <string>

namespace core
{
class Application
{
public:
    int Run();

private:
    void Initialize();
    void Shutdown();
    void PerformRespawn(gameplay::RespawnReason reason);
    void PerformDeathRespawn();
    void RestartRun();
    void TryFinishPendingLevelTransition();
    void TryFinishPendingFreshRun();
    void ResetGameplayAfterPlayAgain();
    void ReturnToMainMenuFromResults();
#if defined(PLATFORMER_ENABLE_DEBUG_UI)
    void SetLevelEditorActive(bool active);
    // Physics rebuilds use TryRebuild so a failure leaves the active world
    // intact and this returns true.
    bool HandleLevelEditorRequest(editor::LevelEditorRequest request);
    bool ApplyLevelEditorPreview();
    bool ReloadRuntimeLevelFromStaged();
    bool StartCookStageAndReload();
    void FinishCookStageAndReloadIfReady();
    void ImportStaticGlbAsset();
    void DeleteContentBrowserAsset();
    bool OpenAuthoredLevelFromEditor();
    void CreateAuthoredLevelFromEditor();
    void ResetGameplayAfterCommittedLevel();
    void SaveLevelEditorSource();
#endif
    void ResetGameplayAfterLevelTransition();
    void EmitGameplaySfx(gameplay::GameplaySfxEmit emit);
    void ReanchorPlayerMovementSfx();
    void SynchronizePressurePlateSfx();
    void ObservePressurePlateSfx(bool emitEdges);

    world::LevelDefinition levelDefinition{};
    platform::Window window;
    render::Renderer renderer;
    platform::GameplayAudio gameplayAudio;
    gameplay::GameplaySfxRequestState gameplaySfxRequests{};
    gameplay::PlayerMovementSfxState playerMovementSfx{};
    gameplay::PressurePlateSfxState pressurePlateSfx{};
    gameplay::Player player{{}, world::kPlayerVisualSize};
    gameplay::PlatformerCamera camera;
    gameplay::RespawnState respawnState;
    gameplay::LevelCompletionState levelCompletionState;
    gameplay::LevelTransitionSchedule levelTransition;
    gameplay::RunCompleteState runCompleteState;
    gameplay::TopLevelFlow topLevelFlow = gameplay::TopLevelFlow::MainMenu;
    gameplay::MainMenuState mainMenuState{};
    gameplay::PauseMenuState pauseMenuState{};
    gameplay::PlayerHealthState playerHealth{};
    gameplay::HazardContactState hazardContact{};
    gameplay::DamageVignetteState damageVignette{};
    gameplay::PlayerDeathState playerDeath{};
    std::string currentRuntimeLevelId;
    gameplay::CollectibleRunState collectibleRunState;
    gameplay::Inventory inventory{};
    gameplay::InventoryUiState inventoryUi{};
    gameplay::ItemPickupRunState itemPickupRunState{};
    gameplay::ItemPickupCollectionFeedbackState itemPickupCollectionFeedback{};
    gameplay::ItemPickupCollectionHudState itemPickupCollectionHud{};
    gameplay::DoorLockRunState doorLockRunState{};
    gameplay::RunTimerState runTimerState;
    gameplay::SessionBestTimeState sessionBestTimeState;
    persistence::LoadBestTimeStatus bestTimeLoadStatus = persistence::LoadBestTimeStatus::Missing;
    persistence::SaveBestTimeStatus bestTimeSaveStatus = persistence::SaveBestTimeStatus::NotAttempted;
    std::string bestTimeSavePathDisplay;
    std::string runtimeLevelPathDisplay;
    world::LoadLevelFileStatus levelLoadStatus = world::LoadLevelFileStatus::Error;
    int levelFormatVersion = 0;
    physics::PhysicsWorld physicsWorld;
#if defined(PLATFORMER_ENABLE_DEBUG_UI)
    ui::DebugUi debugUi;
    editor::LevelEditorState levelEditorState;
    editor::EditorToolRunner editorToolRunner;
    editor::CookStageReloadWorkflow cookStageReload;
#endif
#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
    render::StaticModelThumbnailStore thumbnailStore;
    render::StaticModelPreviewRenderer modelPreview;
#endif
    bool initialized = false;
    // Set only when an editor physics rebuild fails. Normal gameplay never
    // touches it, so the M31 frame loop condition is unchanged in practice.
    bool fatalError = false;
};
}
