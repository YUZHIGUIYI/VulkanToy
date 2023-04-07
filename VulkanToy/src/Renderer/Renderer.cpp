//
// Created by ZHIKANG on 2023/4/4.
//

#include <VulkanToy/Renderer/Renderer.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>
#include <VulkanToy/Renderer/SceneCamera.h>
#include <VulkanToy/AssetSystem/MeshMisc.h>
#include <VulkanToy/Scene/Scene.h>

namespace VT
{
    Renderer::Renderer()
    {

    }

    Renderer::~Renderer()
    {

    }

    void Renderer::init()
    {
        createSynchronizationPrimitives();
        setupDepthStencil();
        setupRenderPass();
        setupFrameBuffers();
        setupUniformBuffers();
        setupDescriptors();
        setupPipelines();

        VulkanRHI::get()->onAfterSwapChainRebuild.subscribe([](){
            RendererHandle::Get()->rebuildDepthAndFramebuffers();
        });
    }

    void Renderer::release()
    {
        m_uniformBuffer->release();
        m_uniformBuffer.reset();
        // Depth stencil
        if (m_depthStencil.image != VK_NULL_HANDLE)
        {
            vkDestroyImage(VulkanRHI::Device, m_depthStencil.image, nullptr);
        }
        if (m_depthStencil.imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(VulkanRHI::Device, m_depthStencil.imageView, nullptr);
        }
        if (m_depthStencil.memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(VulkanRHI::Device, m_depthStencil.memory, nullptr);
        }

        for (auto& frameBuffer : m_frameBuffers)
        {
            vkDestroyFramebuffer(VulkanRHI::Device, frameBuffer, nullptr);
        }
        vkFreeDescriptorSets(VulkanRHI::Device, VulkanRHI::get()->getDescriptorPoolCache().getPool(),
                                static_cast<uint32_t>(m_descriptorSets.size()), m_descriptorSets.data());
        vkDestroyPipelineLayout(VulkanRHI::Device, m_pipelineLayout, nullptr);
        vkDestroyPipeline(VulkanRHI::Device, m_pipeline, nullptr);
    }

