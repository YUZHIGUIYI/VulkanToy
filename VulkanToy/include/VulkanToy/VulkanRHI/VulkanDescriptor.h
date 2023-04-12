//
// Created by ZHIKANG on 2023/3/28.
//

#pragma once

#include <VulkanToy/Core/Base.h>

namespace VT
{
    class DescriptorPoolCache
    {
    public:
        void init();
        void release();
        void reset();

        VkDescriptorSet allocateSet(VkDescriptorSetLayout layout, const std::string &poolName = "MainPool");
        VkDescriptorPool getPool(const std::string &poolName = "MainPool");

    private:
        VkDescriptorPool createPool(const std::string &poolName);

    private:
        std::unordered_map<std::string, VkDescriptorPool> m_pools;
    };

    class DescriptorLayoutCache
    {
    public:
        static VkDescriptorSetLayout createDescriptorLayout(VkDescriptorSetLayoutCreateInfo &info);
    };

    class DescriptorFactory final
    {
    public:
        // start building
        static DescriptorFactory begin(DescriptorPoolCache* poolCache);

        // Use for bufffers
        DescriptorFactory& bindBuffers(uint32_t binding, uint32_t count, VkDescriptorBufferInfo *bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);

        // Use for textures
        DescriptorFactory& bindImages(uint32_t binding, uint32_t count, VkDescriptorImageInfo *imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);

        bool build(VkDescriptorSet &descriptorSet, VkDescriptorSetLayout &layout);
        bool build(VkDescriptorSet &descriptorSet);

    private:
        struct DescriptorWriteContainer
        {
            VkDescriptorImageInfo *imageInfo = nullptr;
            VkDescriptorBufferInfo *bufferInfo = nullptr;
            uint32_t binding;
            uint32_t count;
            VkDescriptorType type;
            bool isImage = false;
        };

        std::vector<DescriptorWriteContainer> m_descriptorWriteBufInfos{};
        std::vector<VkDescriptorSetLayoutBinding> m_bindings;
        DescriptorPoolCache* m_cache;
    };
}
