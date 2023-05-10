//
// Created by ZHIKANG on 2023/4/14.
//

#pragma once

#include <VulkanToy/VulkanRHI/GPUResource.h>

namespace VT
{
    class PreprocessPass final
    {
    private:
        VkDescriptorSetLayout m_computeDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSet m_computeDescriptorSet = VK_NULL_HANDLE;

        VkPipelineLayout m_computePipelineLayout = VK_NULL_HANDLE;

        VkSampler m_computeSampler = VK_NULL_HANDLE;

        // Environment map (with pre-filtered mip chain)
        Ref<VulkanImage> m_envTexture = nullptr;
        // Irradiance map
        Ref<VulkanImage> m_irmapTexture = nullptr;
        // 2D BRDF LUT for split-sum approximation
        Ref<VulkanImage> m_spBRDF_LUT = nullptr;

    private:
        // Create compute pipeline
        VkPipeline createComputePipeline(const std::string &fileName, VkPipelineLayout pipelineLayout, const VkSpecializationInfo* specializationInfo = nullptr);
        // Create common descriptor set, pipeline layout for pre-processing compute shaders and common textures for processing
        void preprocessInit();
        // Load and pre-process environment map
        void preprocessEnvMap();

    public:
        void init(VkRenderPass renderPass);

        void release();
    };
}






























