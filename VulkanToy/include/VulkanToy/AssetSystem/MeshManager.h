//
// Created by ZHIKANG on 2023/4/2.
//

#pragma once

#include <VulkanToy/AssetSystem/MeshMisc.h>
#include <VulkanToy/AssetSystem/GPUCache.h>
#include <VulkanToy/VulkanRHI/GPUResource.h>

namespace VT
{
    class StaticMeshAssetBin final : public AssetBinInterface
    {
    private:
        std::vector<StaticMeshVertex> m_vertices;
        std::vector<VertexIndexType> m_indices{};

    public:
        StaticMeshAssetBin() = default;
        StaticMeshAssetBin(const std::string &name)
            : AssetBinInterface(buildUUID(), name) {}

        AssetType getAssetType() const override
        {
            return AssetType::StaticMesh;
        }

        const std::vector<VertexIndexType>& getIndices() const
        {
            return m_indices;
        }

        const std::vector<StaticMeshVertex>& getVertices() const
        {
            return m_vertices;
        }
    };

    class GPUMeshAsset final : GPUAssetInterface
    {
    private:
        Ref<VulkanBuffer> m_vertexBuffer = nullptr;
        Ref<VulkanBuffer> m_indexBuffer = nullptr;
        VkIndexType m_indexType = VK_INDEX_TYPE_UINT32;

        // TODO: Accelerate structure

        std::string m_name{};

        uint32_t m_singleIndexSize = 0;
        uint32_t m_indexCount = 0;
        uint32_t m_indexCountUint32Count = 0;

        uint32_t m_singleVertexSize = 0;
        uint32_t m_vertexCount = 0;
        uint32_t m_vertexFloat32Count = 0;

        uint32_t m_vertexBufferBindlessIndex = ~0;
        uint32_t m_indexBufferBindlessIndex = ~0;

    public:
        // Immediately build GPU mesh asset
        GPUMeshAsset(GPUMeshAsset *fallback, bool isPersistent, const std::string &name,
                    VkDeviceSize vertexSize, size_t singleVertexSize,
                    VkDeviceSize indexSize, VkIndexType indexType);
        // Lazily build GPU mesh asset
        GPUMeshAsset(GPUMeshAsset *fallback, bool isPersistent, const std::string &name);

        virtual ~GPUMeshAsset();

        size_t getSize() const override
        {
            return m_vertexBuffer->getMemorySize() + m_indexBuffer->getMemorySize();
        }

        void prepareToUpload();

        void finishUpload();

        auto& getVertexBuffer() { return *m_vertexBuffer; }

        VulkanBuffer* getVertexBufferRaw() { return m_vertexBuffer.get(); }

        auto& getIndexBuffer() { return *m_indexBuffer; }

        VulkanBuffer* getIndexBufferRaw() { return m_indexBuffer.get(); }

        const uint32_t& getIndicesCount() const { return m_indexCount; }

        const uint32_t& getVerticesCount() const { return m_vertexCount; }

        GPUMeshAsset* getReadyAsset()
        {
            if (isAssetLoading())
            {
                VT_CORE_ASSERT(m_fallback, "Loading asset must exist one fallback");
                return dynamic_cast<GPUMeshAsset *>(m_fallback);
            }
            return this;
        }

        uint32_t getVerticesBindlessIndex()
        {
            // TODO
            return getReadyAsset()->m_vertexBufferBindlessIndex;
        }

        uint32_t getIndicesBindlessIndex()
        {
            // TODO
            return getReadyAsset()->m_indexBufferBindlessIndex;
        }
    };

    class MeshContext
    {
    private:
        Scope<GPUAssetCache<GPUMeshAsset>> m_GPUCache;

    public:
        MeshContext() = default;

        void init();
        void release();

        bool isAssetExist(const UUID &id)
        {
            return m_GPUCache->contain(id);
        }

        void insertGPUAsset(const UUID &id, Ref<GPUMeshAsset> &mesh)
        {
            m_GPUCache->insert(id, mesh);
        }

        Ref<GPUMeshAsset> getMesh(const UUID &id)
        {
            return m_GPUCache->tryGet(id);
        }
    };

}
