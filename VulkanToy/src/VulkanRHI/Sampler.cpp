//
// Created by ZHIKANG on 2023/3/31.
//

#include <VulkanToy/VulkanRHI/Sampler.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>

namespace VT
{
    void SamplerCache::initCommonSamplers()
    {

    }

    void SamplerCache::init()
    {
        initCommonSamplers();
    }

    void SamplerCache::release()
    {
        for (auto& sampler : m_samplerMap)
        {
            vkDestroySampler(VulkanRHI::Device, sampler.second, nullptr);
        }
        m_samplerMap.clear();
    }

    bool SamplerCache::isContain(uint8_t keyValue)
    {
        return m_samplerMap.contains(keyValue);
    }

    VkSampler SamplerCache::createSampler(const VkSamplerCreateInfo &info, uint8_t keyValue)
    {
        if (m_samplerMap.contains(keyValue))
        {
            return m_samplerMap[keyValue];
        } else
        {
            VkSampler sampler;
            RHICheck(vkCreateSampler(VulkanRHI::Device, &info, nullptr, &sampler));
            m_samplerMap[keyValue] = sampler;
            return sampler;
        }
    }

    VkSampler& SamplerCache::getSampler(uint8_t keyValue)
    {
        if (m_samplerMap.contains(keyValue))
        {
            return m_samplerMap[keyValue];
        } else
        {
            VT_CORE_CRITICAL("Can not find requested sampler");
        }
    }
}