//
// Created by ZZK on 2023/3/2.
//

#pragma once

#include <VulkanToy/Core/Base.h>
#include <vulkan/vulkan.h>

struct GLFWwindow;

namespace VT
{
    struct SwapChainBuffer
    {
        VkImage image;
        VkImageView view;
    };

    class VulkanSwapChain
    {
    private:
        VkInstance instance;
        VkDevice device;
        VkPhysicalDevice physicalDevice;
        VkSurfaceKHR surface;
        GLFWwindow* window;

        // Function pointers
        PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
        PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
        PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
        PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;
        PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
        PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
        PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
        PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
        PFN_vkQueuePresentKHR fpQueuePresentKHR;

    public:
        VkFormat colorFormat;
        VkColorSpaceKHR colorSpace;
        VkSwapchainKHR swapChain = VK_NULL_HANDLE;
        uint32_t minImageCount;
        uint32_t imageCount;
        std::vector<VkImage> images;
        std::vector<SwapChainBuffer> buffers;
        uint32_t queueNodeIndex = UINT32_MAX;

    public:
        // Using glfw
        void initSurface(void* platformHandle, void* platformWindow);

        void connect(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, GLFWwindow* window);
        void create(uint32_t& width, uint32_t& height, bool vsync = false, bool fullscreen = false);
        VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex);
        VkResult queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE);
        void cleanup();
    };
}











