//
// Created by ZHIKANG on 2023/3/31.
//

#include <VulkanToy/VulkanRHI/RenderPass.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>

namespace VT
{
    void RenderPass::init(const RenderPassDesc &renderPassDesc)
    {
        m_colorClearValue = renderPassDesc.colorClearValue;
        m_depthClearValue = renderPassDesc.depthClearValue;

        uint32_t slot = 0;
        for (auto& attachment : renderPassDesc.colorAttachments)
        {
            // TODO: fix color
            if (!attachment.imageFormat) continue;
            attachColor(attachment, slot++);
        }
        // TODO: fix depth
        if (renderPassDesc.depthAttachment.imageFormat) attachDepth(renderPassDesc.depthAttachment);
        complete();
    }

    void RenderPass::release()
    {
        if (!m_isComplete) return;
        vkDestroyRenderPass(VulkanRHI::Device, m_renderPass, nullptr);
    }

    void RenderPass::attachColor(const RenderPassAttachment &attachment, uint32_t slot)
    {
        VT_CORE_ASSERT(slot < MAX_COLOR_ATTACHMENTS, "Color attachment slot is not available");
        m_colorAttachments[slot] = attachment;
        m_colorAttachments[slot].isValid = true;
    }

    void RenderPass::attachDepth(const RenderPassAttachment &attachment)
    {
        m_depthAttachment = attachment;
        m_depthAttachment.isValid = true;
    }

    void RenderPass::complete()
    {
        // Can not create a render pass twice, might still be in use by device
        if (m_isComplete) return;

        for (uint32_t i = 0; i < MAX_COLOR_ATTACHMENTS; ++i)
        {
            auto& attachment = m_colorAttachments[i];
            if (!attachment.imageFormat) continue;
            completeColorAttachment(attachment, i);
        }
        if (m_depthAttachment.imageFormat) completeDepthAttachment(m_depthAttachment);

        VkSubpassDescription2 subPassDescription{};
        subPassDescription.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
        subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subPassDescription.colorAttachmentCount = static_cast<uint32_t>(m_colorAttachmentReferences.size());
        subPassDescription.pColorAttachments = m_colorAttachmentReferences.data();
        if (m_depthAttachment.imageFormat)
            subPassDescription.pDepthStencilAttachment = &m_depthAttachmentReference;

        m_colorAttachmentCount = subPassDescription.colorAttachmentCount;
        VkRenderPassCreateInfo2 renderPassCreateInfo = Initializers::initRenderPassCreateInfo(uint32_t(m_attachmentDescriptions.size()),
                                                        m_attachmentDescriptions.data(), 1, &subPassDescription);
        renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(m_subPassDependencies.size());
        renderPassCreateInfo.pDependencies = m_subPassDependencies.data();

        RHICheck(vkCreateRenderPass2(VulkanRHI::Device, &renderPassCreateInfo, nullptr, &m_renderPass));
        m_isComplete = true;
    }

    void RenderPass::completeColorAttachment(const RenderPassAttachment &attachment, uint32_t slot)
    {
        VkAttachmentDescription2 colorAttachmentDescription = Initializers::initAttachmentDescription(
                attachment.imageFormat, attachment.initialLayout,
                attachment.outputLayout, attachment.loadOp);
        VkSubpassDependency2 colorDependency{};
        colorDependency.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
        colorDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        colorDependency.dstSubpass = 0;
        colorDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        colorDependency.srcAccessMask = 0;  // TODO: fix
        colorDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        colorDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkAttachmentReference2 colorAttachmentReference = Initializers::initAttachmentReference(slot, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        m_attachmentDescriptions.push_back(colorAttachmentDescription);
        m_subPassDependencies.push_back(colorDependency);
        m_colorAttachmentReferences.push_back(colorAttachmentReference);
    }

    void RenderPass::completeDepthAttachment(const RenderPassAttachment &attachment)
    {
        VkAttachmentDescription2 depthAttachmentDescription = Initializers::initAttachmentDescription(
                attachment.imageFormat, attachment.initialLayout,
                attachment.outputLayout, attachment.loadOp);

        VkSubpassDependency2 depthDependency{};
        depthDependency.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
        depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        depthDependency.dstSubpass = 0;
        depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        depthDependency.srcAccessMask = 0;
        depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        uint32_t depthAttachmentSlot = static_cast<uint32_t>(m_colorAttachmentReferences.size());
        m_depthAttachmentReference = Initializers::initAttachmentReference(depthAttachmentSlot, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

        m_attachmentDescriptions.push_back(depthAttachmentDescription);
        m_subPassDependencies.push_back(depthDependency);
    }
}

























