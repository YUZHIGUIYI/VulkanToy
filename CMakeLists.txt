cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

project(VulkanToy LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(VULKAN_TOY_DEBUG ON CACHE BOOL "Vulkan Debug ON" FORCE)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
    set(VULKAN_TOY_DEBUG OFF)
endif()

if(NOT CMAKE_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)

find_package(Vulkan REQUIRED)
find_program(GLSLC_PROGRAM glslc REQUIRED)

# Compile vertex and fragment shaders
execute_process(COMMAND ${GLSLC_PROGRAM} ${PROJECT_SOURCE_DIR}/data/shaders/PBRTexture.vert -o ${PROJECT_SOURCE_DIR}/data/shaders/spirv/PBRTexture.vert.spv)
execute_process(COMMAND ${GLSLC_PROGRAM} ${PROJECT_SOURCE_DIR}/data/shaders/PBRTexture.frag -o ${PROJECT_SOURCE_DIR}/data/shaders/spirv/PBRTexture.frag.spv)

execute_process(COMMAND ${GLSLC_PROGRAM} ${PROJECT_SOURCE_DIR}/data/shaders/SkyBox.vert -o ${PROJECT_SOURCE_DIR}/data/shaders/spirv/SkyBox.vert.spv)
execute_process(COMMAND ${GLSLC_PROGRAM} ${PROJECT_SOURCE_DIR}/data/shaders/SkyBox.frag -o ${PROJECT_SOURCE_DIR}/data/shaders/spirv/SkyBox.frag.spv)

execute_process(COMMAND ${GLSLC_PROGRAM} ${PROJECT_SOURCE_DIR}/data/shaders/GenBRDFLUT.vert -o ${PROJECT_SOURCE_DIR}/data/shaders/spirv/GenBRDFLUT.vert.spv)
execute_process(COMMAND ${GLSLC_PROGRAM} ${PROJECT_SOURCE_DIR}/data/shaders/GenBRDFLUT.frag -o ${PROJECT_SOURCE_DIR}/data/shaders/spirv/GenBRDFLUT.frag.spv)

execute_process(COMMAND ${GLSLC_PROGRAM} ${PROJECT_SOURCE_DIR}/data/shaders/FilterCube.vert -o ${PROJECT_SOURCE_DIR}/data/shaders/spirv/FilterCube.vert.spv)
execute_process(COMMAND ${GLSLC_PROGRAM} ${PROJECT_SOURCE_DIR}/data/shaders/IrradianceCube.frag -o ${PROJECT_SOURCE_DIR}/data/shaders/spirv/IrradianceCube.frag.spv)
execute_process(COMMAND ${GLSLC_PROGRAM} ${PROJECT_SOURCE_DIR}/data/shaders/PrefilterEnvMap.frag -o ${PROJECT_SOURCE_DIR}/data/shaders/spirv/PrefilterEnvMap.frag.spv)

execute_process(COMMAND ${GLSLC_PROGRAM} ${PROJECT_SOURCE_DIR}/data/shaders/PBR.frag -o ${PROJECT_SOURCE_DIR}/data/shaders/pbrtexture.frag.spv)

include(cmake/AddThirdParties.cmake)
add_subdirectory(VulkanToy)
add_subdirectory(Sandbox)