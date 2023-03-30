//
// Created by ZHIKANG on 2023/3/28.
//

#include <VulkanToy/VulkanRHI/VulkanDescriptor.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>

namespace VT
{
    const char *mainPoolName = "MainPool";
    const char *ImGuiPoolName = "ImGuiPool";

    void DescriptorPoolCache::init()
    {
        createPool(mainPoolName);
        createPool(ImGuiPoolName);
    }

    void DescriptorPoolCache::release()
    {
        for (auto& pool : m_pools)
        {
            vkDestroyDescriptorPool(VulkanRHI::Device, pool.second, nullptr);
        }
    }

    void DescriptorPoolCache::reset()
    {
        for (auto& pool : m_pools)
        {
            vkResetDescriptorPool(VulkanRHI::Device, pool.second, 0);
        }
        m_pools.clear();
    }

    VkDescriptorSet DescriptorPoolCache::allocateSet(VkDescriptorSetLayout layout, const std::string &poolName)
    {
        if (!m_pools.contains(poolName))
        {
            createPool(poolName);
        }

        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.pNext = nullptr;
        allocateInfo.descriptorPool = m_pools[poolName];
        allocateInfo.descriptorSetCount = 1;    // TODO: more
        allocateInfo.pSetLayouts = &layout;

        VkDescriptorSet descriptorSet;
        auto result = vkAllocateDescriptorSets(VulkanRHI::Device, &allocateInfo, &descriptorSet);
        if (result != VK_SUCCESS)
        {
            VT_CORE_CRITICAL("Fail to allocate descriptor set from pool '{0}'", poolName);
        }

        return descriptorSet;
    }

    VkDescriptorPool DescriptorPoolCache::getPool(const std::string &poolName)
    {
        if (m_pools.contains(poolName))
        {
            return m_pools[poolName];
        } else
        {
            return createPool(poolName);
        }
    }

    VkDescriptorPool DescriptorPoolCache::createPool(const std::string &poolName)
    {
        std::vector<VkDescriptorPoolSize> poolSizes{
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, DESCRIPTOR_POOL_SIZE },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, DESCRIPTOR_POOL_SIZE },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, DESCRIPTOR_POOL_SIZE },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, DESCRIPTOR_POOL_SIZE },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, DESCRIPTOR_POOL_SIZE },
            { VK_DESCRIPTOR_TYPE_SAMPLER, DESCRIPTOR_POOL_SIZE }
        };

        VkDescriptorPoolCreateInfo poolCreateInfo{};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCreateInfo.flags = 0;
        poolCreateInfo.maxSets = static_cast<uint32_t>(poolSizes.size() * DESCRIPTOR_POOL_SIZE);
        poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolCreateInfo.pPoolSizes = poolSizes.data();

        VkDescriptorPool pool;
        RHICheck(vkCreateDescriptorPool(VulkanRHI::Device, &poolCreateInfo, nullptr, &pool));

        if (auto it = m_pools.find(poolName); it == m_pools.end())
        {
            m_pools[poolName] = pool;
        }
        return m_pools[poolName];
    }

    DescriptorFactory DescriptorFactory::begin(DescriptorPoolCache *poolCache)
    {
        DescriptorFactory builder{};
        builder.m_cache = poolCache;
        return builder;
    }

    VkDescriptorSetLayout DescriptorLayoutCache::createDescriptorLayout(VkDescriptorSetLayoutCreateInfo &info)
    {
        VkDescriptorSetLayout layout;
        vkCreateDescriptorSetLayout(VulkanRHI::Device, &info, nullptr, &layout);
        return layout;
    }

    DescriptorFactory& DescriptorFactory::bindBuffers(uint32_t binding, uint32_t count, VkDescriptorBufferInfo *bufferInfo,
                                                        VkDescriptorType type, VkShaderStageFlags stageFlags)
    {
        VkDescriptorSetLayoutBinding newBinding{};

        newBinding.descriptorCount = count;
        newBinding.descriptorType = type;
        newBinding.pImmutableSamplers = nullptr;
        newBinding.stageFlags = stageFlags;
        newBinding.binding = binding;

        m_bindings.push_back(newBinding);

        DescriptorWriteContainer descriptorWrite{};
        descriptorWrite.isImage = false;
        descriptorWrite.bufferInfo = bufferInfo;
        descriptorWrite.type = type;
        descriptorWrite.binding = binding;
        descriptorWrite.count = count;
        m_descriptorWriteBufInfos.push_back(descriptorWrite);

        return *this;
    }

    DescriptorFactory& DescriptorFactory::bindImages(uint32_t binding, uint32_t count, VkDescriptorImageInfo *imageInfo,
                                                        VkDescriptorType type, VkShaderStageFlags stageFlags)
    {
        VkDescriptorSetLayoutBinding newBinding{};

        newBinding.descriptorCount = count;
        newBinding.descriptorType = type;
        newBinding.pImmutableSamplers = nullptr;
        newBinding.stageFlags = stageFlags;
        newBinding.binding = binding;

        m_bindings.push_back(newBinding);

        DescriptorWriteContainer descriptorWrite{};
        descriptorWrite.isImage = true;
        descriptorWrite.imageInfo = imageInfo;
        descriptorWrite.type = type;
        descriptorWrite.binding = binding;
        descriptorWrite.count = count;

        m_descriptorWriteBufInfos.push_back(descriptorWrite);

        return *this;
    }

    bool DescriptorFactory::build(VkDescriptorSet &descriptorSet, VkDescriptorSetLayout &layout)
    {
        VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
        layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutCreateInfo.pBindings = m_bindings.data();
        layoutCreateInfo.bindingCount = static_cast<uint32_t>(m_bindings.size());

        VkDescriptorSetLayout retLayout = DescriptorLayoutCache::createDescriptorLayout(layoutCreateInfo);
        VkDescriptorSet retSet = m_cache->allocateSet(retLayout);

        // TODO: fix
        std::vector<VkWriteDescriptorSet> writes{};
        writes.reserve(m_descriptorWriteBufInfos.size());
        for (auto& descriptorWrite : m_descriptorWriteBufInfos)
        {
            VkWriteDescriptorSet newWrite{};
            newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            newWrite.pNext = nullptr;
            newWrite.descriptorCount = descriptorWrite.count;
            newWrite.descriptorType = descriptorWrite.type;

            if (descriptorWrite.isImage)
            {
                newWrite.pImageInfo = descriptorWrite.imageInfo;
            } else
            {
                newWrite.pBufferInfo = descriptorWrite.bufferInfo;
            }

            newWrite.dstBinding = descriptorWrite.binding;
            newWrite.dstSet = retSet;
            writes.push_back(newWrite);
        }

        vkUpdateDescriptorSets(VulkanRHI::Device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

        descriptorSet = retSet;
        layout = retLayout;

        return true;
    }

    bool DescriptorFactory::build(VkDescriptorSet &descriptorSet)
    {
        VkDescriptorSetLayout layout;
        return build(descriptorSet, layout);
    }
}










