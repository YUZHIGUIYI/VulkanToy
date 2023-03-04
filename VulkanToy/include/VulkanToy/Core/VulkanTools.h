//
// Created by ZZK on 2023/3/2.
//

#pragma once

#include <VulkanToy/Core/Base.h>

#include <vulkan/vulkan.h>

namespace VT
{
    namespace TOOLS
    {
        /** @brief Disable message boxes on fatal errors */
        extern bool errorModeSilent;

        /** @brief Returns an error code as a string */
        std::string errorString(VkResult errorCode);

        /** @brief Returns the device type as a string */
        std::string physicalDeviceTypeString(VkPhysicalDeviceType type);

        // Selected a suitable supported depth format starting with 32 bit down to 16 bit
        // Returns false if none of the depth formats in the list is supported by the device
        VkBool32 getSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat *depthFormat);

        // Returns tru a given format support LINEAR filtering
        VkBool32 formatIsFilterable(VkPhysicalDevice physicalDevice, VkFormat format, VkImageTiling tiling);
        // Returns true if a given format has a stencil part
        VkBool32 formatHasStencil(VkFormat format);

        // Put an image memory barrier for setting an image layout on the sub resource into the given command buffer
        void setImageLayout(
                VkCommandBuffer cmdbuffer,
                VkImage image,
                VkImageLayout oldImageLayout,
                VkImageLayout newImageLayout,
                VkImageSubresourceRange subresourceRange,
                VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        // Uses a fixed sub resource layout with first mip level and layer
        void setImageLayout(
                VkCommandBuffer cmdbuffer,
                VkImage image,
                VkImageAspectFlags aspectMask,
                VkImageLayout oldImageLayout,
                VkImageLayout newImageLayout,
                VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

        /** @brief Insert an image memory barrier into the command buffer */
        void insertImageMemoryBarrier(
                VkCommandBuffer cmdbuffer,
                VkImage image,
                VkAccessFlags srcAccessMask,
                VkAccessFlags dstAccessMask,
                VkImageLayout oldImageLayout,
                VkImageLayout newImageLayout,
                VkPipelineStageFlags srcStageMask,
                VkPipelineStageFlags dstStageMask,
                VkImageSubresourceRange subresourceRange);

        // Display error message and exit on fatal error
        void exitFatal(const std::string& message, int32_t exitCode);
        void exitFatal(const std::string& message, VkResult resultCode);

        // Load a SPIR-V shader (binary)
        VkShaderModule loadShader(const char *fileName, VkDevice device);

        /** @brief Checks if a file exists */
        bool fileExists(const std::string &filename);

        uint32_t alignedSize(uint32_t value, uint32_t alignment);
    }
}
