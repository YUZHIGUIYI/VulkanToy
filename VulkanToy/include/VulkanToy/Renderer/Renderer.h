//
// Created by ZHIKANG on 2023/4/4.
//

#pragma once

#include <VulkanToy/Core/RuntimeModule.h>
#include <VulkanToy/VulkanRHI/GPUResource.h>
#include <VulkanToy/Renderer/PassCollector.h>

namespace VT
{
    struct RenderTarget
    {
        Ref<VulkanImage> colorImage = nullptr;
        Ref<VulkanImage> depthImage = nullptr;
        VkFormat colorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        VkFormat depthFormat = VK_FORMAT_D24_UNORM_S8_UINT;
        uint32_t width;
        uint32_t height;

        void release();

        static RenderTarget create(uint32_t width, uint32_t height, uint32_t samples, VkFormat colorFormat, VkFormat depthFormat);
    };

    class Renderer final
    {
    private:
        VkRenderPass m_renderPass = VK_NULL_HANDLE;
        std::vector<RenderTarget> m_renderTargets;
        std::vector<VkFramebuffer> m_frameBuffers;
        PassCollector m_passCollector{};

    public:
        Renderer();

        void init();
        void release();

        void tick(const RuntimeModuleTickData &tickData);

        void rebuildRenderTargetsAndFramebuffers();

    private:
        void setupRenderTargets();
        void setupRenderPass();
        void setupFrameBuffers();
        void setupPipelines();
    };

    using RendererHandle = Singleton<Renderer>;
}
