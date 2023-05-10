//
// Created by ZHIKANG on 2023/4/2.
//

#pragma once

#include <VulkanToy/AssetSystem/AssetCommon.h>
#include <VulkanToy/AssetSystem/GPUCache.h>
#include <VulkanToy/AssetSystem/AsyncUploader.h>

namespace VT
{
    class GPUImageAsset;

    namespace EngineImages
    {
        extern std::weak_ptr<GPUImageAsset> GAlbedoImageAsset;
        extern const UUID GAlbedoImageUUID;

        extern std::weak_ptr<GPUImageAsset> GNormalImageAsset;
        extern const UUID GNormalImageUUID;

        extern std::weak_ptr<GPUImageAsset> GMetallicImageAsset;
        extern const UUID GMetallicImageUUID;

        extern std::weak_ptr<GPUImageAsset> GRoughnessImageAsset;
        extern const UUID GRoughnessImageUUID;
    }

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

        [[nodiscard]] AssetType getAssetType() const override
        {
            return AssetType::Texture;
        }

        [[nodiscard]] const std::vector<std::vector<uint8_t>>& getMipmapData() const
        {
            return m_mipmapData;
        }

        [[nodiscard]] const std::vector<uint8_t>& getRawData() const
        {
            return m_rawData;
        }
    };

    class GPUImageAsset final : public GPUAssetInterface
    {
    private:
        Ref<VulkanImage> m_image = nullptr;

    public:
        GPUImageAsset(const std::string &name, bool isPersistent, VkFormat format,
            uint32_t layers, uint32_t levels, uint32_t width, uint32_t height);

        ~GPUImageAsset() override;

        void release();

        // Prepare image layout when start to upload
        void prepareToUpload(CommandBufferBase &cmd, VkImageSubresourceRange range);

        // Finish image layout when ready for shader read
        void finishUpload(CommandBufferBase &cmd, VkImageSubresourceRange range);

        size_t getSize() const override
        {
            return m_image->getMemorySize();
        }

        auto& getImage() { return m_image->getImage(); }

        Ref<VulkanImage> getVulkanImage() const { return m_image; }
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

        // Build load task from file path - store in Texture Manager
        static void buildFromPath(
            const std::filesystem::path &path,
            const UUID &uuid,
            VkFormat format,
            TextureType textureType);

        // Build load task from file path - staging
        static Ref<TextureRawDataLoadTask> buildFromPath2(
            const std::filesystem::path &path,
            VkFormat format,
            TextureType textureType);
    };
}
