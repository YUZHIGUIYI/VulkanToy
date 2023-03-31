//
// Created by ZHIKANG on 2023/3/29.
//

#pragma once

#include <VulkanToy/Core/Base.h>

namespace VT
{
    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR        capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;
    };

    enum class VMAUsageFlags
    {
        GPUOnly,
        StageCopyForUpload,
        ReadBack
    };

    namespace Initializers
    {
        VkSamplerCreateInfo initSamplerCreateInfo(VkFilter filters, VkSamplerAddressMode samplerAddressMode,
                                                    VkSamplerMipmapMode mipmapMode, float maxLod, float mipLodBias);

        VkRenderPassCreateInfo2 initRenderPassCreateInfo(uint32_t attachmentCount, VkAttachmentDescription2 *attachmentDescription,
                                                            uint32_t subPassCount, VkSubpassDescription2 *subPassDescription, VkRenderPassCreateFlags flags = 0);

        VkAttachmentDescription2 initAttachmentDescription(VkFormat format,  VkImageLayout initialLayout,
                                                            VkImageLayout finalLayout, VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                            VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE, VkAttachmentDescriptionFlags flags = 0);

        VkAttachmentReference2 initAttachmentReference(uint32_t attachment, VkImageLayout layout);
    }
}
