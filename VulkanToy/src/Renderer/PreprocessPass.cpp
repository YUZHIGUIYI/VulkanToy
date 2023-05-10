//
// Created by ZHIKANG on 2023/4/14.
//

#include <VulkanToy/Renderer/PreprocessPass.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>
#include <VulkanToy/AssetSystem/TextureManager.h>
#include <VulkanToy/AssetSystem/MaterialManager.h>

namespace VT
{
    // Function to calculate mipmap levels
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
    // Push constants for compute shader - pre-filtered specular environment map
    struct SpecularFilterPushConstants
    {
        uint32_t level;
        float roughness;
    };

    void PreprocessPass::preprocessInit()
    {
        // Create sampler
        {
            // Compute sampler
            VkSamplerCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            createInfo.minFilter = VK_FILTER_LINEAR;
            createInfo.magFilter = VK_FILTER_LINEAR;
            createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
            RHICheck(vkCreateSampler(VulkanRHI::Device, &createInfo, nullptr, &m_computeSampler));

            // Environment cubemap sampler
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            createInfo.mipLodBias = 0.0f;
            createInfo.minLod = 0.0f;
            createInfo.maxLod = FLT_MAX;
            createInfo.anisotropyEnable = VK_TRUE;
            createInfo.maxAnisotropy = VulkanRHI::get()->getPhysicalDeviceProperties().limits.maxSamplerAnisotropy;
            VulkanRHI::SamplerManager->createSampler(createInfo, static_cast<uint8_t>(TextureType::Cube));

            // Irradiance sampler
            VulkanRHI::SamplerManager->createSampler(createInfo, static_cast<uint8_t>(TextureType::Irradiance));

            // Prefiltered map sampler
            VulkanRHI::SamplerManager->createSampler(createInfo, static_cast<uint8_t>(TextureType::Prefiltered));

            // BRDF LUT Sampler
            createInfo.anisotropyEnable = VK_FALSE;
            VulkanRHI::SamplerManager->createSampler(createInfo, static_cast<uint8_t>(TextureType::BRDFLUT));
        }

        // Descriptor set layout and its set
        {
            const std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings{
                {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, &m_computeSampler},
                {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
                {2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          kEnvMapLevels - 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}
            };

            VkDescriptorSetLayoutCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            createInfo.bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size());
            createInfo.pBindings = descriptorSetLayoutBindings.data();
            RHICheck(vkCreateDescriptorSetLayout(VulkanRHI::Device, &createInfo, nullptr,
                                                    &m_computeDescriptorSetLayout));

            m_computeDescriptorSet = VulkanRHI::get()->getDescriptorPoolCache().allocateSet(m_computeDescriptorSetLayout);
        }

        // Pipeline layout
        {
            const std::vector<VkDescriptorSetLayout> pipelineSetLayouts{ m_computeDescriptorSetLayout };
            const std::vector<VkPushConstantRange> pipelinePushConstantRanges{
                {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SpecularFilterPushConstants)}
            };
            VkPipelineLayoutCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            createInfo.setLayoutCount = static_cast<uint32_t>(pipelineSetLayouts.size());
            createInfo.pSetLayouts = pipelineSetLayouts.data();
            createInfo.pushConstantRangeCount = static_cast<uint32_t>(pipelinePushConstantRanges.size());
            createInfo.pPushConstantRanges = pipelinePushConstantRanges.data();

            RHICheck(vkCreatePipelineLayout(VulkanRHI::Device, &createInfo, nullptr, &m_computePipelineLayout));
        }

