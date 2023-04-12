//
// Created by ZZK on 2023/3/1.
//

#pragma once

#include "VulkanToy/Core/Base.h"

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
