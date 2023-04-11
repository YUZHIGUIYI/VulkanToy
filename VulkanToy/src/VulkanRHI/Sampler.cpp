//
// Created by ZHIKANG on 2023/3/31.
//

#include <VulkanToy/VulkanRHI/Sampler.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>

namespace VT
{
    void SamplerCache::initCommonDescriptorSet()
    {
//        VkDescriptorImageInfo pointClampEdgeInfo{};
//        pointClampEdgeInfo.sampler = createSampler(Initializers::initPointClampEdgeSamplerInfo(), "PointClampEdge");
//
//        VkDescriptorImageInfo pointClampBorder0000Info{};
//        pointClampBorder0000Info.sampler = createSampler(Initializers::initPointClampBorder0000SamplerInfo(), "PointClampBorder0000");
//
//        VkDescriptorImageInfo pointRepeatInfo{};
//        pointRepeatInfo.sampler = createSampler(Initializers::initPointRepeatSamplerInfo(), "PointRepeat");
//
//        VkDescriptorImageInfo pointClampBorder1111Info{};
//        pointClampBorder1111Info.sampler = createSampler(Initializers::initPointClampBorder1111SamplerInfo(), "PointClampBorder1111");
//
//        VkDescriptorImageInfo linearClampEdgeInfo{};
//        linearClampEdgeInfo.sampler = createSampler(Initializers::initLinearClampEdgeMipPointSamplerInfo(), "LinearClampEdge");
//
//        VkDescriptorImageInfo linearClampBorder0000Info{};
//        linearClampBorder0000Info.sampler = createSampler(Initializers::initLinearClampBorder0000MipPointSamplerInfo(), "LinearClampBorder0000");
//
//        VkDescriptorImageInfo linearRepeatInfo{};
//        linearRepeatInfo.sampler = createSampler(Initializers::initLinearRepeatMipPointSamplerInfo(), "LinearRepeat");
//
//        VkDescriptorImageInfo linearClampBorder1111Info{};
//        linearClampBorder1111Info.sampler = createSampler(Initializers::initLinearClampBorder1111MipPointSamplerInfo(), "LinearClampBorder1111");
//
//        VulkanRHI::get()->descriptorFactoryBegin()
//                    .bindImages(0, 1, &pointClampEdgeInfo, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
//                    .bindImages(1, 1, &pointClampBorder0000Info, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
//                    .bindImages(2, 1, &pointRepeatInfo, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
//                    .bindImages(3, 1, &pointClampBorder1111Info, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
//                    .bindImages(4, 1, &linearClampEdgeInfo, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
//                    .bindImages(5, 1, &linearClampBorder0000Info, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
//                    .bindImages(6, 1, &linearRepeatInfo, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
//                    .bindImages(7, 1, &linearClampBorder1111Info, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
//                    .build(m_cacheCommonDescriptor, m_cacheCommonDescriptorSetLayout);
    }

    void SamplerCache::init()
    {
        // TODO
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

    VkSampler SamplerCache::getSampler(uint8_t keyValue)
    {
        if (m_samplerMap.contains(keyValue))
        {
            return m_samplerMap[keyValue];
        } else
        {
            VT_CORE_CRITICAL("Can not find requested sampler");
        }
    }

    VkDescriptorSet SamplerCache::getCommonDescriptorSet()
    {
        if (m_cacheCommonDescriptor == VK_NULL_HANDLE)
        {
            initCommonDescriptorSet();
        }

        return m_cacheCommonDescriptor;
    }

    VkDescriptorSetLayout SamplerCache::getCommonDescriptorSetLayout()
    {
        if (m_cacheCommonDescriptorSetLayout == VK_NULL_HANDLE)
        {
            initCommonDescriptorSet();
        }

        return m_cacheCommonDescriptorSetLayout;
    }
}