//
// Created by ZHIKANG on 2023/4/4.
//

#include <VulkanToy/Renderer/Renderer.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>
#include <VulkanToy/AssetSystem/MeshMisc.h>

namespace VT
{
    void RenderTarget::release()
    {
        if (colorImage)
        {
            colorImage->release();
            colorImage = nullptr;
        }
        if (depthImage)
        {
            depthImage->release();
            depthImage = nullptr;
        }
    }

    RenderTarget RenderTarget::create(uint32_t width, uint32_t height,
                                        uint32_t samples, VkFormat colorFormat, VkFormat depthFormat)
    {
        RenderTarget target{};
        target.width = width;
        target.height = height;
        target.colorFormat = colorFormat;
        target.depthFormat = depthFormat;

        VkImageUsageFlags colorImageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (samples == 1) colorImageUsage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        if (colorFormat != VK_FORMAT_UNDEFINED)
        {
            target.colorImage = VulkanImage::create(width, height, 1, 1, colorFormat, samples, colorImageUsage, true);
        }
        if (depthFormat != VK_FORMAT_UNDEFINED)
        {
            target.depthImage = VulkanImage::create(width, height, 1, 1, depthFormat, samples, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, false);
        }

        return target;
    }

    Renderer::Renderer()
    {
        m_passCollector.emplace_back(CreateRef<PreprocessPass>());
        m_passCollector.emplace_back(CreateRef<SkyboxPass>());
        m_passCollector.emplace_back(CreateRef<PBRPass>());

        // Tone-mapping pass
        m_passCollector.emplace_back(CreateRef<TonemapPass>());
    }

    void Renderer::init()
    {
        setupRenderTargets();
        setupRenderPass();
        setupFrameBuffers();
        setupPipelines();

        VulkanRHI::get()->onAfterSwapChainRebuild.subscribe([] ()
        {
            RendererHandle::Get()->rebuildRenderTargetsAndFramebuffers();
        });
    }

    void Renderer::release()
    {
        // Render targets
        for (auto&& renderTarget : m_renderTargets)
        {
            renderTarget.release();
        }
        // Frame buffers
        for (auto& frameBuffer : m_frameBuffers)
        {
            vkDestroyFramebuffer(VulkanRHI::Device, frameBuffer, nullptr);
        }
        // Pass collector
        for (auto& passInterface : m_passCollector)
        {
            std::visit([] (auto&& pass)
            {
                pass->release();
            }, passInterface);
        }
    }

