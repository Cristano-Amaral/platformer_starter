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

# M103 uses raylib's GLB vertex bone streams and per-draw bone-matrix upload.
# The engine owns playback/state selection and the project shaders own skinning.
target_compile_definitions(raylib PRIVATE SUPPORT_GPU_SKINNING=1)
