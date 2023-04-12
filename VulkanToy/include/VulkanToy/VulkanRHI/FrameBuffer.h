//
// Created by ZHIKANG on 2023/3/31.
//

#pragma once

#include <VulkanToy/Core/Base.h>
#include <VulkanToy/VulkanRHI/GPUResource.h>
#include <VulkanToy/VulkanRHI/RenderPass.h>

namespace VT
{
    struct FrameBufferAttachmentDesc
    {
        Ref<VulkanImage> image = nullptr;

        uint32_t layer = 0;
        bool writeEnabled = true;
    };

    struct FrameBufferDesc
    {
        Ref<RenderPass> renderPass = nullptr;

        FrameBufferAttachmentDesc colorAttachments[MAX_COLOR_ATTACHMENTS]{};
        FrameBufferAttachmentDesc depthAttachment{};

        VkExtent2D extent;
        uint32_t layers = 1;
    };

    struct FrameBufferAttachment
    {
        Ref<VulkanImage> image = nullptr;
        uint32_t layer = 0;
        bool isValid = false;
        bool writeEnabled = true;
    };

    class FrameBuffer
    {
    public:
        void init(const FrameBufferDesc &frameBufferDesc = FrameBufferDesc{});
        void release();

        void refresh();
        void changeColorAttachmentImage(const Ref<VulkanImage> &image, const uint32_t slot);
        void changeDepthAttachmentImage(const Ref<VulkanImage> &image);

        Ref<VulkanImage> &getColorImage(uint32_t slot);
        Ref<VulkanImage> &getDepthImage();

        VkFramebuffer m_frameBuffer = VK_NULL_HANDLE;
        Ref<RenderPass> m_vulkanRenderPass = nullptr;

        FrameBufferAttachment m_colorAttachments[MAX_COLOR_ATTACHMENTS]{};
        FrameBufferAttachment m_depthAttachment{};

        VkExtent2D m_extent;
        uint32_t m_layers = 1;

        bool m_isComplete = false;
    };
}









