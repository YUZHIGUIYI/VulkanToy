//
// Created by ZZK on 2023/3/2.
//

#include <VulkanToy/VulkanRHI/VulkanSwapChain.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>

namespace VT
{
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
    {
        switch (VulkanRHI::eDisplayMode)
        {
            case VulkanRHI::DisplayMode::DISPLAYMODE_HDR:
            {
                for (const auto& availableFormat : availableFormats)
                {
                    if (availableFormat.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT)
                    {
                        if (availableFormat.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32)
                        {
                            return availableFormat;
                        }
                    }
                }
                break;
            }
            case VulkanRHI::DisplayMode::DISPLAYMODE_SDR:
            {
                for (const auto& availableFormat : availableFormats)
                {
                    if (availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                    {
                        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
                        {
                            return availableFormat;
                        }
                    }
                }
                break;
            }
            default:
                break;
        }
        VT_CORE_WARN("Currently using non-srgb unorm format back format back buffer, may cause some problem");
        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
    {
        // TODO: try to use mailbox
        for (const auto& availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR)
            {
                return availablePresentMode;
            }
        }
        for (const auto& availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    }

    VkExtent2D chooseSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            return capabilities.currentExtent;
        } else
        {
            int width, height;
            glfwGetFramebufferSize(VulkanRHI::get()->getWindow(), &width, &height);

            VkExtent2D actualExtent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

            actualExtent.width = std::max(
                capabilities.minImageExtent.width,
                std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(
                capabilities.minImageExtent.height,
                std::min(capabilities.maxImageExtent.height, actualExtent.height));
            return actualExtent;
        }
    }

    // Initialize swap chain
    void VulkanSwapChain::init()
    {
        auto swapChainSupportDetail = VulkanRHI::get()->querySwapChainSupportDetail();

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupportDetail.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupportDetail.presentModes);
        VkExtent2D extent = chooseSwapChainExtent(swapChainSupportDetail.capabilities);
        imageCount = swapChainSupportDetail.capabilities.minImageCount + 1;
        if (swapChainSupportDetail.capabilities.maxImageCount > 0 && imageCount > swapChainSupportDetail.capabilities.maxImageCount)
        {
            imageCount = swapChainSupportDetail.capabilities.maxImageCount;
        }
        minImageCount = imageCount;
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = VulkanRHI::get()->getSurface();
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        // Use graphics family queue to draw and swap
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;

        createInfo.preTransform = swapChainSupportDetail.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        RHICheck(vkCreateSwapchainKHR(VulkanRHI::Device, &createInfo, nullptr, &swapChain));

        // Allocate images
        vkGetSwapchainImagesKHR(VulkanRHI::Device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(VulkanRHI::Device, swapChain, &imageCount, swapChainImages.data());

        colorFormat = surfaceFormat.format;
        swapChainExtent = extent;

        // Create image views needed for swap chain
        swapChainImageViews.resize(imageCount);
        for (size_t i = 0; i < imageCount; ++i)
        {
            VkImageViewCreateInfo viewCreateInfo{};
            viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewCreateInfo.image = swapChainImages[i];
            viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewCreateInfo.format = colorFormat;
            viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewCreateInfo.subresourceRange.baseMipLevel = 0;
            viewCreateInfo.subresourceRange.levelCount = 1;
            viewCreateInfo.subresourceRange.baseArrayLayer = 0;
            viewCreateInfo.subresourceRange.layerCount = 1;

            RHICheck(vkCreateImageView(VulkanRHI::Device, &viewCreateInfo, nullptr, &swapChainImageViews[i]));
        }
        VT_CORE_TRACE("Create vulkan swap chain successfully, back buffer count is {0}", imageCount);
    }

    void VulkanSwapChain::release()
    {
        for (auto imageView : swapChainImageViews)
        {
            vkDestroyImageView(VulkanRHI::Device, imageView, nullptr);
        }
        swapChainImageViews.resize(0);
        vkDestroySwapchainKHR(VulkanRHI::Device, swapChain, nullptr);
    }

    void VulkanSwapChain::rebuild()
    {
        release();
        init();
    }

    /**
    * Set instance, physical and logical device to use for the swapchain and get all required function pointers
    *
    * @param instance Vulkan instance to use
    * @param physicalDevice Physical device used to query properties and formats relevant to the swapchain
    * @param device Logical representation of the device to create the swapchain for
    *
    */
    void VulkanSwapChain::connect(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, GLFWwindow* window)
    {
        this->instance = instance;
        this->physicalDevice = physicalDevice;
        this->device = device;
        this->window = window;
        fpGetPhysicalDeviceSurfaceSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceSupportKHR"));
        fpGetPhysicalDeviceSurfaceCapabilitiesKHR =  reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
        fpGetPhysicalDeviceSurfaceFormatsKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR"));
        fpGetPhysicalDeviceSurfacePresentModesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR"));

        fpCreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(vkGetDeviceProcAddr(device, "vkCreateSwapchainKHR"));
        fpDestroySwapchainKHR = reinterpret_cast<PFN_vkDestroySwapchainKHR>(vkGetDeviceProcAddr(device, "vkDestroySwapchainKHR"));
        fpGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(vkGetDeviceProcAddr(device, "vkGetSwapchainImagesKHR"));
        fpAcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(vkGetDeviceProcAddr(device, "vkAcquireNextImageKHR"));
        fpQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(vkGetDeviceProcAddr(device, "vkQueuePresentKHR"));
    }

    /**
    * Acquires the next image in the swap chain
    *
    * @param presentCompleteSemaphore (Optional) Semaphore that is signaled when the image is ready for use
    * @param imageIndex Pointer to the image index that will be increased if the next image could be acquired
    *
    * @note The function will always wait until the next image has been acquired by setting timeout to UINT64_MAX
    *
    * @return VkResult of the image acquisition
    */
    VkResult VulkanSwapChain::acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t *imageIndex)
    {
        // By setting timeout to UINT64_MAX, we will always wait until the next image has been acquired or an actual error is thrown
        return fpAcquireNextImageKHR(device, swapChain, UINT64_MAX, presentCompleteSemaphore, (VkFence) nullptr, imageIndex);
    }

    /**
    * Queue an image for presentation
    *
    * @param queue Presentation queue for presenting the image
    * @param imageIndex Index of the swapchain image to queue for presentation
    * @param waitSemaphore (Optional) Semaphore that is waited on before the image is presented (only used if != VK_NULL_HANDLE)
    *
    * @return VkResult of the queue presentation
    */
    VkResult VulkanSwapChain::queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore)
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = nullptr;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChain;
        presentInfo.pImageIndices = &imageIndex;
        // Check if a wait semaphore has been specified to wait for before presenting the image
        if (waitSemaphore != VK_NULL_HANDLE)
        {
            presentInfo.pWaitSemaphores = &waitSemaphore;
            presentInfo.waitSemaphoreCount = 1;
        }
        return fpQueuePresentKHR(queue, &presentInfo);
    }
}








































