cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

project(VulkanToy LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(VULKAN_TOY_DEBUG ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
    set(VULKAN_TOY_DEBUG OFF)
endif()

if(NOT CMAKE_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)

set(STB_INCLUDE_PATH ${CMAKE_CURRENT_LIST_DIR}/external/stb)

find_package(Vulkan REQUIRED)

find_program(GLSLC_PROGRAM glslc REQUIRED)

add_subdirectory(external/glfw)

add_subdirectory(external/glm)

add_subdirectory(external/tinyobjloader)

add_subdirectory(VulkanToyApp)