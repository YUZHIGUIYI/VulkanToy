//
// Created by ZHIKANG on 2023/3/31.
//

#include <VulkanToy/VulkanRHI/FrameBuffer.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>

namespace VT
{
    void FrameBuffer::init(const FrameBufferDesc &frameBufferDesc)
    {
        m_extent = frameBufferDesc.extent;
        m_layers = frameBufferDesc.layers;

        for (uint32_t i = 0; i < MAX_COLOR_ATTACHMENTS; ++i)
        {
            auto& attachmentDesc = frameBufferDesc.colorAttachments[i];
            FrameBufferAttachment attachment{
                .image = attachmentDesc.image,
                .layer = attachmentDesc.layer,
                .isValid = (attachmentDesc.image != nullptr),
                .writeEnabled = attachmentDesc.writeEnabled
            };
            m_colorAttachments[i] = attachment;
        }

        if (frameBufferDesc.depthAttachment.image != nullptr)
        {
            auto& attachmentDesc = frameBufferDesc.depthAttachment;
            FrameBufferAttachment attachment{
                .image = attachmentDesc.image,
                .layer = attachmentDesc.layer,
                .isValid = (attachmentDesc.image != nullptr),
                .writeEnabled = attachmentDesc.writeEnabled
            };

            m_depthAttachment = attachment;
        }

        refresh();

        m_isComplete = true;
    }

    void FrameBuffer::release()
    {
        vkDestroyFramebuffer(VulkanRHI::Device, m_frameBuffer, nullptr);
    }

    void FrameBuffer::refresh()
    {
        // TODO: fix
        std::vector<VkImageView> imageViews;

        const auto& rpColorAttachments = m_vulkanRenderPass->m_colorAttachments;
        const auto& rpDepthAttachment = m_vulkanRenderPass->m_depthAttachment;

        // Frame buffer requires an image view for each attachment in the render pass
        // This does not mean we need to write to all attachments
        // The writing to attachments is being configured in the pipeline setup and is based on which color attachments the user specified in the frame buffer
        for (uint32_t i = 0; i < MAX_COLOR_ATTACHMENTS; ++i)
        {
            auto& rpColorAttachment = rpColorAttachments[i];
            auto& colorAttachment = m_colorAttachments[i];

            VT_CORE_ASSERT(rpColorAttachment.isValid == colorAttachment.isValid, "Framebuffer color attachment to render pass color attachment mismatch");
            if (!rpColorAttachment.isValid) continue;

            VT_CORE_ASSERT(rpColorAttachment.imageFormat == colorAttachment.image->getFormat(), "Image format does not match the format of the attachment in the render pass");
            // Check if we want to write to this attachment
            if (colorAttachment.isValid)
            {
                VT_CORE_ASSERT(colorAttachment.layer < colorAttachment.image->m_createInfo.arrayLayers, "Image does not contain this layer");
                imageViews.push_back(colorAttachment.image->m_cacheImageViews[colorAttachment.layer]);
            }
        }

        VT_CORE_ASSERT(rpDepthAttachment.isValid == m_depthAttachment.isValid, "Frame buffer depth attachment to render pass depth attachment mismatch");
        if (m_depthAttachment.isValid)
        {
            VT_CORE_ASSERT(m_depthAttachment.layer < m_depthAttachment.image->m_createInfo.arrayLayers, "Image does not contain this layer");
            imageViews.push_back(m_depthAttachment.image->m_cacheImageViews[m_depthAttachment.layer]);
        }

        if (m_isComplete)
        {
            // Delete the previous frame buffer
            vkDestroyFramebuffer(VulkanRHI::Device, m_frameBuffer, nullptr);
        }

        VkFramebufferCreateInfo frameBufferInfo{};
        frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferInfo.renderPass = m_vulkanRenderPass->m_renderPass;
        frameBufferInfo.attachmentCount = static_cast<uint32_t>(imageViews.size());
        frameBufferInfo.pAttachments = imageViews.data();
        frameBufferInfo.width = m_extent.width;
        frameBufferInfo.height = m_extent.height;
        frameBufferInfo.layers = m_layers;
        RHICheck(vkCreateFramebuffer(VulkanRHI::Device, &frameBufferInfo, nullptr, &m_frameBuffer));
    }

    void FrameBuffer::changeColorAttachmentImage(const Ref<VulkanImage> &image, const uint32_t slot)
    {
        VT_CORE_ASSERT(slot < MAX_COLOR_ATTACHMENTS, "Color attachment slot is not available");
        VT_CORE_ASSERT(m_colorAttachments[slot].isValid, "Color attachment is not valid");
        m_colorAttachments[slot].image = image;
    }

    void FrameBuffer::changeDepthAttachmentImage(const Ref<VulkanImage> &image)
    {
        VT_CORE_ASSERT(m_depthAttachment.isValid, "Depth attachment is not valid");
        m_depthAttachment.image = image;
    }

    Ref<VulkanImage> &FrameBuffer::getColorImage(uint32_t slot)
    {
        VT_CORE_ASSERT(slot < MAX_COLOR_ATTACHMENTS, "Color attachment slot is not available");
        return m_colorAttachments[slot].image;
    }

    Ref<VulkanImage> &FrameBuffer::getDepthImage()
    {
        return m_depthAttachment.image;
    }
}






