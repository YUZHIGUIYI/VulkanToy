//
// Created by ZHIKANG on 2023/4/2.
//


#include <VulkanToy/AssetSystem/MeshManager.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>

namespace VT
{
    static std::string getRuntimeUniqueMeshAssetName(const std::string &in)
    {
        static size_t GRuntimeId = 0;
        GRuntimeId++;
        return "MeshAssetId:" + std::to_string(GRuntimeId) + in;
    }

    static uint32_t indexTypeToSize(VkIndexType type)
    {
        switch (type)
        {
            case VK_INDEX_TYPE_UINT16:    return sizeof(uint16_t);
            case VK_INDEX_TYPE_UINT32:    return sizeof(uint32_t);
            case VK_INDEX_TYPE_UINT8_EXT: return sizeof(uint8_t);
            default:
                VT_CORE_CRITICAL("Unknown VkIndexType");
        }
        return 0;
    }

    // Immediately build GPU mesh asset
    GPUMeshAsset::GPUMeshAsset(GPUMeshAsset *fallback, bool isPersistent, const std::string &name,
        VkDeviceSize vertexSize, size_t singleVertexSize, VkDeviceSize indexSize,
        VkIndexType indexType)
    : GPUAssetInterface(fallback, isPersistent), m_name(name)
    {
        VT_CORE_ASSERT(m_vertexBuffer == nullptr, "Ensure that GPU mesh asset initialized only once");
        VT_CORE_ASSERT(m_indexBuffer == nullptr, "Ensure that GPU mesh asset initialized only once");

        auto bufferFlagBasic = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        VmaAllocationCreateFlags vmaBufferFlags{};

        m_vertexBuffer = VulkanBuffer::create2(
            getRuntimeUniqueMeshAssetName(name).c_str(),
            bufferFlagBasic | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            vmaBufferFlags,
            vertexSize);
        m_indexBuffer = VulkanBuffer::create2(
            getRuntimeUniqueMeshAssetName(name).c_str(),
            bufferFlagBasic | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            vmaBufferFlags,
            indexSize);

        m_indexType = indexType;
        m_singleIndexSize = indexTypeToSize(indexType);
        m_indexCount = uint32_t(indexSize) / indexTypeToSize(indexType);
        m_indexCountUint32Count = uint32_t(indexSize) / sizeof(uint32_t);

        m_singleVertexSize = uint32_t(singleVertexSize);
        m_vertexCount = uint32_t(vertexSize) / m_singleVertexSize;
        m_vertexFloat32Count = uint32_t(vertexSize) / sizeof(float);
    }

    GPUMeshAsset::GPUMeshAsset(GPUMeshAsset *fallback, bool isPersistent, const std::string &name)
    : GPUAssetInterface(fallback, isPersistent), m_name(name)
    {

    }

    GPUMeshAsset::~GPUMeshAsset()
    {
        if (!m_isPersistent)
        {
            if (m_vertexBufferBindlessIndex != ~0)
            {
                // TODO
            }
            if (m_indexBufferBindlessIndex != ~0)
            {
                // TODO
            }
        }

        m_vertexBuffer.reset();
        m_indexBuffer.reset();
    }

    void GPUMeshAsset::prepareToUpload()
    {
        VT_CORE_ASSERT(m_vertexBufferBindlessIndex == ~0, "Prepare upload");
        VT_CORE_ASSERT(m_indexBufferBindlessIndex == ~0, "Prepare upload");
    }

    void GPUMeshAsset::finishUpload()
    {
        // TODO: separate
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = *m_vertexBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range  = m_vertexBuffer->getMemorySize();    // TODO: fix
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        VkDescriptorSet descriptorSetUpdateAfterBind{};
        write.dstSet = descriptorSetUpdateAfterBind;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.dstBinding = 0;
        write.pBufferInfo = &bufferInfo;
        write.descriptorCount = 1;
        write.dstArrayElement = 0;  // TODO: fix
        vkUpdateDescriptorSets(VulkanRHI::Device, 1, &write, 0, nullptr);
        m_vertexBufferBindlessIndex = write.dstArrayElement;

        // TODO: separate
        VkDescriptorBufferInfo bufferInfo2{};
        bufferInfo2.buffer = *m_indexBuffer;
        bufferInfo2.offset = 0;
        bufferInfo2.range  = m_indexBuffer->getMemorySize();
        VkWriteDescriptorSet write2{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        VkDescriptorSet descriptorSetUpdateAfterBind2{};
        write.dstSet = descriptorSetUpdateAfterBind2;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.dstBinding = 0;
        write.pBufferInfo = &bufferInfo2;
        write.descriptorCount = 1;
        write.dstArrayElement = 0;  // TODO: fix
        vkUpdateDescriptorSets(VulkanRHI::Device, 1, &write2, 0, nullptr);
        m_indexBufferBindlessIndex = write2.dstArrayElement;

        VT_CORE_ASSERT(m_vertexBufferBindlessIndex != ~0, "Fail to upload")
        VT_CORE_ASSERT(m_indexBufferBindlessIndex != ~0, "Fail to upload");
    }
}