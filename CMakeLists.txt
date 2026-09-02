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
    game/source/core/Vec3.h
    game/source/gameplay/PlatformerCamera.cpp
    game/source/gameplay/PlatformerCamera.h
    game/source/gameplay/Player.cpp
    game/source/gameplay/Player.h
    game/source/gameplay/LevelCompletionState.h
    game/source/gameplay/RespawnState.h
    game/source/input/Input.cpp
    game/source/input/Input.h
    game/source/input/InputState.h
    game/source/physics/PhysicsWorld.cpp
    game/source/physics/PhysicsWorld.h
    game/source/platform/RuntimePaths.cpp
    game/source/platform/RuntimePaths.h
    game/source/platform/Time.cpp
    game/source/platform/Time.h
    game/source/platform/Window.cpp
    game/source/platform/Window.h
    game/source/render/Renderer.cpp
    game/source/render/Renderer.h
    game/source/ui/debug/DebugMetrics.cpp
    game/source/ui/debug/DebugMetrics.h
    game/source/ui/debug/DebugUi.cpp
    game/source/ui/debug/DebugUi.h
    game/source/ui/debug/DebugUiBackend.cpp
    game/source/ui/debug/DebugUiBackend.h
    game/source/world/GreyboxWorld.h
    game/source/world/LevelGoal.h
    game/source/world/MovingPlatform.h
    game/source/world/RespawnWorld.h
    game/source/world/Slope.h
)

if(WIN32)
    target_sources(Platformer3D PRIVATE game/source/platform/RuntimePathsWindows.cpp)
else()
    target_sources(Platformer3D PRIVATE game/source/platform/RuntimePathsPosix.cpp)
endif()

target_include_directories(Platformer3D PRIVATE
    game/source
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:Development>>:${imgui_SOURCE_DIR}>
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:Development>>:${rlimgui_SOURCE_DIR}>
)
target_link_libraries(Platformer3D PRIVATE raylib Jolt)
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

set_target_properties(Platformer3D PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Cooker owns source -> cooked. CMake only stages cooked files next to the exe:
#   <exe dir>/assets/textures/test_checker.png
#   <exe dir>/assets/models/test_static.glb
#   <exe dir>/assets/models/test_authored.glb
#   <exe dir>/assets/models/test_textured.glb
# Do not stage textures/test_textured_basecolor.png; that PNG is Blender authoring input only.
# Runtime lookup uses the executable directory, not the process CWD.
set(PLATFORMER_COOKED_DIR "${CMAKE_SOURCE_DIR}/game/assets/cooked")
set(PLATFORMER_RUNTIME_ASSETS
    textures/test_checker.png
    models/test_static.glb
    models/test_authored.glb
    models/test_textured.glb
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
