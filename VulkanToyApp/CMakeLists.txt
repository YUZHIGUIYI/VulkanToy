macro(CopyShaders target_name)
    execute_process(COMMAND ${GLSLC_PROGRAM} ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/shader_base.vert -o ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/vert.spv)
    execute_process(COMMAND ${GLSLC_PROGRAM} ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/shader_base.frag -o ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/frag.spv)
    execute_process(COMMAND ${GLSLC_PROGRAM} ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/shader_base.comp -o ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/comp.spv)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/vert.spv $<TARGET_FILE_DIR:${target_name}>)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/frag.spv $<TARGET_FILE_DIR:${target_name}>)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/comp.spv $<TARGET_FILE_DIR:${target_name}>)
endmacro(CopyShaders)

file(GLOB_RECURSE VulkanToyAppSrc CONFIGURE_DEPENDS *.h *.cpp)
add_executable(Application ${VulkanToyAppSrc})

target_link_libraries(Application PUBLIC Vulkan::Vulkan)
target_link_libraries(Application PUBLIC glfw)
target_link_libraries(Application PUBLIC glm::glm)
target_link_libraries(Application PUBLIC tinyobjloader)

target_include_directories(Application PUBLIC Vulkan_INCLUDE_DIRS)
target_include_directories(Application PUBLIC ${STB_INCLUDE_PATH})

if(VULKAN_TOY_DEBUG)
    target_compile_definitions(Application PUBLIC TOY_DEBUG)
endif()

message(STATUS "Run GLSLC to compile shaders!")
CopyShaders(Application)
message(STATUS "Compile shader finished!")

add_custom_command(TARGET Application
        POST_BUILD
        COMMAND
        ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets/textures $<TARGET_FILE_DIR:Application>/textures)
add_custom_command(TARGET Application
        POST_BUILD
        COMMAND
        ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets/models $<TARGET_FILE_DIR:Application>/models)