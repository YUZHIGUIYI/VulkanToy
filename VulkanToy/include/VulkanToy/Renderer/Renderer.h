//
// Created by ZHIKANG on 2023/4/4.
//

#pragma once

#include <VulkanToy/Core/Base.h>
#include <VulkanToy/Core/RuntimeModule.h>
#include <VulkanToy/VulkanRHI/GPUResource.h>

namespace VT
{

    class Renderer
    {
    private:
        struct DepthStencil
        {
            VkImage image = VK_NULL_HANDLE;
            VkImageView imageView = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
        } m_depthStencil;

        VkRenderPass m_renderPass = VK_NULL_HANDLE;
        std::vector<VkFramebuffer> m_frameBuffers;
        // Descriptor layout
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;

        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_pipeline = VK_NULL_HANDLE;

    public:
        Renderer();
        ~Renderer();

        void init();
        void release();

        void tick(const RuntimeModuleTickData &tickData);

        void rebuildDepthAndFramebuffers();

    private:
        void createSynchronizationPrimitives();
        void setupDepthStencil();
        void setupRenderPass();
        void setupFrameBuffers();

        void setupDescriptorLayout();
        void setupPipelines();
    };

    using RendererHandle = Singleton<Renderer>;
}
