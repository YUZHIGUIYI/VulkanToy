//
// Created by ZHIKANG on 2023/3/31.
//

#include <VulkanToy/VulkanRHI/VulkanRHICommon.h>

namespace VT::Initializers
{
    VkSamplerCreateInfo initSamplerCreateInfo(VkFilter filters, VkSamplerAddressMode samplerAddressMode,
                                                VkSamplerMipmapMode mipmapMode, float maxLod, float mipLodBias)
    {

        VkSamplerCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.pNext = nullptr;
        info.magFilter = filters;
        info.minFilter = filters;
        info.addressModeU = samplerAddressMode;
        info.addressModeV = samplerAddressMode;
        info.addressModeW = samplerAddressMode;
        info.mipmapMode = mipmapMode;
        info.maxLod = maxLod;
        info.mipLodBias = mipLodBias;
        return info;
    }

    VkRenderPassCreateInfo2 initRenderPassCreateInfo(uint32_t attachmentCount,
                                                VkAttachmentDescription2 *attachmentDescription,
                                                uint32_t subPassCount,
                                                VkSubpassDescription2 *subPassDescription,
                                                VkRenderPassCreateFlags flags)
    {
        VkRenderPassCreateInfo2 renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
        renderPassInfo.attachmentCount = attachmentCount;
        renderPassInfo.pAttachments = attachmentDescription;
        renderPassInfo.subpassCount = subPassCount;
        renderPassInfo.pSubpasses = subPassDescription;

        return renderPassInfo;
    }

    VkAttachmentDescription2 initAttachmentDescription(VkFormat format, VkImageLayout initialLayout,
                                                        VkImageLayout finalLayout,
                                                        VkAttachmentLoadOp loadOp,
                                                        VkAttachmentStoreOp storeOp,
                                                        VkAttachmentDescriptionFlags flags)
    {
        VkAttachmentDescription2 attachmentDescription{};
        attachmentDescription.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
        attachmentDescription.format = format;
        attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDescription.loadOp = loadOp;
        attachmentDescription.storeOp = storeOp;
        attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDescription.initialLayout = initialLayout;
        attachmentDescription.finalLayout = finalLayout;

        return attachmentDescription;
    }

    VkAttachmentReference2 initAttachmentReference(uint32_t attachment, VkImageLayout layout)
    {
        VkAttachmentReference2 attachmentReference{};
        attachmentReference.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
        attachmentReference.attachment = attachment;
        attachmentReference.layout = layout;

        return attachmentReference;
    }
}