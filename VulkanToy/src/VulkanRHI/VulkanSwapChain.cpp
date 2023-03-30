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

    /** @brief Creates the platform specific surface abstraction of the native platform window used for presentation */
    void VulkanSwapChain::initSurface()
    {
        VkResult err = VK_SUCCESS;

        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != err)
            throw std::runtime_error("Failed to create window surface!");

        // Get available queue family properties
        uint32_t queueCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);
        VT_CORE_ASSERT(queueCount >= 1, "Fail to get device physical queue family properties");

        std::vector<VkQueueFamilyProperties> queueProps(queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps.data());

        // Iterate over each queue to learn whether it supports presenting:
        // Find a queue with present support
        // Will be used to present the swap chain images to the windowing system
        std::vector<VkBool32> supportsPresent(queueCount);
        for (uint32_t i = 0; i < queueCount; ++i)
            fpGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent[i]);

        // Search for a graphics and a present queue in the array of queue
        // families, try to find one that support both
        uint32_t  graphicsQueueNodeIndex = UINT32_MAX;
        uint32_t  presentQueueNodeIndex = UINT32_MAX;
        for (uint32_t i = 0 ; i < queueCount; ++i)
        {
            if (queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                if (graphicsQueueNodeIndex == UINT32_MAX)
                {
                    graphicsQueueNodeIndex = i;
                }
                if (supportsPresent[i] == VK_TRUE)
                {
                    graphicsQueueNodeIndex = i;
                    presentQueueNodeIndex = i;
                    break;
                }
            }
        }
        if (presentQueueNodeIndex == UINT32_MAX)
        {
            // If there's no queue that supports both present and graphics
            // try to find a separate present queue
            for (uint32_t i = 0; i < queueCount; ++i)
            {
                if (supportsPresent[i] == VK_TRUE)
                {
                    presentQueueNodeIndex = i;
                    break;
                }
            }
        }

        // Exit if either a graphics or a presenting queue hasn't been found
        if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX)
        {
            VT_CORE_ASSERT(false, "Could not find a graphics and/or presenting queue");
        }

        // TODO: Add support for separate graphics and presenting queue
        if (graphicsQueueNodeIndex != presentQueueNodeIndex)
        {
            VT_CORE_ASSERT(false, "Separate graphics and presenting queues are not supported yet!");
        }

        queueNodeIndex = graphicsQueueNodeIndex;

        // Get list of supported surface formats
        uint32_t  formatCount;
        if (fpGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr) != VK_SUCCESS)
            VT_CORE_ASSERT(false, "Fail to get physical device surface formats");
        VT_CORE_ASSERT(formatCount > 0, "Physical device surface formats is zero");

        std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
        if (fpGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data()) != VK_SUCCESS)
            VT_CORE_ASSERT(false, "Fail to get physical device surface formats");

        // If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
        // there is no preferred format, so we assume VK_FORMAT_B8G8R8A8_UNORM
        if ((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED))
        {
            colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
            colorSpace = surfaceFormats[0].colorSpace;
        } else
        {
            // iterate over the list of available surface format and
            // check for the presence of VK_FORMAT_B8G8R8A8_UNORM
            bool found_B8G8R8A8_UNORM = false;
            for (auto&& surfaceFormat : surfaceFormats)
            {
                if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
                {
                    colorFormat = surfaceFormat.format;
                    colorSpace = surfaceFormat.colorSpace;
                    found_B8G8R8A8_UNORM = true;
                    break;
                }
            }

            // in case VK_FORMAT_B8G8R8A8_UNORM is not available
            // select the first available color format
            if (!found_B8G8R8A8_UNORM)
            {
                colorFormat = surfaceFormats[0].format;
                colorSpace = surfaceFormats[0].colorSpace;
            }
        }
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
    * Create the swapchain and get its images with given width and height
    *
    * @param width Pointer to the width of the swapchain (may be adjusted to fit the requirements of the swapchain)
    * @param height Pointer to the height of the swapchain (may be adjusted to fit the requirements of the swapchain)
    * @param vsync (Optional) Can be used to force vsync-ed rendering (by using VK_PRESENT_MODE_FIFO_KHR as presentation mode)
    */
    void VulkanSwapChain::create(uint32_t& width, uint32_t& height, bool vsync, bool fullscreen)
    {
        // Store the current swap chain handle so we can use it later on to ease up recreation
        VkSwapchainKHR oldSwapchain = swapChain;

        // Get physical device surface properties and formats
        VkSurfaceCapabilitiesKHR surfCaps;
        if (fpGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfCaps) != VK_SUCCESS)
            VT_CORE_ASSERT(false, "Fail to get device surface capabilities KHR");

        // Get available present modes
        uint32_t presentModeCount;
        if (fpGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr) != VK_SUCCESS)
            VT_CORE_ASSERT(false, "Fail to get device surface present modes KHR");
        VT_CORE_ASSERT(presentModeCount > 0, "Device surface present modes is zero");

        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        if (fpGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()) != VK_SUCCESS)
            VT_CORE_ASSERT(false, "Fail to get device surface present modes KHR");

        VkExtent2D swapchainExtent{};
        // If width (and height) equals the special value 0xFFFFFFFF, the size of the surface will be set by the swapchain
        if (surfCaps.currentExtent.width == (uint32_t)-1)
        {
            // If the surface size is undefined, the size is set to
            // the size of the images requested.
            swapchainExtent.width = std::clamp(width, surfCaps.minImageExtent.width, surfCaps.maxImageExtent.width);
            swapchainExtent.height =  std::clamp(height, surfCaps.minImageExtent.height, surfCaps.maxImageExtent.width);
        } else
        {
            // If the surface size is defined, the swap chain size must match
            swapchainExtent = surfCaps.currentExtent;
            width = surfCaps.currentExtent.width;
            height = surfCaps.currentExtent.height;
        }

        // Select a present mode for the swapchain

        // The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
        // This mode waits for the vertical blank ("v-sync")
        VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

        // If v-sync is not requested, try to find a mailbox mode
        // It's the lowest latency non-tearing present mode available
        if (!vsync)
        {
            for (size_t i = 0; i < presentModeCount; i++)
            {
                if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                    break;
                }
                if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
                {
                    swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                }
            }
        }

        // Determine the number of images
        uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
        minImageCount = surfCaps.minImageCount;
