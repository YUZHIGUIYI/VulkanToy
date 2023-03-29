//
// Created by ZZK on 2023/3/1.
//

#pragma once

#include "VulkanToy/Core/Base.h"
#include "VulkanToy/Core/VulkanBuffer.h"

namespace VT
{
    struct VulkanDevice
    {
        /** @brief Physical device representation */
        VkPhysicalDevice physicalDevice;
        /** @brief Logical device representation (application's view of the device) */
        VkDevice logicalDevice;
        /** @brief Properties of the physical device including limits that the application can check against */
        VkPhysicalDeviceProperties properties;
        /** @brief Features of the physical device that an application can use to check if a feature is supported */
        VkPhysicalDeviceFeatures features;
        /** @brief Features that have been enabled for use on the physical device */
        VkPhysicalDeviceFeatures enabledFeatures;
        /** @brief Memory types and heaps of the physical device */
        VkPhysicalDeviceMemoryProperties memoryProperties;
        /** @brief Queue family properties of the physical device */
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;

        VkPhysicalDeviceDescriptorIndexingPropertiesEXT descriptorIndexingProperties{};

        VkFormat cacheSupportDepthStencilFormat;
        VkFormat cacheSupportDepthOnlyFormat;

        bool enableDebugMarkers = false;

        struct QueuesInfo
        {
            uint32_t graphicsFamily = ~0;
            uint32_t copyFamily  = ~0;
            uint32_t computeFamily = ~0;

            std::vector<VkQueue> graphicsQueues;
            std::vector<VkQueue> copyQueues;
            std::vector<VkQueue> computeQueues;
        };
        QueuesInfo queueInfos;

        struct CommandPool
        {
            VkQueue queue = VK_NULL_HANDLE;
            VkCommandPool pool = VK_NULL_HANDLE;
        };

        // Major graphics queue with priority 1.0f
        CommandPool majorGraphicsPool;
        // Major compute queue with priority 0.8f, for Async Scheduler
        CommandPool majorComputePool;
        // Second major queue with priority 0.8f, for Async Scheduler
        CommandPool secondMajorGraphicsPool;
        // Other command pool with priority 0.5f
        std::vector<CommandPool> graphicsPools;
        std::vector<CommandPool> computePools;
        // Copy pool used for async uploader
        std::vector<CommandPool> copyPools;

        uint32_t        getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound = nullptr) const;
        uint32_t        getQueueFamilyIndex(VkQueueFlags queueFlags) const;
        VkResult        createLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures, std::vector<const char *> enabledExtensions, void *pNextChain, bool useSwapChain = true, VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
        VkResult        createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer *buffer, VkDeviceMemory *memory, void *data = nullptr);
        VkResult        createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VT::Buffer *buffer, VkDeviceSize size, void *data = nullptr);
        void            copyBuffer(VT::Buffer *src, VT::Buffer *dst, VkQueue queue, VkBufferCopy *copyRegion = nullptr);
        VkCommandPool   createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin = false);
        VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, bool begin = false);
        void            flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free = true);
        void            flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free = true);
        bool            extensionSupported(const std::string& extension);
        VkFormat        getSupportedDepthFormat(bool checkSamplingSupport);

        operator VkDevice() const
        {
            return logicalDevice;
        };

        VulkanDevice() = default;
        ~VulkanDevice() = default;

        bool isPhysicalDeviceSuitable(std::vector<char const *> const &requestExtensions);
        void pickupSuitableGPU(std::vector<char const *> const &requestExtensions);
        void createLogicDevice(VkPhysicalDeviceFeatures features, const std::vector<const char *> &requestExtensions, void *nextChain);

        VkFormat findSupportedFormat(std::vector<VkFormat> const &candidates, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);
        int32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memoryPropertyFlags);

        void initCommandPool();
        void releaseCommandPool();

        void init(VkPhysicalDeviceFeatures features, const std::vector<const char *> &requestExtensions, void *nextChain);
        void release();
    };
}
