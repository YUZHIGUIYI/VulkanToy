message(STATUS "Run glslc to compile shaders...")
execute_process(COMMAND ${GLSLC_PROGRAM} ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/shader_base.vert -o ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vert.spv)
execute_process(COMMAND ${GLSLC_PROGRAM} ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/shader_base.frag -o ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/frag.spv)
message(STATUS "Compile shader finished...")

file(GLOB_RECURSE VulkanToyAppSrc CONFIGURE_DEPENDS *.h *.cpp)
add_executable(Application ${VulkanToyAppSrc})

target_link_libraries(Application PUBLIC Vulkan::Vulkan)
target_link_libraries(Application PUBLIC glfw)
target_link_libraries(Application PUBLIC glm::glm)

target_include_directories(Application PUBLIC Vulkan_INCLUDE_DIRS)
target_include_directories(Application PUBLIC ${STB_INCLUDE_PATH})

if(VULKAN_TOY_DEBUG)
    target_compile_definitions(Application PUBLIC TOY_DEBUG)
endif()

add_custom_command(TARGET Application
        POST_BUILD
        COMMAND
        ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets/textures ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/textures)