cmake_minimum_required(VERSION 3.10)
project(music_playr)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(music_playr
  src/main.cpp
  src/audio.cpp
  src/player.cpp
  src/glad.c
  src/ImGui/imgui.cpp
  src/ImGui/imgui_draw.cpp
  src/ImGui/imgui_demo.cpp
  src/ImGui/imgui_widgets.cpp
  src/ImGui/imgui_tables.cpp
  src/ImGui/backends/imgui_impl_glfw.cpp
  src/ImGui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(music_playr PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include/ImGui
  ${CMAKE_CURRENT_SOURCE_DIR}/include/
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

target_link_libraries(music_playr PRIVATE
    glfw
    OpenGL::GL
)

if(NOT CMAKE_BUILD_TYPE)
  set(default_build_type "Debug")
  message(STATUS "Set the build type to `${default_build_type}` as none was specified.")
  set(CMAKE_BUILD_TYPE ${default_build_type} CACHE STRING "Chooce the build type." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
  "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()
message(STATUS "music_playr Build Type: ${CMAKE_BUILD_TYPE}")

# Set the version for music_playr
set(music_playr_Version_Major 0)
set(music_playr_Version_Minor 1)
set(music_playr_Version_Patch 0)
set(music_playr_Version_Status "-dev")
set(PROJECT_VERSION
  "${music_playr_Version_Major}.${music_playr_Version_Minor}.${music_playr_Version_Patch}${music_playr_Version_Status}"
)
message(STATUS "${PROJECT_NAME} version: ${PROJECT_VERSION}")
