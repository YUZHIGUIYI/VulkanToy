//
// Created by ZHIKANG on 2023/3/31.
//

#pragma once

#include <VulkanToy/Core/Base.h>

namespace VT
{
    class SamplerCache
    {
    private:
        std::unordered_map<uint8_t, VkSampler> m_samplerMap;

        VkDescriptorSet m_cacheCommonDescriptor = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_cacheCommonDescriptorSetLayout = VK_NULL_HANDLE;

    private:
        void initCommonDescriptorSet();

    public:
        SamplerCache() = default;
        void init();
        void release();

        bool isContain(uint8_t keyValue);
        VkSampler createSampler(const VkSamplerCreateInfo &info, uint8_t keyValue);
        VkSampler getSampler(uint8_t keyValue);

        // Common descriptor set
        // See layout in glsl file
        VkDescriptorSet getCommonDescriptorSet();
        VkDescriptorSetLayout getCommonDescriptorSetLayout();
    };
}
