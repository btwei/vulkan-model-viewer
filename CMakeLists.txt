# Copyright (c) 2025 Benjamin Wei
# This code is licensed under the MIT license (see http://opensource.org/licenses/MIT)

cmake_minimum_required(VERSION 3.15)
project(ModelViewer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Find packages
find_package(Vulkan REQUIRED)

# Add SDL as a subdirectory
add_subdirectory(external/SDL EXCLUDE_FROM_ALL)

# Add Dear ImGUI as a library 
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})

target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui PRIVATE SDL3::SDL3)

target_link_libraries(${PROJECT_NAME} PRIVATE imgui)

# Add VulkanMemoryAllocator as a library
add_library(VulkanMemoryAllocator INTERFACE)
target_include_directories(VulkanMemoryAllocator INTERFACE ${CMAKE_SOURCE_DIR}/external/VulkanMemoryAllocator/include)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${Vulkan_LIBRARIES}
        SDL3::SDL3
        VulkanMemoryAllocator
)