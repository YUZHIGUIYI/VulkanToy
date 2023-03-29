//
// Created by ZZK on 2023/3/1.
//

#include <VulkanToy/VulkanRHI/VulkanDevice.h>
#include <VulkanToy/Core/VulkanInitializers.h>
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

    /**
	* Get the index of a memory type that has all the requested property bits set
	*
	* @param typeBits Bit mask with bits set for each memory type supported by the resource to request for (from VkMemoryRequirements)
	* @param properties Bit mask of properties for the memory type to request
	* @param (Optional) memTypeFound Pointer to a bool that is set to true if a matching memory type has been found
	*
	* @return Index of the requested memory type
	*
	* @throw Throws an exception if memTypeFound is null and no memory type could be found that supports the requested properties
	*/
    uint32_t VulkanDevice::getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound) const
    {
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
        {
            if ((typeBits & 1) == 1)
            {
                if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
                {
                    if (memTypeFound)
                    {
                        *memTypeFound = true;
                    }
                    return i;
                }
            }
            typeBits >>= 1;
        }

        if (memTypeFound)
        {
            *memTypeFound = false;
            return 0;
        } else
        {
            VT_CORE_ASSERT(memTypeFound, "Could not find a matching memory type");
        }
    }

    /**
	* Get the index of a queue family that supports the requested queue flags
	* SRS - support VkQueueFlags parameter for requesting multiple flags vs. VkQueueFlagBits for a single flag only
	*
	* @param queueFlags Queue flags to find a queue family index for
	*
	* @return Index of the queue family index that matches the flags
	*
	* @throw Throws an exception if no queue family index could be found that supports the requested flags
	*/
	uint32_t VulkanDevice::getQueueFamilyIndex(VkQueueFlags queueFlags) const
    {
        // Dedicated queue for compute
        // Try to find a queue family index that supports compute but not graphics
        if ((queueFlags & VK_QUEUE_COMPUTE_BIT) == queueFlags)
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); ++i)
            {
                if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
                {
                    return i;
                }
            }
        }

        // Dedicated queue for transfer
        // Try to find a queue family index that supports transfer but not graphics and compute
        if ((queueFlags & VK_QUEUE_TRANSFER_BIT) == queueFlags)
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
            {
                if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
                {
                    return i;
                }
            }
        }

        // For other queue types or if no separate compute queue is present, return the first one to support the requested flags
        for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
        {
            if ((queueFamilyProperties[i].queueFlags & queueFlags) == queueFlags)
            {
                return i;
            }
        }

        VT_CORE_ASSERT(false, "Could not find a matching queue family index");
    }

    /**
	* Create a buffer on the device
	*
	* @param usageFlags Usage flag bit mask for the buffer (i.e. index, vertex, uniform buffer)
	* @param memoryPropertyFlags Memory properties for this buffer (i.e. device local, host visible, coherent)
	* @param size Size of the buffer in byes
	* @param buffer Pointer to the buffer handle acquired by the function
	* @param memory Pointer to the memory handle acquired by the function
	* @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over)
	*
	* @return VK_SUCCESS if buffer handle and memory have been created and (optionally passed) data has been copied
	*/
    VkResult VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags,
                                        VkDeviceSize size, VkBuffer *buffer, VkDeviceMemory *memory, void *data)
    {
        // Create the buffer handle
        VkBufferCreateInfo bufferCreateInfo = INITIALIZERS::bufferCreateInfo(usageFlags, size);
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, buffer) != VK_SUCCESS)
        {
            VT_CORE_ASSERT(false, "Fail to create buffer");
        }

        // Create the memory backing up the buffer handle
        VkMemoryRequirements memReqs;
        VkMemoryAllocateInfo memAlloc = INITIALIZERS::memoryAllocateInfo();
        vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memReqs);
        memAlloc.allocationSize = memReqs.size;
        // Find a memory type index that fits the properties of the buffer
        memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
        // If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the appropriate flag during allocation
        VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
        if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
        {
            allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
            allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
            memAlloc.pNext = &allocFlagsInfo;
        }
        if (vkAllocateMemory(logicalDevice, &memAlloc, nullptr, memory) != VK_SUCCESS)
        {
            VT_CORE_ASSERT(false, "Fail to allocate memory");
        }

        // If a pointer to the buffer data has been passed, map the buffer and copy over the data
        if (!data)
        {
            void* mapped;
            if (vkMapMemory(logicalDevice, *memory, 0, size, 0, &mapped) != VK_SUCCESS)
                VT_CORE_ASSERT(false, "Fail to map memory");
            std::memcpy(mapped, data, size);
            // If host coherency hasn't been requested, do a manual flush to make writes visible
            if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            {
                VkMappedMemoryRange mappedRange = INITIALIZERS::mappedMemoryRange();
                mappedRange.memory = *memory;
                mappedRange.offset = 0;
                mappedRange.size = size;
                vkFlushMappedMemoryRanges(logicalDevice, 1, &mappedRange);
            }
            vkUnmapMemory(logicalDevice, *memory);
        }

        // Attach the memory to the buffer object
        if (vkBindBufferMemory(logicalDevice, *buffer, *memory, 0) != VK_SUCCESS)
            VT_CORE_ASSERT(false, "Fail to bind buffer memory");

        return VK_SUCCESS;
    }

    /**
	* Create a buffer on the device
	*
	* @param usageFlags Usage flag bit mask for the buffer (i.e. index, vertex, uniform buffer)
	* @param memoryPropertyFlags Memory properties for this buffer (i.e. device local, host visible, coherent)
	* @param buffer Pointer to a vk::Vulkan buffer object
	* @param size Size of the buffer in bytes
	* @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over)
	*
	* @return VK_SUCCESS if buffer handle and memory have been created and (optionally passed) data has been copied
	*/
    VkResult VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags,
                                        Buffer *buffer, VkDeviceSize size, void *data)
    {
        buffer->device = logicalDevice;

        // Create the buffer handle
        VkBufferCreateInfo bufferCreateInfo = INITIALIZERS::bufferCreateInfo(usageFlags, size);
        if (vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &buffer->buffer) != VK_SUCCESS)
            VT_CORE_ASSERT(false, "Fail to create buffer");

        // Create the memory backing up the buffer handle
        VkMemoryRequirements memReqs;
        VkMemoryAllocateInfo memAlloc = INITIALIZERS::memoryAllocateInfo();
        vkGetBufferMemoryRequirements(logicalDevice, buffer->buffer, &memReqs);
        memAlloc.allocationSize = memReqs.size;
        // Find a memory type index that fits the properties of the buffer
        memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
        // If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the appropriate flag during allocation
        VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
        if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
        {
            allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
            allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
            memAlloc.pNext = &allocFlagsInfo;
        }
        if (vkAllocateMemory(logicalDevice, &memAlloc, nullptr, &buffer->memory) != VK_SUCCESS)
            VT_CORE_ASSERT(false, "Fail to allocate memory");

        buffer->alignment = memReqs.alignment;
        buffer->size = size;
        buffer->usageFlags = usageFlags;
        buffer->memoryPropertyFlags = memoryPropertyFlags;

        // If a pointer to the buffer data has been passed, map the buffer and copy over the data
        if (!data)
        {
            if (buffer->map() != VK_SUCCESS)
                VT_CORE_ASSERT(false, "Fail to map memory");
            std::memcpy(buffer->mapped, data, size);
            if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
                buffer->flush();

            buffer->unmap();
        }

        // Initialize a default descriptor that covers the whole buffer size
        buffer->setupDescriptor();

        // Attach the memory to the buffer object
        return buffer->bind();
    }

    /**
	* Copy buffer data from src to dst using VkCmdCopyBuffer
	*
	* @param src Pointer to the source buffer to copy from
	* @param dst Pointer to the destination buffer to copy to
	* @param queue Pointer
	* @param copyRegion (Optional) Pointer to a copy region, if NULL, the whole buffer is copied
	*
	* @note Source and destination pointers must have the appropriate transfer usage flags set (TRANSFER_SRC / TRANSFER_DST)
	*/
    void VulkanDevice::copyBuffer(Buffer *src, Buffer *dst, VkQueue queue, VkBufferCopy *copyRegion)
    {
        VT_CORE_ASSERT(dst->size <= src->size, "Fail to copy buffer");
        VT_CORE_ASSERT(src->buffer, "Fail to copy buffer");
        VkCommandBuffer copyCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
        VkBufferCopy bufferCopy{};
        if (copyRegion == nullptr)
        {
            bufferCopy.size = src->size;
        } else
        {
            bufferCopy = *copyRegion;
        }

        vkCmdCopyBuffer(copyCmd, src->buffer, dst->buffer, 1, &bufferCopy);

        flushCommandBuffer(copyCmd, queue);
    }

    /**
	* Create a command pool for allocation command buffers from
	*
	* @param queueFamilyIndex Family index of the queue to create the command pool for
	* @param createFlags (Optional) Command pool creation flags (Defaults to VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
	*
	* @note Command buffers allocated from the created pool can only be submitted to a queue with the same family index
	*
	* @return A handle to the created command buffer
	*/
	VkCommandPool VulkanDevice::createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags)
    {
        VkCommandPoolCreateInfo cmdPoolInfo{};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
        cmdPoolInfo.flags = createFlags;
        VkCommandPool cmdPool;
        if (vkCreateCommandPool(logicalDevice, &cmdPoolInfo, nullptr, &cmdPool) != VK_SUCCESS)
            VT_CORE_ASSERT(false, "Fail to create command pool");
        return cmdPool;
    }

    /**
	* Allocate a command buffer from the command pool
	*
	* @param level Level of the new command buffer (primary or secondary)
	* @param pool Command pool from which the command buffer will be allocated
	* @param (Optional) begin If true, recording on the new command buffer will be started (vkBeginCommandBuffer) (Defaults to false)
	*
	* @return A handle to the allocated command buffer
	*/
    VkCommandBuffer VulkanDevice::createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin)
    {
        VkCommandBufferAllocateInfo cmdBufAllocateInfo = INITIALIZERS::commandBufferAllocateInfo(pool, level, 1);
        VkCommandBuffer cmdBuffer;
        if (vkAllocateCommandBuffers(logicalDevice, &cmdBufAllocateInfo, &cmdBuffer) != VK_SUCCESS)
            VT_CORE_ASSERT(false, "Fail to allocate command buffers");
        // If requested, also start recording for the new command buffer
        if (begin)
        {
            VkCommandBufferBeginInfo cmdBufInfo = INITIALIZERS::commandBufferBeginInfo();
            if (vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo) != VK_SUCCESS)
                VT_CORE_ASSERT(false, "Fail to begin command buffer");
        }
        return cmdBuffer;
    }

    VkCommandBuffer VulkanDevice::createCommandBuffer(VkCommandBufferLevel level, bool begin)
    {
        return createCommandBuffer(level, commandPool, begin);
    }

    /**
	* Finish command buffer recording and submit it to a queue
	*
	* @param commandBuffer Command buffer to flush
	* @param queue Queue to submit the command buffer to
	* @param pool Command pool on which the command buffer has been created
	* @param free (Optional) Free the command buffer once it has been submitted (Defaults to true)
	*
	* @note The queue that the command buffer is submitted to must be from the same family index as the pool it was allocated from
	* @note Uses a fence to ensure command buffer has finished executing
	*/
	void VulkanDevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free)
    {
        if (commandBuffer == VK_NULL_HANDLE)
            return;
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
            VT_CORE_ASSERT(false, "Fail to end command buffer");

        VkSubmitInfo submitInfo = INITIALIZERS::submitInfo();
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        // Create fence to ensure that the command buffer has finished executing
        VkFenceCreateInfo fenceInfo = INITIALIZERS::fenceCreateInfo(VK_FLAGS_NONE);
        VkFence fence;
        if (vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence) != VK_SUCCESS)
            VT_CORE_ASSERT(false, "Fail to create fence");
        // Submit to the queue
        if (vkQueueSubmit(queue, 1, &submitInfo, fence) != VK_SUCCESS)
            VT_CORE_ASSERT(false, "Fail to submit command buffer to queue");
        // Wait for the fence to signal that command buffer has finished executing
        vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
        vkDestroyFence(logicalDevice, fence, nullptr);
        if (free)
        {
            vkFreeCommandBuffers(logicalDevice, pool, 1, &commandBuffer);
        }
    }

    void VulkanDevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
    {
        return flushCommandBuffer(commandBuffer, queue, commandPool, free);
    }

    /**
	* Check if an extension is supported by the (physical device)
	*
	* @param extension Name of the extension to check
	*
	* @return True if the extension is supported (present in the list read at device creation time)
	*/
	bool VulkanDevice::extensionSupported(const std::string& extension)
    {
        return std::find(supportedExtensions.begin(), supportedExtensions.end(), extension) != supportedExtensions.end();
    }

    /**
	* Select the best-fit depth format for this device from a list of possible depth (and stencil) formats
	*
	* @param checkSamplingSupport Check if the format can be sampled from (e.g. for shader reads)
	*
	* @return The depth format that best fits for the current device
	*
	* @throw Throws an exception if no depth format fits the requirements
	*/
	VkFormat VulkanDevice::getSupportedDepthFormat(bool checkSamplingSupport)
    {
        // All depth formats may be optional, so we need to find a suitable depth format to use
        std::vector<VkFormat> depthFormats = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM };
        for (auto& format : depthFormats)
        {
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
            // Format must support depth stencil attachment for optimal tiling
            if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                if (checkSamplingSupport)
                {
                    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
                    {
                        continue;
                    }
                }
                return format;
            }
        }
        throw std::runtime_error("Could not find a matching depth format");
    }
}






























