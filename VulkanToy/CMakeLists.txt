file(GLOB_RECURSE VULKAN_TOY_FILES CONFIGURE_DEPENDS "include/*.h" "src/*.cpp")

add_library(VulkanToy ${VULKAN_TOY_FILES})

target_include_directories(VulkanToy PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(VulkanToy PUBLIC ${Vulkan_INCLUDE_DIR})
target_include_directories(VulkanToy PUBLIC ${STB_INCLUDE_PATH})

message(STATUS ${Vulkan_INCLUDE_DIRS})
target_link_libraries(VulkanToy PUBLIC Vulkan::Vulkan)
target_link_libraries(VulkanToy PUBLIC glfw)
target_link_libraries(VulkanToy PUBLIC glm::glm)
target_link_libraries(VulkanToy PUBLIC tinyobjloader)
target_link_libraries(VulkanToy PUBLIC spdlog_header_only)
target_link_libraries(VulkanToy PUBLIC ImGuiDocking)

target_precompile_headers(VulkanToy PUBLIC "include/Pch.h")