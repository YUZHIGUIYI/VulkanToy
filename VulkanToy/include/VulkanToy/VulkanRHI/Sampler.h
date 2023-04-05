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
        std::unordered_map<std::string, VkSampler> m_samplerMap;

        VkDescriptorSet m_cacheCommonDescriptor = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_cacheCommonDescriptorSetLayout = VK_NULL_HANDLE;

    private:
        void initCommonDescriptorSet();

    public:
        SamplerCache() = default;
        void init();
        void release();

        VkSampler createSampler(const VkSamplerCreateInfo &info, const std::string &samplerName);

        // Common descriptor set
        // See layout in glsl file
        VkDescriptorSet getCommonDescriptorSet();
        VkDescriptorSetLayout getCommonDescriptorSetLayout();
    };
}
