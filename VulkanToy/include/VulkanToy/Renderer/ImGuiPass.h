//
// Created by ZHIKANG on 2023/4/1.
//

#pragma once

#include <VulkanToy/Core/Base.h>
#include <VulkanToy/VulkanRHI/GPUResource.h>

namespace VT
{
    class ImGuiPass
    {
    private:
        // Event delegate

        // GPU resource
        struct ImGuiPassGPUResource
        {
            VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
            VkRenderPass renderPass = VK_NULL_HANDLE;

            std::vector<VkFramebuffer> frameBuffers;
            std::vector<VkCommandPool> commandPools;
            std::vector<VkCommandBuffer> commandBuffers;
        } m_renderResource;

        glm::vec4 m_clearColor{ 0.45f, 0.55f, 0.60f, 1.00f };

        bool m_isInit = false;

    private:
        const VkFormat m_drawUIFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        Ref<VulkanImage> m_drawUIImages;

        void buildRenderPass();
        void releaseRenderPass(bool isFullRelease);

    public:
        VkCommandBuffer getCommandBuffer(uint32_t index)
        {
            return m_renderResource.commandBuffers[index];
        }

        void init();
        void release();

        void renderFrame(uint32_t backBufferIndex);
    };
}















