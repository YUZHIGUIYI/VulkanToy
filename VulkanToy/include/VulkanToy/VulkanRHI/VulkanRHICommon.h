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

    class VulkanSubmitInfo
    {
    private:
        VkSubmitInfo submitInfo{};

    public:
        VulkanSubmitInfo()
        {
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            std::vector<VkPipelineStageFlags> waitStages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            submitInfo.pWaitDstStageMask = waitStages.data();
        }

        VkSubmitInfo& getSubmitInfo() { return submitInfo; }

        VulkanSubmitInfo& setWaitStage(VkPipelineStageFlags *waitStages)
        {
            submitInfo.pWaitDstStageMask = waitStages;
            return *this;
        }

        VulkanSubmitInfo& setWaitSemaphore(VkSemaphore *wait, uint32_t count)
        {
            submitInfo.waitSemaphoreCount = count;
            submitInfo.pWaitSemaphores = wait;
            return *this;
        }

        VulkanSubmitInfo& setSignalSemaphore(VkSemaphore *signal, uint32_t count)
        {
            submitInfo.signalSemaphoreCount = count;
            submitInfo.pSignalSemaphores = signal;
            return *this;
        }

        VulkanSubmitInfo& setCommandBuffer(VkCommandBuffer *cmd, uint32_t count)
        {
            submitInfo.commandBufferCount = count;
            submitInfo.pCommandBuffers = cmd;
            return *this;
        }

        VulkanSubmitInfo& clear()
        {
            submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            std::vector<VkPipelineStageFlags> waitStages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            submitInfo.pWaitDstStageMask = waitStages.data();
            return *this;
        }
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

        VkPipelineVertexInputStateCreateInfo initPipelineVertexInputStateCreateInfo(
                VkVertexInputBindingDescription* inputBindingDescriptions = nullptr,
                uint32_t inputBindingDescriptionsCount = 0,
                VkVertexInputAttributeDescription* inputAttributeDescriptions = nullptr,
                uint32_t inputAttributeDescriptionsCount = 0);

        VkPipelineInputAssemblyStateCreateInfo initPipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology topology);

        VkPipelineDepthStencilStateCreateInfo initPipelineDepthStencilStateCreateInfo(bool depthTest, bool depthWrite, VkCompareOp compareOperation);

        VkPipelineRasterizationStateCreateInfo initPipelineRasterizationStateCreateInfo(VkPolygonMode polygonMode,
                                                                                        VkCullModeFlagBits cullMode = VK_CULL_MODE_BACK_BIT);

        VkPipelineColorBlendAttachmentState initPipelineColorBlendAttachmentState();

        VkPipelineMultisampleStateCreateInfo initPipelineMultisampleStateCreateInfo();

        VkImageSubresourceRange initBasicImageSubresource();

        // Sampler create info
        VkSamplerCreateInfo initBasicSamplerInfo();

        VkSamplerCreateInfo initPointClampEdgeSamplerInfo();

        VkSamplerCreateInfo initPointClampBorder0000SamplerInfo();

        VkSamplerCreateInfo initPointClampBorder1111SamplerInfo();

        VkSamplerCreateInfo initPointRepeatSamplerInfo();

        VkSamplerCreateInfo initLinearClampEdgeSamplerInfo();

        VkSamplerCreateInfo initLinearClampEdgeMipPointSamplerInfo();

        VkSamplerCreateInfo initLinearClampBorder0000MipPointSamplerInfo();

        VkSamplerCreateInfo initLinearClampBorder1111MipPointSamplerInfo();

        VkSamplerCreateInfo initLinearRepeatMipPointSamplerInfo();

        // Pipeline input assembly state create info
        VkPipelineInputAssemblyStateCreateInfo initPipelineInputAssemblyState(VkPrimitiveTopology topology,
            VkPipelineInputAssemblyStateCreateFlags flags, VkBool32 primitiveRestartEnable);

        // Pipeline rasterization state create info
        VkPipelineRasterizationStateCreateInfo initPipelineRasterizationState(
                VkPolygonMode polygonMode,
                VkCullModeFlags cullMode,
                VkFrontFace frontFace,
                VkPipelineRasterizationStateCreateFlags flags = 0);

        // Pipeline color blend attachment state
        VkPipelineColorBlendAttachmentState initPipelineColorBlendAttachmentState(
                VkColorComponentFlags colorWriteMask,
                VkBool32 blendEnable);

        // Pipeline color blend state create info
        VkPipelineColorBlendStateCreateInfo initPipelineColorBlendState(
                uint32_t attachmentCount,
                const VkPipelineColorBlendAttachmentState *pAttachments);

        // Pipeline depth stencil state create info
        VkPipelineDepthStencilStateCreateInfo initPipelineDepthStencilState(
                VkBool32 depthTestEnable,
                VkBool32 depthWriteEnable,
                VkCompareOp depthCompareOp);

        // Pipeline viewport state create info
        VkPipelineViewportStateCreateInfo initPipelineViewportState(
                uint32_t viewportCount,
                uint32_t scissorCount,
                VkPipelineViewportStateCreateFlags flags = 0);

        // Pipeline multisample state create info
        VkPipelineMultisampleStateCreateInfo initPipelineMultisampleState(
                VkSampleCountFlagBits rasterizationSamples,
                VkPipelineMultisampleStateCreateFlags flags = 0);

        // Pipeline dynamic state create info
        VkPipelineDynamicStateCreateInfo initPipelineDynamicState(
                const std::vector<VkDynamicState> &pDynamicStates,
                VkPipelineDynamicStateCreateFlags flags = 0);

        // Pipeline layout create info
        VkPipelineLayoutCreateInfo initPipelineLayout(
            const VkDescriptorSetLayout *pSetLayouts,
            uint32_t setLayoutCount = 1);

        // Pipeline push constant range
        VkPushConstantRange initPushConstantRange(VkShaderStageFlags stageFlags, uint32_t size, uint32_t offset);

        // Pipeline create info
        VkGraphicsPipelineCreateInfo initPipeline(VkPipelineLayout layout, VkRenderPass renderPass, VkPipelineCreateFlags flags = 0);

        // Pipeline shader stage create info
        VkPipelineShaderStageCreateInfo initPipelineShaderStage(VkShaderModule &shaderModule, VkShaderStageFlagBits stage);

        // Command buffer begin info
        VkCommandBufferBeginInfo initCommandBufferBeginInfo();

        // Render pass begin info
        VkRenderPassBeginInfo initRenderPassBeginInfo();

        // Viewport
        VkViewport initViewport(float width, float height, float minDepth, float maxDepth);

        // Rect2D scissor
        VkRect2D initRect2D(int32_t width, int32_t height, int32_t offsetX, int32_t offsetY);

        // Descriptor set
        VkDescriptorSetLayoutBinding initDescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding, uint32_t descriptorCount = 1);

        VkDescriptorSetLayoutCreateInfo initDescriptorSetLayoutCreateInfo(const std::vector<VkDescriptorSetLayoutBinding>& bindings);

        VkDescriptorSetAllocateInfo initDescriptorSetAllocateInfo(VkDescriptorPool descriptorPool,
            const VkDescriptorSetLayout* pSetLayouts,
            uint32_t descriptorSetCount);

        VkWriteDescriptorSet initWriteDescriptorSet(VkDescriptorSet dstSet,
                                                    VkDescriptorType type,
                                                    uint32_t binding,
                                                    VkDescriptorBufferInfo* bufferInfo,
                                                    uint32_t descriptorCount = 1);

        VkWriteDescriptorSet initWriteDescriptorSet(VkDescriptorSet dstSet,
                                                    VkDescriptorType type,
                                                    uint32_t binding,
                                                    VkDescriptorImageInfo *imageInfo,
                                                    uint32_t descriptorCount = 1);
    }
}







