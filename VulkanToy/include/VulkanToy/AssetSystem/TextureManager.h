//
// Created by ZHIKANG on 2023/4/2.
//

#pragma once

#include <VulkanToy/AssetSystem/AssetCommon.h>
#include <VulkanToy/AssetSystem/GPUCache.h>
#include <VulkanToy/AssetSystem/AsyncUploader.h>

namespace VT
{
    class ImageAssetBin final : public AssetBinInterface
    {
    private:
        // Raw image data, store for asset data callback
        std::vector<uint8_t> m_rawData;
        std::vector<std::vector<uint8_t>> m_mipmapData;

    public:
        ImageAssetBin() = default;
        ImageAssetBin(const std::string &name)
            : AssetBinInterface(buildUUID(), name) {}

        // TODO: finish
        void buildMipmapDataRGBA8(float cutOff);

        AssetType getAssetType() const override
        {
            return AssetType::Texture;
        }

        const std::vector<std::vector<uint8_t>>& getMipmapData() const
        {
            return m_mipmapData;
        }

        const std::vector<uint8_t>& getRawData() const
        {
            return m_rawData;
        }
    };

    class GPUImageAsset final : public GPUAssetInterface
    {
    private:
        Ref<VulkanImage> m_image = nullptr;
        uint32_t m_bindlessIndex = ~0;

    public:
        GPUImageAsset(GPUImageAsset* fallback, bool isPersistent, VkFormat format,
                        const std::string &name, uint32_t mipmpCount,
                        uint32_t width, uint32_t height, uint32_t depth);

        virtual ~GPUImageAsset();

        void release();

        uint32_t getBindlessIndex()
        {
            return getReadyAsset()->m_bindlessIndex;
        }

        // Prepare image layout when start to upload
        void prepareToUpload(CommandBufferBase &cmd, VkImageSubresourceRange range);

        // Finish image layout when ready for shader read
        void finishUpload(CommandBufferBase &cmd, VkImageSubresourceRange range);

        size_t getSize() const override
        {
            return m_image->getMemorySize();
        }

        auto& getImage() { return *m_image; }

        Ref<VulkanImage> getVulkanImage() const { return m_image; }

        GPUImageAsset* getReadyAsset()
        {
            if (isAssetLoading())
            {
                VT_CORE_ASSERT(m_fallback, "Loading asset must exit one fallback");
                return dynamic_cast<GPUImageAsset *>(m_fallback);
            }
            return this;
        }
    };

    class TextureContext final
    {
    private:
        Scope<GPUAssetCache<GPUImageAsset>> m_GPUCache;

    public:
        TextureContext() = default;

        void init();
        void release();

        bool isAssetExist(const UUID &id)
        {
            return m_GPUCache->contain(id);
        }

        void insertGPUAsset(const UUID &uuid, Ref<GPUImageAsset> image)
        {
            m_GPUCache->insert(uuid, image);
        }

        Ref<GPUImageAsset> getImage(const UUID &id)
        {
            return m_GPUCache->tryGet(id);
        }
    };

    using TextureManager = Singleton<TextureContext>;

    struct AssetTextureLoadTask : AssetLoadTask
    {
        AssetTextureLoadTask() = default;

        // Working image
        Ref<GPUImageAsset> imageAssetGPU = nullptr;

        uint32_t getUploadSize() const override
        {
            return static_cast<uint32_t>(imageAssetGPU->getSize());
        }

        void finishCallback() override
        {
            imageAssetGPU->setAsyncLoadState(false);
        }
    };

    // Load from raw data, no mipmap, persistent, no compress
    struct TextureRawDataLoadTask : AssetTextureLoadTask
    {
        std::vector<uint8_t> cacheRawData;

        TextureRawDataLoadTask() = default;

        void uploadDevice(uint32_t stageBufferOffset,
                        void *mapped,
                        CommandBufferBase &commandBuffer,
                        VulkanBuffer &stageBuffer) override;

        // Build load task from file path - slow
        static Ref<TextureRawDataLoadTask> buildFromPath(
                const std::filesystem::path &path,
                const UUID &uuid,
                VkFormat format);

        // Build load task from same value
        static Ref<TextureRawDataLoadTask> buildFlatTexture(
                const std::string &name,
                const UUID &uuid,
                const glm::uvec4 &color,
                const glm::uvec3 &size = { 1u, 1u, 1u },
                VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
    };
}
