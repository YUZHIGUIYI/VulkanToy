# KTX library
file(GLOB_RECURSE KTX_HEADERS "{KTX_DIR}/*.h")
set(KTX_SOURCES
        ${KTX_DIR}/lib/texture.c
        ${KTX_DIR}/lib/hashlist.c
        ${KTX_DIR}/lib/checkheader.c
        ${KTX_DIR}/lib/swap.c
        ${KTX_DIR}/lib/memstream.c
        ${KTX_DIR}/lib/filestream.c)
add_library(KTX STATIC ${KTX_HEADERS} ${KTX_SOURCES})
target_link_libraries(KTX PUBLIC Vulkan::Vulkan)
target_include_directories(KTX PUBLIC ${Vulkan_INCLUDE_DIR})
target_include_directories(KTX PUBLIC ${KTX_DIR}/include)
target_include_directories(KTX PRIVATE ${KTX_DIR}/other_include)

# VulkanToy library
file(GLOB_RECURSE VULKAN_TOY_FILES CONFIGURE_DEPENDS "include/*.h" "src/*.cpp")

add_library(VulkanToy ${VULKAN_TOY_FILES})

target_include_directories(VulkanToy PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(VulkanToy PUBLIC ${Vulkan_INCLUDE_DIR})
target_include_directories(VulkanToy PUBLIC ${STB_INCLUDE_PATH})
target_include_directories(VulkanToy PUBLIC ${UUID_INCLUDE_PATH})

message(STATUS ${Vulkan_INCLUDE_DIRS})
target_link_libraries(VulkanToy PUBLIC Vulkan::Vulkan)
target_link_libraries(VulkanToy PUBLIC glfw)
target_link_libraries(VulkanToy PUBLIC glm::glm)
target_link_libraries(VulkanToy PUBLIC tinyobjloader)
target_link_libraries(VulkanToy PUBLIC spdlog_header_only)
target_link_libraries(VulkanToy PUBLIC ImGuiDocking)
target_link_libraries(VulkanToy PUBLIC assimp)
target_link_libraries(VulkanToy PUBLIC VulkanMemoryAllocator)
target_link_libraries(VulkanToy PUBLIC KTX)

target_precompile_headers(VulkanToy PUBLIC "include/Pch.h")