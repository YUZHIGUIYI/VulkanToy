//
// Created by ZHIKANG on 2023/4/2.
//

#include <VulkanToy/AssetSystem/TextureManager.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>

#include <stb_image.h>
#include <stb_image_resize.h>

namespace VT
{
    const UUID EngineImages::GAlbedoImageUUID = "5albedo01-xb71-8d2y-a98f-a0f4fdje20gg";
    std::weak_ptr<GPUImageAsset>  EngineImages::GAlbedoImageAsset = {};

    const UUID EngineImages::GNormalImageUUID = "3normal75-0002-7d7y-a98f-a0f4f4d1fd53";
    std::weak_ptr<GPUImageAsset>  EngineImages::GNormalImageAsset = {};

    const UUID EngineImages::GMetallicImageUUID = "5metallic82-0853-7d7y-q21-a0fpi91fdid";
    std::weak_ptr<GPUImageAsset>  EngineImages::GMetallicImageAsset = {};

    const UUID EngineImages::GRoughnessImageUUID = "9roughness-u612-7d7y-a98f-a0f4f4d1fd53";
    std::weak_ptr<GPUImageAsset>  EngineImages::GRoughnessImageAsset = {};

    static std::string getRuntimeUniqueImageAssetName(const std::string &in)
    {
        static size_t GRuntimeId = 0;
        GRuntimeId++;
        return "VulkanToyImageAssetId:" + std::to_string(GRuntimeId) + in;
    }

    GPUImageAsset::GPUImageAsset(const std::string &name, bool isPersistent, VkFormat format,
                                uint32_t layers, uint32_t levels, uint32_t width, uint32_t height)
    : GPUAssetInterface(isPersistent)
    {
        VT_CORE_ASSERT(m_image == nullptr, "Ensure image asset only init once");

        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.flags = (layers == 6) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = format;
        imageCreateInfo.extent = { width, height, 1 };
        imageCreateInfo.arrayLayers = layers;
        imageCreateInfo.mipLevels = levels;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if (levels > 1)
        {
            imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        m_image = VulkanImage::create(
                getRuntimeUniqueImageAssetName(name).c_str(),
                imageCreateInfo,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    GPUImageAsset::~GPUImageAsset() noexcept
    {
        if (!m_isPersistent)
        {
            // TODO:
        }
    }

    void GPUImageAsset::release()
    {
        m_image->release();
    }

    void GPUImageAsset::prepareToUpload(CommandBufferBase &cmd, VkImageSubresourceRange range)
    {
        // TODO: check
        m_image->transitionLayout(cmd.cmd, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);
    }

    void GPUImageAsset::finishUpload(CommandBufferBase &cmd, VkImageSubresourceRange range)
    {
        // TODO: check
        m_image->transitionLayout(cmd.cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);
    }

    void TextureContext::init()
    {
        m_GPUCache = CreateScope<GPUAssetCache<GPUImageAsset>>(1024, 512);
    }

    void TextureContext::release()
    {
        m_GPUCache->clear();

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
        region.imageExtent = imageAssetGPU->getVulkanImage()->getExtent();

        vkCmdCopyBufferToImage(commandBuffer.cmd,
                                stageBuffer.getBuffer(),
                                imageAssetGPU->getImage(),
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                1,
                                &region);

        imageAssetGPU->finishUpload(commandBuffer, Initializers::initBasicImageSubresource());
    }

    void TextureRawDataLoadTask::buildFromPath(const std::filesystem::path &path, const UUID &uuid, VkFormat format, TextureType textureType)
    {
        if (TextureManager::Get()->isAssetExist(uuid))
        {
            VT_CORE_WARN("Persistent asset has existed, do not register again");
            return;
        }

        int32_t texWidth, texHeight, texChannels;
        stbi_uc* pixels = nullptr;
        if (textureType != TextureType::Metallic && textureType != TextureType::Roughness)
        {
            pixels = stbi_load(path.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            texChannels = 4;
        } else
        {
            pixels = stbi_load(path.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_grey);
            texChannels = 1;
        }

        if (!pixels)
        {
            VT_CORE_ERROR("Fail to load image '{0}'", path.string());
            return;
        }

        VkDeviceSize imageSize = texWidth * texHeight * texChannels;
        uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        // Staging buffer
        auto stagingBuffer = VulkanBuffer::create2(
                "Staging buffer",
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
                imageSize);
        RHICheck(stagingBuffer->map());
        stagingBuffer->copyData(pixels, static_cast<size_t>(imageSize));
        stagingBuffer->unmap();

        // Create image buffer
        auto newImageAsset = CreateRef<GPUImageAsset>(
                path.stem().string(),
                true,
                format,
                1,
                mipLevels,
                texWidth,
                texHeight);
        // Create image view
        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        newImageAsset->getVulkanImage()->createView(subresourceRange, VK_IMAGE_VIEW_TYPE_2D);
        // Create sampler if not exists
        if (!VulkanRHI::SamplerManager->isContain(static_cast<uint8_t>(textureType)))
        {
            VkPhysicalDeviceProperties properties = VulkanRHI::get()->getPhysicalDeviceProperties();
            VkSamplerCreateInfo samplerCI = Initializers::initSamplerLinear();
            samplerCI.compareOp = VK_COMPARE_OP_NEVER;
            samplerCI.mipLodBias = 0.0f;
            samplerCI.minLod = 0.0f;
            samplerCI.maxLod = static_cast<float>(mipLevels);
            samplerCI.anisotropyEnable = VK_TRUE;
            samplerCI.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
            samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            samplerCI.unnormalizedCoordinates = VK_FALSE;
            VulkanRHI::SamplerManager->createSampler(samplerCI, static_cast<uint8_t>(textureType));
        }
        // Transition image layout
        {
            const auto barrier = ImageMemoryBarrier{ newImageAsset->getImage(), 0, VK_ACCESS_TRANSFER_WRITE_BIT,
                                                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL }.mipLevels(0, 1);
            VulkanImage::transitionImageLayout(barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        }

        // Copy staging buffer to image
        newImageAsset->getVulkanImage()->copyFromStagingBuffer(stagingBuffer->getBuffer(), texWidth, texHeight);
        // Generate mipmaps, otherwise transition image layout only
        if (mipLevels > 1)
        {
            const auto barrier = ImageMemoryBarrier{newImageAsset->getImage(), VK_ACCESS_TRANSFER_WRITE_BIT, 0,
                                                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL}.mipLevels(0, 1);
            VulkanImage::transitionImageLayout(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

            newImageAsset->getVulkanImage()->generateMipmaps();
        } else
        {
            const auto barrier = ImageMemoryBarrier{ newImageAsset->getImage(), VK_ACCESS_TRANSFER_WRITE_BIT, 0,
                                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }.mipLevels(0, 1);
            VulkanImage::transitionImageLayout(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
        }
        // Release staging buffer
        stagingBuffer->release();
        // Insert GPU image asset
        TextureManager::Get()->insertGPUAsset(uuid, newImageAsset);
        // Free pixels resource
        stbi_image_free(pixels);
    }

    Ref<TextureRawDataLoadTask> TextureRawDataLoadTask::buildFlatTexture(const std::string &name, const UUID &uuid,
                                                                        const glm::uvec4 &color, const glm::uvec3 &size, VkFormat format)
    {
        if (TextureManager::Get()->isAssetExist(uuid))
        {
            VT_CORE_WARN("Persistent asset has existed, do not register again");
            return nullptr;
        }

        auto newAsset = CreateRef<GPUImageAsset>(
                name,
                true,
                format,
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












