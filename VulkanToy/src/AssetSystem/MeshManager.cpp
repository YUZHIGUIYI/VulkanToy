//
// Created by ZHIKANG on 2023/4/2.
//


#include <VulkanToy/AssetSystem/MeshManager.h>
#include <VulkanToy/AssetSystem/AssimpModelProcess.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>

namespace VT
{
    const UUID EngineMeshes::GBoxUUID = "12a68c4e-8352-4d97-a914-a0f4f4d1fd28";
    const UUID EngineMeshes::GSphereUUID = "45f0d878-6d3f-11ed-a1eb-0242ac120002";
    const UUID EngineMeshes::GCerberusUUID = "76f0v327-6d3e-41ui-n92j-9733lk128178";
    const UUID EngineMeshes::GSkyBoxUUID = "90nhgo521-xxki-ilfg-kh43-0ui6lk163e3";

    std::weak_ptr<GPUMeshAsset> EngineMeshes::GBoxPtrRef = {};
    std::weak_ptr<GPUMeshAsset> EngineMeshes::GSpherePtrRef = {};
    std::weak_ptr<GPUMeshAsset> EngineMeshes::GCerberusRef = {};
    std::weak_ptr<GPUMeshAsset> EngineMeshes::GSkyBoxRef = {};

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

    // Separate
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                        VkBuffer &buffer, VkDeviceMemory &bufferMemory)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(VulkanRHI::Device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
            throw std::runtime_error("Failed to create buffer!");

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(VulkanRHI::Device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = VulkanRHI::get()->findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(VulkanRHI::Device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
        {
            VT_CORE_CRITICAL("Fail to allocate buffer memory");
        }

        vkBindBufferMemory(VulkanRHI::Device, buffer, bufferMemory, 0);
    }

    // Immediately build GPU mesh asset
    GPUMeshAsset::GPUMeshAsset(GPUMeshAsset *fallback, bool isPersistent, const std::string &name,
        VkDeviceSize vertexSize, size_t singleVertexSize, VkDeviceSize indexSize,
        VkIndexType indexType)
    : GPUAssetInterface(fallback, isPersistent), m_name(name)
    {
        VT_CORE_ASSERT(m_vertexBuffer == nullptr, "Ensure that GPU mesh asset initialized only once");
        VT_CORE_ASSERT(m_indexBuffer == nullptr, "Ensure that GPU mesh asset initialized only once");

        auto bufferFlagBasic = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
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

    void GPUMeshAsset::release()
    {
        m_vertexBuffer->release();
        m_indexBuffer->release();
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
        bufferInfo.buffer = m_vertexBuffer->getBuffer();
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
        bufferInfo2.buffer = m_indexBuffer->getBuffer();
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

    void MeshContext::init()
    {
        m_GPUCache = CreateScope<GPUAssetCache<GPUMeshAsset>>(512, 256);
    }

    void MeshContext::release()
    {
        m_GPUCache->clear();
        m_GPUCache.reset();
    }

    void StaticMeshRawDataLoadTask::uploadDevice(uint32_t stageBufferOffset, void *mapped,
                                                CommandBufferBase &commandBuffer, VulkanBuffer &stageBuffer)
    {
        VT_CORE_ASSERT(AssetMeshLoadTask::getUploadSize() == static_cast<uint32_t>(cacheVertexData.size() + cacheIndexData.size()), "Upload device size mismatch");

        uint32_t indexOffsetInSrcBuffer = 0;
        uint32_t vertexOffsetInSrcBuffer = indexOffsetInSrcBuffer + static_cast<uint32_t>(cacheIndexData.size());

        std::memcpy((void *)((char *)mapped + indexOffsetInSrcBuffer), cacheIndexData.data(), cacheIndexData.size());
        std::memcpy((void *)((char *)mapped + vertexOffsetInSrcBuffer), cacheVertexData.data(), cacheVertexData.size());

        // TODO: check
        meshAssetGPU->prepareToUpload();

        {
            VkBufferCopy regionIndex{};
            regionIndex.size = VkDeviceSize(cacheIndexData.size());
            regionIndex.srcOffset = indexOffsetInSrcBuffer;
            regionIndex.dstOffset = 0;
            vkCmdCopyBuffer(commandBuffer.cmd, stageBuffer.getBuffer(), meshAssetGPU->getIndexBuffer(), 1, &regionIndex);
        }

        {
            VkBufferCopy regionVertex{};
            regionVertex.size = VkDeviceSize(cacheVertexData.size());
            regionVertex.srcOffset = vertexOffsetInSrcBuffer;
            regionVertex.dstOffset = 0;
            vkCmdCopyBuffer(commandBuffer.cmd, stageBuffer.getBuffer(), meshAssetGPU->getVertexBuffer(), 1, &regionVertex);
        }

        // TODO: check
        meshAssetGPU->finishUpload();
    }

    Ref<StaticMeshRawDataLoadTask> StaticMeshRawDataLoadTask::buildFromData(const std::string &name, const UUID &uuid,
                                                                            bool isPersistent, uint8_t *indices,
                                                                            size_t indexSize, VkIndexType indexType,
                                                                            uint8_t *vertices, size_t vertexSize,
                                                                            size_t singleVertexSize)
    {
        if (isPersistent)
        {
            if (MeshManager::Get()->isAssetExist(uuid))
            {
                // TODO: fix
                VT_CORE_WARN("Persistent asset has exist, do not register again");
                return nullptr;
            }
        }

        auto newTask = CreateRef<StaticMeshRawDataLoadTask>();
        newTask->cacheVertexData.resize(vertexSize);
        newTask->cacheIndexData.resize(indexSize);

        std::memcpy((void *)(newTask->cacheVertexData.data()), (void *)vertices, vertexSize);
        std::memcpy((void *)(newTask->cacheIndexData.data()), (void *)indices, indexSize);

        // TODO: check
        GPUMeshAsset* fallback = nullptr;
        auto newAsset = CreateRef<GPUMeshAsset>(
                fallback,
                isPersistent,
                name,
                vertexSize,
                singleVertexSize,
                indexSize,
                indexType);
        MeshManager::Get()->insertGPUAsset(uuid, newAsset);
        newTask->meshAssetGPU = newAsset;

        return newTask;
    }

    Ref<StaticMeshRawDataLoadTask> StaticMeshRawDataLoadTask::buildFromPath(const std::string &name,
                                                                            const std::filesystem::path &path,
                                                                            const UUID &uuid, bool isPersistent)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path.string(),
            aiProcessPreset_TargetRealtime_Fast | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes |
                aiProcess_PreTransformVertices | aiProcess_OptimizeMeshes | aiProcess_Debone | aiProcess_ValidateDataStructure);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            VT_CORE_ERROR("Error::Assimp::{0}", importer.GetErrorString());
            return nullptr;
        }

        AssimpModelProcess processor{ path.parent_path() };
        // processor.processNode(scene->mRootNode, scene);        // Old method
        processor.processMesh(scene->mMeshes[0], scene);    // New method

        if (isPersistent)
        {
            if (MeshManager::Get()->isAssetExist(uuid))
            {
                VT_CORE_WARN("In file '{0}', in line '{1}', persistent asset has exist, do not register again", __FILE__, __LINE__);
                return nullptr;
            }
        }

        auto newTask = CreateRef<StaticMeshRawDataLoadTask>();
        newTask->cacheVertexData.resize(processor.m_vertices.size() * sizeof(processor.m_vertices[0]));
        newTask->cacheIndexData.resize(processor.m_indices.size() * sizeof(processor.m_indices[0]));

        std::memcpy((void *)(newTask->cacheVertexData.data()), (void *)processor.m_vertices.data(), newTask->cacheVertexData.size());
        std::memcpy((void *)(newTask->cacheIndexData.data()), (void *)processor.m_indices.data(), newTask->cacheIndexData.size());

        // TODO: separate
        // Staging buffer vertex
        VkDeviceSize vertexBufferSize = sizeof(StaticMeshVertex) * processor.m_vertices.size();
        VkBuffer stagingVertexBuffer;
        VkDeviceMemory stagingVertexBufferMemory;
        createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingVertexBuffer, stagingVertexBufferMemory);
        // Staging buffer index
        VkDeviceSize indexBufferSize = sizeof(VertexIndexType) * processor.m_indices.size();
        VkBuffer stagingIndexBuffer;
        VkDeviceMemory stagingIndexBufferMemory;
        createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingIndexBuffer, stagingIndexBufferMemory);

        void* vertexData;
        vkMapMemory(VulkanRHI::Device, stagingVertexBufferMemory, 0, vertexBufferSize, 0, &vertexData);
        std::memcpy(vertexData, processor.m_vertices.data(), (size_t)vertexBufferSize);
        vkUnmapMemory(VulkanRHI::Device, stagingVertexBufferMemory);

        void* indexData;
        vkMapMemory(VulkanRHI::Device, stagingIndexBufferMemory, 0, indexBufferSize, 0, &indexData);
        std::memcpy(indexData, processor.m_indices.data(), (size_t)indexBufferSize);
        vkUnmapMemory(VulkanRHI::Device, stagingIndexBufferMemory);

        // TODO: check
        GPUMeshAsset* fallback = nullptr;
        VT_CORE_ASSERT(sizeof(VertexIndexType) == 4, "Currently VertexIndexType must be uint32_t");
        auto newAsset = CreateRef<GPUMeshAsset>(
                fallback,
                isPersistent,
                name,
                processor.m_vertices.size() * sizeof(processor.m_vertices[0]),
                sizeof(processor.m_vertices[0]),
                processor.m_indices.size() * sizeof(processor.m_indices[0]),
                VK_INDEX_TYPE_UINT32);

        // TODO: separate
        {
            VkCommandBuffer commandBuffer = VulkanRHI::get()->createMajorGraphicsCommandBuffer();
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            // Vertex and index
            VkBufferCopy  copyRegionVertex{};
            copyRegionVertex.size = vertexBufferSize;
            vkCmdCopyBuffer(commandBuffer, stagingVertexBuffer, newAsset->getVertexBuffer(), 1, &copyRegionVertex);

            VkBufferCopy copyRegionIndex{};
            copyRegionIndex.size = indexBufferSize;
            vkCmdCopyBuffer(commandBuffer, stagingIndexBuffer, newAsset->getIndexBuffer(), 1, &copyRegionIndex);

            vkEndCommandBuffer(commandBuffer);
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            vkQueueSubmit(VulkanRHI::get()->getMajorGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(VulkanRHI::get()->getMajorGraphicsQueue());

            vkFreeCommandBuffers(VulkanRHI::Device, VulkanRHI::get()->getMajorGraphicsCommandPool(), 1, &commandBuffer);
        }

        {
            vkDestroyBuffer(VulkanRHI::Device, stagingVertexBuffer, nullptr);
            vkFreeMemory(VulkanRHI::Device, stagingVertexBufferMemory, nullptr);
            vkDestroyBuffer(VulkanRHI::Device, stagingIndexBuffer, nullptr);
            vkFreeMemory(VulkanRHI::Device, stagingIndexBufferMemory, nullptr);
        }

        MeshManager::Get()->insertGPUAsset(uuid, newAsset);
        newTask->meshAssetGPU = newAsset;

        return newTask;
    }
}















