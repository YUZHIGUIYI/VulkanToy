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

file(GLOB_RECURSE SANDBOX_FILES CONFIGURE_DEPENDS include/*.h src/*.cpp)

add_executable(Sandbox ${SANDBOX_FILES})

target_link_libraries(Sandbox PUBLIC VulkanToy)

target_include_directories(Sandbox PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

message(STATUS "Run GLSLC to compile shaders!")
CopyShaders(Sandbox)
message(STATUS "Compile shader finished!")

add_custom_command(TARGET Sandbox
        POST_BUILD
        COMMAND
        ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets/textures $<TARGET_FILE_DIR:Sandbox>/textures)
add_custom_command(TARGET Sandbox
        POST_BUILD
        COMMAND
        ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets/models $<TARGET_FILE_DIR:Sandbox>/models)