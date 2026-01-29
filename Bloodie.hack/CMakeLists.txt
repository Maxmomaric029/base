cmake_minimum_required(VERSION 3.14)
project(BloodieHack)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG        docking
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_DIR ${imgui_SOURCE_DIR})

file(GLOB SOURCES "src/*.cpp" "src/*.h")

list(APPEND SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx9.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
)

add_executable(BloodieHack ${SOURCES})

target_include_directories(BloodieHack PRIVATE src ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(BloodieHack d3d9)
