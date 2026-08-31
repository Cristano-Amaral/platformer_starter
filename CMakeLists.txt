cmake_minimum_required(VERSION 3.25)

project(Platformer3D VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Multi-config generators (Visual Studio, Xcode) use these directly.
if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Development;Release" CACHE STRING "" FORCE)
endif()

add_executable(Platformer3D
    game/source/Main.cpp
    game/source/core/App.cpp
    game/source/core/App.h
)

target_include_directories(Platformer3D PRIVATE game/source)

if(MSVC)
    target_compile_options(Platformer3D PRIVATE /W4 /permissive-)
else()
    target_compile_options(Platformer3D PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Development: symbols + optimization + development tools.
target_compile_definitions(Platformer3D PRIVATE
    $<$<CONFIG:Debug>:GAME_DEBUG=1;GAME_DEVELOPMENT_TOOLS=1>
    $<$<CONFIG:Development>:GAME_DEVELOPMENT=1;GAME_DEVELOPMENT_TOOLS=1>
    $<$<CONFIG:Release>:GAME_RELEASE=1>
)

if(MSVC)
    target_compile_options(Platformer3D PRIVATE
        $<$<CONFIG:Development>:/O2;/Zi>
    )
    target_link_options(Platformer3D PRIVATE
        $<$<CONFIG:Development>:/DEBUG>
    )
else()
    target_compile_options(Platformer3D PRIVATE
        $<$<CONFIG:Development>:-O2;-g>
    )
endif()

set_target_properties(Platformer3D PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