    void Renderer::tick(const RuntimeModuleTickData &tickData)
    {
        // TODO: include ImGui

        // VulkanRHI - acquire next image
        uint32_t imageIndex = VulkanRHI::get()->acquireNextPresentImage();

        // Record
        VkCommandBufferBeginInfo cmdBufInfo = Initializers::initCommandBufferBeginInfo();

        std::array<VkClearValue, 2> clearValues{};
//        VkClearValue clearValues[2];
//        clearValues[0].color = { { 0.1f, 0.1f, 0.1f, 1.0f } };
//        clearValues[1].depthStencil = { 1.0f, 0 };
        clearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo renderPassBeginInfo = Initializers::initRenderPassBeginInfo();
        renderPassBeginInfo.renderPass = m_renderPass;
        renderPassBeginInfo.renderArea.offset.x = 0;
        renderPassBeginInfo.renderArea.offset.y = 0;
        auto extent = VulkanRHI::get()->getSwapChainExtent();
        renderPassBeginInfo.renderArea.extent.width = extent.width;
        renderPassBeginInfo.renderArea.extent.height = extent.height;
        renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassBeginInfo.pClearValues = clearValues.data();

        // Start target frame buffer
        renderPassBeginInfo.framebuffer = m_frameBuffers[imageIndex];

        auto& currentCmd = VulkanRHI::get()->getDrawCommandBuffer(imageIndex);
        RHICheck(vkBeginCommandBuffer(currentCmd, &cmdBufInfo));

        vkCmdBeginRenderPass(currentCmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = Initializers::initViewport((float)extent.width, (float)extent.height, 0.0f, 1.0f);
        vkCmdSetViewport(currentCmd, 0, 1, &viewport);

        VkRect2D scissor = Initializers::initRect2D((int32_t)extent.width, (int32_t)extent.height, 0, 0);
        vkCmdSetScissor(currentCmd, 0, 1, &scissor);

        // Pass render
        for (auto&& passInterface : m_passCollector)
        {
            std::visit([currentCmd] (auto&& pass)
            {
                using T = std::decay_t<decltype(pass)>;
                if constexpr (std::is_same_v<T, Ref<SkyboxPass>> || std::is_same_v<T, Ref<PBRPass>> || std::is_same_v<T, Ref<TonemapPass>>)
                {
                    pass->onRenderTick(currentCmd);
                }
            }, passInterface);
        }

        vkCmdEndRenderPass(currentCmd);

        RHICheck(vkEndCommandBuffer(currentCmd));

        // Vulkan submit info
        VkPipelineStageFlags waitFlags = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        auto frameStartSemaphore = VulkanRHI::get()->getCurrentFrameWaitSemaphore();
        auto frameEndSemaphore = VulkanRHI::get()->getCurrentFrameFinishSemaphore();
        VulkanSubmitInfo graphicsCmdSubmitInfo{};
        graphicsCmdSubmitInfo.setWaitStage(&waitFlags)
                            .setWaitSemaphore(&frameStartSemaphore, 1)
                            .setSignalSemaphore(&frameEndSemaphore, 1)
                            .setCommandBuffer(&currentCmd, 1);

        // Reset fence
        VulkanRHI::get()->resetFence();

        // Submit command buffer
        VulkanRHI::get()->submit(1, &graphicsCmdSubmitInfo.getSubmitInfo());

        // Present
        VulkanRHI::get()->present();
    }

    void Renderer::rebuildRenderTargetsAndFramebuffers()
    {
        for (auto&& renderTarget : m_renderTargets)
        {
            renderTarget.release();
        }

        for (auto&& framebuffer : m_frameBuffers)
        {
            vkDestroyFramebuffer(VulkanRHI::Device, framebuffer, nullptr);
        }

        setupRenderTargets();
        setupFrameBuffers();

        // Create descriptor image info for tone-mapping pass - no sampler required
        // TODO: support MSAA later
        auto numFrames = m_renderTargets.size();
        std::vector<VkDescriptorImageInfo> descriptors;
        descriptors.reserve(numFrames);
        for (uint32_t i = 0; i < numFrames; ++i)
        {
            const VkDescriptorImageInfo imageInfo{ VK_NULL_HANDLE, m_renderTargets[i].colorImage->getView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
            descriptors.push_back(imageInfo);
        }

        // Update descriptor sets for tone-mapping pass
        for (auto&& passInterface : m_passCollector)
        {
            std::visit([&descriptors] (auto&& pass)
            {
                using T = std::decay_t<decltype(pass)>;
                if constexpr (std::is_same_v<T, Ref<TonemapPass>>)
                {
                    pass->updateDescriptorSets(descriptors);
                }
            }, passInterface);
        }
    }

    void Renderer::setupRenderTargets()
    {
        // Create render targets - need to rebuild when resize window
        const VkFormat colorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        const VkFormat depthFormat = VulkanRHI::get()->getSupportDepthStencilFormat();

        auto swapChainExtent = VulkanRHI::get()->getSwapChainExtent();
        auto numFrames = VulkanRHI::get()->getSwapChain().imageCount;
        m_renderTargets.resize(numFrames);
        for (uint32_t i = 0; i < numFrames; ++i)
        {
            m_renderTargets[i] = RenderTarget::create(swapChainExtent.width, swapChainExtent.height, 1, colorFormat, depthFormat);
        }
    }

    void Renderer::setupRenderPass()
    {
        // Attachments
        std::vector<VkAttachmentDescription> attachments{
            // Main color attachment - 0
            {
                0, m_renderTargets[0].colorFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            },
            // Main depth-stencil attachment - 1
            {
                0, m_renderTargets[0].depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            },
            // Swapchain color attachment - 2
            {
                0, VulkanRHI::get()->getSwapChainFormat(), VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            }
        };

        // Main pass
        const std::array<VkAttachmentReference, 1> mainPassColorRefs{
            // Main color attachment - 0
            { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
        };
        const std::array<VkAttachmentReference, 1> mainPassDepthStencilRef{
            // Main depth-stencil attachment - 1
            { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }
        };
        VkSubpassDescription mainPass{};
        mainPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        mainPass.colorAttachmentCount = static_cast<uint32_t>(mainPassColorRefs.size());
        mainPass.pColorAttachments = mainPassColorRefs.data();
        mainPass.pDepthStencilAttachment = mainPassDepthStencilRef.data();

        // Tonemapping subpass
        const std::array<VkAttachmentReference, 1> tonemapPassInputRefs{
            // Main color attachment - 0
            { 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
        };
        const std::array<VkAttachmentReference, 1> tonemapPassColorRefs{
            // Swapchain color attachment - 2
            { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
        };
        VkSubpassDescription tonemapPass{};
        tonemapPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        tonemapPass.colorAttachmentCount = static_cast<uint32_t>(tonemapPassColorRefs.size());
        tonemapPass.pColorAttachments = tonemapPassColorRefs.data();
        tonemapPass.inputAttachmentCount = static_cast<uint32_t>(tonemapPassInputRefs.size());
        tonemapPass.pInputAttachments = tonemapPassInputRefs.data();

        const std::array<VkSubpassDescription, 2> subpasses{ mainPass, tonemapPass };

        // Main -> Tonemapping dependency
        const VkSubpassDependency mainToTonemapDependency{
            0,
            1,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_DEPENDENCY_BY_REGION_BIT
        };

        VkRenderPassCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        createInfo.pAttachments = attachments.data();
        createInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
        createInfo.pSubpasses = subpasses.data();
        createInfo.dependencyCount = 1;
        createInfo.pDependencies = &mainToTonemapDependency;

        RHICheck(vkCreateRenderPass(VulkanRHI::Device, &createInfo, nullptr, &m_renderPass));
    }

    void Renderer::setupFrameBuffers()
    {
        // Depth/Stencil attachment, color attachment is different
        VkFramebufferCreateInfo framebufferCreateInfo{};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.pNext = nullptr;
        framebufferCreateInfo.renderPass = m_renderPass;
        auto extent = VulkanRHI::get()->getSwapChainExtent();
        framebufferCreateInfo.width = extent.width;
        framebufferCreateInfo.height = extent.height;
        framebufferCreateInfo.layers = 1;

        // Create frame buffers for every swap chain image
        m_frameBuffers.resize(VulkanRHI::get()->getSwapChain().imageCount);
        for (uint32_t i = 0; i < m_frameBuffers.size(); ++i)
        {
            std::vector<VkImageView> attachments{
                m_renderTargets[i].colorImage->getView(),
                m_renderTargets[i].depthImage->getView(),
                VulkanRHI::get()->getSwapChain().swapChainImageViews[i]
            };

            // TODO: consider MSAA

            framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferCreateInfo.pAttachments = attachments.data();

            RHICheck(vkCreateFramebuffer(VulkanRHI::Device, &framebufferCreateInfo, nullptr, &m_frameBuffers[i]));
        }
    }

    void Renderer::setupPipelines()
    {
        // Create descriptor image info for tone-mapping pass - no sampler required
        // TODO: support MSAA later
        auto numFrames = m_renderTargets.size();
        std::vector<VkDescriptorImageInfo> descriptors;
        descriptors.reserve(numFrames);
        for (uint32_t i = 0; i < numFrames; ++i)
        {
            const VkDescriptorImageInfo imageInfo{ VK_NULL_HANDLE, m_renderTargets[i].colorImage->getView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
            descriptors.push_back(imageInfo);
        }

        // Initialize passes
        for (auto&& passInterface : m_passCollector)
        {
            std::visit([this, &descriptors] (auto&& pass)
            {
                using T = std::decay_t<decltype(pass)>;
                if constexpr (std::is_same_v<T, Ref<TonemapPass>>)
                {
                    pass->init(m_renderPass, descriptors);
                } else
                {
                    pass->init(m_renderPass);
                }
            }, passInterface);
        }
    }
}



















