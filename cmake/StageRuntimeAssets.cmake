# Canonical runtime asset staging. Invoked as:
#   cmake
#     -DPLATFORMER_COOKED_DIR=<abs cooked root>
#     -DPLATFORMER_STAGE_DEST=<abs destination assets root>
#     -P cmake/StageRuntimeAssets.cmake
#
# Optional test seam:
#   -DPLATFORMER_STAGE_ASSET_LIST=rel/a;rel/b
# When unset, cmake/RuntimeAssets.cmake is the inventory.
#
# Does not cook, compile, link, or delete stale destination files.

cmake_minimum_required(VERSION 3.25)

if(NOT DEFINED PLATFORMER_COOKED_DIR OR PLATFORMER_COOKED_DIR STREQUAL "")
    message(FATAL_ERROR "PLATFORMER_COOKED_DIR is required")
endif()
if(NOT DEFINED PLATFORMER_STAGE_DEST OR PLATFORMER_STAGE_DEST STREQUAL "")
    message(FATAL_ERROR "PLATFORMER_STAGE_DEST is required")
endif()

file(TO_CMAKE_PATH "${PLATFORMER_COOKED_DIR}" PLATFORMER_COOKED_DIR)
file(TO_CMAKE_PATH "${PLATFORMER_STAGE_DEST}" PLATFORMER_STAGE_DEST)

if(NOT IS_DIRECTORY "${PLATFORMER_COOKED_DIR}")
    message(FATAL_ERROR "cooked root is not a directory: ${PLATFORMER_COOKED_DIR}")
endif()

if(DEFINED PLATFORMER_STAGE_ASSET_LIST AND NOT PLATFORMER_STAGE_ASSET_LIST STREQUAL "")
    set(PLATFORMER_RUNTIME_ASSETS ${PLATFORMER_STAGE_ASSET_LIST})
else()
    include("${CMAKE_CURRENT_LIST_DIR}/RuntimeAssets.cmake")
    # M47: imported static models are extra cooked files, not required inventory.
    # Configure-time still uses only cmake/RuntimeAssets.cmake. Each staging run
    # discovers additional cooked models/*.glb so Cook & Stage can ship imports
    # without editing the required list.
    file(GLOB _platformer_extra_cooked_models
        RELATIVE "${PLATFORMER_COOKED_DIR}"
        "${PLATFORMER_COOKED_DIR}/models/*.glb")
    list(SORT _platformer_extra_cooked_models)
    foreach(_platformer_extra_model IN LISTS _platformer_extra_cooked_models)
        if(_platformer_extra_model STREQUAL "")
            continue()
        endif()
        file(TO_CMAKE_PATH "${_platformer_extra_model}" _platformer_extra_model)
    list(FIND PLATFORMER_RUNTIME_ASSETS "${_platformer_extra_model}" _platformer_extra_index)
        if(_platformer_extra_index EQUAL -1)
            list(APPEND PLATFORMER_RUNTIME_ASSETS "${_platformer_extra_model}")
        endif()
    endforeach()
    unset(_platformer_extra_cooked_models)
    unset(_platformer_extra_model)
    unset(_platformer_extra_index)
endif()

message(STATUS "Staging runtime assets")
message(STATUS "cooked: ${PLATFORMER_COOKED_DIR}")
message(STATUS "destination: ${PLATFORMER_STAGE_DEST}")

foreach(relative IN LISTS PLATFORMER_RUNTIME_ASSETS)
    if(relative STREQUAL "")
        continue()
    endif()
    set(source "${PLATFORMER_COOKED_DIR}/${relative}")
    set(destination "${PLATFORMER_STAGE_DEST}/${relative}")
    if(NOT EXISTS "${source}")
        message(FATAL_ERROR "missing cooked asset: ${relative}")
    endif()
    cmake_path(GET relative PARENT_PATH relative_dir)
    if(relative_dir STREQUAL "")
        set(destination_dir "${PLATFORMER_STAGE_DEST}")
    else()
        set(destination_dir "${PLATFORMER_STAGE_DEST}/${relative_dir}")
    endif()
    file(MAKE_DIRECTORY "${destination_dir}")
    file(COPY_FILE "${source}" "${destination}" ONLY_IF_DIFFERENT)
    message(STATUS "staged ${relative}")
endforeach()

list(LENGTH PLATFORMER_RUNTIME_ASSETS _platformer_stage_count)
message(STATUS "Staging complete (${_platformer_stage_count} files)")
