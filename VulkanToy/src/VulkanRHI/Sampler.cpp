//
// Created by ZHIKANG on 2023/3/31.
//

#include <VulkanToy/VulkanRHI/Sampler.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>

namespace VT
{
    void Sampler::init(const SamplerDesc &samplerDesc)
    {
        VkSamplerCreateInfo samplerInfo = Initializers::initSamplerCreateInfo(samplerDesc.filter, samplerDesc.mode,
                                            samplerDesc.mipmapMode, samplerDesc.maxLod, samplerDesc.mipLodBias);
        if (samplerDesc.anisotropicFiltering)
        {
            VkPhysicalDeviceProperties deviceProperties = VulkanRHI::get()->getPhysicalDeviceProperties();
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = deviceProperties.limits.maxSamplerAnisotropy;
        }
        if (samplerDesc.compareEnabled)
        {
            samplerInfo.compareEnable = VK_TRUE;
            samplerInfo.compareOp = samplerDesc.compareOp;
        }

        RHICheck(vkCreateSampler(VulkanRHI::Device, &samplerInfo, nullptr, &m_sampler));
    }

    void Sampler::relese()
    {
        if (m_sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(VulkanRHI::Device, m_sampler, nullptr);
        }
    }
}