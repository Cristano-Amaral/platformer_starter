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
    game/source/editor/EditorCamera.cpp
    game/source/editor/EditorCamera.h
    game/source/editor/EditorHierarchy.h
    game/source/editor/EditorInput.h
    game/source/editor/EditorMath.h
    game/source/editor/EditorPicking.cpp
    game/source/editor/EditorPicking.h
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/editor/LevelEditor.cpp
    game/source/editor/LevelEditor.h
    game/source/gameplay/PlatformerCamera.cpp
    game/source/gameplay/PlatformerCamera.h
    game/source/gameplay/Player.cpp
    game/source/gameplay/Player.h
    game/source/gameplay/CollectibleRunState.h
    game/source/gameplay/LevelCompletionState.h
    game/source/gameplay/RespawnState.h
    game/source/gameplay/RunTimerState.h
    game/source/gameplay/SessionBestTimeState.h
    game/source/input/Input.cpp
    game/source/input/Input.h
    game/source/input/InputState.h
    game/source/physics/PhysicsWorld.cpp
    game/source/physics/PhysicsWorld.h
    game/source/persistence/BestTimeSave.cpp
    game/source/persistence/BestTimeSave.h
    game/source/platform/FileReplace.h
    game/source/platform/RuntimePaths.cpp
    game/source/platform/RuntimePaths.h
    game/source/platform/Time.cpp
    game/source/platform/Time.h
    game/source/platform/Window.cpp
    game/source/platform/Window.h
    game/source/render/Renderer.cpp
    game/source/render/Renderer.h
    game/source/render/CameraView.h
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
    game/source/world/LevelGoal.h
    game/source/world/LevelFile.cpp
    game/source/world/LevelFile.h
    game/source/world/LevelWriter.cpp
    game/source/world/LevelWriter.h
    game/source/world/MovingPlatform.h
    game/source/world/RespawnWorld.h
    game/source/world/Slope.h
)

if(WIN32)
    target_sources(Platformer3D PRIVATE
        game/source/platform/FileReplaceWindows.cpp
        game/source/platform/RuntimePathsWindows.cpp)
else()
    target_sources(Platformer3D PRIVATE
        game/source/platform/FileReplacePosix.cpp
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
target_compile_definitions(Platformer3D PRIVATE
    $<$<CONFIG:Development>:PLATFORMER_ENABLE_LEVEL_AUTHORING=1>
    "$<$<CONFIG:Development>:PLATFORMER_AUTHORING_SOURCE_ROOT=\"${PLATFORMER_AUTHORING_SOURCE_ROOT}\">"
)

set_target_properties(Platformer3D PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Cooker owns source -> cooked. CMake only stages cooked files next to the exe:
#   <exe dir>/assets/textures/test_checker.png
#   <exe dir>/assets/models/test_static.glb
#   <exe dir>/assets/models/test_authored.glb
#   <exe dir>/assets/models/test_textured.glb
#   <exe dir>/assets/levels/level_01.level
# Do not stage textures/test_textured_basecolor.png; that PNG is Blender authoring input only.
# Runtime lookup uses the executable directory, not the process CWD.
# The staged level file is required at runtime; Application loads it once in Initialize.
set(PLATFORMER_COOKED_DIR "${CMAKE_SOURCE_DIR}/game/assets/cooked")
set(PLATFORMER_RUNTIME_ASSETS
    textures/test_checker.png
    models/test_static.glb
    models/test_authored.glb
    models/test_textured.glb
    levels/level_01.level
)

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

set(PLATFORMER_STAGE_ASSET_COMMANDS)
foreach(relative IN LISTS PLATFORMER_RUNTIME_ASSETS)
    cmake_path(GET relative PARENT_PATH relative_dir)
    list(APPEND PLATFORMER_STAGE_ASSET_COMMANDS
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_FILE_DIR:Platformer3D>/assets/${relative_dir}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${PLATFORMER_COOKED_DIR}/${relative}"
            "$<TARGET_FILE_DIR:Platformer3D>/assets/${relative}"
    )
endforeach()

add_custom_command(TARGET Platformer3D POST_BUILD
    ${PLATFORMER_STAGE_ASSET_COMMANDS}
    COMMENT "Staging cooked runtime assets next to the executable"
)

# Level Format v1 parser/writer test binary. Not shipped. Does not link
# raylib/Jolt/ImGui. The platform file-replacement TU is the only OS-specific
# dependency, needed because SaveLevelFile promotes through that boundary
# instead of duplicating ReplaceFileW/MoveFileExW.
add_executable(LevelFileTest
    game/source/platform/FileReplace.h
    game/source/world/LevelFile.cpp
    game/source/world/LevelFile.h
    game/source/world/LevelFileTest.cpp
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
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

# Focused harness for the M32 editor Apply Preview rebuild cycle. Not shipped.
# Links Jolt because that is exactly what is being validated; no raylib/ImGui.
# It only reads the canonical level and never writes a file.
add_executable(PhysicsRebuildTest
    game/source/physics/PhysicsRebuildTest.cpp
    game/source/physics/PhysicsWorld.cpp
    game/source/physics/PhysicsWorld.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
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

# Milestone 33 picking/selection math. Not shipped. No window, raylib,
# Jolt, or ImGui: the editor must pick authored proxies with project-owned CPU
# math rather than Jolt raycasts.
add_executable(EditorPickingTest
    game/source/editor/EditorCamera.cpp
    game/source/editor/EditorCamera.h
    game/source/editor/EditorHierarchy.h
    game/source/editor/EditorInput.h
    game/source/editor/EditorMath.h
    game/source/editor/EditorPicking.cpp
    game/source/editor/EditorPicking.h
    game/source/editor/EditorPickingTest.cpp
    game/source/editor/EditorSelection.cpp
    game/source/editor/EditorSelection.h
    game/source/render/CameraView.h
    game/source/world/LevelDefinition.cpp
    game/source/world/LevelDefinition.h
)
target_include_directories(EditorPickingTest PRIVATE game/source)
if(MSVC)
    target_compile_options(EditorPickingTest PRIVATE /W4 /permissive-)
else()
    target_compile_options(EditorPickingTest PRIVATE -Wall -Wextra -Wpedantic)
endif()