#if (defined(VK_USE_PLATFORM_MACOS_MVK) && defined(VK_EXAMPLE_XCODE_GENERATED))
        // SRS - Work around known MoltenVK issue re 2x frame rate when vsync (VK_PRESENT_MODE_FIFO_KHR) enabled
	struct utsname sysInfo;
	uname(&sysInfo);
	// SRS - When vsync is on, use minImageCount when not in fullscreen or when running on Apple Silcon
	// This forces swapchain image acquire frame rate to match display vsync frame rate
	if (vsync && (!fullscreen || strcmp(sysInfo.machine, "arm64") == 0))
	{
		desiredNumberOfSwapchainImages = surfCaps.minImageCount;
	}
#endif
        if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount))
            desiredNumberOfSwapchainImages = surfCaps.maxImageCount;

        // Find the transformation of the surface
        VkSurfaceTransformFlagsKHR preTransform;
        if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        {
            // We prefer a non-rotated transform
            preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        } else
        {
            preTransform = surfCaps.currentTransform;
        }

        // Find a supported composite alpha format (not all devices supported alpha opaque
        VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        // Simply select the first composite alpha format available
        std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags{
                VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
                VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
        };
        for (auto& compositeAlphaFlag : compositeAlphaFlags)
        {
            if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag)
            {
                compositeAlpha = compositeAlphaFlag;
                break;
            }
        }

        VkSwapchainCreateInfoKHR swapchainCI{};
        swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCI.surface = surface;
        swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
        swapchainCI.imageFormat = colorFormat;
        swapchainCI.imageColorSpace = colorSpace;
        swapchainCI.imageExtent = { swapchainExtent.width, swapchainExtent.height };
        swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
        swapchainCI.imageArrayLayers = 1;
        swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCI.queueFamilyIndexCount = 0;
        swapchainCI.presentMode = swapchainPresentMode;
        // Setting oldSwapChain to the saved handle of the previous swapchain aids in resource reuse and makes sure that we can still present already acquired images
        swapchainCI.oldSwapchain = oldSwapchain;
        // Setting clipped to VT_TRUE allows the implementation to discard rendering outside of the surface area
        swapchainCI.clipped = VK_TRUE;
        swapchainCI.compositeAlpha = compositeAlpha;

        // Enable transfer source on swap chain images if supported
        if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
            swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        // Enable transfer destination on swap chain images if supported
        if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        if (fpCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapChain) != VK_SUCCESS)
            VT_CORE_ASSERT(false, "Fail to create swap chain KHR");

        // If an existing swap chain is re-created, destroy the old swap chain
        if (oldSwapchain != VK_NULL_HANDLE)
        {
            for (uint32_t i = 0; i <  imageCount; ++i)
            {
                vkDestroyImageView(device, buffers[i].view, nullptr);
            }
            fpDestroySwapchainKHR(device, oldSwapchain, nullptr);
        }
        if (fpGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr) != VK_SUCCESS)
            VT_CORE_ASSERT(false, "Fail to get swap chain images KHR");

        // Get the swap chain images
        images.resize(imageCount);
        if (fpGetSwapchainImagesKHR(device, swapChain, &imageCount, images.data()) != VK_SUCCESS)
            VT_CORE_ASSERT(false, "Fail to get swap chain images KHR");

        // Get the swap chain buffers containing the image and imageview
        buffers.resize(imageCount);
        for (uint32_t i = 0; i < imageCount; ++i)
        {
            VkImageViewCreateInfo colorAttachmentView{};
            colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            colorAttachmentView.pNext= nullptr;
            colorAttachmentView.format = colorFormat;
            colorAttachmentView.components = {
                VK_COMPONENT_SWIZZLE_R,
                VK_COMPONENT_SWIZZLE_G,
                VK_COMPONENT_SWIZZLE_B,
                VK_COMPONENT_SWIZZLE_A
            };
            colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            colorAttachmentView.subresourceRange.baseMipLevel = 0;
            colorAttachmentView.subresourceRange.levelCount = 1;
            colorAttachmentView.subresourceRange.baseArrayLayer = 0;
            colorAttachmentView.subresourceRange.layerCount = 1;
            colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
            colorAttachmentView.flags = 0;

            buffers[i].image = images[i];

            colorAttachmentView.image = buffers[i].image;

            if (vkCreateImageView(device, &colorAttachmentView, nullptr, &buffers[i].view) != VK_SUCCESS)
                VT_CORE_ASSERT(false, "Fail to create image view");
        }
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
    VkResult  VulkanSwapChain::queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore)
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

    /**
    * Destroy and free Vulkan resources used for the swapchain
    */
    void VulkanSwapChain::release()
    {
        if (swapChain != VK_NULL_HANDLE)
        {
            for (uint32_t i = 0; i < imageCount; ++i)
            {
                vkDestroyImageView(device, buffers[i].view, nullptr);
            }
        }
        if (surface != VK_NULL_HANDLE)
        {
            fpDestroySwapchainKHR(device, swapChain, nullptr);
            vkDestroySurfaceKHR(instance, surface, nullptr);
        }
        surface = VK_NULL_HANDLE;
        swapChain = VK_NULL_HANDLE;
    }
}








































