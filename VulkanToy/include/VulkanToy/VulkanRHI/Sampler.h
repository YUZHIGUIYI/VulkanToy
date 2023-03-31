//
// Created by ZHIKANG on 2023/3/31.
//

#pragma once

#include <VulkanToy/Core/Base.h>

namespace VT
{
    struct SamplerDesc
    {
        float maxLod = 0.0f;
        float mipLodBias = 0.0f;
        VkFilter filter = VK_FILTER_NEAREST;
        VkSamplerAddressMode mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        VkCompareOp compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        bool compareEnabled = false;
        bool anisotropicFiltering = false;
    };

    struct Sampler
    {
        VkSampler m_sampler = VK_NULL_HANDLE;
        void init(const SamplerDesc& samplerDesc = SamplerDesc{});
        void relese();
    };
}
