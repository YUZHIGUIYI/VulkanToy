//
// Created by ZHIKANG on 2023/4/4.
//

#pragma once

#include <VulkanToy/Core/RuntimeModule.h>
#include <VulkanToy/VulkanRHI/GPUResource.h>
#include <VulkanToy/Renderer/PassCollector.h>

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

        PassCollector m_passCollector{};

    public:
        Renderer();
        ~Renderer();

        void init();
        void release();

        void tick(const RuntimeModuleTickData &tickData);

        void rebuildDepthAndFramebuffers();

    private:
        void setupDepthStencil();
        void setupRenderPass();
        void setupFrameBuffers();

        void setupPipelines();
    };

    using RendererHandle = Singleton<Renderer>;
}
