//
// Created by ZHIKANG on 2023/4/2.
//

#include <VulkanToy/AssetSystem/TextureManager.h>

#include <stb_image.h>
#include <stb_image_resize.h>

namespace VT
{
    static std::string getRuntimeUniqueImageAssetName(const std::string &in)
    {
        static size_t GRuntimeId = 0;
        GRuntimeId++;
        return "VulkanToyImageAssetId:" + std::to_string(GRuntimeId) + in;
    }

    // TODO: fix this mipmap function
    void ImageAssetBin::buildMipmapDataRGBA8(float cutOff)
    {
        float alphaCoverMip0 = 1.0f;
        m_mipmapData.resize(8); // TODO: real

        // TODO: look whether it is good when convert to linear space do mipmap


        for (size_t mip = 0; mip < m_mipmapData.size(); ++mip)
        {
            auto& dstMipData = m_mipmapData[mip];

            uint32_t dstWidth = 0;
        }
    }

    GPUImageAsset::GPUImageAsset(GPUImageAsset *fallback, bool isPersistent, VkFormat format, const std::string &name,
                                uint32_t mipmpCount, uint32_t width, uint32_t height, uint32_t depth)
    : GPUAssetInterface(fallback, isPersistent)
    {
        VT_CORE_ASSERT(m_image == nullptr, "Ensure image asset only init once");

        VkImageCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.flags = {};
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = format;
        info.extent.width = width;
        info.extent.height = height;
        info.extent.depth = depth;
        info.arrayLayers = 1;
        info.mipLevels = mipmpCount;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        m_image = VulkanImage::create(
            getRuntimeUniqueImageAssetName(name).c_str(),
            info,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    GPUImageAsset::~GPUImageAsset() noexcept
    {
        if (!m_isPersistent)
        {
            // TODO:
        }
    }

    void GPUImageAsset::prepareToUpload(CommandBufferBase &cmd, VkImageSubresourceRange range)
    {
        // TODO: check
        m_image->transitionLayout(cmd.cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);
    }

    void GPUImageAsset::finishUpload(CommandBufferBase &cmd, VkImageSubresourceRange range)
    {
        // TODO: check
        m_image->transitionLayout(cmd.cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);
        //

    }

    void TextureContext::init()
    {
        m_GPUCache = CreateScope<GPUAssetCache<GPUImageAsset>>(1024, 512);
    }

    void TextureContext::release()
    {
        m_GPUCache.reset();
    }

    void TextureRawDataLoadTask::uploadDevice(uint32_t stageBufferOffset, void *mapped,
                                            CommandBufferBase &commandBuffer, VulkanBuffer &stageBuffer)
    {
        VT_CORE_ASSERT(cacheRawData.size() <= AssetTextureLoadTask::getUploadSize());

        std::memcpy(mapped, cacheRawData.data(), cacheRawData.size());

        // TODO: check
        imageAssetGPU->prepareToUpload(commandBuffer, Initializers::initBasicImageSubresource());

        VkBufferImageCopy region{};
        region.bufferOffset = stageBufferOffset;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = imageAssetGPU->getImage().getExtent();

        vkCmdCopyBufferToImage(commandBuffer.cmd,
                                stageBuffer,
                                imageAssetGPU->getImage().getImage(),
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                1,
                                &region);

        imageAssetGPU->finishUpload(commandBuffer, Initializers::initBasicImageSubresource());
    }

    Ref<TextureRawDataLoadTask> TextureRawDataLoadTask::buildFromPath(const std::filesystem::path &path,
                                                                        const UUID &uuid, VkFormat format)
    {
        int32_t texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(path.string().c_str(), &texWidth, &texHeight, &texChannels, GAssetTextureChannels);

        if (!pixels)
        {
            VT_CORE_ERROR("Fail to load image '{0}'", path.string());
            return nullptr;
        }

        if (TextureManager::Get()->isAssetExist(uuid))
        {
            VT_CORE_WARN("Persistent asset has existed, do not register again");
            return nullptr;
        }

        auto newAsset = CreateRef<GPUImageAsset>(
                nullptr,
                true,
                format,
                path.stem().string(),
                1,
                texWidth,
                texHeight,
                1);
        TextureManager::Get()->insertGPUAsset(uuid, newAsset);

        // Create new task
        Ref<TextureRawDataLoadTask> newTask = CreateRef<TextureRawDataLoadTask>();
        newTask->imageAssetGPU = newAsset;

        // Prepare to upload dat
        newTask->cacheRawData.resize(texWidth * texHeight * 1 * GAssetTextureChannels);
        std::memcpy(newTask->cacheRawData.data(), pixels, newTask->cacheRawData.size());

        // NOTE: GPU memory align, which make small texture size min is 512 bytes,
        //       and may size no equal, but at least one thing is guarantee, that is cache data size must less than upload size
        VT_CORE_ASSERT(newTask->cacheRawData.size() <= newTask->getUploadSize(), "cache data size must less than upload size");

        stbi_image_free(pixels);
        return newTask;
    }

    Ref<TextureRawDataLoadTask> TextureRawDataLoadTask::buildFlatTexture(const std::string &name, const UUID &uuid,
                                                                        const glm::uvec4 &color,
                                                                        const glm::uvec3 &size, VkFormat format)
    {
        if (TextureManager::Get()->isAssetExist(uuid))
        {
            VT_CORE_WARN("Persistent asset has existed, do not register again");
            return nullptr;
        }

        auto newAsset = CreateRef<GPUImageAsset>(
                nullptr,
                true,
                format,
                name,
                1,
                size.x,
                size.y,
                size.z);
        TextureManager::Get()->insertGPUAsset(uuid, newAsset);

        // Create new task
        Ref<TextureRawDataLoadTask> newTask = CreateRef<TextureRawDataLoadTask>();
        newTask->imageAssetGPU = newAsset;

        // Prepare to upload data
        newTask->cacheRawData.resize(size.x * size.y * size.z * GAssetTextureChannels);
        for (size_t i = 0; i < newTask->cacheRawData.size(); i += GAssetTextureChannels)
        {
            newTask->cacheRawData[i + 0] = uint8_t(color.x);
            newTask->cacheRawData[i + 1] = uint8_t(color.y);
            newTask->cacheRawData[i + 2] = uint8_t(color.z);
            newTask->cacheRawData[i + 3] = uint8_t(color.w);
        }

        // NOTE: GPU memory align, which make small texture size min is 512 bytes,
        //       and may size no equal, but at least one thing is guarantee, that is cache data size must less than upload size
        VT_CORE_ASSERT(newTask->cacheRawData.size() <= newTask->getUploadSize(), "cache data size must less than upload size");

        return newTask;
    }
}

