        // Allocate common textures for later processing
        {
            //  Environment map (with pre-filtered mip chain)
            uint32_t mipLevels = numMipmapLevels(kEnvMapSize, kEnvMapSize);
            m_envTexture = VulkanImage::create(kEnvMapSize, kEnvMapSize, 6, VK_FORMAT_R16G16B16A16_SFLOAT,
                                                mipLevels, VK_IMAGE_USAGE_STORAGE_BIT, "Environment texture");
            // Irradiance map
            m_irmapTexture = VulkanImage::create(kIrradianceMapSize, kIrradianceMapSize, 6, VK_FORMAT_R16G16B16A16_SFLOAT,
                                                1, VK_IMAGE_USAGE_STORAGE_BIT, "Irradiance texture");
            // 2D LUT for split-sum approximation
            m_spBRDF_LUT = VulkanImage::create(kBRDF_LUT_Size, kBRDF_LUT_Size, 1, VK_FORMAT_R16G16_SFLOAT,
                                                1, VK_IMAGE_USAGE_STORAGE_BIT, "SP BRDF texture");
        }
    }

    VkPipeline PreprocessPass::createComputePipeline(const std::string &fileName, VkPipelineLayout pipelineLayout, const VkSpecializationInfo* specializationInfo)
    {
        // TODO: destroy shader module
        auto shaderModuleComp = VulkanRHI::ShaderManager->getShader(fileName);

        const VkPipelineShaderStageCreateInfo shaderStage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                                                            VK_SHADER_STAGE_COMPUTE_BIT, shaderModuleComp, "main", specializationInfo};
        VkComputePipelineCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        createInfo.stage = shaderStage;
        createInfo.layout = pipelineLayout;

        VkPipeline computePipeline{};
        RHICheck(vkCreateComputePipelines(VulkanRHI::Device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &computePipeline));

        return computePipeline;
    }

    void PreprocessPass::preprocessEnvMap()
    {
        const uint32_t mipLevels = numMipmapLevels(kEnvMapSize, kEnvMapSize);
        Ref<VulkanImage> envTextureUnfiltered = VulkanImage::create(kEnvMapSize, kEnvMapSize, 6, VK_FORMAT_R16G16B16A16_SFLOAT,
                                                                    mipLevels, VK_IMAGE_USAGE_STORAGE_BIT, "envTextureUnfiltered");
        // Load and convert equirectangular environment map to cubemap texture
        {
            // Load HDR image
            auto textureLoadTask = TextureRawDataLoadTask::buildFromPath2("../data/environment.hdr", VK_FORMAT_R32G32B32A32_SFLOAT, TextureType::HDR);
            Ref<VulkanImage> envTextureEquirect = textureLoadTask->imageAssetGPU->getVulkanImage();

            // Create compute pipeline
            VkPipeline pipeline = createComputePipeline("EquirectToCube.comp.spv", m_computePipelineLayout);

            // Update descriptor set
            const VkDescriptorImageInfo inputTexture{ VK_NULL_HANDLE, envTextureEquirect->getView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
            const VkDescriptorImageInfo outputTexture{ VK_NULL_HANDLE, envTextureUnfiltered->getView(), VK_IMAGE_LAYOUT_GENERAL };
            VulkanRHI::get()->updateDescriptorSet(m_computeDescriptorSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, { inputTexture });
            VulkanRHI::get()->updateDescriptorSet(m_computeDescriptorSet, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, { outputTexture });

            // Record command
            VulkanRHI::executeImmediatelyMajorGraphics([this, pipeline, envTextureUnfiltered] (VkCommandBuffer cmd)
            {
                const auto preDispatchBarrier = ImageMemoryBarrier{ envTextureUnfiltered->getImage(), 0, VK_ACCESS_SHADER_WRITE_BIT,
                                                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL }; //.mipLevels(0, 1);
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                                        nullptr, 0, nullptr, 1, &preDispatchBarrier.barrier);

                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout, 0, 1,
                                        &m_computeDescriptorSet, 0, nullptr);
                vkCmdDispatch(cmd, kEnvMapSize / 32, kEnvMapSize / 32, 6);

                const auto postDispatchBarrier = ImageMemoryBarrier{ envTextureUnfiltered->getImage(), VK_ACCESS_SHADER_WRITE_BIT, 0,
                                                                    VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL }; //.mipLevels(0, 1);
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0,
                                        nullptr, 0, nullptr, 1, &postDispatchBarrier.barrier);
            });

            // Destroy unused resources
            // TODO: fix release
            vkDestroyPipeline(VulkanRHI::Device, pipeline, nullptr);
            textureLoadTask->imageAssetGPU->release();

            // Generate mipmap
            envTextureUnfiltered->generateMipmaps();
        }

        // Compute pre-filtered specular environment map
        {
            // Create compute pipeline
            const uint32_t numMipTailLevels = kEnvMapLevels - 1;
            VkPipeline pipeline{};
            {
                const VkSpecializationMapEntry specializationMapEntry{ 0, 0, sizeof(uint32_t) };
                const uint32_t specializationData[]{ numMipTailLevels };
                const VkSpecializationInfo specializationInfo{ 1, &specializationMapEntry, sizeof(specializationData), specializationData };
                pipeline = createComputePipeline("SPMap.comp.spv", m_computePipelineLayout, &specializationInfo);
            }

            // Record command
            std::vector<VkImageView> envTextureMipTailViews;
            envTextureMipTailViews.reserve(kEnvMapLevels - 1);
            VulkanRHI::executeImmediatelyMajorGraphics([this, pipeline, envTextureUnfiltered, &envTextureMipTailViews, numMipTailLevels] (VkCommandBuffer cmd)
            {
                // Copy base mipmap level into destination environment map
                std::vector<ImageMemoryBarrier> preCopyBarriers{
                    ImageMemoryBarrier{ envTextureUnfiltered->getImage(), 0, VK_ACCESS_TRANSFER_READ_BIT,
                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL }.mipLevels(0, 1),
                    ImageMemoryBarrier{ m_envTexture->getImage(), 0, VK_ACCESS_TRANSFER_WRITE_BIT,
                                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL }
                };
                std::vector<ImageMemoryBarrier> postCopyBarriers{
                    ImageMemoryBarrier{ envTextureUnfiltered->getImage(), VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }.mipLevels(0, 1),
                    ImageMemoryBarrier{ m_envTexture->getImage(), VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL }
                };

                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                                        nullptr, 0, nullptr,static_cast<uint32_t>(preCopyBarriers.size()),
                                        reinterpret_cast<const VkImageMemoryBarrier *>(preCopyBarriers.data()));

                VkImageCopy copyRegion{};
                copyRegion.extent = { m_envTexture->getExtent().width, m_envTexture->getExtent().height, 1 };
                copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copyRegion.srcSubresource.layerCount = m_envTexture->getInfo().arrayLayers;
                copyRegion.dstSubresource = copyRegion.srcSubresource;
                vkCmdCopyImage(cmd, envTextureUnfiltered->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                m_envTexture->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                1, &copyRegion);

                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                                        nullptr, 0, nullptr,static_cast<uint32_t>(postCopyBarriers.size()),
                                        reinterpret_cast<const VkImageMemoryBarrier *>(postCopyBarriers.data()));

                // Pre-filter rest of the mip-chain
                std::vector<VkDescriptorImageInfo> envTextureMipTailDescriptors;
                envTextureMipTailDescriptors.reserve(kEnvMapLevels - 1);
                VkDescriptorImageInfo inputTexture{ VK_NULL_HANDLE, envTextureUnfiltered->getView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                VulkanRHI::get()->updateDescriptorSet(m_computeDescriptorSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, { inputTexture });

                for (uint32_t level = 1; level < kEnvMapLevels; ++level)
                {
                    VkImageSubresourceRange subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = level, .levelCount = 1,
                                                    .baseArrayLayer = 0, .layerCount = VK_REMAINING_ARRAY_LAYERS};
                    envTextureMipTailViews.push_back(VulkanImage::createView(m_envTexture, subresourceRange));
                    envTextureMipTailDescriptors.push_back(VkDescriptorImageInfo{ VK_NULL_HANDLE, envTextureMipTailViews[level - 1], VK_IMAGE_LAYOUT_GENERAL });
                }
                VulkanRHI::get()->updateDescriptorSet(m_computeDescriptorSet, 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, envTextureMipTailDescriptors);

                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout, 0, 1,
                                            &m_computeDescriptorSet, 0, nullptr);

                const float deltaRoughness = 1.0f / std::max(float(numMipTailLevels), 1.0f);
                for (uint32_t level = 1, size = kEnvMapSize / 2; level < kEnvMapLevels; ++level, size /= 2)
                {
                    const uint32_t numGroups = std::max<uint32_t>(1, size / 32);

                    const SpecularFilterPushConstants pushConstants{ level - 1, float(level) * deltaRoughness };
                    vkCmdPushConstants(cmd, m_computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SpecularFilterPushConstants), &pushConstants);
                    vkCmdDispatch(cmd, numGroups, numGroups, 6);
                }

                const auto barrier = ImageMemoryBarrier{ m_envTexture->getImage(), VK_ACCESS_SHADER_WRITE_BIT, 0,
                                                            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0,
                                        nullptr, 0, nullptr, 1, &barrier.barrier);
            });

            // Destroy image views
            for (auto mipTailView : envTextureMipTailViews)
            {
                vkDestroyImageView(VulkanRHI::Device, mipTailView, nullptr);
            }
            // Destroy resource of envTextureUnfiltered
            envTextureUnfiltered->release();
        }

        // Compute diffuse irradiance cubemap
        {
            VkPipeline pipeline = createComputePipeline("IrCube.comp.spv", m_computePipelineLayout);

            const VkDescriptorImageInfo inputTexture{ VK_NULL_HANDLE, m_envTexture->getView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
            const VkDescriptorImageInfo outputTexture{ VK_NULL_HANDLE, m_irmapTexture->getView(), VK_IMAGE_LAYOUT_GENERAL };
            VulkanRHI::get()->updateDescriptorSet(m_computeDescriptorSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, { inputTexture });
            VulkanRHI::get()->updateDescriptorSet(m_computeDescriptorSet, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, { outputTexture });

            // Record command
            VulkanRHI::executeImmediatelyMajorGraphics([this, pipeline] (VkCommandBuffer cmd)
            {
                const auto preDispatchBarrier = ImageMemoryBarrier{ m_irmapTexture->getImage(), 0, VK_ACCESS_SHADER_WRITE_BIT,
                                                                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL };
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                                        nullptr, 0, nullptr, 1, &preDispatchBarrier.barrier);

                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout, 0, 1, &m_computeDescriptorSet,
                                        0, nullptr);
                vkCmdDispatch(cmd, kIrradianceMapSize / 32, kIrradianceMapSize / 32, 6);

                const auto postDispatchBarrier = ImageMemoryBarrier{ m_irmapTexture->getImage(), VK_ACCESS_SHADER_WRITE_BIT, 0,
                                                                        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0,
                                        nullptr, 0, nullptr, 1, &postDispatchBarrier.barrier);
            });

            // Destroy compute pipeline
            vkDestroyPipeline(VulkanRHI::Device, pipeline, nullptr);
        }

        // Compute Cook-Torrance BRDF 2D LUT for split-sum approximation
        {
            VkPipeline pipeline = createComputePipeline("SPBRDF.comp.spv", m_computePipelineLayout);

            const VkDescriptorImageInfo outputTexture{ VK_NULL_HANDLE, m_spBRDF_LUT->getView(), VK_IMAGE_LAYOUT_GENERAL };
            VulkanRHI::get()->updateDescriptorSet(m_computeDescriptorSet, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, { outputTexture });

            VulkanRHI::executeImmediatelyMajorGraphics([this, pipeline] (VkCommandBuffer cmd)
            {
                const auto preDispatchBarrier = ImageMemoryBarrier{ m_spBRDF_LUT->getImage(), 0, VK_ACCESS_SHADER_WRITE_BIT,
                                                                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL };
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                                        nullptr, 0, nullptr, 1, &preDispatchBarrier.barrier);

                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout, 0, 1, &m_computeDescriptorSet,
                                        0, nullptr);
                vkCmdDispatch(cmd, kBRDF_LUT_Size / 32, kBRDF_LUT_Size / 32, 6);

                const auto postDispatchBarrier = ImageMemoryBarrier{ m_spBRDF_LUT->getImage(), VK_ACCESS_SHADER_WRITE_BIT, 0,
                                                                    VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0,
                                        nullptr, 0, nullptr, 1, &postDispatchBarrier.barrier);
            });

            // Destroy compute pipeline
            vkDestroyPipeline(VulkanRHI::Device, pipeline, nullptr);
        }

        // Clean up
        vkDestroyDescriptorSetLayout(VulkanRHI::Device, m_computeDescriptorSetLayout, nullptr);
        vkDestroySampler(VulkanRHI::Device, m_computeSampler, nullptr);
        vkDestroyPipelineLayout(VulkanRHI::Device, m_computePipelineLayout, nullptr);
    }

    // Currently render pass is useless
    void PreprocessPass::init(VkRenderPass renderPass)
    {
        preprocessInit();
        preprocessEnvMap();

        // PBR material
        StandardPBRMaterial::irradianceTexture = m_irmapTexture;
        StandardPBRMaterial::BRDFLUT = m_spBRDF_LUT;
        StandardPBRMaterial::prefilteredMapTexture = m_envTexture;

        // Skybox texture
        SkyboxMaterial::skyboxTexture = m_envTexture;
    }

    void PreprocessPass::release()
    {

    }
}








