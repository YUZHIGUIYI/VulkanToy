set(SHADER_DIR "${PROJECT_SOURCE_DIR}/data/shaders" CACHE FILEPATH "shader directory" FORCE)
set(SPV_DIR "${PROJECT_SOURCE_DIR}/data/shaders/spirv" CACHE FILEPATH "spirv directory" FORCE)

file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS "${SHADER_DIR}/*.frag" "${SHADER_DIR}/*.vert" "${SHADER_DIR}/*.comp")

# Compile shaders
function(compile_shader TARGET_NAME)
    foreach(SHADER ${SHADER_FILES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        set(SPV_FILE "${SPV_DIR}/${SHADER_NAME}.spv")
        add_custom_command(OUTPUT ${SPV_FILE}
            COMMAND ${GLSLC_PROGRAM} ${SHADER} -o ${SPV_FILE}
            DEPENDS ${SHADER}
            COMMENT "Compiling ${SHADER_NAME}")
        list(APPEND ALL_GENERATED_SPV_FILES ${SPV_FILE})
    endforeach()
    add_custom_target(VK_SHADER ALL DEPENDS ${ALL_GENERATED_SPV_FILES} SOURCES ${SHADER_FILES})
    add_dependencies(${TARGET_NAME} VK_SHADER)
endfunction()