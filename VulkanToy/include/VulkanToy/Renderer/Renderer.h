//
// Created by ZHIKANG on 2023/4/4.
//

#pragma once

#include <VulkanToy/Core/Base.h>
#include <VulkanToy/Core/RuntimeModule.h>
#include <VulkanToy/VulkanRHI/GPUResource.h>

namespace VT
{
    struct CameraParameters
    {
        alignas(16) glm::mat4 projection{};
        alignas(16) glm::mat4 view{};
        alignas(16) glm::vec3 position{};
    };

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
        Ref<VulkanBuffer> m_uniformBuffer;
        // Currently use one descriptor set for camera uniform buffer
        VkDescriptorSet m_descriptorSet;
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;

        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_pipeline = VK_NULL_HANDLE;

        CameraParameters m_cameraParas{};

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

        void setupUniformBuffers();
        void setupDescriptors();
        void setupPipelines();

        void updateUniformBuffers();
    };

    using RendererHandle = Singleton<Renderer>;
}
