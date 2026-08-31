# Pin Jolt Physics to a released tag. Never track master/main/latest.
set(PLATFORMER3D_JOLT_VERSION v5.6.0)

include(FetchContent)

# Match the DLL MSVC runtime used by the rest of the project (raylib default).
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)

# Do not let Jolt replace this project's Debug/Release/Development flags.
set(OVERRIDE_CXX_FLAGS OFF CACHE BOOL "" FORCE)

# Avoid LTO mismatches with targets that do not enable IPO.
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)

# Do not treat Jolt warnings as errors, and do not enable /Wall on dependents.
set(ENABLE_ALL_WARNINGS OFF CACHE BOOL "" FORCE)

# Samples/tests/viewer are already skipped when Jolt is a subproject; keep them off.
set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(TARGET_HELLO_WORLD OFF CACHE BOOL "" FORCE)
set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
set(TARGET_SAMPLES OFF CACHE BOOL "" FORCE)
set(TARGET_VIEWER OFF CACHE BOOL "" FORCE)

# Conservative x86 instruction set: SSE2 baseline, no extra desktop-only ISAs.
set(USE_SSE4_1 OFF CACHE BOOL "" FORCE)
set(USE_SSE4_2 OFF CACHE BOOL "" FORCE)
set(USE_AVX OFF CACHE BOOL "" FORCE)
set(USE_AVX2 OFF CACHE BOOL "" FORCE)
set(USE_AVX512 OFF CACHE BOOL "" FORCE)
set(USE_LZCNT OFF CACHE BOOL "" FORCE)
set(USE_TZCNT OFF CACHE BOOL "" FORCE)
set(USE_F16C OFF CACHE BOOL "" FORCE)
set(USE_FMADD OFF CACHE BOOL "" FORCE)

# No optional GPU/compute backends for this experiment.
set(JPH_USE_DX12 OFF CACHE BOOL "" FORCE)
set(JPH_USE_VK OFF CACHE BOOL "" FORCE)
set(JPH_USE_MTL OFF CACHE BOOL "" FORCE)
set(JPH_USE_CPU_COMPUTE OFF CACHE BOOL "" FORCE)

# Unused Jolt extras.
set(DEBUG_RENDERER_IN_DEBUG_AND_RELEASE OFF CACHE BOOL "" FORCE)
set(DEBUG_RENDERER_IN_DISTRIBUTION OFF CACHE BOOL "" FORCE)
set(PROFILER_IN_DEBUG_AND_RELEASE OFF CACHE BOOL "" FORCE)
set(PROFILER_IN_DISTRIBUTION OFF CACHE BOOL "" FORCE)
set(ENABLE_OBJECT_STREAM OFF CACHE BOOL "" FORCE)
set(ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(FLOATING_POINT_EXCEPTIONS_ENABLED OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    JoltPhysics
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG ${PLATFORMER3D_JOLT_VERSION}
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR Build
)

FetchContent_MakeAvailable(JoltPhysics)
