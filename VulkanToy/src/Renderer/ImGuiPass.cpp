//
// Created by ZHIKANG on 2023/4/1.
//

#include <VulkanToy/Renderer/ImGuiPass.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>

namespace VT
{
    void setupVulkanInitInfo(ImGui_ImplVulkan_InitInfo *initInfo, VkDescriptorPool pool)
    {
        initInfo->Instance = VulkanRHI::get()->getInstance();
        initInfo->PhysicalDevice = VulkanRHI::GPU;
        initInfo->Device = VulkanRHI::Device;
        initInfo->QueueFamily = VulkanRHI::get()->getGraphicsFamily();
        initInfo->Queue = VulkanRHI::get()->getMajorGraphicsQueue();
        initInfo->PipelineCache = VK_NULL_HANDLE;
        initInfo->DescriptorPool = pool;
        initInfo->Allocator = nullptr;
        initInfo->MinImageCount = static_cast<uint32_t>(VulkanRHI::get()->getSwapChainImageViews().size());
        initInfo->ImageCount = initInfo->MinImageCount;
        initInfo->MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo->CheckVkResultFn = RHICheck;
        initInfo->Subpass = 0;
    }

    void ImGuiPass::buildRenderPass()
    {
        vkDeviceWaitIdle(VulkanRHI::Device);

        // Create render pass
        if (m_renderResource.renderPass == VK_NULL_HANDLE)
        {
            VkAttachmentDescription attachmentDesc{};
            attachmentDesc.format = m_drawUIFormat;
            attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
            attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;  // Blit to back buffer

            VkAttachmentReference colorAttachment{};
            colorAttachment.attachment = 0;
            colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachment;

            VkSubpassDependency dependency{};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            VkRenderPassCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            info.attachmentCount = 1;
            info.pAttachments = &attachmentDesc;
            info.subpassCount = 1;
            info.pSubpasses = &subpass;
            info.dependencyCount = 1;
            info.pDependencies = &dependency;
            RHICheck(vkCreateRenderPass(VulkanRHI::Device, &info, nullptr, &m_renderResource.renderPass));
        }

        {
            VkImageCreateInfo imageCreateInfo{};
            imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCreateInfo.flags = 0;
            imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            imageCreateInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
            imageCreateInfo.extent.width = VulkanRHI::get()->getSwapChainExtent().width;
            imageCreateInfo.extent.height = VulkanRHI::get()->getSwapChainExtent().height;
            imageCreateInfo.extent.depth = 1;
            imageCreateInfo.mipLevels = 1;
            imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            imageCreateInfo.arrayLayers = 1;
            imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageCreateInfo.queueFamilyIndexCount = 0;
            imageCreateInfo.pQueueFamilyIndices = nullptr;
            imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            m_drawUIImages = VulkanImage::create("DrawUIImage", imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        }

        // Create frame buffer and command buffer
        {
            VkImageView attachment[1];
            VkFramebufferCreateInfo fbCreateInfo{};
            fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbCreateInfo.renderPass = m_renderResource.renderPass;
            fbCreateInfo.attachmentCount = 1;
            fbCreateInfo.pAttachments = attachment;
            fbCreateInfo.width = VulkanRHI::get()->getSwapChainExtent().width;
            fbCreateInfo.height = VulkanRHI::get()->getSwapChainExtent().height;
            fbCreateInfo.layers = 1;

            auto backBufferSize = VulkanRHI::get()->getSwapChainImageViews().size();
            m_renderResource.frameBuffers.resize(backBufferSize);
            m_renderResource.commandPools.resize(backBufferSize);
            m_renderResource.commandBuffers.resize(backBufferSize);

            for (uint32_t i = 0; i < backBufferSize; ++i)
            {
                VkImageSubresourceRange range{};
                range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                range.baseMipLevel = 0;
                range.levelCount = VK_REMAINING_MIP_LEVELS;
                range.baseArrayLayer = 0;
                range.layerCount = VK_REMAINING_ARRAY_LAYERS;
                attachment[0] = m_drawUIImages->getView(range);
                RHICheck(vkCreateFramebuffer(VulkanRHI::Device, &fbCreateInfo, nullptr, &m_renderResource.frameBuffers[i]));
            }

            for (uint32_t i = 0; i < backBufferSize; ++i)
            {
                // Command pool
                {
                    VkCommandPoolCreateInfo poolCreateInfo{};
                    poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                    poolCreateInfo.queueFamilyIndex = VulkanRHI::get()->getGraphicsFamily();
                    RHICheck(vkCreateCommandPool(VulkanRHI::Device, &poolCreateInfo, nullptr, &m_renderResource.commandPools[i]));
                }
                // Command buffer
                {
                    VkCommandBufferAllocateInfo bufferAllocateInfo{};
                    bufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                    bufferAllocateInfo.commandPool = m_renderResource.commandPools[i];
                    bufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                    bufferAllocateInfo.commandBufferCount = 1;
                    RHICheck(vkAllocateCommandBuffers(VulkanRHI::Device, &bufferAllocateInfo, &m_renderResource.commandBuffers[i]));
                }
            }
        }
    }

    void ImGuiPass::releaseRenderPass(bool isFullRelease)
    {
        vkDeviceWaitIdle(VulkanRHI::Device);

        if (isFullRelease)
        {
            vkDestroyRenderPass(VulkanRHI::Device, m_renderResource.renderPass, nullptr);
        }

        auto backBufferSize = m_renderResource.frameBuffers.size();
        for (uint32_t i = 0; i < backBufferSize; ++i)
        {
            vkFreeCommandBuffers(VulkanRHI::Device, m_renderResource.commandPools[i], 1, &m_renderResource.commandBuffers[i]);
            vkDestroyCommandPool(VulkanRHI::Device, m_renderResource.commandPools[i], nullptr);
            vkDestroyFramebuffer(VulkanRHI::Device, m_renderResource.frameBuffers[i], nullptr);
        }

        m_renderResource.frameBuffers.clear();
        m_renderResource.commandPools.clear();
        m_renderResource.commandBuffers.clear();
    }

    void ImGuiPass::init()
    {
        // prepare descriptor pool
        {
            VkDescriptorPoolSize poolSizes[]{
                { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
            };

            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            poolInfo.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
            poolInfo.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(poolSizes));
            poolInfo.pPoolSizes = poolSizes;
            RHICheck(vkCreateDescriptorPool(VulkanRHI::Device, &poolInfo, nullptr, &m_renderResource.descriptorPool));
        }

        buildRenderPass();

        // Register swap chain rebuild function

        // Initialize vulkan resource
        ImGui_ImplVulkan_InitInfo vkInitInfo{};
        setupVulkanInitInfo(&vkInitInfo, m_renderResource.descriptorPool);

        // Initialize vulkan
        ImGui_ImplVulkan_Init(&vkInitInfo, m_renderResource.renderPass);

        // Upload font texture
        VkCommandPool commandPool = m_renderResource.commandPools[0];
        VkCommandBuffer commandBuffer = m_renderResource.commandBuffers[0];
        RHICheck(vkResetCommandPool(VulkanRHI::Device, commandPool, 0));
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        RHICheck(vkBeginCommandBuffer(commandBuffer, &beginInfo));
        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
        VkSubmitInfo endInfo{};
        endInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        endInfo.commandBufferCount = 1;
        endInfo.pCommandBuffers = &commandBuffer;
        RHICheck(vkEndCommandBuffer(commandBuffer));
        RHICheck(vkQueueSubmit(vkInitInfo.Queue, 1, &endInfo, VK_NULL_HANDLE));
        RHICheck(vkDeviceWaitIdle(vkInitInfo.Device));
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    void ImGuiPass::renderFrame(uint32_t backBufferIndex)
    {
        ImDrawData* mainDrawData = ImGui::GetDrawData();
        {
            RHICheck(vkResetCommandPool(VulkanRHI::Device, m_renderResource.commandPools[backBufferIndex], 0));
            VkCommandBufferBeginInfo info{};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            RHICheck(vkBeginCommandBuffer(m_renderResource.commandBuffers[backBufferIndex], &info));
            VulkanRHI::setPerfMarkerBegin(m_renderResource.commandBuffers[backBufferIndex], "ImGUI", { 1.0f, 1.0f, 0.0f, 1.0f });
        }
        {
            VkRenderPassBeginInfo rpBeginInfo{};
            rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            rpBeginInfo.renderPass = m_renderResource.renderPass;
            rpBeginInfo.framebuffer = m_renderResource.frameBuffers[backBufferIndex];
            rpBeginInfo.renderArea.extent.width = VulkanRHI::get()->getSwapChainExtent().width;
            rpBeginInfo.renderArea.extent.height = VulkanRHI::get()->getSwapChainExtent().height;
            rpBeginInfo.clearValueCount = 1;

            VkClearValue clearColor{};
            clearColor.color.float32[0] = m_clearColor.x * m_clearColor.w;
            clearColor.color.float32[1] = m_clearColor.y * m_clearColor.w;
            clearColor.color.float32[2] = m_clearColor.z * m_clearColor.w;
            clearColor.color.float32[3] = m_clearColor.w;
            rpBeginInfo.pClearValues = &clearColor;
            vkCmdBeginRenderPass(m_renderResource.commandBuffers[backBufferIndex], &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);   // TODO: fix
        }

        ImGui_ImplVulkan_RenderDrawData(mainDrawData, m_renderResource.commandBuffers[backBufferIndex]);

        vkCmdEndRenderPass(m_renderResource.commandBuffers[backBufferIndex]);
        VulkanRHI::setPerfMarkerEnd(m_renderResource.commandBuffers[backBufferIndex]);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = VulkanRHI::get()->getSwapChainImages().at(backBufferIndex);
        VkImageSubresourceRange range{};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = VK_REMAINING_MIP_LEVELS;
        range.baseArrayLayer = 0;
        range.layerCount = VK_REMAINING_ARRAY_LAYERS;
        barrier.subresourceRange = range;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(m_renderResource.commandBuffers[backBufferIndex],
                VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                {},
                0,
                nullptr,
                0,
                nullptr,
                1,
                &barrier);

        VkImageSubresourceLayers copyLayer{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,};

        VkImageBlit copyRegion{
            .srcSubresource = copyLayer,
            .dstSubresource = copyLayer,};

        copyRegion.srcOffsets[0] = { 0, 0, 0 };
        copyRegion.dstOffsets[0] = copyRegion.srcOffsets[0];
        copyRegion.srcOffsets[1] = { (int)m_drawUIImages.get()->getExtent().width, (int)m_drawUIImages.get()->getExtent().height, 1 };
        copyRegion.dstOffsets[1] = copyRegion.srcOffsets[1];

        vkCmdBlitImage(m_renderResource.commandBuffers[backBufferIndex],
                        m_drawUIImages->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VulkanRHI::get()->getSwapChainImages().at(backBufferIndex), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        1, &copyRegion, VK_FILTER_NEAREST);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(m_renderResource.commandBuffers[backBufferIndex],
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                {},
                0,
                nullptr,
                0,
                nullptr,
                1,
                &barrier);

        RHICheck(vkEndCommandBuffer(m_renderResource.commandBuffers[backBufferIndex]));
    }

    void ImGuiPass::release()
    {
        // unregister swap chain rebuild functions

        // shut down vulkan
        ImGui_ImplVulkan_Shutdown();

        releaseRenderPass(true);
        vkDestroyDescriptorPool(VulkanRHI::Device, m_renderResource.descriptorPool, nullptr);
    }
}












