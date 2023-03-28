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
        /** @brief List of extensions supported by the device */
        std::vector<std::string> supportedExtensions;
        /** @brief Default command pool for the graphics queue family index */
        VkCommandPool commandPool = VK_NULL_HANDLE;
        /** @brief Set to true when the debug marker extension is detected */
        bool enableDebugMarkers = false;

        /** @brief Contains queue family indices */
        struct
        {
            uint32_t graphics;
            uint32_t compute;
            uint32_t transfer;
        } queueFamilyIndices;

        struct QueuesInfo
        {
            uint32_t graphicsFamily = ~0;
            uint32_t copyFamily  = ~0;
            uint32_t computeFamily = ~0;

            std::vector<VkQueue> graphicsQueues;
            std::vector<VkQueue> copyQueues;
            std::vector<VkQueue> computeQueues;
        };
        QueuesInfo m_queueInfos;

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

        void init(VkPhysicalDevice physicalDevice);
        void release();
    };
}
