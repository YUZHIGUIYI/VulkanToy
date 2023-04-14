//
// Created by ZHIKANG on 2023/4/14.
//

#include <VulkanToy/Renderer/PreprocessPass.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>
#include <VulkanToy/AssetSystem/AssetCommon.h>

#include <ktx.h>
#include <ktxvulkan.h>

namespace VT
{
    // numMipmapLevels function
    template<typename T>
    static constexpr T numMipmapLevels(T width, T height)
    {
        T levels = 1;
        while((width | height) >> levels)
        {
            ++levels;
        }
        return levels;
    }
    // Parameters
    static constexpr uint32_t kEnvMapSize = 1024;
    static constexpr uint32_t kIrradianceMapSize = 32;
    static constexpr uint32_t kBRDF_LUT_Size = 256;
    static constexpr uint32_t kEnvMapLevels = numMipmapLevels(kEnvMapSize, kEnvMapSize);
    static constexpr VkDeviceSize kUniformBufferSize = 64 * 1024;
    // Push constants
    struct SpecularFilterPushConstants
    {
        uint32_t level;
        float roughness;
    };

    Ref<VkImageMemoryBarrier> createImageMemoryBarrier(Ref<VulkanImage> vulkanImage, VkImageLayout oldLayout, VkImageLayout newLayout,
                                                        VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask)
    {
        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = vulkanImage->getInfo().mipLevels;      // VK_REMAINING_MIP_LEVELS
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = vulkanImage->getInfo().arrayLayers;    // VK_REMAINING_ARRAY_LAYERS

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = vulkanImage->getImage();
        barrier.subresourceRange = subresourceRange;
        barrier.srcAccessMask = srcAccessMask;
        barrier.dstAccessMask = dstAccessMask;

        return CreateRef<VkImageMemoryBarrier>(barrier);
    }

    Ref<VulkanImage> createVulkanImage(uint32_t width, uint32_t height, uint32_t layers, VkFormat format, uint32_t levels = 0, VkImageUsageFlags additionalUsage = 0, const std::string &name = "Temp")
    {
        // envTextureUnfiltered image
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
        if (additionalUsage != 0)
        {
            imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | additionalUsage;
        }
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        auto imageCache =  VulkanImage::create(
                name.c_str(), imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkImageViewType viewType = (layers == 6) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = levels;    // VK_REMAINING_MIP_LEVELS
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = layers;        // VK_REMAINING_ARRAY_LAYERS
        imageCache->createView(subresourceRange, viewType);

        return imageCache;
    }

    Ref<VulkanImage> PreprocessPass::loadFromFile(const std::string &filename)
    {
        ktxTexture* texture;
        ktxResult result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);
        VT_CORE_ASSERT(result == KTX_SUCCESS, "Fail to load environment cube");

        uint32_t width = texture->baseWidth;
        uint32_t height = texture->baseHeight;
        uint32_t mipLevels = texture->numLevels;

        ktx_uint8_t* ktxTextureData = ktxTexture_GetData(texture);
        auto ktxTextureSize = ktxTexture_GetSize(texture);

        // Staging buffer
        auto stagingBuffer = VulkanBuffer::create2(
                "Staging buffer",
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
                (VkDeviceSize)ktxTextureSize);
        RHICheck(stagingBuffer->map());
        stagingBuffer->copyData(ktxTextureData, ktxTextureSize);
        stagingBuffer->unmap();

        // Setup buffer copy regions for each mip level
        std::vector<VkBufferImageCopy> bufferCopyRegions;
        bufferCopyRegions.reserve(mipLevels * 6);
        for (uint32_t face = 0; face < 6; ++face)
        {
            for (uint32_t level = 0; level < mipLevels; ++level)
            {
                ktx_size_t offset;
                KTX_error_code errorCode = ktxTexture_GetImageOffset(texture, level, 0, face, &offset);
                VT_CORE_ASSERT(errorCode == KTX_SUCCESS, "Fail to get environment cube offset");

                VkBufferImageCopy bufferCopyRegion{};
                bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                bufferCopyRegion.imageSubresource.mipLevel = level;
                bufferCopyRegion.imageSubresource.baseArrayLayer = face;
                bufferCopyRegion.imageSubresource.layerCount = 1;
                bufferCopyRegion.imageExtent.width = texture->baseWidth >> level;
                bufferCopyRegion.imageExtent.height = texture->baseHeight >> level;
                bufferCopyRegion.imageExtent.depth = 1;
                bufferCopyRegion.bufferOffset = offset;

                bufferCopyRegions.push_back(bufferCopyRegion);
            }
        }

        // Create optimal tiled target image
        auto environmentCube = createVulkanImage(width, height, 6, VK_FORMAT_R16G16B16A16_SFLOAT, mipLevels, 0, "CubeMap");

        VulkanRHI::executeImmediatelyMajorGraphics([this, environmentCube, stagingBuffer, &bufferCopyRegions] (VkCommandBuffer cmd) {
            auto preImageMemoryBarrier = createImageMemoryBarrier(environmentCube, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                            0, VK_ACCESS_TRANSFER_WRITE_BIT);
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
                                    nullptr, 0, nullptr, 1, preImageMemoryBarrier.get());

            vkCmdCopyBufferToImage(cmd, stagingBuffer->getBuffer(), environmentCube->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                    static_cast<uint32_t>(bufferCopyRegions.size()), bufferCopyRegions.data());

            auto postImageMemoryBarrier = createImageMemoryBarrier(environmentCube, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
                                    nullptr, 0, nullptr, 1, postImageMemoryBarrier.get());
        });

        ktxTexture_Destroy(texture);
        stagingBuffer->release();
        stagingBuffer.reset();

        return environmentCube;
    }

    void PreprocessPass::setupTextures()
    {

    }

    void PreprocessPass::createSampler()
    {
        VT_CORE_ASSERT(m_environmentCube != nullptr, "Can not create default environment cube sampler");

        // Create a default sampler
        VkSamplerCreateInfo samplerCreateInfo{};
        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.mipLodBias = 0.0f;
        samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
        samplerCreateInfo.minLod = 0.0f;
        //// Max level-of-detail should match mip level count
        samplerCreateInfo.maxLod = static_cast<float>(m_environmentCube->getInfo().mipLevels);
        //// Only enable anisotropic filtering if enabled on the device
        samplerCreateInfo.anisotropyEnable = VK_TRUE;
        samplerCreateInfo.maxAnisotropy = VulkanRHI::get()->getPhysicalDeviceProperties().limits.maxSamplerAnisotropy;
        samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        m_environmentCubeSampler = VulkanRHI::SamplerManager->createSampler(samplerCreateInfo, static_cast<uint8_t>(TextureType::Cube));
    }

    void PreprocessPass::setupDescriptor()
    {

    }

    void PreprocessPass::setupPipelineLayout()
    {

    }

    void PreprocessPass::setupPipeline(const std::string &filePath, const VkSpecializationInfo *specializationInfo)
    {

    }

    void PreprocessPass::updateDescriptorSet(uint32_t dstBinding, VkDescriptorType descriptorType,
                                                const std::vector<VkDescriptorImageInfo> &descriptors)
    {

    }

    void PreprocessPass::init()
    {
        m_environmentCube = loadFromFile("../data/textures/gcanyon_cube.ktx");
        createSampler();
    }

    void PreprocessPass::release()
    {

    }
}































