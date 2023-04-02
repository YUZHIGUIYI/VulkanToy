//
// Created by ZHIKANG on 2023/4/2.
//

#pragma once

#include <VulkanToy/AssetSystem/AssetCommon.h>
#include <VulkanToy/VulkanRHI/GPUResource.h>
#include <VulkanToy/VulkanRHI/CommandBuffer.h>

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
        void buildMipmapDataRGBA8(float cutOff) {}

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
}
