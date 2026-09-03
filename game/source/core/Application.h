#pragma once

#include "gameplay/CollectibleRunState.h"
#include "gameplay/LevelCompletionState.h"
#include "gameplay/PlatformerCamera.h"
#include "gameplay/Player.h"
#include "gameplay/RespawnState.h"
#include "gameplay/RunTimerState.h"
#include "gameplay/SessionBestTimeState.h"
#include "physics/PhysicsWorld.h"
#include "persistence/BestTimeSave.h"
#include "platform/Window.h"
#include "render/Renderer.h"
#include "world/LevelDefinition.h"
#include "world/LevelFile.h"
#include "world/RespawnWorld.h"

#if defined(PLATFORMER_ENABLE_DEBUG_UI)
#include "editor/LevelEditor.h"
#include "ui/debug/DebugUi.h"
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
    void RestartRun();
#if defined(PLATFORMER_ENABLE_DEBUG_UI)
    void SetLevelEditorActive(bool active);
    // Returns false when a physics rebuild left the process unusable. The
    // frame loop then exits and Run() reports failure.
    bool HandleLevelEditorRequest(editor::LevelEditorRequest request);
    bool ApplyLevelEditorPreview();
    void SaveLevelEditorSource();
#endif

    world::LevelDefinition levelDefinition{};
    platform::Window window;
    render::Renderer renderer;
    gameplay::Player player{{}, world::kPlayerVisualSize};
    gameplay::PlatformerCamera camera;
    gameplay::RespawnState respawnState;
    gameplay::LevelCompletionState levelCompletionState;
    gameplay::CollectibleRunState collectibleRunState;
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
#endif
    bool initialized = false;
    // Set only when an editor physics rebuild fails. Normal gameplay never
    // touches it, so the M31 frame loop condition is unchanged in practice.
    bool fatalError = false;
};
}
