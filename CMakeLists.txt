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

# Add subdirectories
add_subdirectory(external/SDL EXCLUDE_FROM_ALL)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${Vulkan_LIBRARIES}
        SDL3::SDL3
)