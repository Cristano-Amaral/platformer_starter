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

add_executable(Platformer3D
    game/source/Main.cpp
    game/source/core/Application.cpp
    game/source/core/Application.h
    game/source/core/Vec3.h
    game/source/gameplay/PlatformerCamera.h
    game/source/gameplay/Player.cpp
    game/source/gameplay/Player.h
    game/source/input/Input.cpp
    game/source/input/Input.h
    game/source/input/InputState.h
    game/source/platform/Time.cpp
    game/source/platform/Time.h
    game/source/platform/Window.cpp
    game/source/platform/Window.h
    game/source/render/Renderer.cpp
    game/source/render/Renderer.h
    game/source/world/Collision.cpp
    game/source/world/Collision.h
    game/source/world/GreyboxWorld.h
)

target_include_directories(Platformer3D PRIVATE game/source)
target_link_libraries(Platformer3D PRIVATE raylib)

if(MSVC)
    target_compile_options(Platformer3D PRIVATE /W4 /permissive-)
else()
    target_compile_options(Platformer3D PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Config-specific macros. Optimization/symbols for Development come from
# CMAKE_<LANG>_FLAGS_DEVELOPMENT; MSVC /DEBUG comes from CMAKE_EXE_LINKER_FLAGS_DEVELOPMENT.
target_compile_definitions(Platformer3D PRIVATE
    $<$<CONFIG:Debug>:GAME_DEBUG=1;GAME_DEVELOPMENT_TOOLS=1>
    $<$<CONFIG:Development>:GAME_DEVELOPMENT=1;GAME_DEVELOPMENT_TOOLS=1>
    $<$<CONFIG:Release>:GAME_RELEASE=1>
)

set_target_properties(Platformer3D PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
