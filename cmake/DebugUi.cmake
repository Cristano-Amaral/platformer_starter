# Pin Dear ImGui and the rlImGui integration that matches raylib 6.0.
# Never track main/master.
set(PLATFORMER3D_IMGUI_VERSION v1.92.7)
set(PLATFORMER3D_RLIMGUI_TAG Raylib_6_0)

include(FetchContent)

if(POLICY CMP0169)
    # FetchContent_Populate is the intended path here because imgui/rlImGui
    # ship no CMakeLists.txt. Keep the 3.25-compatible populate flow.
    cmake_policy(SET CMP0169 OLD)
endif()

# imgui has no CMakeLists.txt, so populate the source tree and define our own
# static library instead of FetchContent_MakeAvailable.
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG ${PLATFORMER3D_IMGUI_VERSION}
    GIT_SHALLOW TRUE
)

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
set_target_properties(imgui PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    EXCLUDE_FROM_ALL TRUE
)
set_property(TARGET imgui PROPERTY EXCLUDE_FROM_DEFAULT_BUILD_RELEASE TRUE)

FetchContent_Declare(
    rlimgui
    GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
    GIT_TAG ${PLATFORMER3D_RLIMGUI_TAG}
    GIT_SHALLOW TRUE
)

FetchContent_GetProperties(rlimgui)
if(NOT rlimgui_POPULATED)
    FetchContent_Populate(rlimgui)
endif()

add_library(rlimgui STATIC
    ${rlimgui_SOURCE_DIR}/rlImGui.cpp
)
target_include_directories(rlimgui PUBLIC
    ${rlimgui_SOURCE_DIR}
    ${rlimgui_SOURCE_DIR}/extras
)
# Metrics panel does not need Font Awesome; skip the bundled icon font.
target_compile_definitions(rlimgui PUBLIC NO_FONT_AWESOME)
target_link_libraries(rlimgui PUBLIC imgui raylib)
set_target_properties(rlimgui PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    EXCLUDE_FROM_ALL TRUE
)
set_property(TARGET rlimgui PROPERTY EXCLUDE_FROM_DEFAULT_BUILD_RELEASE TRUE)
