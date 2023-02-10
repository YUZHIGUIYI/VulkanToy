message(STATUS "Run glslc to compile shaders ...")
execute_process(COMMAND ${GLSLC_PROGRAM} ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/shader_base.vert -o ${PROJECT_SOURCE_DIR}/bin/vert.spv)
execute_process(COMMAND ${GLSLC_PROGRAM} ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/shader_base.frag -o ${PROJECT_SOURCE_DIR}/bin/frag.spv)
message(STATUS "Compile shader completed")

file(GLOB_RECURSE VulkanToyAppSrc CONFIGURE_DEPENDS *.h *.cpp)
add_executable(Application ${VulkanToyAppSrc})

target_link_libraries(Application PUBLIC Vulkan::Vulkan)
target_link_libraries(Application PUBLIC glfw)
target_link_libraries(Application PUBLIC glm::glm)

target_include_directories(Application PUBLIC Vulkan_INCLUDE_DIRS)

if(VULKAN_TOY_DEBUG)
    target_compile_definitions(Application PUBLIC TOY_DEBUG)
endif()