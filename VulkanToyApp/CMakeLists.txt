file(GLOB_RECURSE VulkanToyAppSrc CONFIGURE_DEPENDS *.h *.cpp)
add_executable(Application ${VulkanToyAppSrc})

target_link_libraries(Application PUBLIC Vulkan::Vulkan)
target_link_libraries(Application PUBLIC glfw)
target_link_libraries(Application PUBLIC glm::glm)

target_include_directories(Application PUBLIC Vulkan_INCLUDE_DIRS)

if(VULKAN_TOY_DEBUG)
    target_compile_definitions(Application PUBLIC TOY_DEBUG)
endif()