    void Renderer::tick(const RuntimeModuleTickData &tickData)
    {
        // TODO: include ImGui

        // VulkanRHI - acquire next image
        uint32_t imageIndex = VulkanRHI::get()->acquireNextPresentImage();

        updateUniformBuffers();

        // Record
        VkCommandBufferBeginInfo cmdBufInfo = Initializers::initCommandBufferBeginInfo();

        VkClearValue clearValues[2];
        clearValues[0].color = { { 0.1f, 0.1f, 0.1f, 1.0f } };
        clearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo renderPassBeginInfo = Initializers::initRenderPassBeginInfo();
        renderPassBeginInfo.renderPass = m_renderPass;
        renderPassBeginInfo.renderArea.offset.x = 0;
        renderPassBeginInfo.renderArea.offset.y = 0;
        auto extent = VulkanRHI::get()->getSwapChainExtent();
        renderPassBeginInfo.renderArea.extent.width = extent.width;
        renderPassBeginInfo.renderArea.extent.height = extent.height;
        renderPassBeginInfo.clearValueCount = 2;
        renderPassBeginInfo.pClearValues = clearValues;

        // Start target frame buffer
        renderPassBeginInfo.framebuffer = m_frameBuffers[imageIndex];

        auto& currentCmd = VulkanRHI::get()->getDrawCommandBuffer(imageIndex);
        RHICheck(vkBeginCommandBuffer(currentCmd, &cmdBufInfo));

        vkCmdBeginRenderPass(currentCmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = Initializers::initViewport((float)extent.width, (float)extent.height, 0.0f, 1.0f);
        vkCmdSetViewport(currentCmd, 0, 1, &viewport);

        VkRect2D scissor = Initializers::initRect2D((int32_t)extent.width, (int32_t)extent.height, 0, 0);
        vkCmdSetScissor(currentCmd, 0, 1, &scissor);

        // Static mesh component
        vkCmdBindDescriptorSets(currentCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                &m_descriptorSets[imageIndex], 0,nullptr);
        vkCmdBindPipeline(currentCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

        // Scene
        auto& components = SceneHandle::Get()->getComponents();
        for (auto component : components)
        {
            component->onRenderTick(currentCmd, m_pipelineLayout);
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

    void Renderer::rebuildDepthAndFramebuffers()
    {
        if (m_depthStencil.image != VK_NULL_HANDLE)
        {
            vkDestroyImage(VulkanRHI::Device, m_depthStencil.image, nullptr);
        }
        if (m_depthStencil.imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(VulkanRHI::Device, m_depthStencil.imageView, nullptr);
        }
        if (m_depthStencil.memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(VulkanRHI::Device, m_depthStencil.memory, nullptr);
        }
        for (auto framebuffer : m_frameBuffers)
        {
            vkDestroyFramebuffer(VulkanRHI::Device, framebuffer, nullptr);
        }
        m_depthStencil = {};

        setupDepthStencil();
        setupFrameBuffers();
    }

    void Renderer::createSynchronizationPrimitives()
    {

    }

    void Renderer::setupDepthStencil()
    {
        // TODO: replace, because it have completed in GPUResource, just call create
        VkImageCreateInfo imageCI{};
        imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCI.imageType = VK_IMAGE_TYPE_2D;
        imageCI.format = VulkanRHI::get()->getSupportDepthStencilFormat();
        auto swapChainExtent = VulkanRHI::get()->getSwapChainExtent();
        imageCI.extent = { swapChainExtent.width, swapChainExtent.height, 1 };
        imageCI.mipLevels = 1;
        imageCI.arrayLayers = 1;
        imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        RHICheck(vkCreateImage(VulkanRHI::Device, &imageCI, nullptr, &m_depthStencil.image));
        VkMemoryRequirements memReqs{};
        vkGetImageMemoryRequirements(VulkanRHI::Device, m_depthStencil.image, &memReqs);

        VkMemoryAllocateInfo memAlloc{};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = VulkanRHI::get()->findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        RHICheck(vkAllocateMemory(VulkanRHI::Device, &memAlloc, nullptr, &m_depthStencil.memory));
        RHICheck(vkBindImageMemory(VulkanRHI::Device, m_depthStencil.image, m_depthStencil.memory, 0));

        VkImageViewCreateInfo imageViewCI{};
        imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCI.image = m_depthStencil.image;
        imageViewCI.format = VulkanRHI::get()->getSupportDepthStencilFormat();
        imageViewCI.subresourceRange.baseMipLevel = 0;
        imageViewCI.subresourceRange.levelCount = 1;
        imageViewCI.subresourceRange.baseArrayLayer = 0;
        imageViewCI.subresourceRange.layerCount = 1;
        imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        // Stencil aspect should only be set on depth + stencil formats
        if (imageViewCI.format >= VK_FORMAT_D16_UNORM_S8_UINT)
        {
            imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        vkCreateImageView(VulkanRHI::Device, &imageViewCI, nullptr, &m_depthStencil.imageView);
    }

    // TODO: separate
    void Renderer::setupRenderPass()
    {
        std::array<VkAttachmentDescription, 2> attachments{};
        // Color attachment
        attachments[0].format = VulkanRHI::get()->getSwapChainFormat();
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        // Depth attachment
        attachments[1].format = VulkanRHI::get()->getSupportDepthStencilFormat();
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorReference{};
        colorReference.attachment = 0;
        colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthReference{};
        depthReference.attachment = 1;
        depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpassDescription{};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorReference;
        subpassDescription.pDepthStencilAttachment = &depthReference;
        subpassDescription.inputAttachmentCount = 0;
        subpassDescription.pInputAttachments = nullptr;
        subpassDescription.preserveAttachmentCount = 0;
        subpassDescription.pPreserveAttachments = nullptr;
        subpassDescription.pResolveAttachments = nullptr;

        // Subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies{};
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        dependencies[0].dependencyFlags = 0;

        dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].dstSubpass = 0;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].srcAccessMask = 0;
        dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        dependencies[1].dependencyFlags = 0;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpassDescription;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        RHICheck(vkCreateRenderPass(VulkanRHI::Device, &renderPassInfo, nullptr, &m_renderPass));
    }

    void Renderer::setupFrameBuffers()
    {
        // Depth/Stencil attachment is the same for all frame buffers
        // Color attachment is different
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
            std::vector<VkImageView> attachments{ VulkanRHI::get()->getSwapChain().swapChainImageViews[i], m_depthStencil.imageView };
            framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferCreateInfo.pAttachments = attachments.data();
            RHICheck(vkCreateFramebuffer(VulkanRHI::Device, &framebufferCreateInfo, nullptr, &m_frameBuffers[i]));
        }
    }

    void Renderer::setupUniformBuffers()
    {
        // TODO: check
        m_uniformBuffer = VulkanBuffer::create2(
                "CameraUniformBuffer",
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
                sizeof(CameraParameters));

        // Map persistent
        RHICheck(m_uniformBuffer->map());

        // Update uniform buffers
        updateUniformBuffers();
    }

    void Renderer::setupDescriptors()
    {
        // Have completed in VulkanDescriptor.cpp file
        auto size = VulkanRHI::get()->getSwapChainImages().size();
        m_descriptorSets.resize(size);
        for (size_t i = 0; i < size; ++i)
        {
            bool result = VulkanRHI::get()->descriptorFactoryBegin().bindBuffers(
                            0, 1, &m_uniformBuffer->getDescriptorBufferInfo(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
                    .build(m_descriptorSets[i], m_descriptorSetLayout);
            VT_CORE_ASSERT(result, "Fail to set up vulkan descriptor set");
        }

    }

    void Renderer::setupPipelines()
    {
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = Initializers::initPipelineInputAssemblyState(
                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

        VkPipelineRasterizationStateCreateInfo rasterizationState = Initializers::initPipelineRasterizationState(
                VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

        VkPipelineColorBlendAttachmentState blendAttachmentState = Initializers::initPipelineColorBlendAttachmentState(
                0xf, VK_FALSE);

        VkPipelineColorBlendStateCreateInfo colorBlendState = Initializers::initPipelineColorBlendState(
                1, &blendAttachmentState);

        VkPipelineDepthStencilStateCreateInfo depthStencilState = Initializers::initPipelineDepthStencilState(
                VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

        VkPipelineViewportStateCreateInfo viewportState = Initializers::initPipelineViewportState(
                1, 1);

        VkPipelineMultisampleStateCreateInfo multisampleState = Initializers::initPipelineMultisampleState(
                VK_SAMPLE_COUNT_1_BIT);

        std::vector<VkDynamicState> dynamicStateEnables{
            VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState = Initializers::initPipelineDynamicState(dynamicStateEnables);

        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = Initializers::initPipelineLayout(&m_descriptorSetLayout, 1);

        std::vector<VkPushConstantRange> pushConstantRanges{
            Initializers::initPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0),
            Initializers::initPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(int32_t), sizeof(glm::mat4))
        };
        pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRanges.size();
        pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
        RHICheck(vkCreatePipelineLayout(VulkanRHI::Device, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout));

        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

        // Create pipeline
        VkGraphicsPipelineCreateInfo pipelineCI = Initializers::initPipeline(m_pipelineLayout, m_renderPass);
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

        auto shaderModuleVert = VulkanRHI::ShaderManager->getShader("sample.vert.spv");
        auto shaderModuleFrag = VulkanRHI::ShaderManager->getShader("sample.frag.spv");
        shaderStages[0] = Initializers::initPipelineShaderStage(shaderModuleVert, VK_SHADER_STAGE_VERTEX_BIT);
        shaderStages[1] = Initializers::initPipelineShaderStage(shaderModuleFrag, VK_SHADER_STAGE_FRAGMENT_BIT);

        RHICheck(vkCreateGraphicsPipelines(VulkanRHI::Device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &m_pipeline));
    }

    void Renderer::updateUniformBuffers()
    {
        m_cameraParas.projection = SceneCameraHandle::Get()->getProjection();
        m_cameraParas.view = SceneCameraHandle::Get()->getViewMatrix();
        m_cameraParas.position = SceneCameraHandle::Get()->getPosition();

        m_uniformBuffer->copyData(&m_cameraParas, sizeof(CameraParameters));
    }
}



















