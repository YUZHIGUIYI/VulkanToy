//
// Created by ZZK on 2023/3/1.
//

#include <VulkanToy/VulkanRHI/VulkanDevice.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>

namespace VT
{
    static bool checkDeviceExtensionSupport(const std::vector<const char *>& requestDeviceExtensions, VkPhysicalDevice physicalDevice)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(requestDeviceExtensions.begin(), requestDeviceExtensions.end());
        for (const auto& extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }
        return requiredExtensions.empty();
    }

    bool VulkanDevice::isPhysicalDeviceSuitable(const std::vector<const char *> &requestExtensions)
    {
        bool isAllQueueExist = false;
        {
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

            bool isSupportGraphics = false;
            bool isSupportCompute = false;
            bool isSupportCopy = false;
            for (const auto& queueFamily : queueFamilies)
            {
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    isSupportGraphics = true;
                }
                if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
                {
                    isSupportCompute = true;
                }
                if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
                {
                    isSupportCopy = true;
                }
            }
            isAllQueueExist = isSupportGraphics && isSupportCompute && isSupportCopy;
        }

        bool isExtensionsSupport = checkDeviceExtensionSupport(requestExtensions, physicalDevice);

        bool isSwapChainAdequate = false;
        if (isExtensionsSupport)
        {
            auto swapChainSupport = VulkanRHI::get()->querySwapChainSupportDetail();
            isSwapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return isAllQueueExist && isExtensionsSupport && isSwapChainAdequate;
    }

    void VulkanDevice::pickupSuitableGPU(const std::vector<const char *> &requestExtensions)
    {
        uint32_t physicalDeviceCount;
        RHICheck(vkEnumeratePhysicalDevices(VulkanRHI::get()->getInstance(), &physicalDeviceCount, nullptr));
        if (physicalDeviceCount < 1)
        {
            VT_CORE_CRITICAL("No gpu support vulkan api on this computer");
        }
        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        RHICheck(vkEnumeratePhysicalDevices(VulkanRHI::get()->getInstance(), &physicalDeviceCount, physicalDevices.data()));
        for (const auto& GPU : physicalDevices)
        {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(GPU, &deviceProperties);
            if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                physicalDevice = GPU;
                if (isPhysicalDeviceSuitable(requestExtensions))
                {
                    VT_CORE_INFO("Using discrete GPU: {0}", deviceProperties.deviceName);
                    properties = deviceProperties;
                    return;
                }
            }
        }

        VT_CORE_WARN("No discrete GPU found, using default one");

        for (const auto& GPU : physicalDevices)
        {
            physicalDevice = GPU;
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(GPU, &deviceProperties);
            if (isPhysicalDeviceSuitable(requestExtensions))
            {
                VT_CORE_INFO("Using discrete GPU: {0}", deviceProperties.deviceName);
                properties = deviceProperties;
                return;
            }
        }
        VT_CORE_CRITICAL("No suitable GPU found");
    }

    // Create logical device
    void VulkanDevice::createLogicDevice(VkPhysicalDeviceFeatures features, const std::vector<const char *> &requestExtensions, void *nextChain)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        uint32_t graphicsQueueCount = 0;
        uint32_t copyQueueCount = 0;
        uint32_t computeQueueCount = 0;

        uint32_t queueIndex = 0;
        bool isGraphicsQueueSet = false;
        bool isCopyQueueSet = false;
        bool isComputeQueueSet = false;
        for (const auto& queueFamily : queueFamilies)
        {
            const bool isSupportSparseBinding = queueFamily.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT;
            const bool isSupportGraphics = queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT;
            const bool isSupportCompute = queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT;
            const bool isSupportCopy = queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT;

            if (isSupportGraphics && (!isGraphicsQueueSet))
            {
                queueInfos.graphicsFamily = queueIndex;
                graphicsQueueCount = queueFamily.queueCount;
                isGraphicsQueueSet = true;
            } else if (isSupportCompute && (!isComputeQueueSet))
            {
                queueInfos.computeFamily = queueIndex;
                computeQueueCount = queueFamily.queueCount;
                isComputeQueueSet = true;
            } else if (isSupportCopy && (!isCopyQueueSet))
            {
                queueInfos.copyFamily = queueIndex;
                copyQueueCount = queueFamily.queueCount;
                isCopyQueueSet = true;
            }

            queueIndex++;
        }
        VT_CORE_ASSERT(graphicsQueueCount > 2, "Need more than one queue to do some async dispatch");
        VT_CORE_ASSERT(copyQueueCount > 2, "Need more than one queue to do some async dispatch");

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        // Prepare queue priority, all 0.5f
        std::vector<float> graphicsQueuePriority(graphicsQueueCount, 0.5f);
        std::vector<float> computeQueuePriority(computeQueueCount, 0.5f);
        std::vector<float> copyQueuePriority(copyQueueCount, 0.5f);

        // Major queue used for present and render UI
        graphicsQueuePriority[0] = 1.0f;
        /// Major compute queue
        computeQueuePriority[0] = 0.8f;
        graphicsQueuePriority[1] = 0.8f;

        // Create queue information
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueInfos.graphicsFamily;
        queueCreateInfo.queueCount = graphicsQueueCount;
        queueCreateInfo.pQueuePriorities = graphicsQueuePriority.data();
        queueCreateInfos.push_back(queueCreateInfo);

        if (computeQueueCount > 0)
        {
            queueCreateInfo.queueFamilyIndex = queueInfos.computeFamily;
            queueCreateInfo.queueCount = computeQueueCount;
            queueCreateInfo.pQueuePriorities = computeQueuePriority.data();
            queueCreateInfos.push_back(queueCreateInfo);
        }

        queueCreateInfo.queueFamilyIndex = queueInfos.copyFamily;
        queueCreateInfo.queueCount = copyQueueCount;
        queueCreateInfo.pQueuePriorities = copyQueuePriority.data();
        queueCreateInfos.push_back(queueCreateInfo);

        VkDeviceCreateInfo deviceCreateInfo{};
        VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

        // Use physical device features2
        deviceCreateInfo.pEnabledFeatures = nullptr;
        physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        physicalDeviceFeatures2.features = features;
        physicalDeviceFeatures2.pNext = nextChain;
        deviceCreateInfo.pNext = &physicalDeviceFeatures2;

        // Device extensions
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requestExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = requestExtensions.data();

        // No special device layer, all controlled by instance layer
        deviceCreateInfo.ppEnabledLayerNames = nullptr;
        deviceCreateInfo.enabledLayerCount = 0;

        RHICheck(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice));

        // Get major queues
        queueInfos.graphicsQueues.resize(graphicsQueueCount);
        for (uint32_t index = 0; index < graphicsQueueCount; ++index)
        {
            vkGetDeviceQueue(logicalDevice, queueInfos.graphicsFamily, index, &queueInfos.graphicsQueues[index]);
        }
        queueInfos.computeQueues.resize(computeQueueCount);
        for (uint32_t index = 0; index < computeQueueCount; ++index)
        {
            vkGetDeviceQueue(logicalDevice, queueInfos.computeFamily, index, &queueInfos.computeQueues[index]);
        }
        queueInfos.copyQueues.resize(copyQueueCount);
        for (uint32_t index = 0; index < copyQueueCount; ++index)
        {
            vkGetDeviceQueue(logicalDevice, queueInfos.copyFamily, index, &queueInfos.copyQueues[index]);
        }
    }

    VkFormat VulkanDevice::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
    {
        for (auto format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & featureFlags) == featureFlags)
            {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & featureFlags) == featureFlags)
            {
                return format;
            }
        }
        VT_CORE_CRITICAL("Can not find supported format");
    }

    int32_t VulkanDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memoryPropertyFlags)
    {
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
        {
            if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags)
            {
                return i;
            }
        }
        VT_CORE_CRITICAL("No suitable memory type found");
    }

    // Initialize command pool
    void VulkanDevice::initCommandPool()
    {
        VkCommandPoolCreateInfo poolCreateInfo{};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        // Graphics command pools
        VT_CORE_ASSERT(queueInfos.graphicsQueues.size() > 2, "GPU does not support more than one graphics queue");
        poolCreateInfo.queueFamilyIndex = queueInfos.graphicsFamily;

        majorGraphicsPool.queue = queueInfos.graphicsQueues[0];
        secondMajorGraphicsPool.queue = queueInfos.graphicsQueues[1];

        RHICheck(vkCreateCommandPool(logicalDevice, &poolCreateInfo, nullptr, &majorGraphicsPool.pool));
        RHICheck(vkCreateCommandPool(logicalDevice, &poolCreateInfo, nullptr, &secondMajorGraphicsPool.pool));

        if (queueInfos.graphicsQueues.size() > 2)
        {
            graphicsPools.resize(queueInfos.graphicsQueues.size() - 2);
            uint32_t index = 2;
            for (auto& pool : graphicsPools)
            {
                pool.queue = queueInfos.graphicsQueues[index];
                RHICheck(vkCreateCommandPool(logicalDevice, &poolCreateInfo, nullptr, &pool.pool));
                ++index;
            }
        }

        // Compute command pool
        VT_CORE_ASSERT(queueInfos.computeQueues.size() > 1, "GPU does not support more than one compute queue");
        poolCreateInfo.queueFamilyIndex = queueInfos.computeFamily;
        majorComputePool.queue = queueInfos.computeQueues[0];
        RHICheck(vkCreateCommandPool(logicalDevice, &poolCreateInfo, nullptr, &majorComputePool.pool));
        if (queueInfos.computeQueues.size() > 1)
        {
            computePools.resize(queueInfos.computeQueues.size() - 1);
            uint32_t index = 1;
            for (auto& pool : computePools)
            {
                pool.queue = queueInfos.computeQueues[index];
                RHICheck(vkCreateCommandPool(logicalDevice, &poolCreateInfo, nullptr, &pool.pool));
                ++index;
            }
        }

        // Copy command pools
        poolCreateInfo.queueFamilyIndex = queueInfos.copyFamily;
        if (queueInfos.copyQueues.size() > 0)
        {
            copyPools.resize(queueInfos.copyQueues.size());
            uint32_t index = 0;
            for (auto& pool : copyPools)
            {
                pool.queue = queueInfos.copyQueues[index];
                RHICheck(vkCreateCommandPool(logicalDevice, &poolCreateInfo, nullptr, &pool.pool));
                ++index;
            }
        }
    }

    void VulkanDevice::releaseCommandPool()
    {
        // Destroy major command pool.
        vkDestroyCommandPool(logicalDevice, majorGraphicsPool.pool, nullptr);

        // Async queue.
        vkDestroyCommandPool(logicalDevice, majorComputePool.pool, nullptr);
        vkDestroyCommandPool(logicalDevice, secondMajorGraphicsPool.pool, nullptr);

        // Destroy other command pool.
        for (auto& pool : graphicsPools)
        {
            vkDestroyCommandPool(logicalDevice, pool.pool, nullptr);
        }
        for (auto& pool : computePools)
        {
            vkDestroyCommandPool(logicalDevice, pool.pool, nullptr);
        }
        for (auto& pool : copyPools)
        {
            vkDestroyCommandPool(logicalDevice, pool.pool, nullptr);
        }
    }

    // Initialize vulkan device
    void VulkanDevice::init(VkPhysicalDeviceFeatures features, const std::vector<const char *> &requestExtensions, void *nextChain)
    {
        // Pick up best GPU on this machine
        pickupSuitableGPU(requestExtensions);

        // Store GPU memory properties
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

        // GPU properties
        VT_CORE_INFO("GPU minimum align memory size: {0}", properties.limits.minUniformBufferOffsetAlignment);

        // Cache useful GPU properties2
        auto vkGetPhysicalDeviceProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(vkGetInstanceProcAddr(
                VulkanRHI::get()->getInstance(), "vkGetPhysicalDeviceProperties2KHR"));
        if (vkGetPhysicalDeviceProperties2KHR)
        {
            VkPhysicalDeviceProperties2KHR deviceProperties{};
            descriptorIndexingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES_EXT;
            deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
            deviceProperties.pNext = &descriptorIndexingProperties;
            vkGetPhysicalDeviceProperties2KHR(physicalDevice, &deviceProperties);
        }

        // Create logical device
        createLogicDevice(features, requestExtensions, nextChain);

        // Cache some support format.
        cacheSupportDepthOnlyFormat = findSupportedFormat(
                { VK_FORMAT_D32_SFLOAT },
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
        cacheSupportDepthStencilFormat = findSupportedFormat(
                { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    // TODO: Release vulkan device and its resources
    void VulkanDevice::release()
    {
        releaseCommandPool();
        if (logicalDevice)
        {
            vkDestroyDevice(logicalDevice, nullptr);
        }
    }
}






























