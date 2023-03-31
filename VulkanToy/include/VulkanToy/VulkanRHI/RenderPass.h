//
// Created by ZHIKANG on 2023/3/31.
//

#pragma once

#include <VulkanToy/Core/Base.h>

namespace VT
{
    struct RenderPassAttachment
    {
        VkFormat imageFormat{};
        VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        VkImageLayout initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkImageLayout outputLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkAccessFlags outputAccessMask = VK_ACCESS_SHADER_READ_BIT;

        bool isValid = false;
    };

    struct RenderPassDesc
    {
        RenderPassAttachment colorAttachments[MAX_COLOR_ATTACHMENTS]{};
        RenderPassAttachment depthAttachment{};

        VkClearValue colorClearValue = { .color = { { 0.0f, 0.0f, 0.0f, 1.0f } } };
        VkClearValue depthClearValue = { .depthStencil = {.depth = 1.0f } };
    };

    class RenderPass
    {
    public:
        void init(const RenderPassDesc &renderPassDesc = RenderPassDesc{});
        void release();

        void attachColor(const RenderPassAttachment &attachment, uint32_t slot);
        void attachDepth(const RenderPassAttachment &attachment);

        void complete();

    private:
        void completeColorAttachment(const RenderPassAttachment &attachment, uint32_t slot);
        void completeDepthAttachment(const RenderPassAttachment &attachment);

        std::vector<VkAttachmentDescription2> m_attachmentDescriptions;
        std::vector<VkSubpassDependency2> m_subPassDependencies;
        std::vector<VkAttachmentReference2> m_colorAttachmentReferences;
        VkAttachmentReference2 m_depthAttachmentReference;

    public:
        VkRenderPass m_renderPass = VK_NULL_HANDLE;

        RenderPassAttachment m_colorAttachments[MAX_COLOR_ATTACHMENTS]{};
        RenderPassAttachment m_depthAttachment{};

        uint32_t m_colorAttachmentCount = 0;

        VkClearValue m_colorClearValue = { .color = { { 1.0f, 1.0f, 1.0f, 1.0f } } };
        VkClearValue m_depthClearValue = { .depthStencil = {.depth = 1.0f } };

        bool m_isComplete = true;
    };
}
