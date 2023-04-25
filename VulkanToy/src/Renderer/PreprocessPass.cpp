//
// Created by ZHIKANG on 2023/4/14.
//

#include <VulkanToy/Renderer/PreprocessPass.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>
#include <VulkanToy/AssetSystem/AssetCommon.h>
#include <VulkanToy/AssetSystem/MeshManager.h>

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

    Ref<VkImageMemoryBarrier> createImageMemoryBarrier2(Ref<VulkanImage> vulkanImage, VkImageLayout oldLayout, VkImageLayout newLayout,
                                                        VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageSubresourceRange &range)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = vulkanImage->getImage();
        barrier.subresourceRange = range;
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
            imageCreateInfo.usage |= additionalUsage;
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
                ktx_size_t offset = 0;
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

    void PreprocessPass::generateBRDFLUT()
    {
        auto tStart = std::chrono::steady_clock::now();

        const VkFormat format = VK_FORMAT_R16G16_SFLOAT;    // R16G16 is supported pretty much everywhere
        const uint32_t dim = 512;

        // Image and its view
        m_lutBRDF = createVulkanImage(dim, dim, 1, format, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, "LUTBRDF");
        // Sampler
        VkSamplerCreateInfo samplerCI{};
        samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCI.magFilter = VK_FILTER_LINEAR;
        samplerCI.minFilter = VK_FILTER_LINEAR;
        samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCI.mipLodBias = 0.0f;
        samplerCI.minLod = 0.0f;
        samplerCI.maxLod = 1.0f;
        samplerCI.maxAnisotropy = 1.0f;
        samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        m_lutBRDFSampler = VulkanRHI::SamplerManager->createSampler(samplerCI, static_cast<uint8_t>(TextureType::BRDFLUT));
        // Descriptor image information
        auto imageView = m_lutBRDF->getView();
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = imageView;
        imageInfo.sampler = m_lutBRDFSampler;

        // FB, Att, RP, Pipe, etc - color attachment
        VkAttachmentDescription attachmentDesc{};
        attachmentDesc.format = format;
        attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkAttachmentReference colorReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        VkSubpassDescription subpassDescription{};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorReference;

        // Use subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies{};
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        // Create actual render pass
        VkRenderPassCreateInfo renderPassCreateInfo{};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount = 1;
        renderPassCreateInfo.pAttachments = &attachmentDesc;
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpassDescription;
        renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassCreateInfo.pDependencies = dependencies.data();

        VkRenderPass renderPass{};
        RHICheck(vkCreateRenderPass(VulkanRHI::Device, &renderPassCreateInfo, nullptr, &renderPass));

        VkFramebufferCreateInfo framebufferCreateInfo{};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = renderPass;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.pAttachments = &imageView;
        framebufferCreateInfo.width = dim;
        framebufferCreateInfo.height = dim;
        framebufferCreateInfo.layers = 1;
        VkFramebuffer framebuffer{};
        RHICheck(vkCreateFramebuffer(VulkanRHI::Device, &framebufferCreateInfo, nullptr, &framebuffer));

        // Descriptor set layout
        VkDescriptorSetLayout descriptorSetLayout{};
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = Initializers::initDescriptorSetLayoutCreateInfo(setLayoutBindings);
        RHICheck(vkCreateDescriptorSetLayout(VulkanRHI::Device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayout));

        // Descriptor pool
        std::vector<VkDescriptorPoolSize> poolSizes{ {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1} };
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes = poolSizes.data();
        descriptorPoolInfo.maxSets = 2;

        VkDescriptorPool descriptorPool{};
        RHICheck(vkCreateDescriptorPool(VulkanRHI::Device, &descriptorPoolInfo, nullptr, &descriptorPool));

        // Descriptor sets
        VkDescriptorSet descriptorSet{};
        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
        descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.descriptorPool = descriptorPool;
        descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;
        descriptorSetAllocateInfo.descriptorSetCount = 1;
        RHICheck(vkAllocateDescriptorSets(VulkanRHI::Device, &descriptorSetAllocateInfo, &descriptorSet));

        // Pipeline layout
        VkPipelineLayout pipelineLayout{};
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
        RHICheck(vkCreatePipelineLayout(VulkanRHI::Device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

        // Pipeline
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = Initializers::initPipelineInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
        VkPipelineRasterizationStateCreateInfo rasterizationState = Initializers::initPipelineRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        VkPipelineColorBlendAttachmentState blendAttachmentState = Initializers::initPipelineColorBlendAttachmentState(0xf, VK_FALSE);
        VkPipelineColorBlendStateCreateInfo colorBlendState = Initializers::initPipelineColorBlendState(1, &blendAttachmentState);
        VkPipelineDepthStencilStateCreateInfo depthStencilState = Initializers::initPipelineDepthStencilState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkPipelineViewportStateCreateInfo viewportState = Initializers::initPipelineViewportState(1, 1);
        VkPipelineMultisampleStateCreateInfo multisampleState = Initializers::initPipelineMultisampleState(VK_SAMPLE_COUNT_1_BIT);
        std::vector<VkDynamicState> dynamicStateEnables{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState = Initializers::initPipelineDynamicState(dynamicStateEnables);

        VkPipelineVertexInputStateCreateInfo emptyInputState{};
        emptyInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
        auto shaderModuleVert = VulkanRHI::ShaderManager->getShader("GenBRDFLUT.vert.spv");
        auto shaderModuleFrag = VulkanRHI::ShaderManager->getShader("GenBRDFLUT.frag.spv");
        shaderStages[0] = Initializers::initPipelineShaderStage(shaderModuleVert, VK_SHADER_STAGE_VERTEX_BIT);
        shaderStages[1] = Initializers::initPipelineShaderStage(shaderModuleFrag, VK_SHADER_STAGE_FRAGMENT_BIT);

        VkGraphicsPipelineCreateInfo pipelineCI = Initializers::initPipeline(pipelineLayout, renderPass);
        pipelineCI.pInputAssemblyState = &inputAssemblyState;
        pipelineCI.pRasterizationState = &rasterizationState;
        pipelineCI.pColorBlendState = &colorBlendState;
        pipelineCI.pMultisampleState = &multisampleState;
        pipelineCI.pViewportState = &viewportState;
        pipelineCI.pDepthStencilState = &depthStencilState;
        pipelineCI.pDynamicState = &dynamicState;
        pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineCI.pStages = shaderStages.data();
        pipelineCI.pVertexInputState = &emptyInputState;

        VkPipeline pipeline{};
        RHICheck(vkCreateGraphicsPipelines(VulkanRHI::Device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline));

        // Render
        VulkanRHI::executeImmediatelyMajorGraphics([dim, renderPass, framebuffer, pipeline] (VkCommandBuffer cmd) {
            VkClearValue clearValues[1];
            clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

            VkRenderPassBeginInfo renderPassBeginInfo{};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.renderPass = renderPass;
            renderPassBeginInfo.renderArea.extent.width = dim;
            renderPassBeginInfo.renderArea.extent.height = dim;
            renderPassBeginInfo.clearValueCount = 1;
            renderPassBeginInfo.pClearValues = clearValues;
            renderPassBeginInfo.framebuffer = framebuffer;

            vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            VkViewport viewport = Initializers::initViewport((float)dim, (float)dim, 0.0f, 1.0f);
            VkRect2D scissor = Initializers::initRect2D((int32_t)dim, (int32_t)dim, 0, 0);

            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &scissor);
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vkCmdDraw(cmd, 3, 1, 0, 0);
            vkCmdEndRenderPass(cmd);
        });

        vkDestroyPipeline(VulkanRHI::Device, pipeline, nullptr);
        vkDestroyPipelineLayout(VulkanRHI::Device, pipelineLayout, nullptr);
        vkDestroyRenderPass(VulkanRHI::Device, renderPass, nullptr);
        vkDestroyFramebuffer(VulkanRHI::Device, framebuffer, nullptr);
        vkDestroyDescriptorSetLayout(VulkanRHI::Device, descriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(VulkanRHI::Device, descriptorPool, nullptr);

        auto tEnd = std::chrono::steady_clock::now();
        auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
        VT_CORE_INFO("Generating BRDF LUT took '{0}' ms", tDiff);
    }

    void PreprocessPass::generateIrradianceCube()
    {
        auto tStart = std::chrono::steady_clock::now();

        const VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
        const uint32_t dim = 64;
        const uint32_t numMips = static_cast<uint32_t>(std::floor(std::log2(dim))) + 1;

        // Irradiance cube map
        // Image and its view
        m_irradianceCube = createVulkanImage(dim, dim, 6, format, numMips, 0, "IrradianceCube");
        // Sampler
        VkSamplerCreateInfo samplerCI{};
        samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCI.magFilter = VK_FILTER_LINEAR;
        samplerCI.minFilter = VK_FILTER_LINEAR;
        samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCI.mipLodBias = 0.0f;
        samplerCI.minLod = 0.0f;
        samplerCI.maxLod = static_cast<float>(numMips);
        samplerCI.maxAnisotropy = 1.0f;
        samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        m_irradianceCubeSampler = VulkanRHI::SamplerManager->createSampler(samplerCI, static_cast<uint8_t>(TextureType::Irradiance));

        // FB, Att, RP, Pipe, etc - color attachment
        VkAttachmentDescription attachmentDesc{};
        attachmentDesc.format = format;
        attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkAttachmentReference colorReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        VkSubpassDescription subpassDescription{};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorReference;

        // Use subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies{};
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        // Create actual render pass
        VkRenderPassCreateInfo renderPassCreateInfo{};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount = 1;
        renderPassCreateInfo.pAttachments = &attachmentDesc;
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpassDescription;
        renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassCreateInfo.pDependencies = dependencies.data();

        VkRenderPass renderPass{};
        RHICheck(vkCreateRenderPass(VulkanRHI::Device, &renderPassCreateInfo, nullptr, &renderPass));

        struct Offscreen {
            Ref<VulkanImage> vulkanImage;
            VkFramebuffer framebuffer;
        } offscreen;
        // Offscreen frame buffer
        {
            // Color attachment
            offscreen.vulkanImage = createVulkanImage(dim, dim, 1, format, 1,
                                                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, "IrradianceOffscreenImage");

            VkFramebufferCreateInfo framebufferCreateInfo{};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCreateInfo.renderPass = renderPass;
            framebufferCreateInfo.attachmentCount = 1;
            auto imageView = offscreen.vulkanImage->getView();
            framebufferCreateInfo.pAttachments = &imageView;
            framebufferCreateInfo.width = dim;
            framebufferCreateInfo.height = dim;
            framebufferCreateInfo.layers = 1;
            RHICheck(vkCreateFramebuffer(VulkanRHI::Device, &framebufferCreateInfo, nullptr, &offscreen.framebuffer));

            VkImageSubresourceRange subresourceRange{};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = 1;
            subresourceRange.layerCount = 1;
            offscreen.vulkanImage->transitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, subresourceRange);
        }

        // Descriptor set layout
        VkDescriptorSetLayout descriptorSetLayout{};
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{
            Initializers::initDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
        };
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = Initializers::initDescriptorSetLayoutCreateInfo(setLayoutBindings);
        RHICheck(vkCreateDescriptorSetLayout(VulkanRHI::Device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayout));

        // Descriptor pool
        std::vector<VkDescriptorPoolSize> poolSizes{ {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 } };
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes = poolSizes.data();
        descriptorPoolInfo.maxSets = 2;

        VkDescriptorPool descriptorPool{};
        RHICheck(vkCreateDescriptorPool(VulkanRHI::Device, &descriptorPoolInfo, nullptr, &descriptorPool));

        // Descriptor sets and update
        VkDescriptorSet descriptorSet{};
        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
        descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.descriptorPool = descriptorPool;
        descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;
        descriptorSetAllocateInfo.descriptorSetCount = 1;
        RHICheck(vkAllocateDescriptorSets(VulkanRHI::Device, &descriptorSetAllocateInfo, &descriptorSet));

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_environmentCube->getView();
        imageInfo.sampler = m_environmentCubeSampler;
        VkWriteDescriptorSet writeDescriptorSet {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = descriptorSet;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.pImageInfo = &imageInfo;
        writeDescriptorSet.descriptorCount = 1;
        vkUpdateDescriptorSets(VulkanRHI::Device, 1, &writeDescriptorSet, 0, nullptr);

        // Pipeline layout
        struct PushBlock {
            glm::mat4 mvp;
            // Sampling details
            float deltaPhi = (2.0f * 3.1415926535) / 20.0f;
            float deltaTheta = (0.5f * 3.1415926535) / 64.0f;
        } pushBlock;

        VkPipelineLayout pipelineLayout{};
        std::vector<VkPushConstantRange> pushConstantRanges{
            Initializers::initPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushBlock), 0)
        };
        VkPipelineLayoutCreateInfo pipelineLayoutCI = Initializers::initPipelineLayout(&descriptorSetLayout, 1);
        pipelineLayoutCI.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
        pipelineLayoutCI.pPushConstantRanges = pushConstantRanges.data();
        RHICheck(vkCreatePipelineLayout(VulkanRHI::Device, &pipelineLayoutCI, nullptr, &pipelineLayout));

        // Pipeline
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = Initializers::initPipelineInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
        VkPipelineRasterizationStateCreateInfo rasterizationState = Initializers::initPipelineRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        VkPipelineColorBlendAttachmentState blendAttachmentState = Initializers::initPipelineColorBlendAttachmentState(0xf, VK_FALSE);
        VkPipelineColorBlendStateCreateInfo colorBlendState = Initializers::initPipelineColorBlendState(1, &blendAttachmentState);
        VkPipelineDepthStencilStateCreateInfo depthStencilState = Initializers::initPipelineDepthStencilState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkPipelineViewportStateCreateInfo viewportState = Initializers::initPipelineViewportState(1, 1);
        VkPipelineMultisampleStateCreateInfo multisampleState = Initializers::initPipelineMultisampleState(VK_SAMPLE_COUNT_1_BIT);
        std::vector<VkDynamicState> dynamicStateEnables{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState = Initializers::initPipelineDynamicState(dynamicStateEnables);

        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
        auto shaderModuleVert = VulkanRHI::ShaderManager->getShader("FilterCube.vert.spv");
        auto shaderModuleFrag = VulkanRHI::ShaderManager->getShader("IrradianceCube.frag.spv");
        shaderStages[0] = Initializers::initPipelineShaderStage(shaderModuleVert, VK_SHADER_STAGE_VERTEX_BIT);
        shaderStages[1] = Initializers::initPipelineShaderStage(shaderModuleFrag, VK_SHADER_STAGE_FRAGMENT_BIT);

        VkGraphicsPipelineCreateInfo pipelineCI = Initializers::initPipeline(pipelineLayout, renderPass);
        pipelineCI.pInputAssemblyState = &inputAssemblyState;
        pipelineCI.pRasterizationState = &rasterizationState;
        pipelineCI.pColorBlendState = &colorBlendState;
        pipelineCI.pMultisampleState = &multisampleState;
        pipelineCI.pViewportState = &viewportState;
        pipelineCI.pDepthStencilState = &depthStencilState;
        pipelineCI.pDynamicState = &dynamicState;
        pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineCI.pStages = shaderStages.data();
        pipelineCI.pVertexInputState = StaticMeshVertex::getPipelineVertexInputState(
                { VertexComponent::Position, VertexComponent::Normal, VertexComponent::UV });

        VkPipeline pipeline{};
        RHICheck(vkCreateGraphicsPipelines(VulkanRHI::Device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline));

        // Transition layout of irradiance cube
        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = numMips;
        subresourceRange.layerCount = 6;

        m_irradianceCube->transitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
                                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, subresourceRange);

        // Render
        VulkanRHI::executeImmediatelyMajorGraphics([this, renderPass, descriptorSet, pipelineLayout, pipeline, &offscreen, &pushBlock, numMips, dim] (VkCommandBuffer cmd) {
            VkClearValue clearValues[1];
            clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 0.0f } };

            VkRenderPassBeginInfo renderPassBeginInfo{};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.renderPass = renderPass;
            renderPassBeginInfo.renderArea.extent.width = dim;
            renderPassBeginInfo.renderArea.extent.height = dim;
            renderPassBeginInfo.clearValueCount = 1;
            renderPassBeginInfo.pClearValues = clearValues;
            renderPassBeginInfo.framebuffer = offscreen.framebuffer;

            std::vector<glm::mat4> matrices{
                // POSITIVE_X
                glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // NEGATIVE_X
                glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // POSITIVE_Y
                glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // NEGATIVE_Y
                glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // POSITIVE_Z
                glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // NEGATIVE_Z
                glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            };

            VkViewport viewport = Initializers::initViewport((float)dim, (float)dim, 0.0f, 1.0f);
            VkRect2D scissor = Initializers::initRect2D((int32_t)dim, (int32_t)dim, 0, 0);

            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            VkImageSubresourceRange subresourceRange{};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = 1;
            subresourceRange.layerCount = 1;

            const VkDeviceSize offsets[1] = {0};
            auto para = static_cast<float>( 3.14159265358979 / 2.0);
            auto cacheGPUMeshAsset = EngineMeshes::GSkyBoxRef.lock();
            VT_CORE_ASSERT(cacheGPUMeshAsset != nullptr, "Sky box mesh is nullptr");
            for (uint32_t m = 0; m < numMips; ++m)
            {
                for (uint32_t f = 0; f < 6; ++f)
                {
                    viewport.width = static_cast<float>(dim * std::pow(0.5f, m));
                    viewport.height = static_cast<float>(dim * std::pow(0.5f, m));
                    vkCmdSetViewport(cmd, 0, 1, &viewport);

                    // Render scene from cube face's point of view
                    vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                    // Update shader push constant block
                    glm::mat4 projection = glm::perspective(para, 1.0f, 0.1f, 512.0f);
                    pushBlock.mvp = projection * matrices[f];

                    vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlock), &pushBlock);

                    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                                            0, 1, &descriptorSet, 0, nullptr);
                    // Draw sky box
                    vkCmdBindVertexBuffers(cmd, 0, 1, &cacheGPUMeshAsset->getVertexBuffer(), offsets);
                    vkCmdBindIndexBuffer(cmd, cacheGPUMeshAsset->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
                    vkCmdDrawIndexed(cmd, cacheGPUMeshAsset->getIndicesCount(), 1, 0, 0, 0);

                    vkCmdEndRenderPass(cmd);

                    auto preImageMemoryBarrier = createImageMemoryBarrier2(offscreen.vulkanImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                                        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, subresourceRange);
                    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
                                            nullptr, 0, nullptr, 1, preImageMemoryBarrier.get());

                    // Copy region for transfer from frame buffer to cube face
                    VkImageCopy copyRegion{};
                    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    copyRegion.srcSubresource.baseArrayLayer = 0;
                    copyRegion.srcSubresource.mipLevel = 0;
                    copyRegion.srcSubresource.layerCount = 1;
                    copyRegion.srcOffset = { 0, 0, 0 };

                    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    copyRegion.dstSubresource.baseArrayLayer = f;
                    copyRegion.dstSubresource.mipLevel = m;
                    copyRegion.dstSubresource.layerCount = 1;
                    copyRegion.dstOffset = { 0, 0, 0 };

                    copyRegion.extent.width = static_cast<uint32_t>(viewport.width);
                    copyRegion.extent.height = static_cast<uint32_t>(viewport.height);
                    copyRegion.extent.depth = 1;

                    vkCmdCopyImage(cmd, offscreen.vulkanImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                    m_irradianceCube->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                    1, &copyRegion);

                    // Transfer framebuffer color attachment back
                    auto postImageMemoryBarrier = createImageMemoryBarrier2(offscreen.vulkanImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                                            VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, subresourceRange);
                    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
                                            nullptr, 0, nullptr, 1, postImageMemoryBarrier.get());
                }
            }
            auto lastImageMemoryBarrier = createImageMemoryBarrier(m_irradianceCube, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
                                    nullptr, 0, nullptr, 1, lastImageMemoryBarrier.get());
        });

        vkDestroyRenderPass(VulkanRHI::Device, renderPass, nullptr);
        vkDestroyFramebuffer(VulkanRHI::Device, offscreen.framebuffer, nullptr);

        offscreen.vulkanImage->release();

        vkDestroyDescriptorPool(VulkanRHI::Device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(VulkanRHI::Device, descriptorSetLayout, nullptr);
        vkDestroyPipeline(VulkanRHI::Device, pipeline, nullptr);
        vkDestroyPipelineLayout(VulkanRHI::Device, pipelineLayout, nullptr);

        auto tEnd = std::chrono::high_resolution_clock::now();
        auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
        VT_CORE_INFO("Generating irradiance cube with {0} mip levels took '{1}' ms", numMips, tDiff);
    }

    void PreprocessPass::generatePrefilteredCube()
    {
        auto tStart = std::chrono::steady_clock::now();

        const VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
        const uint32_t dim = 512;
        const uint32_t numMips = static_cast<uint32_t>(std::floor(std::log2(dim))) + 1;

        // Pre-filtered environment cube map
        // Image and its view
        m_prefilteredCube = createVulkanImage(dim, dim, 6, format, numMips, 0, "PrefilteredCube");
        // Sampler
        VkSamplerCreateInfo samplerCI{};
        samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCI.magFilter = VK_FILTER_LINEAR;
        samplerCI.minFilter = VK_FILTER_LINEAR;
        samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCI.minLod = 0.0f;
        samplerCI.maxLod = static_cast<float>(numMips);
        samplerCI.maxAnisotropy = 1.0f;
        samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        m_prefilteredCubeSampler = VulkanRHI::SamplerManager->createSampler(samplerCI, static_cast<uint8_t>(TextureType::Prefiltered));

        // FB, Att, RP, Pipe, etc - color attachment
        VkAttachmentDescription attachmentDesc{};
        attachmentDesc.format = format;
        attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkAttachmentReference colorReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        VkSubpassDescription subpassDescription{};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorReference;

        // Use subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies{};
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        // Create actual render pass
        VkRenderPassCreateInfo renderPassCreateInfo{};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount = 1;
        renderPassCreateInfo.pAttachments = &attachmentDesc;
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpassDescription;
        renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassCreateInfo.pDependencies = dependencies.data();

        VkRenderPass renderPass{};
        RHICheck(vkCreateRenderPass(VulkanRHI::Device, &renderPassCreateInfo, nullptr, &renderPass));

        struct Offscreen
        {
            Ref<VulkanImage> vulkanImage;
            VkFramebuffer framebuffer;
        } offscreen;
        // Offscreen framebuffer
        {
            // Color attachment
            offscreen.vulkanImage = createVulkanImage(dim, dim, 1, format, 1,
                                                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, "PrefilteredOffscreenImage");

            VkFramebufferCreateInfo framebufferCreateInfo{};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCreateInfo.renderPass = renderPass;
            framebufferCreateInfo.attachmentCount = 1;
            auto imageView = offscreen.vulkanImage->getView();
            framebufferCreateInfo.pAttachments = &imageView;
            framebufferCreateInfo.width = dim;
            framebufferCreateInfo.height = dim;
            framebufferCreateInfo.layers = 1;
            RHICheck(vkCreateFramebuffer(VulkanRHI::Device, &framebufferCreateInfo, nullptr, &offscreen.framebuffer));

            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = 1;
            subresourceRange.layerCount = 1;
            offscreen.vulkanImage->transitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, subresourceRange);
        }
        // Descriptor set layout
        VkDescriptorSetLayout descriptorSetLayout{};
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{
            Initializers::initDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
        };
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = Initializers::initDescriptorSetLayoutCreateInfo(setLayoutBindings);
        RHICheck(vkCreateDescriptorSetLayout(VulkanRHI::Device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayout));

        // Descriptor pool
        std::vector<VkDescriptorPoolSize> poolSizes{ {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 } };
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes = poolSizes.data();
        descriptorPoolInfo.maxSets = 2;

        VkDescriptorPool descriptorPool{};
        RHICheck(vkCreateDescriptorPool(VulkanRHI::Device, &descriptorPoolInfo, nullptr, &descriptorPool));

        // Descriptor sets and update
        VkDescriptorSet descriptorSet{};
        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
        descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.descriptorPool = descriptorPool;
        descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;
        descriptorSetAllocateInfo.descriptorSetCount = 1;
        RHICheck(vkAllocateDescriptorSets(VulkanRHI::Device, &descriptorSetAllocateInfo, &descriptorSet));

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_environmentCube->getView();
        imageInfo.sampler = m_environmentCubeSampler;
        VkWriteDescriptorSet writeDescriptorSet {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = descriptorSet;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.pImageInfo = &imageInfo;
        writeDescriptorSet.descriptorCount = 1;
        vkUpdateDescriptorSets(VulkanRHI::Device, 1, &writeDescriptorSet, 0, nullptr);

        // Pipeline layout
        struct PushBlock
        {
            glm::mat4 mvp;
            float roughness;
            uint32_t numSamples = 1024u;
        } pushBlock;

        VkPipelineLayout pipelineLayout{};
        std::vector<VkPushConstantRange> pushConstantRanges{
            Initializers::initPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushBlock), 0)
        };
        VkPipelineLayoutCreateInfo pipelineLayoutCI = Initializers::initPipelineLayout(&descriptorSetLayout, 1);
        pipelineLayoutCI.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
        pipelineLayoutCI.pPushConstantRanges = pushConstantRanges.data();
        RHICheck(vkCreatePipelineLayout(VulkanRHI::Device, &pipelineLayoutCI, nullptr, &pipelineLayout));

        // Pipeline
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = Initializers::initPipelineInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
        VkPipelineRasterizationStateCreateInfo rasterizationState = Initializers::initPipelineRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        VkPipelineColorBlendAttachmentState blendAttachmentState = Initializers::initPipelineColorBlendAttachmentState(0xf, VK_FALSE);
        VkPipelineColorBlendStateCreateInfo colorBlendState = Initializers::initPipelineColorBlendState(1, &blendAttachmentState);
        VkPipelineDepthStencilStateCreateInfo depthStencilState = Initializers::initPipelineDepthStencilState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkPipelineViewportStateCreateInfo viewportState = Initializers::initPipelineViewportState(1, 1);
        VkPipelineMultisampleStateCreateInfo multisampleState = Initializers::initPipelineMultisampleState(VK_SAMPLE_COUNT_1_BIT);
        std::vector<VkDynamicState> dynamicStateEnables{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState = Initializers::initPipelineDynamicState(dynamicStateEnables);

        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
        auto shaderModuleVert = VulkanRHI::ShaderManager->getShader("FilterCube.vert.spv");
        auto shaderModuleFrag = VulkanRHI::ShaderManager->getShader("PrefilterEnvMap.frag.spv");
        shaderStages[0] = Initializers::initPipelineShaderStage(shaderModuleVert, VK_SHADER_STAGE_VERTEX_BIT);
        shaderStages[1] = Initializers::initPipelineShaderStage(shaderModuleFrag, VK_SHADER_STAGE_FRAGMENT_BIT);

        VkGraphicsPipelineCreateInfo pipelineCI = Initializers::initPipeline(pipelineLayout, renderPass);
        pipelineCI.pInputAssemblyState = &inputAssemblyState;
        pipelineCI.pRasterizationState = &rasterizationState;
        pipelineCI.pColorBlendState = &colorBlendState;
        pipelineCI.pMultisampleState = &multisampleState;
        pipelineCI.pViewportState = &viewportState;
        pipelineCI.pDepthStencilState = &depthStencilState;
        pipelineCI.pDynamicState = &dynamicState;
        pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineCI.pStages = shaderStages.data();
        pipelineCI.pVertexInputState = StaticMeshVertex::getPipelineVertexInputState(
                { VertexComponent::Position, VertexComponent::Normal, VertexComponent::UV });

        VkPipeline pipeline{};
        RHICheck(vkCreateGraphicsPipelines(VulkanRHI::Device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline));

        // Transition layout of prefiltered cube
        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = numMips;
        subresourceRange.layerCount = 6;

        m_prefilteredCube->transitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
                                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, subresourceRange);

        // Render
        VulkanRHI::executeImmediatelyMajorGraphics([this, renderPass, descriptorSet, pipelineLayout, pipeline, &offscreen, &pushBlock, numMips, dim] (VkCommandBuffer cmd) {
            VkClearValue clearValues[1];
            clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 0.0f } };

            VkRenderPassBeginInfo renderPassBeginInfo{};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.renderPass = renderPass;
            renderPassBeginInfo.renderArea.extent.width = dim;
            renderPassBeginInfo.renderArea.extent.height = dim;
            renderPassBeginInfo.clearValueCount = 1;
            renderPassBeginInfo.pClearValues = clearValues;
            renderPassBeginInfo.framebuffer = offscreen.framebuffer;

            std::vector<glm::mat4> matrices{
                // POSITIVE_X
                glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // NEGATIVE_X
                glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // POSITIVE_Y
                glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // NEGATIVE_Y
                glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // POSITIVE_Z
                glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // NEGATIVE_Z
                glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            };

            VkViewport viewport = Initializers::initViewport((float)dim, (float)dim, 0.0f, 1.0f);
            VkRect2D scissor = Initializers::initRect2D((int32_t)dim, (int32_t)dim, 0, 0);

            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            VkImageSubresourceRange subresourceRange{};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = 1;
            subresourceRange.layerCount = 1;

            const VkDeviceSize offsets[1] = {0};
            auto para = static_cast<float>( 3.14159265358979 / 2.0);
            auto cacheGPUMeshAsset = EngineMeshes::GSkyBoxRef.lock();
            VT_CORE_ASSERT(cacheGPUMeshAsset != nullptr, "Sky box mesh is nullptr");
            for (uint32_t m = 0; m < numMips; ++m)
            {
                pushBlock.roughness = (float)m / (float)(numMips - 1);
                for (uint32_t f = 0; f < 6; ++f)
                {
                    viewport.width = static_cast<float>(dim * std::pow(0.5f, m));
                    viewport.height = static_cast<float>(dim * std::pow(0.5f, m));
                    vkCmdSetViewport(cmd, 0, 1, &viewport);

                    // Render scene from cube face's point of view
                    vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                    // Update shader push constant block
                    glm::mat4 projection = glm::perspective(para, 1.0f, 0.1f, 512.0f);
                    pushBlock.mvp = projection * matrices[f];

                    vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlock), &pushBlock);

                    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet,
                                                0,nullptr);

                    // Draw sky box
                    vkCmdBindVertexBuffers(cmd, 0, 1, &cacheGPUMeshAsset->getVertexBuffer(), offsets);
                    vkCmdBindIndexBuffer(cmd, cacheGPUMeshAsset->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
                    vkCmdDrawIndexed(cmd, cacheGPUMeshAsset->getIndicesCount(), 1, 0, 0, 0);

                    vkCmdEndRenderPass(cmd);

                    // Pre-image layout transition
                    auto preImageMemoryBarrier = createImageMemoryBarrier2(offscreen.vulkanImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                                           VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, subresourceRange);
                    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
                                         nullptr, 0, nullptr, 1, preImageMemoryBarrier.get());

                    // Copy region for transfer from framebuffer to cube face
                    VkImageCopy copyRegion{};
                    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    copyRegion.srcSubresource.baseArrayLayer = 0;
                    copyRegion.srcSubresource.mipLevel = 0;
                    copyRegion.srcSubresource.layerCount = 1;
                    copyRegion.srcOffset = { 0, 0, 0 };

                    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    copyRegion.dstSubresource.baseArrayLayer = f;
                    copyRegion.dstSubresource.mipLevel = m;
                    copyRegion.dstSubresource.layerCount = 1;
                    copyRegion.dstOffset = { 0, 0, 0 };

                    copyRegion.extent = { static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height), 1 };

                    vkCmdCopyImage(cmd, offscreen.vulkanImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   m_prefilteredCube->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   1, &copyRegion);

                    // Post-image layout transition
                    auto postImageMemoryBarrier = createImageMemoryBarrier2(offscreen.vulkanImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                                           VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, subresourceRange);
                    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
                                         nullptr, 0, nullptr, 1, postImageMemoryBarrier.get());
                }
            }
            // Last image-transition of prefiltered cube
            auto lastImageMemoryBarrier = createImageMemoryBarrier(m_prefilteredCube, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                                   VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
                                 nullptr, 0, nullptr, 1, lastImageMemoryBarrier.get());
        });

        vkDestroyRenderPass(VulkanRHI::Device, renderPass, nullptr);
        vkDestroyFramebuffer(VulkanRHI::Device, offscreen.framebuffer, nullptr);

        offscreen.vulkanImage->release();

        vkDestroyDescriptorPool(VulkanRHI::Device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(VulkanRHI::Device, descriptorSetLayout, nullptr);
        vkDestroyPipeline(VulkanRHI::Device, pipeline, nullptr);
        vkDestroyPipelineLayout(VulkanRHI::Device, pipelineLayout, nullptr);

        auto tEnd = std::chrono::high_resolution_clock::now();
        auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
        VT_CORE_INFO("Generating prefiltered environment cube with {0} mip levels took '{1}' ms", numMips, tDiff);
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

        generateBRDFLUT();
        generateIrradianceCube();
        generatePrefilteredCube();
    }

    void PreprocessPass::release()
    {

    }
}































