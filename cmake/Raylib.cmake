# Pin raylib to a released tag. Never track master.
set(PLATFORMER3D_RAYLIB_VERSION 6.0)

include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG ${PLATFORMER3D_RAYLIB_VERSION}
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(raylib)
