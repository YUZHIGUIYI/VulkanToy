//
// Created by ZHIKANG on 2023/3/31.
//

#pragma once

#include <VulkanToy/Core/Base.h>

namespace VT
{
    class SamplerCache final
    {
    private:
        std::unordered_map<uint8_t, VkSampler> m_samplerMap;

    private:
        void initCommonSamplers();

    public:
        SamplerCache() = default;
        void init();
        void release();

        bool isContain(uint8_t keyValue);
        VkSampler createSampler(const VkSamplerCreateInfo &info, uint8_t keyValue);
        VkSampler& getSampler(uint8_t keyValue);
    };
}
