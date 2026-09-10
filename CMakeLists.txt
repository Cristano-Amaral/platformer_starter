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
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
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
    game/source/editor/CookStageReloadWorkflow.cpp
    game/source/editor/CookStageReloadWorkflow.h
    game/source/editor/EditorCamera.cpp
    game/source/editor/EditorCamera.h
    game/source/editor/ContentBrowser.cpp
    game/source/editor/ContentBrowser.h
    game/source/editor/ContentBrowserView.cpp
    game/source/editor/ContentBrowserView.h
    game/source/editor/StaticModelThumbnailCache.cpp
    game/source/editor/StaticModelThumbnailCache.h
    game/source/editor/StaticModelFraming.cpp
    game/source/editor/StaticModelFraming.h
    game/source/editor/EditorGizmo.cpp
    game/source/editor/EditorGizmo.h
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
    game/source/editor/EditorBuildPreference.cpp
    game/source/editor/EditorBuildPreference.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/EditorToolCommands.cpp
    game/source/editor/EditorToolCommands.h
    game/source/editor/EditorToolRunner.cpp
    game/source/editor/EditorToolRunner.h
    game/source/editor/EditorWorkspace.cpp
    game/source/editor/EditorWorkspace.h
    game/source/editor/LevelEditor.cpp
    game/source/editor/LevelEditor.h
    game/source/editor/RuntimeLevelReload.cpp
    game/source/editor/RuntimeLevelReload.h
    game/source/gameplay/PlatformerCamera.cpp
    game/source/gameplay/PlatformerCamera.h
    game/source/gameplay/Player.cpp
    game/source/gameplay/Player.h
    game/source/gameplay/CollectibleRunState.h
    game/source/gameplay/Inventory.cpp
    game/source/gameplay/Inventory.h
    game/source/gameplay/InventoryUi.cpp
    game/source/gameplay/InventoryUi.h
    game/source/gameplay/ItemPickupRuntime.cpp
    game/source/gameplay/ItemPickupRuntime.h
    game/source/gameplay/DoorLockRuntime.cpp
    game/source/gameplay/DoorLockRuntime.h
    game/source/world/ItemPickup.h
    game/source/gameplay/LevelCompletionState.h
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
    game/source/render/Renderer.cpp
    game/source/render/Renderer.h
    game/source/render/CameraView.h
    game/source/render/StaticModelThumbnail.cpp
    game/source/render/StaticModelThumbnail.h
    game/source/render/StaticModelPreview.cpp
    game/source/render/StaticModelPreview.h
    game/source/render/StaticModelScene.cpp
    game/source/render/StaticModelScene.h
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
    game/source/world/StaticProp.h
    game/source/world/StaticProp.cpp
    game/source/world/LevelGoal.h
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
#   <exe dir>/assets/models/test_static.glb
#   <exe dir>/assets/models/test_authored.glb
#   <exe dir>/assets/models/test_textured.glb
#   <exe dir>/assets/levels/level_01.level
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
target_compile_definitions(StaticGlbImportTest PRIVATE
    "PLATFORMER_TEST_STATIC_GLB=\"${PLATFORMER_TEST_STATIC_GLB}\""
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
    game/source/editor/ContentBrowser.cpp
    game/source/editor/ContentBrowser.h
    game/source/editor/ContentBrowserTest.cpp
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
target_include_directories(ContentBrowserTest PRIVATE game/source)
if(MSVC)
    target_compile_options(ContentBrowserTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(ContentBrowserTest PRIVATE -Wall -Wextra -Wpedantic)
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
    game/source/editor/ContentBrowser.cpp
    game/source/editor/ContentBrowser.h
    game/source/editor/ContentBrowserView.cpp
    game/source/editor/ContentBrowserView.h
    game/source/editor/ContentBrowserThumbnailTest.cpp
    game/source/editor/EditorLayout.cpp
    game/source/editor/EditorLayout.h
    game/source/editor/EditorMath.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
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
    target_sources(ContentBrowserThumbnailTest PRIVATE game/source/platform/RuntimePathsWindows.cpp)
else()
    target_sources(ContentBrowserThumbnailTest PRIVATE game/source/platform/RuntimePathsPosix.cpp)
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
    game/source/editor/ContentBrowser.cpp
    game/source/editor/ContentBrowser.h
    game/source/editor/ContentBrowserView.cpp
    game/source/editor/ContentBrowserView.h
    game/source/editor/EditorLayout.cpp
    game/source/editor/EditorLayout.h
    game/source/editor/EditorMath.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/EditorWorkspace.cpp
    game/source/editor/EditorWorkspace.h
    game/source/editor/StaticModelFraming.cpp
    game/source/editor/StaticModelFraming.h
    game/source/editor/StaticModelPreviewTest.cpp
    game/source/editor/StaticModelThumbnailCache.cpp
    game/source/editor/StaticModelThumbnailCache.h
    game/source/platform/RuntimePaths.cpp
    game/source/platform/RuntimePaths.h
    game/source/render/CameraView.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
if(WIN32)
    target_sources(StaticModelPreviewTest PRIVATE game/source/platform/RuntimePathsWindows.cpp)
else()
    target_sources(StaticModelPreviewTest PRIVATE game/source/platform/RuntimePathsPosix.cpp)
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
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
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
target_compile_definitions(LevelFileTest PRIVATE
    "PLATFORMER_LEVEL01_SOURCE_PATH=\"${PLATFORMER_LEVEL01_SOURCE_PATH}\""
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
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/gameplay/Inventory.cpp
    game/source/gameplay/Inventory.h
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
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/PressurePlate.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
    game/source/world/LevelFile.cpp
    game/source/world/LevelFile.h
)
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
    game/source/editor/EditorCamera.cpp
    game/source/editor/EditorCamera.h
    game/source/editor/EditorHierarchy.h
    game/source/editor/EditorInput.h
    game/source/editor/EditorMath.h
    game/source/editor/EditorGizmo.cpp
    game/source/editor/EditorGizmo.h
    game/source/editor/EditorPicking.cpp
    game/source/editor/EditorPicking.h
    game/source/editor/EditorPickingTest.cpp
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
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

# Milestone 34 translation-gizmo math and editor-layout path. No window, raylib,
# Jolt, or ImGui: live drag/render is exercised by Platformer3D, not this test.
add_executable(EditorGizmoTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/editor/AuthoredObjectLifecycle.cpp
    game/source/editor/AuthoredObjectLifecycle.h
    game/source/editor/EditorGizmo.cpp
    game/source/editor/EditorGizmo.h
    game/source/editor/EditorGizmoTest.cpp
    game/source/editor/EditorLayout.cpp
    game/source/editor/EditorLayout.h
    game/source/editor/EditorMath.h
    game/source/editor/EditorNudge.cpp
    game/source/editor/EditorNudge.h
    game/source/editor/EditorPicking.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
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
    game/source/editor/AuthoredObjectLifecycleTest.cpp
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
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

# Milestone 42: Object Palette placement mode and candidate resolution. No ImGui.
add_executable(EditorPlacementTest
    game/source/assets/StaticGlb.cpp
    game/source/assets/StaticGlb.h
    game/source/editor/AuthoredObjectLifecycle.cpp
    game/source/editor/AuthoredObjectLifecycle.h
    game/source/editor/EditorPicking.cpp
    game/source/editor/EditorPicking.h
    game/source/editor/EditorPlacement.cpp
    game/source/editor/EditorPlacement.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
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
    game/source/editor/EditorGizmo.cpp
    game/source/editor/EditorGizmo.h
    game/source/editor/EditorHierarchy.h
    game/source/editor/EditorPicking.cpp
    game/source/editor/EditorPicking.h
    game/source/editor/EditorPlacement.cpp
    game/source/editor/EditorPlacement.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/EditorWorkspace.cpp
    game/source/editor/EditorWorkspace.h
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
    game/source/editor/EditorBuildPreference.cpp
    game/source/editor/EditorBuildPreference.h
    game/source/editor/EditorCamera.cpp
    game/source/editor/EditorCamera.h
    game/source/editor/EditorGizmo.cpp
    game/source/editor/EditorGizmo.h
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
    game/source/editor/CanonicalSceneCleanupTest.cpp
    game/source/editor/EditorHierarchy.h
    game/source/editor/EditorPicking.cpp
    game/source/editor/EditorPicking.h
    game/source/editor/EditorPlacement.cpp
    game/source/editor/EditorPlacement.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
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
    game/source/editor/AuthoredLifecycleIntegrationTest.cpp
    game/source/editor/EditorGizmo.cpp
    game/source/editor/EditorGizmo.h
    game/source/editor/EditorHierarchy.h
    game/source/editor/EditorPicking.cpp
    game/source/editor/EditorPicking.h
    game/source/editor/EditorPlacement.cpp
    game/source/editor/EditorPlacement.h
    game/source/editor/StaticPropPlacement.cpp
    game/source/editor/StaticPropPlacement.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/LevelEditor.cpp
    game/source/editor/LevelEditor.h
    game/source/editor/ContentBrowser.h
    game/source/editor/EditorWorkspace.cpp
    game/source/editor/EditorWorkspace.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
    game/source/world/StaticProp.cpp
    game/source/world/StaticProp.h
)
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
    game/source/editor/EditorMath.h
    game/source/editor/EditorPicking.cpp
    game/source/editor/EditorPicking.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
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
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
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
