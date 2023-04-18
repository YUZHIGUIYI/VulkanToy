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
        Ref<VulkanImage> m_environmentCube = nullptr;
        Ref<VulkanImage> m_lutBRDF = nullptr;
        Ref<VulkanImage> m_irradianceCube = nullptr;
        Ref<VulkanImage> m_prefilteredCube = nullptr;
        VkSampler m_environmentCubeSampler = VK_NULL_HANDLE;
        VkSampler m_lutBRDFSampler = VK_NULL_HANDLE;
        VkSampler m_irradianceCubeSampler = VK_NULL_HANDLE;
        VkSampler m_prefilteredCubeSampler = VK_NULL_HANDLE;

    private:
        void setupTextures();
        void createSampler();

        void generateBRDFLUT();
        void generateIrradianceCube();
        void generatePrefilteredCube();

        void setupDescriptor();
        void setupPipelineLayout();
        void setupPipeline(const std::string &filePath, const VkSpecializationInfo *specializationInfo = nullptr);

        void updateDescriptorSet(uint32_t dstBinding, VkDescriptorType descriptorType, const std::vector<VkDescriptorImageInfo> &descriptors);

    public:
        Ref<VulkanImage> loadFromFile(const std::string &filename);
        void init();
        void release();

        Ref<VulkanImage> getEnvironmentCube() { return m_environmentCube; }
    };

    using PreprocessPassHandle = Singleton<PreprocessPass>;
}






























