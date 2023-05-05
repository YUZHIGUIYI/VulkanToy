file(GLOB_RECURSE SANDBOX_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/include/*.h" "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")

add_executable(Sandbox ${SANDBOX_FILES})

target_link_libraries(Sandbox PUBLIC VulkanToy)

target_include_directories(Sandbox PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")