cmake_minimum_required(VERSION 3.25)

project(Platformer3D VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Multi-config generators (Visual Studio, Xcode) use these directly.
if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Development;Release" CACHE STRING "" FORCE)
endif()

# Development must exist as a real config so FetchContent deps (raylib/glfw)
# compile with symbols + optimization instead of empty flags.
if(MSVC)
    if(NOT CMAKE_C_FLAGS_DEVELOPMENT)
        set(CMAKE_C_FLAGS_DEVELOPMENT "/O2 /Zi /DNDEBUG" CACHE STRING "C flags for Development" FORCE)
    endif()
    if(NOT CMAKE_CXX_FLAGS_DEVELOPMENT)
        set(CMAKE_CXX_FLAGS_DEVELOPMENT "/O2 /Zi" CACHE STRING "CXX flags for Development" FORCE)
    endif()
    if(NOT CMAKE_EXE_LINKER_FLAGS_DEVELOPMENT)
        set(CMAKE_EXE_LINKER_FLAGS_DEVELOPMENT "/DEBUG" CACHE STRING "Linker flags for Development" FORCE)
    endif()
else()
    if(NOT CMAKE_C_FLAGS_DEVELOPMENT)
        set(CMAKE_C_FLAGS_DEVELOPMENT "-O2 -g -DNDEBUG" CACHE STRING "C flags for Development" FORCE)
    endif()
    if(NOT CMAKE_CXX_FLAGS_DEVELOPMENT)
        set(CMAKE_CXX_FLAGS_DEVELOPMENT "-O2 -g" CACHE STRING "CXX flags for Development" FORCE)
    endif()
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Raylib.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/DebugUi.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Jolt.cmake")

add_executable(Platformer3D
    game/source/Main.cpp
    game/source/core/Application.cpp
    game/source/core/Application.h
    game/source/core/RunTimeFormat.h
    game/source/core/Vec3.h
    game/source/editor/AuthoringPaths.cpp
    game/source/editor/AuthoringPaths.h
    game/source/editor/AuthoredLevelCatalog.cpp
    game/source/editor/AuthoredLevelCatalog.h
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/assets/RuntimePng.h
    game/source/assets/RuntimePngResolve.h
    game/source/assets/RuntimePngImport.cpp
    game/source/assets/RuntimePngImport.h
    game/source/assets/RuntimePngDelete.cpp
    game/source/assets/RuntimePngDelete.h
    game/source/assets/SourceTextureCatalog.cpp
    game/source/assets/SourceTextureCatalog.h
    game/source/assets/ModelMaterialPresentation.cpp
    game/source/assets/ModelMaterialPresentation.h
    game/source/assets/StaticGlbImport.cpp
    game/source/assets/StaticGlbImport.h
    game/source/assets/StaticModelCatalog.cpp
    game/source/assets/StaticModelCatalog.h
    game/source/assets/StaticModelDelete.cpp
    game/source/assets/StaticModelDelete.h
    game/source/editor/AuthoredLifecycleCommands.cpp
    game/source/editor/AuthoredLifecycleCommands.h
    game/source/editor/AuthoredObjectLifecycle.cpp
    game/source/editor/AuthoredObjectLifecycle.h
    game/source/editor/AuthoringGroups.cpp
    game/source/editor/AuthoringGroups.h
    game/source/editor/CookStageReloadWorkflow.cpp
    game/source/editor/CookStageReloadWorkflow.h
    game/source/editor/EditorCamera.cpp
    game/source/editor/EditorCamera.h
    game/source/editor/ContentBrowser.cpp
    game/source/editor/ContentBrowser.h
    game/source/editor/ContentBrowserOrganization.cpp
    game/source/editor/ContentBrowserOrganization.h
    game/source/editor/ContentBrowserView.cpp
    game/source/editor/ContentBrowserView.h
    game/source/editor/TextureThumbnailLifecycle.h
    game/source/editor/TerrainTextureReference.h
    game/source/editor/StaticModelThumbnailCache.cpp
    game/source/editor/StaticModelThumbnailCache.h
    game/source/editor/StaticModelFraming.cpp
    game/source/editor/StaticModelFraming.h
    game/source/editor/EditorGizmo.cpp
    game/source/editor/EditorGizmo.h
    game/source/editor/EditorSnap.cpp
    game/source/editor/EditorSnap.h
    game/source/editor/EditorViewportGrid.cpp
    game/source/editor/EditorViewportGrid.h
    game/source/editor/EditorHierarchy.h
    game/source/editor/EditorInput.h
    game/source/editor/EditorLayout.cpp
    game/source/editor/EditorLayout.h
    game/source/editor/EditorLayoutUi.h
    game/source/editor/EditorMath.h
    game/source/editor/EditorNudge.cpp
    game/source/editor/EditorNudge.h
    game/source/editor/EditorOrientation.cpp
    game/source/editor/EditorOrientation.h
    game/source/editor/EditorPicking.cpp
    game/source/editor/EditorPicking.h
    game/source/editor/EditorPlacement.cpp
    game/source/editor/EditorPlacement.h
    game/source/editor/StaticPropPlacement.cpp
    game/source/editor/StaticPropPlacement.h
    game/source/editor/TerrainSculpt.h
    game/source/editor/TerrainPaint.h
    game/source/editor/EditorBuildPreference.cpp
    game/source/editor/EditorBuildPreference.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/EditorSelectionSet.h
    game/source/editor/EditorGroupTranslate.h
    game/source/editor/EditorGroupRotate.h
    game/source/editor/EditorToolCommands.cpp
    game/source/editor/EditorToolCommands.h
    game/source/editor/EditorToolRunner.cpp
    game/source/editor/EditorToolRunner.h
    game/source/editor/EditorWorkspace.cpp
    game/source/editor/EditorWorkspace.h
    game/source/editor/ItemDatabaseEditor.cpp
    game/source/editor/ItemDatabaseEditor.h
    game/source/editor/ItemIdInspectorEdit.h
    game/source/editor/LevelEditor.cpp
    game/source/editor/LevelEditor.h
    game/source/editor/RuntimeLevelReload.cpp
    game/source/editor/RuntimeLevelReload.h
    game/source/gameplay/PlatformerCamera.cpp
    game/source/gameplay/PlatformerCamera.h
    game/source/gameplay/Player.cpp
    game/source/gameplay/Player.h
    game/source/gameplay/PlayerPresentation.h
    game/source/gameplay/CollectibleRunState.h
    game/source/gameplay/Inventory.cpp
    game/source/gameplay/Inventory.h
    game/source/gameplay/GameplayIdentity.h
    game/source/gameplay/GameplayStat.h
    game/source/gameplay/ItemDefinition.h
    game/source/gameplay/GameplayDefinition.cpp
    game/source/gameplay/GameplayDefinition.h
    game/source/gameplay/GameplayDefinitionFile.cpp
    game/source/gameplay/GameplayDefinitionFile.h
    game/source/gameplay/InventoryUi.cpp
    game/source/gameplay/InventoryUi.h
    game/source/gameplay/ItemPickupRuntime.cpp
    game/source/gameplay/ItemPickupRuntime.h
    game/source/gameplay/ItemPickupCollectionFeedback.h
    game/source/gameplay/ItemPickupCollectionHud.h
    game/source/gameplay/GameplayObjectiveHud.h
    game/source/gameplay/GameplayAudio.h
    game/source/gameplay/PlayerHealth.h
    game/source/gameplay/PlayerDeath.h
    game/source/gameplay/DoorLockRuntime.cpp
    game/source/gameplay/DoorLockRuntime.h
    game/source/world/ItemPickup.h
    game/source/gameplay/GameFlowState.h
    game/source/gameplay/LevelCompletionState.h
    game/source/gameplay/LevelTransition.cpp
    game/source/gameplay/LevelTransition.h
    game/source/gameplay/RespawnState.h
    game/source/gameplay/RunTimerState.h
    game/source/gameplay/SessionBestTimeState.h
    game/source/input/Input.cpp
    game/source/input/Input.h
    game/source/input/InputState.h
    game/source/physics/PhysicsCapacity.h
    game/source/physics/DynamicBoxGrab.h
    game/source/physics/PhysicsWorld.cpp
    game/source/physics/PhysicsWorld.h
    game/source/persistence/BestTimeSave.cpp
    game/source/persistence/BestTimeSave.h
    game/source/platform/FileReplace.h
    game/source/platform/HostProcess.h
    game/source/platform/RuntimePaths.cpp
    game/source/platform/RuntimePaths.h
    game/source/platform/Time.cpp
    game/source/platform/Time.h
    game/source/platform/Window.cpp
    game/source/platform/Window.h
    game/source/platform/GameplayAudio.cpp
    game/source/platform/GameplayAudio.h
    game/source/render/Renderer.cpp
    game/source/render/Renderer.h
    game/source/render/LightingEnvironment.cpp
    game/source/render/LightingEnvironment.h
    game/source/render/LocalLights.h
    game/source/render/WorldLighting.cpp
    game/source/render/WorldLighting.h
    game/source/render/LevelGoalVisualization.cpp
    game/source/render/LevelGoalVisualization.h
    game/source/render/CameraView.h
    game/source/render/StaticModelThumbnail.cpp
    game/source/render/StaticModelThumbnail.h
    game/source/render/TextureThumbnail.cpp
    game/source/render/TextureThumbnail.h
    game/source/render/StaticModelPreview.cpp
    game/source/render/StaticModelPreview.h
    game/source/render/LoadedModelMaterials.cpp
    game/source/render/LoadedModelMaterials.h
    game/source/render/StaticModelScene.cpp
    game/source/render/StaticModelScene.h
    game/source/render/TerrainMesh.cpp
    game/source/render/TerrainMesh.h
    game/source/render/GroundCoverGpu.cpp
    game/source/render/GroundCoverGpu.h
    game/source/render/ItemPickupTargetHighlight.h
    game/source/render/ItemPickupCollectionFeedbackDraw.cpp
    game/source/render/ItemPickupCollectionFeedbackDraw.h
    game/source/ui/debug/DebugMetrics.cpp
    game/source/ui/debug/DebugMetrics.h
    game/source/ui/debug/DebugUi.cpp
    game/source/ui/debug/DebugUi.h
    game/source/ui/debug/DebugUiBackend.cpp
    game/source/ui/debug/DebugUiBackend.h
    game/source/world/GreyboxWorld.h
    game/source/world/CollectibleWorld.h
    game/source/world/HazardWorld.h
    game/source/world/LevelDefinition.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LocalLight.h
    game/source/world/StaticProp.h
    game/source/world/StaticProp.cpp
    game/source/world/Terrain.h
    game/source/world/TerrainGeometry.h
    game/source/world/TerrainMaterialShading.h
    game/source/world/TerrainSculpt.h
    game/source/world/TerrainPaint.h
    game/source/world/LevelGoal.h
    game/source/world/LevelIdentity.h
    game/source/world/LevelFile.cpp
    game/source/world/LevelFile.h
    game/source/world/LevelWriter.cpp
    game/source/world/LevelWriter.h
    game/source/world/MovingPlatform.h
    game/source/world/PressurePlate.h
    game/source/world/Door.h
    game/source/world/RespawnWorld.h
    game/source/world/Slope.h
)

if(WIN32)
    target_sources(Platformer3D PRIVATE
        game/source/platform/FileReplaceWindows.cpp
        game/source/platform/RuntimePathsWindows.cpp
        game/source/platform/OpenFileDialog.h
        $<$<CONFIG:Development>:game/source/platform/WindowsHostProcess.cpp>
        $<$<CONFIG:Development>:game/source/platform/WindowsOpenFileDialog.cpp>
        $<$<NOT:$<CONFIG:Development>>:game/source/platform/HostProcessStub.cpp>
        $<$<NOT:$<CONFIG:Development>>:game/source/platform/OpenFileDialogStub.cpp>)
else()
    target_sources(Platformer3D PRIVATE
        game/source/platform/FileReplacePosix.cpp
        game/source/platform/HostProcessStub.cpp
        game/source/platform/OpenFileDialogStub.cpp
        game/source/platform/RuntimePathsPosix.cpp)
endif()

target_include_directories(Platformer3D PRIVATE
    game/source
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:Development>>:${imgui_SOURCE_DIR}>
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:Development>>:${rlimgui_SOURCE_DIR}>
)
target_link_libraries(Platformer3D PRIVATE raylib Jolt)
if(WIN32)
    target_link_libraries(Platformer3D PRIVATE ole32 shell32 uuid)
endif()
target_link_libraries(Platformer3D PRIVATE
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:Development>>:rlimgui>
)

if(MSVC)
    target_compile_options(Platformer3D PRIVATE /W4 /permissive-)
else()
    target_compile_options(Platformer3D PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Config-specific macros. Optimization/symbols for Development come from
# CMAKE_<LANG>_FLAGS_DEVELOPMENT; MSVC /DEBUG comes from CMAKE_EXE_LINKER_FLAGS_DEVELOPMENT.
target_compile_definitions(Platformer3D PRIVATE
    $<$<CONFIG:Debug>:GAME_DEBUG=1;GAME_DEVELOPMENT_TOOLS=1;PLATFORMER_ENABLE_DEBUG_UI=1>
    $<$<CONFIG:Development>:GAME_DEVELOPMENT=1;GAME_DEVELOPMENT_TOOLS=1;PLATFORMER_ENABLE_DEBUG_UI=1>
    $<$<CONFIG:Release>:GAME_RELEASE=1>
)

# Milestone 32 authoring boundary. The level editor writes the canonical
# project source, which the runtime never reads, so the path cannot come from
# RuntimeAssetPath. It is injected here explicitly: no process CWD, no walking
# parent directories looking for .git/CMakeLists.txt/game/assets/source.
#
# Development only. Debug compiles the editor scaffolding with authoring
# disabled, and Release receives neither the macro nor the path literal, so the
# shipped binary carries no developer repository path and stays relocatable.
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/game/assets/source" PLATFORMER_AUTHORING_SOURCE_ROOT)
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" PLATFORMER_REPOSITORY_ROOT)
target_compile_definitions(Platformer3D PRIVATE
    $<$<CONFIG:Development>:PLATFORMER_ENABLE_LEVEL_AUTHORING=1>
    "$<$<CONFIG:Development>:PLATFORMER_AUTHORING_SOURCE_ROOT=\"${PLATFORMER_AUTHORING_SOURCE_ROOT}\">"
    $<$<CONFIG:Development>:PLATFORMER_ENABLE_EDITOR_TOOLS=1>
    "$<$<CONFIG:Development>:PLATFORMER_REPOSITORY_ROOT=\"${PLATFORMER_REPOSITORY_ROOT}\">"
)

set_target_properties(Platformer3D PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Cooker owns source -> cooked. Staging copies cooked files next to the exe:
#   <exe dir>/assets/textures/test_checker.png
#   <exe dir>/assets/textures/test_ground_cover_tuft.png
#   <exe dir>/assets/models/player.glb
#   <exe dir>/assets/models/test_static.glb
#   <exe dir>/assets/models/test_authored.glb
#   <exe dir>/assets/models/test_textured.glb
#   <exe dir>/assets/levels/level_01.level
#   <exe dir>/assets/levels/level_02.level
# Extra valid cooked levels/*.level files are discovered at staging time.
# Staged levels/*.level files not in that current inventory are removed.
#   <exe dir>/assets/sounds/item_pickup_collect.wav
#   <exe dir>/assets/sounds/player_damage.wav
#   <exe dir>/assets/sounds/player_death.wav
#   <exe dir>/assets/sounds/player_respawn.wav
#   <exe dir>/assets/sounds/player_footstep.wav
#   <exe dir>/assets/sounds/player_jump.wav
#   <exe dir>/assets/sounds/player_land.wav
#   <exe dir>/assets/sounds/ui_navigate.wav
#   <exe dir>/assets/sounds/ui_confirm.wav
#   <exe dir>/assets/sounds/pause_open.wav
#   <exe dir>/assets/sounds/pause_close.wav
#   <exe dir>/assets/sounds/inventory_open.wav
#   <exe dir>/assets/sounds/inventory_close.wav
#   <exe dir>/assets/shaders/world_lit.vs
#   <exe dir>/assets/shaders/world_lit.fs
#   <exe dir>/assets/shaders/shadow_depth.vs
#   <exe dir>/assets/shaders/shadow_depth.fs
# Do not stage textures/test_textured_basecolor.png; that PNG is Blender authoring input only.
# Runtime lookup uses the executable directory, not the process CWD.
# The staged level file is required at runtime; Application loads it once in Initialize.
# Inventory lives in cmake/RuntimeAssets.cmake so POST_BUILD and editor
# Stage Runtime Assets share one list. cmake/StageRuntimeAssets.cmake is the
# implementation; this POST_BUILD invokes it and does not compile/link.
set(PLATFORMER_COOKED_DIR "${CMAKE_SOURCE_DIR}/game/assets/cooked")
include("${CMAKE_SOURCE_DIR}/cmake/RuntimeAssets.cmake")

set(PLATFORMER_MISSING_COOKED_ASSETS)
foreach(relative IN LISTS PLATFORMER_RUNTIME_ASSETS)
    if(NOT EXISTS "${PLATFORMER_COOKED_DIR}/${relative}")
        list(APPEND PLATFORMER_MISSING_COOKED_ASSETS "  ${PLATFORMER_COOKED_DIR}/${relative}")
    endif()
endforeach()
if(PLATFORMER_MISSING_COOKED_ASSETS)
    list(JOIN PLATFORMER_MISSING_COOKED_ASSETS "\n" PLATFORMER_MISSING_COOKED_ASSETS_TEXT)
    message(FATAL_ERROR
        "Required cooked runtime asset is missing:\n"
        "${PLATFORMER_MISSING_COOKED_ASSETS_TEXT}\n"
        "The game never loads assets from game/assets/source.\n"
        "From the repository root, run:\n"
        "  python tools/cook_assets.py")
endif()

file(TO_CMAKE_PATH "${PLATFORMER_COOKED_DIR}" PLATFORMER_COOKED_DIR)
add_custom_command(TARGET Platformer3D POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        "-DPLATFORMER_COOKED_DIR=${PLATFORMER_COOKED_DIR}"
        "-DPLATFORMER_STAGE_DEST=$<TARGET_FILE_DIR:Platformer3D>/assets"
        -P "${CMAKE_SOURCE_DIR}/cmake/StageRuntimeAssets.cmake"
    COMMENT "Staging cooked runtime assets next to the executable"
)

# Milestone 47: static GLB import, catalog discovery, and path-safety. Not shipped.
# No window, raylib, Jolt, or ImGui. Uses a temporary source root so canonical
# assets are not modified.
add_executable(StaticGlbImportTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/assets/StaticGlbImport.cpp
    game/source/assets/StaticGlbImport.h
    game/source/assets/StaticGlbImportTest.cpp
    game/source/assets/StaticModelCatalog.cpp
    game/source/assets/StaticModelCatalog.h
)
target_include_directories(StaticGlbImportTest PRIVATE game/source)
if(MSVC)
    target_compile_options(StaticGlbImportTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(StaticGlbImportTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
file(TO_CMAKE_PATH
    "${CMAKE_SOURCE_DIR}/game/assets/source/models/test_static.glb"
    PLATFORMER_TEST_STATIC_GLB)
file(TO_CMAKE_PATH
    "${CMAKE_SOURCE_DIR}/game/assets/source/models/test_authored.glb"
    PLATFORMER_TEST_AUTHORED_GLB)
file(TO_CMAKE_PATH
    "${CMAKE_SOURCE_DIR}/game/assets/source/models/test_textured.glb"
    PLATFORMER_TEST_TEXTURED_GLB)
file(TO_CMAKE_PATH
    "${CMAKE_SOURCE_DIR}/game/assets/source/models/player.glb"
    PLATFORMER_PLAYER_GLB)
file(TO_CMAKE_PATH
    "${CMAKE_SOURCE_DIR}/game/assets/source/textures/test_checker.png"
    PLATFORMER_TEST_CHECKER_PNG)
file(TO_CMAKE_PATH
    "${CMAKE_SOURCE_DIR}/game/assets/source/textures/test_ground_cover_tuft.png"
    PLATFORMER_TEST_GROUND_COVER_TUFT_PNG)
target_compile_definitions(StaticGlbImportTest PRIVATE
    "PLATFORMER_TEST_STATIC_GLB=\"${PLATFORMER_TEST_STATIC_GLB}\""
)

# Milestone 84: imported Base Color / Base Color Texture presentation. CPU only.
add_executable(ModelMaterialPresentationTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/assets/ModelMaterialPresentation.cpp
    game/source/assets/ModelMaterialPresentation.h
    game/source/assets/ModelMaterialPresentationTest.cpp
)
target_include_directories(ModelMaterialPresentationTest PRIVATE game/source)
if(MSVC)
    target_compile_options(ModelMaterialPresentationTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(ModelMaterialPresentationTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_compile_definitions(ModelMaterialPresentationTest PRIVATE
    "PLATFORMER_TEST_STATIC_GLB=\"${PLATFORMER_TEST_STATIC_GLB}\""
    "PLATFORMER_TEST_AUTHORED_GLB=\"${PLATFORMER_TEST_AUTHORED_GLB}\""
    "PLATFORMER_TEST_TEXTURED_GLB=\"${PLATFORMER_TEST_TEXTURED_GLB}\""
    "PLATFORMER_PLAYER_GLB=\"${PLATFORMER_PLAYER_GLB}\""
)

# Milestone 48: Content Browser query/filter/delete authority. Not ImGui.
# Uses a temporary source/cooked/staged tree so canonical assets are not modified.
add_executable(ContentBrowserTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/assets/StaticGlbImport.cpp
    game/source/assets/StaticGlbImport.h
    game/source/assets/StaticModelCatalog.cpp
    game/source/assets/StaticModelCatalog.h
    game/source/assets/StaticModelDelete.cpp
    game/source/assets/StaticModelDelete.h
    game/source/assets/SourceTextureCatalog.cpp
    game/source/assets/SourceTextureCatalog.h
    game/source/editor/ContentBrowser.cpp
    game/source/editor/ContentBrowser.h
    game/source/editor/ContentBrowserOrganization.cpp
    game/source/editor/ContentBrowserOrganization.h
    game/source/editor/ContentBrowserView.cpp
    game/source/editor/ContentBrowserView.h
    game/source/editor/ContentBrowserTest.cpp
    game/source/editor/EditorLayout.cpp
    game/source/editor/EditorLayout.h
    game/source/platform/FileReplace.h
    game/source/platform/RuntimePaths.cpp
    game/source/platform/RuntimePaths.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
if(WIN32)
    target_sources(ContentBrowserTest PRIVATE
        game/source/platform/RuntimePathsWindows.cpp
        game/source/platform/FileReplaceWindows.cpp)
else()
    target_sources(ContentBrowserTest PRIVATE
        game/source/platform/RuntimePathsPosix.cpp
        game/source/platform/FileReplacePosix.cpp)
endif()
target_include_directories(ContentBrowserTest PRIVATE game/source)
if(MSVC)
    target_compile_options(ContentBrowserTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(ContentBrowserTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(WIN32)
    target_link_libraries(ContentBrowserTest PRIVATE ole32 shell32 uuid)
endif()
target_compile_definitions(ContentBrowserTest PRIVATE
    "PLATFORMER_TEST_STATIC_GLB=\"${PLATFORMER_TEST_STATIC_GLB}\""
)

# Milestone 48.1: thumbnail cache identity, invalidation, framing, and view-mode
# persistence. Not ImGui and not raylib.
add_executable(ContentBrowserThumbnailTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/assets/StaticGlbImport.cpp
    game/source/assets/StaticGlbImport.h
    game/source/assets/StaticModelCatalog.cpp
    game/source/assets/StaticModelCatalog.h
    game/source/assets/StaticModelDelete.cpp
    game/source/assets/StaticModelDelete.h
    game/source/assets/SourceTextureCatalog.cpp
    game/source/assets/SourceTextureCatalog.h
    game/source/editor/ContentBrowser.cpp
    game/source/editor/ContentBrowser.h
    game/source/editor/ContentBrowserOrganization.cpp
    game/source/editor/ContentBrowserOrganization.h
    game/source/editor/ContentBrowserView.cpp
    game/source/editor/ContentBrowserView.h
    game/source/editor/ContentBrowserThumbnailTest.cpp
    game/source/editor/EditorLayout.cpp
    game/source/editor/EditorLayout.h
    game/source/platform/FileReplace.h
    game/source/editor/EditorMath.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/EditorSelectionSet.h
    game/source/editor/EditorGroupTranslate.h
    game/source/editor/EditorGroupRotate.h
    game/source/editor/StaticModelThumbnailCache.cpp
    game/source/editor/StaticModelThumbnailCache.h
    game/source/editor/StaticModelFraming.cpp
    game/source/editor/StaticModelFraming.h
    game/source/platform/RuntimePaths.cpp
    game/source/platform/RuntimePaths.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
if(WIN32)
    target_sources(ContentBrowserThumbnailTest PRIVATE
        game/source/platform/RuntimePathsWindows.cpp
        game/source/platform/FileReplaceWindows.cpp)
else()
    target_sources(ContentBrowserThumbnailTest PRIVATE
        game/source/platform/RuntimePathsPosix.cpp
        game/source/platform/FileReplacePosix.cpp)
endif()
target_include_directories(ContentBrowserThumbnailTest PRIVATE game/source)
if(MSVC)
    target_compile_options(ContentBrowserThumbnailTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(ContentBrowserThumbnailTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(WIN32)
    target_link_libraries(ContentBrowserThumbnailTest PRIVATE ole32 shell32 uuid)
endif()
target_compile_definitions(ContentBrowserThumbnailTest PRIVATE
    "PLATFORMER_TEST_STATIC_GLB=\"${PLATFORMER_TEST_STATIC_GLB}\""
)

# Milestone 89: standalone PNG import, identity, collision, catalog, and recipe cook mapping.
add_executable(RuntimePngImportTest
    game/source/assets/RuntimePng.h
    game/source/assets/RuntimePngImport.cpp
    game/source/assets/RuntimePngImport.h
    game/source/assets/RuntimePngDelete.cpp
    game/source/assets/RuntimePngDelete.h
    game/source/assets/SourceTextureCatalog.cpp
    game/source/assets/SourceTextureCatalog.h
    game/source/assets/RuntimePngImportTest.cpp
)
target_include_directories(RuntimePngImportTest PRIVATE game/source)
if(MSVC)
    target_compile_options(RuntimePngImportTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(RuntimePngImportTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_compile_definitions(RuntimePngImportTest PRIVATE
    "PLATFORMER_TEST_CHECKER_PNG=\"${PLATFORMER_TEST_CHECKER_PNG}\""
    "PLATFORMER_TEST_GROUND_COVER_TUFT_PNG=\"${PLATFORMER_TEST_GROUND_COVER_TUFT_PNG}\""
)

# Milestone 89: Content Browser collections, logical folders, Favorites, search,
# layout persistence, thumbnail lifecycle, and Terrain delete protection.
add_executable(ContentBrowserAssetLibraryTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/assets/StaticGlbImport.cpp
    game/source/assets/StaticGlbImport.h
    game/source/assets/StaticModelCatalog.cpp
    game/source/assets/StaticModelCatalog.h
    game/source/assets/RuntimePng.h
    game/source/assets/RuntimePngImport.cpp
    game/source/assets/RuntimePngImport.h
    game/source/assets/SourceTextureCatalog.cpp
    game/source/assets/SourceTextureCatalog.h
    game/source/editor/ContentBrowser.cpp
    game/source/editor/ContentBrowser.h
    game/source/editor/ContentBrowserOrganization.cpp
    game/source/editor/ContentBrowserOrganization.h
    game/source/editor/ContentBrowserView.cpp
    game/source/editor/ContentBrowserView.h
    game/source/editor/ContentBrowserAssetLibraryTest.cpp
    game/source/editor/EditorLayout.cpp
    game/source/editor/EditorLayout.h
    game/source/editor/StaticModelFraming.cpp
    game/source/editor/StaticModelFraming.h
    game/source/editor/StaticModelThumbnailCache.cpp
    game/source/editor/StaticModelThumbnailCache.h
    game/source/editor/TerrainTextureReference.h
    game/source/editor/TextureThumbnailLifecycle.h
    game/source/platform/FileReplace.h
    game/source/platform/RuntimePaths.cpp
    game/source/platform/RuntimePaths.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
if(WIN32)
    target_sources(ContentBrowserAssetLibraryTest PRIVATE
        game/source/platform/RuntimePathsWindows.cpp
        game/source/platform/FileReplaceWindows.cpp)
else()
    target_sources(ContentBrowserAssetLibraryTest PRIVATE
        game/source/platform/RuntimePathsPosix.cpp
        game/source/platform/FileReplacePosix.cpp)
endif()
target_include_directories(ContentBrowserAssetLibraryTest PRIVATE game/source)
if(MSVC)
    target_compile_options(ContentBrowserAssetLibraryTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(ContentBrowserAssetLibraryTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(WIN32)
    target_link_libraries(ContentBrowserAssetLibraryTest PRIVATE ole32 shell32 uuid)
endif()
target_compile_definitions(ContentBrowserAssetLibraryTest PRIVATE
    "PLATFORMER_TEST_STATIC_GLB=\"${PLATFORMER_TEST_STATIC_GLB}\""
    "PLATFORMER_TEST_CHECKER_PNG=\"${PLATFORMER_TEST_CHECKER_PNG}\""
    "PLATFORMER_TEST_GROUND_COVER_TUFT_PNG=\"${PLATFORMER_TEST_GROUND_COVER_TUFT_PNG}\""
)

# Milestone 48.2: Model Preview selection/load/orbit/framing math. Not ImGui/raylib.
add_executable(StaticModelPreviewTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/assets/StaticGlbImport.cpp
    game/source/assets/StaticGlbImport.h
    game/source/assets/StaticModelCatalog.cpp
    game/source/assets/StaticModelCatalog.h
    game/source/assets/StaticModelDelete.cpp
    game/source/assets/StaticModelDelete.h
    game/source/assets/SourceTextureCatalog.cpp
    game/source/assets/SourceTextureCatalog.h
    game/source/editor/ContentBrowser.cpp
    game/source/editor/ContentBrowser.h
    game/source/editor/ContentBrowserOrganization.cpp
    game/source/editor/ContentBrowserOrganization.h
    game/source/editor/ContentBrowserView.cpp
    game/source/editor/ContentBrowserView.h
    game/source/editor/EditorLayout.cpp
    game/source/editor/EditorLayout.h
    game/source/editor/EditorMath.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/EditorSelectionSet.h
    game/source/editor/EditorGroupTranslate.h
    game/source/editor/EditorGroupRotate.h
    game/source/editor/EditorWorkspace.cpp
    game/source/editor/EditorWorkspace.h
    game/source/editor/StaticModelFraming.cpp
    game/source/editor/StaticModelFraming.h
    game/source/editor/StaticModelPreviewTest.cpp
    game/source/editor/StaticModelThumbnailCache.cpp
    game/source/editor/StaticModelThumbnailCache.h
    game/source/platform/FileReplace.h
    game/source/platform/RuntimePaths.cpp
    game/source/platform/RuntimePaths.h
    game/source/render/CameraView.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
if(WIN32)
    target_sources(StaticModelPreviewTest PRIVATE
        game/source/platform/RuntimePathsWindows.cpp
        game/source/platform/FileReplaceWindows.cpp)
else()
    target_sources(StaticModelPreviewTest PRIVATE
        game/source/platform/RuntimePathsPosix.cpp
        game/source/platform/FileReplacePosix.cpp)
endif()
target_include_directories(StaticModelPreviewTest PRIVATE game/source)
if(MSVC)
    target_compile_options(StaticModelPreviewTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(StaticModelPreviewTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(WIN32)
    target_link_libraries(StaticModelPreviewTest PRIVATE ole32 shell32 uuid)
endif()
target_compile_definitions(StaticModelPreviewTest PRIVATE
    "PLATFORMER_TEST_STATIC_GLB=\"${PLATFORMER_TEST_STATIC_GLB}\""
)

# Level Format v1 parser/writer/reload-core test binary. Not shipped. Does not link
# raylib/Jolt/ImGui. The platform file-replacement TU is the only OS-specific
# dependency, needed because SaveLevelFile promotes through that boundary
# instead of duplicating ReplaceFileW/MoveFileExW. M39 reload prepare/reconcile
# lives here so candidate parse can be tested without the game.
add_executable(LevelFileTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/editor/AuthoredObjectLifecycle.cpp
    game/source/editor/AuthoredObjectLifecycle.h
    game/source/editor/AuthoringGroups.cpp
    game/source/editor/AuthoringGroups.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/EditorSelectionSet.h
    game/source/editor/EditorGroupTranslate.h
    game/source/editor/EditorGroupRotate.h
    game/source/editor/RuntimeLevelReload.cpp
    game/source/editor/RuntimeLevelReload.h
    game/source/platform/FileReplace.h
    game/source/world/LevelFile.cpp
    game/source/world/LevelFile.h
    game/source/world/LevelFileTest.cpp
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
    game/source/world/LevelWriter.cpp
    game/source/world/LevelWriter.h
    game/source/world/TerrainSculpt.h
    game/source/world/TerrainPaint.h
)
if(WIN32)
    target_sources(LevelFileTest PRIVATE game/source/platform/FileReplaceWindows.cpp)
else()
    target_sources(LevelFileTest PRIVATE game/source/platform/FileReplacePosix.cpp)
endif()
target_include_directories(LevelFileTest PRIVATE game/source)
if(MSVC)
    target_compile_options(LevelFileTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(LevelFileTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
file(TO_CMAKE_PATH
    "${CMAKE_SOURCE_DIR}/game/assets/source/levels/level_01.level"
    PLATFORMER_LEVEL01_SOURCE_PATH)
file(TO_CMAKE_PATH
    "${CMAKE_SOURCE_DIR}/game/assets/source/levels/level_02.level"
    PLATFORMER_LEVEL02_SOURCE_PATH)
target_compile_definitions(LevelFileTest PRIVATE
    "PLATFORMER_LEVEL01_SOURCE_PATH=\"${PLATFORMER_LEVEL01_SOURCE_PATH}\""
    "PLATFORMER_LEVEL02_SOURCE_PATH=\"${PLATFORMER_LEVEL02_SOURCE_PATH}\""
)

# Milestone 86: CPU Terrain tessellation/normals. No window, Jolt, or raylib.
add_executable(TerrainGeometryTest
    game/source/world/TerrainGeometryTest.cpp
    game/source/world/Terrain.h
    game/source/world/TerrainGeometry.h
)
target_include_directories(TerrainGeometryTest PRIVATE game/source)
if(MSVC)
    target_compile_options(TerrainGeometryTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(TerrainGeometryTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 87: deterministic Terrain sculpt math/stroke. No window, Jolt, or raylib.
add_executable(TerrainSculptTest
    game/source/world/TerrainSculptTest.cpp
    game/source/world/Terrain.h
    game/source/world/TerrainGeometry.h
    game/source/world/TerrainSculpt.h
)
target_include_directories(TerrainSculptTest PRIVATE game/source)
if(MSVC)
    target_compile_options(TerrainSculptTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(TerrainSculptTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 88: Terrain base material identity/tiling/UV. No window, Jolt, or raylib.
add_executable(TerrainMaterialTest
    game/source/world/TerrainMaterialTest.cpp
    game/source/world/Terrain.h
    game/source/world/TerrainGeometry.h
    game/source/world/TerrainMaterialShading.h
    game/source/world/TerrainSculpt.h
    game/source/world/TerrainPaint.h
    game/source/editor/TerrainPaint.h
    game/source/editor/TerrainSculpt.h
    game/source/editor/TerrainPickerCard.h
    game/source/assets/RuntimePng.h
)
target_include_directories(TerrainMaterialTest PRIVATE game/source)
if(MSVC)
    target_compile_options(TerrainMaterialTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(TerrainMaterialTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 90: Terrain material painting weights/strokes. No window, Jolt, or raylib.
add_executable(TerrainPaintTest
    game/source/world/TerrainPaintTest.cpp
    game/source/world/Terrain.h
    game/source/world/TerrainGeometry.h
    game/source/world/TerrainSculpt.h
    game/source/world/TerrainPaint.h
    game/source/assets/RuntimePng.h
)
target_include_directories(TerrainPaintTest PRIVATE game/source)
if(MSVC)
    target_compile_options(TerrainPaintTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(TerrainPaintTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 93: Terrain vegetation palette, occupancy, and derived instances.
add_executable(TerrainVegetationTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
    game/source/world/TerrainVegetationTest.cpp
    game/source/world/Terrain.h
    game/source/world/TerrainGeometry.h
    game/source/world/TerrainSculpt.h
    game/source/world/TerrainVegetation.h
    game/source/editor/TerrainVegetation.h
)
target_include_directories(TerrainVegetationTest PRIVATE game/source)
if(MSVC)
    target_compile_options(TerrainVegetationTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(TerrainVegetationTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 96: Terrain Grass / Ground Cover CPU authoring, distribution, and
# instancing-plan tests. No window, raylib, Jolt, or ImGui.
add_executable(TerrainGroundCoverTest
    game/source/world/TerrainGroundCoverTest.cpp
    game/source/world/Terrain.h
    game/source/world/TerrainGeometry.h
    game/source/world/TerrainSculpt.h
    game/source/world/TerrainVegetation.h
    game/source/world/TerrainGroundCover.h
    game/source/editor/TerrainSculpt.h
    game/source/editor/TerrainGroundCover.h
)
target_include_directories(TerrainGroundCoverTest PRIVATE game/source)
if(MSVC)
    target_compile_options(TerrainGroundCoverTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(TerrainGroundCoverTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_compile_definitions(TerrainGroundCoverTest PRIVATE
    "PLATFORMER_TEST_CHECKER_PNG=\"${PLATFORMER_TEST_CHECKER_PNG}\""
    "PLATFORMER_TEST_GROUND_COVER_TUFT_PNG=\"${PLATFORMER_TEST_GROUND_COVER_TUFT_PNG}\""
)


# Milestone 54: player runtime Inventory API and lifecycle policy. Not shipped.
# No window, raylib, Jolt, or ImGui.
add_executable(InventoryTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/gameplay/Inventory.cpp
    game/source/gameplay/Inventory.h
    game/source/gameplay/InventoryTest.cpp
    game/source/platform/FileReplace.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/LevelFile.cpp
    game/source/world/LevelFile.h
    game/source/world/LevelWriter.cpp
    game/source/world/LevelWriter.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
if(WIN32)
    target_sources(InventoryTest PRIVATE game/source/platform/FileReplaceWindows.cpp)
else()
    target_sources(InventoryTest PRIVATE game/source/platform/FileReplacePosix.cpp)
endif()
target_include_directories(InventoryTest PRIVATE game/source)
if(MSVC)
    target_compile_options(InventoryTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(InventoryTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_compile_definitions(InventoryTest PRIVATE
    "PLATFORMER_LEVEL01_SOURCE_PATH=\"${PLATFORMER_LEVEL01_SOURCE_PATH}\""
)

# Milestone 98: gameplay-definition identity, stats, registry, and catalog file.
# Not shipped. No window, raylib, Jolt, or ImGui.
add_executable(GameplayDefinitionTest
    game/source/gameplay/GameplayDefinition.cpp
    game/source/gameplay/GameplayDefinition.h
    game/source/gameplay/GameplayDefinitionFile.cpp
    game/source/gameplay/GameplayDefinitionFile.h
    game/source/gameplay/GameplayDefinitionTest.cpp
    game/source/gameplay/GameplayIdentity.h
    game/source/gameplay/GameplayStat.h
    game/source/gameplay/ItemDefinition.h
    game/source/platform/FileReplace.h
)
if(WIN32)
    target_sources(GameplayDefinitionTest PRIVATE game/source/platform/FileReplaceWindows.cpp)
else()
    target_sources(GameplayDefinitionTest PRIVATE game/source/platform/FileReplacePosix.cpp)
endif()
target_include_directories(GameplayDefinitionTest PRIVATE game/source)
if(MSVC)
    target_compile_options(GameplayDefinitionTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(GameplayDefinitionTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
file(TO_CMAKE_PATH
    "${CMAKE_SOURCE_DIR}/game/assets/source/gameplay/definitions.gameplay"
    PLATFORMER_GAMEPLAY_DEFINITIONS_SOURCE_PATH)
target_compile_definitions(GameplayDefinitionTest PRIVATE
    "PLATFORMER_GAMEPLAY_DEFINITIONS_SOURCE_PATH=\"${PLATFORMER_GAMEPLAY_DEFINITIONS_SOURCE_PATH}\""
)

# Milestone 99: Item Database editor seams. No window, raylib, or ImGui.
add_executable(ItemDatabaseEditorTest
    game/source/assets/SourceTextureCatalog.cpp
    game/source/assets/SourceTextureCatalog.h
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/assets/StaticModelCatalog.cpp
    game/source/assets/StaticModelCatalog.h
    game/source/editor/ItemDatabaseEditor.h
    game/source/editor/ItemDatabaseEditorTest.cpp
    game/source/editor/TerrainPickerCard.h
    game/source/gameplay/GameplayDefinition.cpp
    game/source/gameplay/GameplayDefinition.h
    game/source/gameplay/GameplayDefinitionFile.cpp
    game/source/gameplay/GameplayDefinitionFile.h
    game/source/gameplay/GameplayIdentity.h
    game/source/gameplay/GameplayStat.h
    game/source/gameplay/ItemDefinition.h
    game/source/platform/FileReplace.h
)
if(WIN32)
    target_sources(ItemDatabaseEditorTest PRIVATE game/source/platform/FileReplaceWindows.cpp)
else()
    target_sources(ItemDatabaseEditorTest PRIVATE game/source/platform/FileReplacePosix.cpp)
endif()
target_include_directories(ItemDatabaseEditorTest PRIVATE game/source)
if(MSVC)
    target_compile_options(ItemDatabaseEditorTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(ItemDatabaseEditorTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_compile_definitions(ItemDatabaseEditorTest PRIVATE
    "PLATFORMER_GAMEPLAY_DEFINITIONS_SOURCE_PATH=\"${PLATFORMER_GAMEPLAY_DEFINITIONS_SOURCE_PATH}\""
)

# Milestone 56: player-facing Inventory UI presentation/input/lifecycle.
# Not shipped. No window, raylib, Jolt, or ImGui.
add_executable(InventoryUiTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/gameplay/Inventory.cpp
    game/source/gameplay/Inventory.h
    game/source/gameplay/InventoryUi.cpp
    game/source/gameplay/InventoryUi.h
    game/source/gameplay/InventoryUiTest.cpp
    game/source/gameplay/ItemPickupRuntime.cpp
    game/source/gameplay/ItemPickupRuntime.h
    game/source/platform/FileReplace.h
    game/source/world/ItemPickup.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/LevelFile.cpp
    game/source/world/LevelFile.h
    game/source/world/LevelWriter.cpp
    game/source/world/LevelWriter.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
if(WIN32)
    target_sources(InventoryUiTest PRIVATE game/source/platform/FileReplaceWindows.cpp)
else()
    target_sources(InventoryUiTest PRIVATE game/source/platform/FileReplacePosix.cpp)
endif()
target_include_directories(InventoryUiTest PRIVATE game/source)
if(MSVC)
    target_compile_options(InventoryUiTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(InventoryUiTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_compile_definitions(InventoryUiTest PRIVATE
    "PLATFORMER_LEVEL01_SOURCE_PATH=\"${PLATFORMER_LEVEL01_SOURCE_PATH}\""
)

# Focused harness for the M32 editor Apply Preview rebuild cycle. Not shipped.
# Links Jolt because that is exactly what is being validated; no raylib/ImGui.
# It only reads the canonical level and never writes a file.
add_executable(PhysicsRebuildTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/editor/AuthoredObjectLifecycle.cpp
    game/source/editor/AuthoredObjectLifecycle.h
    game/source/editor/AuthoringGroups.cpp
    game/source/editor/AuthoringGroups.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/EditorSelectionSet.h
    game/source/editor/EditorGroupTranslate.h
    game/source/editor/EditorGroupRotate.h
    game/source/gameplay/Inventory.cpp
    game/source/gameplay/Inventory.h
    game/source/gameplay/PlayerHealth.h
    game/source/gameplay/GameFlowState.h
    game/source/physics/PhysicsCapacity.h
    game/source/physics/DynamicBoxGrab.h
    game/source/physics/PhysicsRebuildTest.cpp
    game/source/physics/PhysicsWorld.cpp
    game/source/physics/PhysicsWorld.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
    game/source/world/LevelFile.cpp
    game/source/world/LevelFile.h
)
target_include_directories(PhysicsRebuildTest PRIVATE game/source)
target_link_libraries(PhysicsRebuildTest PRIVATE Jolt)
if(MSVC)
    target_compile_options(PhysicsRebuildTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(PhysicsRebuildTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_compile_definitions(PhysicsRebuildTest PRIVATE
    "PLATFORMER_LEVEL01_SOURCE_PATH=\"${PLATFORMER_LEVEL01_SOURCE_PATH}\""
)

# Milestone 86: Terrain MeshShape collision correspondence. Jolt only; no raylib.
add_executable(TerrainCollisionTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/gameplay/Inventory.cpp
    game/source/gameplay/Inventory.h
    game/source/physics/PhysicsCapacity.h
    game/source/physics/DynamicBoxGrab.h
    game/source/physics/TerrainCollisionTest.cpp
    game/source/physics/PhysicsWorld.cpp
    game/source/physics/PhysicsWorld.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
    game/source/world/LevelFile.cpp
    game/source/world/LevelFile.h
    game/source/world/Terrain.h
    game/source/world/TerrainGeometry.h
    game/source/world/TerrainSculpt.h
)
target_include_directories(TerrainCollisionTest PRIVATE game/source)
target_link_libraries(TerrainCollisionTest PRIVATE Jolt)
if(MSVC)
    target_compile_options(TerrainCollisionTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(TerrainCollisionTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_compile_definitions(TerrainCollisionTest PRIVATE
    "PLATFORMER_LEVEL01_SOURCE_PATH=\"${PLATFORMER_LEVEL01_SOURCE_PATH}\""
)

# Focused Milestone 51 Grab / Carry harness. Not shipped. Jolt only; no raylib.
add_executable(DynamicBoxGrabTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/gameplay/Inventory.cpp
    game/source/gameplay/Inventory.h
    game/source/physics/DynamicBoxGrab.h
    game/source/physics/PhysicsCapacity.h
    game/source/physics/DynamicBoxGrabTest.cpp
    game/source/physics/PhysicsWorld.cpp
    game/source/physics/PhysicsWorld.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
    game/source/world/LevelFile.cpp
    game/source/world/LevelFile.h
)
target_include_directories(DynamicBoxGrabTest PRIVATE game/source)
target_link_libraries(DynamicBoxGrabTest PRIVATE Jolt)
if(MSVC)
    target_compile_options(DynamicBoxGrabTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(DynamicBoxGrabTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_compile_definitions(DynamicBoxGrabTest PRIVATE
    "PLATFORMER_LEVEL01_SOURCE_PATH=\"${PLATFORMER_LEVEL01_SOURCE_PATH}\""
)

# Focused Milestone 52 Pressure Plate overlap harness. Not shipped. Jolt only; no raylib.
add_executable(PressurePlateTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/gameplay/Inventory.cpp
    game/source/gameplay/Inventory.h
    game/source/physics/DynamicBoxGrab.h
    game/source/physics/PhysicsCapacity.h
    game/source/physics/PressurePlateTest.cpp
    game/source/physics/PhysicsWorld.cpp
    game/source/physics/PhysicsWorld.h
    game/source/platform/FileReplace.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/PressurePlate.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
    game/source/world/LevelFile.cpp
    game/source/world/LevelFile.h
    game/source/world/LevelWriter.cpp
    game/source/world/LevelWriter.h
)
if(WIN32)
    target_sources(PressurePlateTest PRIVATE game/source/platform/FileReplaceWindows.cpp)
else()
    target_sources(PressurePlateTest PRIVATE game/source/platform/FileReplacePosix.cpp)
endif()
target_include_directories(PressurePlateTest PRIVATE game/source)
target_link_libraries(PressurePlateTest PRIVATE Jolt)
if(MSVC)
    target_compile_options(PressurePlateTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(PressurePlateTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_compile_definitions(PressurePlateTest PRIVATE
    "PLATFORMER_LEVEL01_SOURCE_PATH=\"${PLATFORMER_LEVEL01_SOURCE_PATH}\""
)

# Focused Milestone 53 Door / Pressure Plate link harness. Not shipped. Jolt only; no raylib.
add_executable(DoorTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/gameplay/Inventory.cpp
    game/source/gameplay/Inventory.h
    game/source/physics/DynamicBoxGrab.h
    game/source/physics/PhysicsCapacity.h
    game/source/physics/DoorTest.cpp
    game/source/physics/PhysicsWorld.cpp
    game/source/physics/PhysicsWorld.h
    game/source/platform/FileReplace.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/Door.h
    game/source/world/PressurePlate.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
    game/source/world/LevelFile.cpp
    game/source/world/LevelFile.h
    game/source/world/LevelWriter.cpp
    game/source/world/LevelWriter.h
)
if(WIN32)
    target_sources(DoorTest PRIVATE game/source/platform/FileReplaceWindows.cpp)
else()
    target_sources(DoorTest PRIVATE game/source/platform/FileReplacePosix.cpp)
endif()
target_include_directories(DoorTest PRIVATE game/source)
target_link_libraries(DoorTest PRIVATE Jolt)
if(MSVC)
    target_compile_options(DoorTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(DoorTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_compile_definitions(DoorTest PRIVATE
    "PLATFORMER_LEVEL01_SOURCE_PATH=\"${PLATFORMER_LEVEL01_SOURCE_PATH}\""
)

# Milestone 55: Item Pickup targeting, TryAdd collection, E-priority with M51,
# LOS, and run-state isolation. Not shipped.
add_executable(ItemPickupTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/gameplay/Inventory.cpp
    game/source/gameplay/Inventory.h
    game/source/gameplay/ItemPickupRuntime.cpp
    game/source/gameplay/ItemPickupRuntime.h
    game/source/gameplay/ItemPickupTest.cpp
    game/source/physics/DynamicBoxGrab.h
    game/source/physics/PhysicsCapacity.h
    game/source/physics/PhysicsWorld.cpp
    game/source/physics/PhysicsWorld.h
    game/source/platform/FileReplace.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/Door.h
    game/source/world/ItemPickup.h
    game/source/world/PressurePlate.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
    game/source/world/LevelFile.cpp
    game/source/world/LevelFile.h
    game/source/world/LevelWriter.cpp
    game/source/world/LevelWriter.h
)
if(WIN32)
    target_sources(ItemPickupTest PRIVATE game/source/platform/FileReplaceWindows.cpp)
else()
    target_sources(ItemPickupTest PRIVATE game/source/platform/FileReplacePosix.cpp)
endif()
target_include_directories(ItemPickupTest PRIVATE game/source)
target_link_libraries(ItemPickupTest PRIVATE Jolt)
if(MSVC)
    target_compile_options(ItemPickupTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(ItemPickupTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_compile_definitions(ItemPickupTest PRIVATE
    "PLATFORMER_LEVEL01_SOURCE_PATH=\"${PLATFORMER_LEVEL01_SOURCE_PATH}\""
)

# Milestone 61: Item Pickup collection feedback. No window, raylib, or Jolt.
add_executable(ItemPickupCollectionFeedbackTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/gameplay/Inventory.cpp
    game/source/gameplay/Inventory.h
    game/source/gameplay/ItemPickupRuntime.cpp
    game/source/gameplay/ItemPickupRuntime.h
    game/source/gameplay/ItemPickupCollectionFeedback.h
    game/source/gameplay/ItemPickupCollectionFeedbackTest.cpp
    game/source/render/ItemPickupTargetHighlight.h
    game/source/world/ItemPickup.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
target_include_directories(ItemPickupCollectionFeedbackTest PRIVATE game/source)
if(MSVC)
    target_compile_options(ItemPickupCollectionFeedbackTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(ItemPickupCollectionFeedbackTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 62: Item Pickup collection HUD notification. No window, raylib, or Jolt.
add_executable(ItemPickupCollectionHudTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/gameplay/Inventory.cpp
    game/source/gameplay/Inventory.h
    game/source/gameplay/ItemPickupRuntime.cpp
    game/source/gameplay/ItemPickupRuntime.h
    game/source/gameplay/ItemPickupCollectionFeedback.h
    game/source/gameplay/ItemPickupCollectionHud.h
    game/source/gameplay/ItemPickupCollectionHudTest.cpp
    game/source/render/ItemPickupTargetHighlight.h
    game/source/world/ItemPickup.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
target_include_directories(ItemPickupCollectionHudTest PRIVATE game/source)
if(MSVC)
    target_compile_options(ItemPickupCollectionHudTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(ItemPickupCollectionHudTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 67: Gameplay Level/objective HUD presentation. No window, raylib, or Jolt.
add_executable(GameplayObjectiveHudTest
    game/source/gameplay/GameFlowState.h
    game/source/gameplay/GameplayObjectiveHud.h
    game/source/gameplay/GameplayObjectiveHudTest.cpp
    game/source/gameplay/ItemPickupCollectionHud.h
    game/source/gameplay/LevelCompletionState.h
    game/source/gameplay/LevelTransition.h
    game/source/gameplay/PlayerHealth.h
    game/source/gameplay/PlayerDeath.h
    game/source/world/LevelGoal.h
    game/source/world/LevelIdentity.h
)
target_include_directories(GameplayObjectiveHudTest PRIVATE game/source)
if(MSVC)
    target_compile_options(GameplayObjectiveHudTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(GameplayObjectiveHudTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 71/72/73/74/75: gameplay SFX request recording, lethal-hit precedence,
# movement cadence, world-interaction edges, Collectible collection, UI/menu
# feedback, and authored/Dirty isolation.
# No window, raylib, or Jolt.
add_executable(GameplayAudioTest
    game/source/gameplay/CollectibleRunState.h
    game/source/gameplay/DoorLockRuntime.cpp
    game/source/gameplay/DoorLockRuntime.h
    game/source/gameplay/GameFlowState.h
    game/source/gameplay/GameplayAudio.h
    game/source/gameplay/GameplayAudioTest.cpp
    game/source/gameplay/Inventory.cpp
    game/source/gameplay/Inventory.h
    game/source/gameplay/InventoryUi.cpp
    game/source/gameplay/InventoryUi.h
    game/source/gameplay/ItemPickupRuntime.cpp
    game/source/gameplay/ItemPickupRuntime.h
    game/source/gameplay/LevelCompletionState.h
    game/source/gameplay/PlayerDeath.h
    game/source/gameplay/PlayerHealth.h
    game/source/gameplay/RespawnState.h
    game/source/platform/GameplayAudio.h
    game/source/world/CollectibleWorld.h
    game/source/world/Door.h
    game/source/world/HazardWorld.h
    game/source/world/ItemPickup.h
    game/source/world/LevelGoal.h
    game/source/world/RespawnWorld.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
)
target_include_directories(GameplayAudioTest PRIVATE game/source)
if(MSVC)
    target_compile_options(GameplayAudioTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(GameplayAudioTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 69: Player Health authority, Hazard contact cadence, Health HUD
# visibility. No window, raylib, or Jolt.
add_executable(PlayerHealthTest
    game/source/gameplay/GameFlowState.h
    game/source/gameplay/GameplayObjectiveHud.h
    game/source/gameplay/LevelCompletionState.h
    game/source/gameplay/LevelTransition.h
    game/source/gameplay/PlayerHealth.h
    game/source/gameplay/PlayerDeath.h
    game/source/gameplay/PlayerHealthTest.cpp
    game/source/gameplay/RespawnState.h
    game/source/gameplay/RunTimerState.h
    game/source/world/HazardWorld.h
    game/source/world/LevelIdentity.h
)
target_include_directories(PlayerHealthTest PRIVATE game/source)
if(MSVC)
    target_compile_options(PlayerHealthTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(PlayerHealthTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 83: player presentation facing/visual-transform/lifecycle. No window,
# raylib, or Jolt.
add_executable(PlayerPresentationTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/gameplay/Inventory.cpp
    game/source/gameplay/Inventory.h
    game/source/gameplay/ItemPickupRuntime.cpp
    game/source/gameplay/ItemPickupRuntime.h
    game/source/gameplay/PlayerPresentation.h
    game/source/gameplay/PlayerPresentationTest.cpp
    game/source/world/ItemPickup.h
    game/source/world/RespawnWorld.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
target_include_directories(PlayerPresentationTest PRIVATE game/source)
if(MSVC)
    target_compile_options(PlayerPresentationTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(PlayerPresentationTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 63: Level Goal overlap, one-shot completion, reset. No window, raylib, or Jolt.
add_executable(LevelGoalTest
    game/source/gameplay/LevelCompletionState.h
    game/source/render/LevelGoalVisualization.h
    game/source/world/LevelGoal.h
    game/source/world/LevelIdentity.h
    game/source/world/LevelGoalTest.cpp
)
target_include_directories(LevelGoalTest PRIVATE game/source)
if(MSVC)
    target_compile_options(LevelGoalTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(LevelGoalTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 64: destination-bearing Level Goal, staged-only transition prepare,
# Inventory carry-over policy, no SceneManager. No window, raylib, or Jolt.
add_executable(LevelTransitionTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/gameplay/Inventory.cpp
    game/source/gameplay/Inventory.h
    game/source/gameplay/InventoryUi.cpp
    game/source/gameplay/InventoryUi.h
    game/source/gameplay/LevelCompletionState.h
    game/source/gameplay/PlayerHealth.h
    game/source/gameplay/LevelTransition.cpp
    game/source/gameplay/LevelTransition.h
    game/source/platform/FileReplace.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/LevelFile.cpp
    game/source/world/LevelFile.h
    game/source/world/LevelIdentity.h
    game/source/world/LevelWriter.cpp
    game/source/world/LevelWriter.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
    game/source/gameplay/LevelTransitionTest.cpp
)
if(WIN32)
    target_sources(LevelTransitionTest PRIVATE game/source/platform/FileReplaceWindows.cpp)
else()
    target_sources(LevelTransitionTest PRIVATE game/source/platform/FileReplacePosix.cpp)
endif()
target_include_directories(LevelTransitionTest PRIVATE game/source)
if(MSVC)
    target_compile_options(LevelTransitionTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(LevelTransitionTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_compile_definitions(LevelTransitionTest PRIVATE
    "PLATFORMER_LEVEL01_SOURCE_PATH=\"${PLATFORMER_LEVEL01_SOURCE_PATH}\""
    "PLATFORMER_LEVEL02_SOURCE_PATH=\"${PLATFORMER_LEVEL02_SOURCE_PATH}\""
)

# Milestone 68: Main Menu, Pause, Play fresh staged level_01, Quit, Run Complete Esc.
# Terminal Run Complete vs destination hold, Play Again, Restart-current-level.
# No window, raylib, ImGui, or Jolt.
add_executable(GameFlowTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/gameplay/DoorLockRuntime.cpp
    game/source/gameplay/DoorLockRuntime.h
    game/source/gameplay/GameFlowState.h
    game/source/gameplay/GameplayObjectiveHud.h
    game/source/gameplay/PlayerHealth.h
    game/source/gameplay/PlayerDeath.h
    game/source/gameplay/GameFlowTest.cpp
    game/source/gameplay/Inventory.cpp
    game/source/gameplay/Inventory.h
    game/source/gameplay/InventoryUi.cpp
    game/source/gameplay/InventoryUi.h
    game/source/gameplay/ItemPickupRuntime.cpp
    game/source/gameplay/ItemPickupRuntime.h
    game/source/gameplay/ItemPickupCollectionFeedback.h
    game/source/gameplay/ItemPickupCollectionHud.h
    game/source/gameplay/LevelCompletionState.h
    game/source/gameplay/LevelTransition.cpp
    game/source/gameplay/LevelTransition.h
    game/source/platform/FileReplace.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/LevelFile.cpp
    game/source/world/LevelFile.h
    game/source/world/LevelIdentity.h
    game/source/world/LevelWriter.cpp
    game/source/world/LevelWriter.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
if(WIN32)
    target_sources(GameFlowTest PRIVATE game/source/platform/FileReplaceWindows.cpp)
else()
    target_sources(GameFlowTest PRIVATE game/source/platform/FileReplacePosix.cpp)
endif()
target_include_directories(GameFlowTest PRIVATE game/source)
if(MSVC)
    target_compile_options(GameFlowTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(GameFlowTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_compile_definitions(GameFlowTest PRIVATE
    "PLATFORMER_LEVEL01_SOURCE_PATH=\"${PLATFORMER_LEVEL01_SOURCE_PATH}\""
    "PLATFORMER_LEVEL02_SOURCE_PATH=\"${PLATFORMER_LEVEL02_SOURCE_PATH}\""
)

# Milestone 64.1: authored source discovery, New Level, Open prepare, Next Level
# selector. No window, raylib, ImGui, or Jolt.
add_executable(AuthoredLevelCatalogTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/editor/AuthoredLevelCatalog.cpp
    game/source/editor/AuthoredLevelCatalog.h
    game/source/editor/AuthoredLevelCatalogTest.cpp
    game/source/physics/PhysicsCapacity.h
    game/source/platform/FileReplace.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/LevelFile.cpp
    game/source/world/LevelFile.h
    game/source/world/LevelIdentity.h
    game/source/world/LevelWriter.cpp
    game/source/world/LevelWriter.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
if(WIN32)
    target_sources(AuthoredLevelCatalogTest PRIVATE game/source/platform/FileReplaceWindows.cpp)
else()
    target_sources(AuthoredLevelCatalogTest PRIVATE game/source/platform/FileReplacePosix.cpp)
endif()
target_include_directories(AuthoredLevelCatalogTest PRIVATE game/source)
if(MSVC)
    target_compile_options(AuthoredLevelCatalogTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(AuthoredLevelCatalogTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_compile_definitions(AuthoredLevelCatalogTest PRIVATE
    "PLATFORMER_LEVEL01_SOURCE_PATH=\"${PLATFORMER_LEVEL01_SOURCE_PATH}\""
    "PLATFORMER_LEVEL02_SOURCE_PATH=\"${PLATFORMER_LEVEL02_SOURCE_PATH}\""
)

# Milestone 63 correction: editor translucent AABB vs gameplay marker-only.
# Hidden window. Not shipped.
add_executable(LevelGoalVisualizationTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/assets/ModelMaterialPresentation.cpp
    game/source/assets/ModelMaterialPresentation.h
    game/source/platform/RuntimePaths.cpp
    game/source/platform/RuntimePaths.h
    game/source/render/LevelGoalVisualization.cpp
    game/source/render/LevelGoalVisualization.h
    game/source/render/LevelGoalVisualizationTest.cpp
    game/source/render/LoadedModelMaterials.cpp
    game/source/render/LoadedModelMaterials.h
    game/source/render/StaticModelScene.cpp
    game/source/render/StaticModelScene.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
if(WIN32)
    target_sources(LevelGoalVisualizationTest PRIVATE game/source/platform/RuntimePathsWindows.cpp)
else()
    target_sources(LevelGoalVisualizationTest PRIVATE game/source/platform/RuntimePathsPosix.cpp)
endif()
target_include_directories(LevelGoalVisualizationTest PRIVATE game/source)
target_link_libraries(LevelGoalVisualizationTest PRIVATE raylib)
if(MSVC)
    target_compile_options(LevelGoalVisualizationTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(LevelGoalVisualizationTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(WIN32)
    target_link_libraries(LevelGoalVisualizationTest PRIVATE ole32 shell32 uuid)
endif()

# Milestone 57: Key Item & Locked Door. Not shipped.
add_executable(LockedDoorTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/gameplay/DoorLockRuntime.cpp
    game/source/gameplay/DoorLockRuntime.h
    game/source/gameplay/Inventory.cpp
    game/source/gameplay/Inventory.h
    game/source/gameplay/InventoryUi.cpp
    game/source/gameplay/InventoryUi.h
    game/source/gameplay/ItemPickupRuntime.cpp
    game/source/gameplay/ItemPickupRuntime.h
    game/source/gameplay/LockedDoorTest.cpp
    game/source/physics/DynamicBoxGrab.h
    game/source/physics/PhysicsCapacity.h
    game/source/physics/PhysicsWorld.cpp
    game/source/physics/PhysicsWorld.h
    game/source/platform/FileReplace.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/Door.h
    game/source/world/ItemPickup.h
    game/source/world/PressurePlate.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
    game/source/world/LevelFile.cpp
    game/source/world/LevelFile.h
    game/source/world/LevelWriter.cpp
    game/source/world/LevelWriter.h
)
if(WIN32)
    target_sources(LockedDoorTest PRIVATE game/source/platform/FileReplaceWindows.cpp)
else()
    target_sources(LockedDoorTest PRIVATE game/source/platform/FileReplacePosix.cpp)
endif()
target_include_directories(LockedDoorTest PRIVATE game/source)
target_link_libraries(LockedDoorTest PRIVATE Jolt)
if(MSVC)
    target_compile_options(LockedDoorTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(LockedDoorTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_compile_definitions(LockedDoorTest PRIVATE
    "PLATFORMER_LEVEL01_SOURCE_PATH=\"${PLATFORMER_LEVEL01_SOURCE_PATH}\""
)

# Milestone 33 picking/selection math. Not shipped. No window, raylib,
# Jolt, or ImGui: the editor must pick authored proxies with project-owned CPU
# math rather than Jolt raycasts.
add_executable(EditorPickingTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/editor/AuthoredObjectLifecycle.cpp
    game/source/editor/AuthoredObjectLifecycle.h
    game/source/editor/AuthoringGroups.cpp
    game/source/editor/AuthoringGroups.h
    game/source/editor/EditorCamera.cpp
    game/source/editor/EditorCamera.h
    game/source/editor/EditorHierarchy.h
    game/source/editor/EditorInput.h
    game/source/editor/EditorMath.h
    game/source/editor/EditorGizmo.cpp
    game/source/editor/EditorGizmo.h
    game/source/editor/EditorSnap.cpp
    game/source/editor/EditorSnap.h
    game/source/editor/EditorViewportGrid.cpp
    game/source/editor/EditorViewportGrid.h
    game/source/editor/EditorPicking.cpp
    game/source/editor/EditorPicking.h
    game/source/editor/EditorPickingTest.cpp
    game/source/editor/TerrainSculpt.h
    game/source/editor/TerrainPaint.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/EditorSelectionSet.h
    game/source/editor/EditorGroupTranslate.h
    game/source/editor/EditorGroupRotate.h
    game/source/editor/EditorWorkspace.cpp
    game/source/editor/EditorWorkspace.h
    game/source/render/CameraView.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
target_include_directories(EditorPickingTest PRIVATE game/source)
if(MSVC)
    target_compile_options(EditorPickingTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(EditorPickingTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 34 translation-gizmo math, editor-layout path, and Milestone 76
# snapping/quantization. No window, raylib, or ImGui: live drag/render is
# exercised by Platformer3D, not this test.
add_executable(EditorGizmoTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/editor/AuthoredObjectLifecycle.cpp
    game/source/editor/AuthoredObjectLifecycle.h
    game/source/editor/AuthoringGroups.cpp
    game/source/editor/AuthoringGroups.h
    game/source/editor/EditorGizmo.cpp
    game/source/editor/EditorGizmo.h
    game/source/editor/EditorSnap.cpp
    game/source/editor/EditorSnap.h
    game/source/editor/EditorViewportGrid.cpp
    game/source/editor/EditorViewportGrid.h
    game/source/editor/EditorGizmoTest.cpp
    game/source/editor/EditorLayout.cpp
    game/source/editor/EditorLayout.h
    game/source/editor/EditorMath.h
    game/source/editor/EditorNudge.cpp
    game/source/editor/EditorNudge.h
    game/source/editor/EditorPicking.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/EditorSelectionSet.h
    game/source/editor/EditorGroupTranslate.h
    game/source/editor/EditorGroupRotate.h
    game/source/platform/RuntimePaths.cpp
    game/source/platform/RuntimePaths.h
    game/source/render/CameraView.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
if(WIN32)
    target_sources(EditorGizmoTest PRIVATE game/source/platform/RuntimePathsWindows.cpp)
else()
    target_sources(EditorGizmoTest PRIVATE game/source/platform/RuntimePathsPosix.cpp)
endif()
target_include_directories(EditorGizmoTest PRIVATE game/source)
if(MSVC)
    target_compile_options(EditorGizmoTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(EditorGizmoTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(WIN32)
    target_link_libraries(EditorGizmoTest PRIVATE ole32 shell32 uuid)
endif()

# Milestone 77: viewport-grid spacing derivation, line classification, bounded
# generation, and layout persistence. No window, raylib, or ImGui.
add_executable(EditorViewportGridTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/editor/AuthoredObjectLifecycle.cpp
    game/source/editor/AuthoredObjectLifecycle.h
    game/source/editor/AuthoringGroups.cpp
    game/source/editor/AuthoringGroups.h
    game/source/editor/EditorGizmo.cpp
    game/source/editor/EditorGizmo.h
    game/source/editor/EditorSnap.cpp
    game/source/editor/EditorSnap.h
    game/source/editor/EditorViewportGrid.cpp
    game/source/editor/EditorViewportGrid.h
    game/source/editor/EditorViewportGridTest.cpp
    game/source/editor/EditorMath.h
    game/source/editor/EditorPicking.cpp
    game/source/editor/EditorPicking.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/EditorSelectionSet.h
    game/source/editor/EditorGroupTranslate.h
    game/source/editor/EditorGroupRotate.h
    game/source/editor/EditorWorkspace.cpp
    game/source/editor/EditorWorkspace.h
    game/source/render/CameraView.h
    game/source/render/Renderer.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
target_include_directories(EditorViewportGridTest PRIVATE game/source)
if(MSVC)
    target_compile_options(EditorViewportGridTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(EditorViewportGridTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 35 Phase A orientation-widget and dolly math. No window.
add_executable(EditorOrientationTest
    game/source/editor/EditorCamera.cpp
    game/source/editor/EditorCamera.h
    game/source/editor/EditorInput.h
    game/source/editor/EditorMath.h
    game/source/editor/EditorOrientation.cpp
    game/source/editor/EditorOrientation.h
    game/source/editor/EditorOrientationTest.cpp
    game/source/render/CameraView.h
)
target_include_directories(EditorOrientationTest PRIVATE game/source)
if(MSVC)
    target_compile_options(EditorOrientationTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(EditorOrientationTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 37/38 process/tool-runner job logic. No window, raylib, or ImGui.
# Windows tests launch cmd.exe fixtures and may invoke cmake -P staging.
# They do not compile the game or run cmake --build / MSBuild.
add_executable(EditorToolRunnerTest
    game/source/editor/EditorToolCommands.cpp
    game/source/editor/EditorToolCommands.h
    game/source/editor/EditorToolRunner.cpp
    game/source/editor/EditorToolRunner.h
    game/source/editor/EditorToolRunnerTest.cpp
    game/source/platform/HostProcess.h
)
if(WIN32)
    target_sources(EditorToolRunnerTest PRIVATE game/source/platform/WindowsHostProcess.cpp)
else()
    target_sources(EditorToolRunnerTest PRIVATE game/source/platform/HostProcessStub.cpp)
endif()
target_include_directories(EditorToolRunnerTest PRIVATE game/source)
if(MSVC)
    target_compile_options(EditorToolRunnerTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(EditorToolRunnerTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_compile_definitions(EditorToolRunnerTest PRIVATE
    "PLATFORMER_REPOSITORY_ROOT=\"${PLATFORMER_REPOSITORY_ROOT}\""
)

# Milestone 36 Phase A workspace visibility and shared action-enable policy.
# No window, raylib, or ImGui.
add_executable(EditorWorkspaceTest
    game/source/editor/EditorWorkspace.cpp
    game/source/editor/EditorWorkspace.h
    game/source/editor/EditorWorkspaceTest.cpp
)
target_include_directories(EditorWorkspaceTest PRIVATE game/source)
if(MSVC)
    target_compile_options(EditorWorkspaceTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(EditorWorkspaceTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 41 Phase A: pure working-copy Add/Duplicate/Delete. No ImGui.
add_executable(AuthoredObjectLifecycleTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/editor/AuthoredObjectLifecycle.cpp
    game/source/editor/AuthoredObjectLifecycle.h
    game/source/editor/AuthoringGroups.cpp
    game/source/editor/AuthoringGroups.h
    game/source/editor/AuthoredObjectLifecycleTest.cpp
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/EditorSelectionSet.h
    game/source/editor/EditorGroupTranslate.h
    game/source/editor/EditorGroupRotate.h
    game/source/editor/ItemIdInspectorEdit.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
target_include_directories(AuthoredObjectLifecycleTest PRIVATE game/source)
if(MSVC)
    target_compile_options(AuthoredObjectLifecycleTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(AuthoredObjectLifecycleTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 81: persistent Authoring Group semantics. No ImGui.
add_executable(AuthoringGroupsTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/editor/AuthoredObjectLifecycle.cpp
    game/source/editor/AuthoredObjectLifecycle.h
    game/source/editor/AuthoringGroups.cpp
    game/source/editor/AuthoringGroups.h
    game/source/editor/AuthoringGroupsTest.cpp
    game/source/editor/EditorGizmo.cpp
    game/source/editor/EditorGizmo.h
    game/source/editor/EditorHierarchy.h
    game/source/editor/EditorSnap.cpp
    game/source/editor/EditorSnap.h
    game/source/editor/EditorMath.h
    game/source/editor/EditorPicking.cpp
    game/source/editor/EditorPicking.h
    game/source/editor/EditorWorkspace.cpp
    game/source/editor/EditorWorkspace.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/EditorSelectionSet.h
    game/source/editor/EditorGroupTranslate.h
    game/source/editor/EditorGroupRotate.h
    game/source/editor/LocalLightAuthoring.h
    game/source/editor/ItemIdInspectorEdit.h
    game/source/platform/FileReplace.h
    game/source/render/CameraView.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/LevelFile.cpp
    game/source/world/LevelFile.h
    game/source/world/LevelWriter.cpp
    game/source/world/LevelWriter.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
if(WIN32)
    target_sources(AuthoringGroupsTest PRIVATE game/source/platform/FileReplaceWindows.cpp)
else()
    target_sources(AuthoringGroupsTest PRIVATE game/source/platform/FileReplacePosix.cpp)
endif()
target_include_directories(AuthoringGroupsTest PRIVATE game/source)
if(MSVC)
    target_compile_options(AuthoringGroupsTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(AuthoringGroupsTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 42: Object Palette placement mode and candidate resolution. No ImGui.
add_executable(EditorPlacementTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/editor/AuthoredObjectLifecycle.cpp
    game/source/editor/AuthoredObjectLifecycle.h
    game/source/editor/AuthoringGroups.cpp
    game/source/editor/AuthoringGroups.h
    game/source/editor/EditorPicking.cpp
    game/source/editor/EditorPicking.h
    game/source/editor/EditorPlacement.cpp
    game/source/editor/EditorPlacement.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/EditorSelectionSet.h
    game/source/editor/EditorGroupTranslate.h
    game/source/editor/EditorGroupRotate.h
    game/source/editor/EditorPlacementTest.cpp
    game/source/editor/EditorWorkspace.cpp
    game/source/editor/EditorWorkspace.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
target_include_directories(EditorPlacementTest PRIVATE game/source)
if(MSVC)
    target_compile_options(EditorPlacementTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(EditorPlacementTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 50: Static Prop placement workflow. No window, raylib, or ImGui.
add_executable(StaticPropPlacementTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/editor/AuthoredLifecycleCommands.cpp
    game/source/editor/AuthoredLifecycleCommands.h
    game/source/editor/AuthoredObjectLifecycle.cpp
    game/source/editor/AuthoredObjectLifecycle.h
    game/source/editor/AuthoringGroups.cpp
    game/source/editor/AuthoringGroups.h
    game/source/editor/EditorGizmo.cpp
    game/source/editor/EditorGizmo.h
    game/source/editor/EditorSnap.cpp
    game/source/editor/EditorSnap.h
    game/source/editor/EditorHierarchy.h
    game/source/editor/EditorPicking.cpp
    game/source/editor/EditorPicking.h
    game/source/editor/EditorPlacement.cpp
    game/source/editor/EditorPlacement.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/EditorSelectionSet.h
    game/source/editor/EditorGroupTranslate.h
    game/source/editor/EditorGroupRotate.h
    game/source/editor/EditorWorkspace.cpp
    game/source/editor/EditorWorkspace.h
    game/source/editor/ItemIdInspectorEdit.h
    game/source/editor/LevelEditor.cpp
    game/source/editor/LevelEditor.h
    game/source/editor/StaticPropPlacement.cpp
    game/source/editor/StaticPropPlacement.h
    game/source/editor/StaticPropPlacementTest.cpp
    game/source/editor/StaticPropTransform.h
    game/source/physics/PhysicsCapacity.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
target_include_directories(StaticPropPlacementTest PRIVATE game/source)
if(MSVC)
    target_compile_options(StaticPropPlacementTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(StaticPropPlacementTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 43: Quick Toolbar mapping, chrome geometry, and build preference.
# No window, raylib, or ImGui.
add_executable(EditorQuickToolbarTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/editor/AuthoredObjectLifecycle.cpp
    game/source/editor/AuthoredObjectLifecycle.h
    game/source/editor/AuthoringGroups.cpp
    game/source/editor/AuthoringGroups.h
    game/source/editor/EditorBuildPreference.cpp
    game/source/editor/EditorBuildPreference.h
    game/source/editor/EditorCamera.cpp
    game/source/editor/EditorCamera.h
    game/source/editor/EditorGizmo.cpp
    game/source/editor/EditorGizmo.h
    game/source/editor/EditorSnap.cpp
    game/source/editor/EditorSnap.h
    game/source/editor/EditorViewportGrid.cpp
    game/source/editor/EditorViewportGrid.h
    game/source/editor/EditorLayout.cpp
    game/source/editor/EditorLayout.h
    game/source/editor/EditorMath.h
    game/source/editor/EditorOrientation.cpp
    game/source/editor/EditorOrientation.h
    game/source/editor/EditorPicking.cpp
    game/source/editor/EditorPicking.h
    game/source/editor/EditorPlacement.cpp
    game/source/editor/EditorPlacement.h
    game/source/editor/EditorQuickToolbarTest.cpp
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/EditorSelectionSet.h
    game/source/editor/EditorGroupTranslate.h
    game/source/editor/EditorGroupRotate.h
    game/source/editor/EditorToolCommands.cpp
    game/source/editor/EditorToolCommands.h
    game/source/editor/EditorWorkspace.cpp
    game/source/editor/EditorWorkspace.h
    game/source/editor/LevelEditor.h
    game/source/platform/RuntimePaths.cpp
    game/source/platform/RuntimePaths.h
    game/source/render/CameraView.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
if(WIN32)
    target_sources(EditorQuickToolbarTest PRIVATE game/source/platform/RuntimePathsWindows.cpp)
else()
    target_sources(EditorQuickToolbarTest PRIVATE game/source/platform/RuntimePathsPosix.cpp)
endif()
target_include_directories(EditorQuickToolbarTest PRIVATE game/source)
if(MSVC)
    target_compile_options(EditorQuickToolbarTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(EditorQuickToolbarTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(WIN32)
    target_link_libraries(EditorQuickToolbarTest PRIVATE ole32 shell32 uuid)
endif()

# Milestone 44: scene-cleanup invariants. No window, raylib, or Jolt.
add_executable(CanonicalSceneCleanupTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/editor/AuthoredObjectLifecycle.cpp
    game/source/editor/AuthoredObjectLifecycle.h
    game/source/editor/AuthoringGroups.cpp
    game/source/editor/AuthoringGroups.h
    game/source/editor/CanonicalSceneCleanupTest.cpp
    game/source/editor/EditorHierarchy.h
    game/source/editor/EditorPicking.cpp
    game/source/editor/EditorPicking.h
    game/source/editor/EditorPlacement.cpp
    game/source/editor/EditorPlacement.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/EditorSelectionSet.h
    game/source/editor/EditorGroupTranslate.h
    game/source/editor/EditorGroupRotate.h
    game/source/editor/EditorWorkspace.cpp
    game/source/editor/EditorWorkspace.h
    game/source/physics/PhysicsCapacity.h
    game/source/render/Renderer.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
target_include_directories(CanonicalSceneCleanupTest PRIVATE game/source)
if(MSVC)
    target_compile_options(CanonicalSceneCleanupTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(CanonicalSceneCleanupTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 41 Phase B: owner-side lifecycle requests without ImGui.
add_executable(AuthoredLifecycleIntegrationTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/editor/AuthoredLifecycleCommands.cpp
    game/source/editor/AuthoredLifecycleCommands.h
    game/source/editor/AuthoredObjectLifecycle.cpp
    game/source/editor/AuthoredObjectLifecycle.h
    game/source/editor/AuthoringGroups.cpp
    game/source/editor/AuthoringGroups.h
    game/source/editor/AuthoredLifecycleIntegrationTest.cpp
    game/source/editor/EditorGizmo.cpp
    game/source/editor/EditorGizmo.h
    game/source/editor/EditorSnap.cpp
    game/source/editor/EditorSnap.h
    game/source/editor/EditorHierarchy.h
    game/source/editor/EditorPicking.cpp
    game/source/editor/EditorPicking.h
    game/source/editor/EditorPlacement.cpp
    game/source/editor/EditorPlacement.h
    game/source/editor/StaticPropPlacement.cpp
    game/source/editor/StaticPropPlacement.h
    game/source/editor/TerrainSculpt.h
    game/source/editor/TerrainPaint.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/EditorSelectionSet.h
    game/source/editor/EditorGroupTranslate.h
    game/source/editor/EditorGroupRotate.h
    game/source/editor/ItemIdInspectorEdit.h
    game/source/editor/LevelEditor.cpp
    game/source/editor/LevelEditor.h
    game/source/editor/ContentBrowser.h
    game/source/editor/EditorWorkspace.cpp
    game/source/editor/EditorWorkspace.h
    game/source/platform/FileReplace.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/LevelFile.cpp
    game/source/world/LevelFile.h
    game/source/world/LevelWriter.cpp
    game/source/world/LevelWriter.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
if(WIN32)
    target_sources(AuthoredLifecycleIntegrationTest PRIVATE game/source/platform/FileReplaceWindows.cpp)
else()
    target_sources(AuthoredLifecycleIntegrationTest PRIVATE game/source/platform/FileReplacePosix.cpp)
endif()
target_include_directories(AuthoredLifecycleIntegrationTest PRIVATE game/source)
if(MSVC)
    target_compile_options(AuthoredLifecycleIntegrationTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(AuthoredLifecycleIntegrationTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 49 Correction 2: Static Prop render transform composition, shared
# model isolation and staged-runtime diagnostic policy. No raylib/GL: the draw
# path is pinned at the authored-transform math boundary.
add_executable(StaticPropRenderTransformTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/editor/AuthoredObjectLifecycle.cpp
    game/source/editor/AuthoredObjectLifecycle.h
    game/source/editor/AuthoringGroups.cpp
    game/source/editor/AuthoringGroups.h
    game/source/editor/EditorMath.h
    game/source/editor/EditorPicking.cpp
    game/source/editor/EditorPicking.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/EditorSelectionSet.h
    game/source/editor/EditorGroupTranslate.h
    game/source/editor/EditorGroupRotate.h
    game/source/editor/EditorWorkspace.cpp
    game/source/editor/EditorWorkspace.h
    game/source/editor/StaticPropRenderTransformTest.cpp
    game/source/editor/StaticPropTransform.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
target_include_directories(StaticPropRenderTransformTest PRIVATE game/source)
if(MSVC)
    target_compile_options(StaticPropRenderTransformTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(StaticPropRenderTransformTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 49 Correction 4: Static Prop Add/Apply/Delete active promotion.
# No window or raylib. Apply is the Application copy of workingCopy into active.
add_executable(StaticPropLifecycleTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/editor/AuthoredObjectLifecycle.cpp
    game/source/editor/AuthoredObjectLifecycle.h
    game/source/editor/AuthoringGroups.cpp
    game/source/editor/AuthoringGroups.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/EditorSelectionSet.h
    game/source/editor/EditorGroupTranslate.h
    game/source/editor/EditorGroupRotate.h
    game/source/editor/StaticPropLifecycleTest.cpp
    game/source/editor/StaticPropTransform.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
target_include_directories(StaticPropLifecycleTest PRIVATE game/source)
if(MSVC)
    target_compile_options(StaticPropLifecycleTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(StaticPropLifecycleTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 49 Correction 3: hidden-window LoadModel isolation. Uses the
# repository test GLB always, and the imported Chest/Barrel when present.
# Not shipped.
add_executable(StaticPropSceneIsolationTest
    game/source/editor/StaticPropSceneIsolationTest.cpp
)
target_include_directories(StaticPropSceneIsolationTest PRIVATE game/source)
target_link_libraries(StaticPropSceneIsolationTest PRIVATE raylib)
if(MSVC)
    target_compile_options(StaticPropSceneIsolationTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(StaticPropSceneIsolationTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
file(TO_CMAKE_PATH
    "${CMAKE_SOURCE_DIR}/game/assets/source/models/Chest by Quaternius - O72u4Drp8k.glb"
    PLATFORMER_CHEST_GLB)
file(TO_CMAKE_PATH
    "${CMAKE_SOURCE_DIR}/game/assets/source/models/Barrel by HFJAKI92 - wrYrHLVtxg.glb"
    PLATFORMER_BARREL_GLB)
target_compile_definitions(StaticPropSceneIsolationTest PRIVATE
    "PLATFORMER_TEST_STATIC_GLB=\"${PLATFORMER_TEST_STATIC_GLB}\""
    "PLATFORMER_CHEST_GLB=\"${PLATFORMER_CHEST_GLB}\""
    "PLATFORMER_BARREL_GLB=\"${PLATFORMER_BARREL_GLB}\""
)

# Milestone 49 clip-plane regression: default-framebuffer greybox after
# Preview-like clip leak vs Gameplay BeginMode3D. Hidden window. Not shipped.
add_executable(GreyboxImmediateStateTest
    game/source/render/GreyboxImmediateStateTest.cpp
)
target_include_directories(GreyboxImmediateStateTest PRIVATE game/source)
target_link_libraries(GreyboxImmediateStateTest PRIVATE raylib)
if(MSVC)
    target_compile_options(GreyboxImmediateStateTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(GreyboxImmediateStateTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_compile_definitions(GreyboxImmediateStateTest PRIVATE
    "PLATFORMER_CHEST_GLB=\"${PLATFORMER_CHEST_GLB}\""
    "PLATFORMER_BARREL_GLB=\"${PLATFORMER_BARREL_GLB}\""
)

# Milestone 58.2: selected-model ghost/highlight uses workingCopy transform and
# oriented picking bounds. No window or raylib.
add_executable(ModelSelectionPreviewTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/editor/AuthoredObjectLifecycle.cpp
    game/source/editor/AuthoredObjectLifecycle.h
    game/source/editor/AuthoringGroups.cpp
    game/source/editor/AuthoringGroups.h
    game/source/editor/EditorGizmo.cpp
    game/source/editor/EditorGizmo.h
    game/source/editor/EditorSnap.cpp
    game/source/editor/EditorSnap.h
    game/source/editor/EditorMath.h
    game/source/editor/EditorPicking.cpp
    game/source/editor/EditorPicking.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/EditorSelectionSet.h
    game/source/editor/EditorGroupTranslate.h
    game/source/editor/EditorGroupRotate.h
    game/source/editor/EditorWorkspace.cpp
    game/source/editor/EditorWorkspace.h
    game/source/editor/ModelSelectionPreviewTest.cpp
    game/source/editor/SelectedModelHighlight.h
    game/source/editor/StaticPropTransform.h
    game/source/gameplay/Inventory.cpp
    game/source/gameplay/Inventory.h
    game/source/gameplay/ItemPickupRuntime.cpp
    game/source/gameplay/ItemPickupRuntime.h
    game/source/render/CameraView.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
target_include_directories(ModelSelectionPreviewTest PRIVATE game/source)
if(MSVC)
    target_compile_options(ModelSelectionPreviewTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(ModelSelectionPreviewTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 58.3: gameplay Item Pickup target highlight presentation.
# Consumes the M55 target index. No window or raylib.
add_executable(ItemPickupTargetHighlightTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/gameplay/Inventory.cpp
    game/source/gameplay/Inventory.h
    game/source/gameplay/ItemPickupRuntime.cpp
    game/source/gameplay/ItemPickupRuntime.h
    game/source/render/ItemPickupTargetHighlight.h
    game/source/render/ItemPickupTargetHighlightTest.cpp
    game/source/world/ItemPickup.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
target_include_directories(ItemPickupTargetHighlightTest PRIVATE game/source)
if(MSVC)
    target_compile_options(ItemPickupTargetHighlightTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(ItemPickupTargetHighlightTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 58.2: selected-model highlight restores depth/blend/cull/matrix.
# Hidden window. Uses the placeholder path so staged assets are not required.
add_executable(SelectedModelHighlightStateTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/assets/ModelMaterialPresentation.cpp
    game/source/assets/ModelMaterialPresentation.h
    game/source/platform/RuntimePaths.cpp
    game/source/platform/RuntimePaths.h
    game/source/render/LoadedModelMaterials.cpp
    game/source/render/LoadedModelMaterials.h
    game/source/render/SelectedModelHighlightStateTest.cpp
    game/source/render/StaticModelScene.cpp
    game/source/render/StaticModelScene.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
if(WIN32)
    target_sources(SelectedModelHighlightStateTest PRIVATE game/source/platform/RuntimePathsWindows.cpp)
else()
    target_sources(SelectedModelHighlightStateTest PRIVATE game/source/platform/RuntimePathsPosix.cpp)
endif()
target_include_directories(SelectedModelHighlightStateTest PRIVATE game/source)
target_link_libraries(SelectedModelHighlightStateTest PRIVATE raylib)
if(MSVC)
    target_compile_options(SelectedModelHighlightStateTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(SelectedModelHighlightStateTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(WIN32)
    target_link_libraries(SelectedModelHighlightStateTest PRIVATE ole32 shell32 uuid)
endif()

# Milestone 61/71: collection burst restore + gameplay SFX load/unload. Hidden window.
add_executable(ItemPickupCollectionFeedbackDrawTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/assets/ModelMaterialPresentation.cpp
    game/source/assets/ModelMaterialPresentation.h
    game/source/platform/GameplayAudio.cpp
    game/source/platform/GameplayAudio.h
    game/source/platform/RuntimePaths.cpp
    game/source/platform/RuntimePaths.h
    game/source/render/ItemPickupCollectionFeedbackDraw.cpp
    game/source/render/ItemPickupCollectionFeedbackDraw.h
    game/source/render/ItemPickupCollectionFeedbackDrawTest.cpp
    game/source/render/LoadedModelMaterials.cpp
    game/source/render/LoadedModelMaterials.h
    game/source/render/StaticModelScene.cpp
    game/source/render/StaticModelScene.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
if(WIN32)
    target_sources(ItemPickupCollectionFeedbackDrawTest PRIVATE game/source/platform/RuntimePathsWindows.cpp)
else()
    target_sources(ItemPickupCollectionFeedbackDrawTest PRIVATE game/source/platform/RuntimePathsPosix.cpp)
endif()
target_include_directories(ItemPickupCollectionFeedbackDrawTest PRIVATE game/source)
target_link_libraries(ItemPickupCollectionFeedbackDrawTest PRIVATE raylib)
if(MSVC)
    target_compile_options(ItemPickupCollectionFeedbackDrawTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(ItemPickupCollectionFeedbackDrawTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(WIN32)
    target_link_libraries(ItemPickupCollectionFeedbackDrawTest PRIVATE ole32 shell32 uuid)
endif()

# Milestone 84: model/material/texture ownership, reload, highlight restore.
# Hidden window. Stages copies next to the test executable.
add_executable(LoadedModelMaterialsTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/assets/ModelMaterialPresentation.cpp
    game/source/assets/ModelMaterialPresentation.h
    game/source/platform/RuntimePaths.cpp
    game/source/platform/RuntimePaths.h
    game/source/render/LoadedModelMaterials.cpp
    game/source/render/LoadedModelMaterials.h
    game/source/render/LoadedModelMaterialsTest.cpp
    game/source/render/GroundCoverGpu.cpp
    game/source/render/GroundCoverGpu.h
    game/source/render/StaticModelScene.cpp
    game/source/render/StaticModelScene.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
if(WIN32)
    target_sources(LoadedModelMaterialsTest PRIVATE game/source/platform/RuntimePathsWindows.cpp)
else()
    target_sources(LoadedModelMaterialsTest PRIVATE game/source/platform/RuntimePathsPosix.cpp)
endif()
target_include_directories(LoadedModelMaterialsTest PRIVATE game/source)
target_link_libraries(LoadedModelMaterialsTest PRIVATE raylib)
if(MSVC)
    target_compile_options(LoadedModelMaterialsTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(LoadedModelMaterialsTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(WIN32)
    target_link_libraries(LoadedModelMaterialsTest PRIVATE ole32 shell32 uuid)
endif()
target_compile_definitions(LoadedModelMaterialsTest PRIVATE
    "PLATFORMER_TEST_STATIC_GLB=\"${PLATFORMER_TEST_STATIC_GLB}\""
    "PLATFORMER_TEST_TEXTURED_GLB=\"${PLATFORMER_TEST_TEXTURED_GLB}\""
    "PLATFORMER_PLAYER_GLB=\"${PLATFORMER_PLAYER_GLB}\""
)

# Milestone 85: CPU lighting environment math/config. No window or raylib.
add_executable(LightingEnvironmentTest
    game/source/render/LightingEnvironment.cpp
    game/source/render/LightingEnvironment.h
    game/source/render/LocalLights.h
    game/source/render/LightingEnvironmentTest.cpp
)
target_include_directories(LightingEnvironmentTest PRIVATE game/source)
if(MSVC)
    target_compile_options(LightingEnvironmentTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(LightingEnvironmentTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 85.3: CPU local-light packing/attenuation. No window or raylib.
add_executable(LocalLightsTest
    game/source/render/LightingEnvironment.cpp
    game/source/render/LightingEnvironment.h
    game/source/render/LocalLights.h
    game/source/render/LocalLightsTest.cpp
)
target_include_directories(LocalLightsTest PRIVATE game/source)
if(MSVC)
    target_compile_options(LocalLightsTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(LocalLightsTest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Milestone 85: shader/shadow resource lifetime. Hidden window. Stages shaders
# next to the test executable. No source-directory fallback.
file(TO_CMAKE_PATH
    "${CMAKE_SOURCE_DIR}/game/assets/source/shaders/world_lit.vs"
    PLATFORMER_WORLD_LIT_VS)
file(TO_CMAKE_PATH
    "${CMAKE_SOURCE_DIR}/game/assets/source/shaders/world_lit.fs"
    PLATFORMER_WORLD_LIT_FS)
file(TO_CMAKE_PATH
    "${CMAKE_SOURCE_DIR}/game/assets/source/shaders/shadow_depth.vs"
    PLATFORMER_SHADOW_DEPTH_VS)
file(TO_CMAKE_PATH
    "${CMAKE_SOURCE_DIR}/game/assets/source/shaders/shadow_depth.fs"
    PLATFORMER_SHADOW_DEPTH_FS)
file(TO_CMAKE_PATH
    "${CMAKE_SOURCE_DIR}/game/assets/source/textures/test_checker.png"
    PLATFORMER_TEST_CHECKER_PNG)
add_executable(WorldLightingResourcesTest
    game/source/platform/RuntimePaths.cpp
    game/source/platform/RuntimePaths.h
    game/source/render/LightingEnvironment.cpp
    game/source/render/LightingEnvironment.h
    game/source/render/LoadedModelMaterials.h
    game/source/render/TerrainMesh.cpp
    game/source/render/TerrainMesh.h
    game/source/render/WorldLighting.cpp
    game/source/render/WorldLighting.h
    game/source/render/WorldLightingResourcesTest.cpp
    game/source/world/Terrain.h
    game/source/world/TerrainGeometry.h
    game/source/world/TerrainMaterialShading.h
    game/source/world/TerrainSculpt.h
    game/source/world/TerrainPaint.h
    game/source/assets/RuntimePng.h
    game/source/assets/RuntimePngResolve.h
)
if(WIN32)
    target_sources(WorldLightingResourcesTest PRIVATE game/source/platform/RuntimePathsWindows.cpp)
else()
    target_sources(WorldLightingResourcesTest PRIVATE game/source/platform/RuntimePathsPosix.cpp)
endif()
target_include_directories(WorldLightingResourcesTest PRIVATE game/source)
target_link_libraries(WorldLightingResourcesTest PRIVATE raylib)
if(MSVC)
    target_compile_options(WorldLightingResourcesTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(WorldLightingResourcesTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(WIN32)
    target_link_libraries(WorldLightingResourcesTest PRIVATE ole32 shell32 uuid)
endif()
target_compile_definitions(WorldLightingResourcesTest PRIVATE
    "PLATFORMER_WORLD_LIT_VS=\"${PLATFORMER_WORLD_LIT_VS}\""
    "PLATFORMER_WORLD_LIT_FS=\"${PLATFORMER_WORLD_LIT_FS}\""
    "PLATFORMER_SHADOW_DEPTH_VS=\"${PLATFORMER_SHADOW_DEPTH_VS}\""
    "PLATFORMER_SHADOW_DEPTH_FS=\"${PLATFORMER_SHADOW_DEPTH_FS}\""
    "PLATFORMER_TEST_CHECKER_PNG=\"${PLATFORMER_TEST_CHECKER_PNG}\""
)

# Milestone 40 Phase A: Cook, Stage & Reload orchestration without ImGui or
# HostProcess. Shares enable helpers with EditorWorkspaceTest.
add_executable(CookStageReloadWorkflowTest
    game/source/editor/CookStageReloadWorkflow.cpp
    game/source/editor/CookStageReloadWorkflow.h
    game/source/editor/EditorWorkspace.cpp
    game/source/editor/EditorWorkspace.h
    game/source/editor/CookStageReloadWorkflowTest.cpp
)
target_include_directories(CookStageReloadWorkflowTest PRIVATE game/source)
if(MSVC)
    target_compile_options(CookStageReloadWorkflowTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(CookStageReloadWorkflowTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
