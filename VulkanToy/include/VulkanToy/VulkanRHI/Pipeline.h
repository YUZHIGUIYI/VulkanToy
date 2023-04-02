//
// Created by ZHIKANG on 2023/4/1.
//

#pragma once

#include <VulkanToy/Core/Base.h>
#include <VulkanToy/VulkanRHI/FrameBuffer.h>
#include <VulkanToy/VulkanRHI/VulkanSwapChain.h>
#include <VulkanToy/VulkanRHI/Shader.h>

namespace VT
{
    struct GraphicsPipelineDesc
    {
        // These need to be set
        Ref<FrameBuffer> frameBuffer = nullptr;
        // Shader

        // These have a valid default state
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = Initializers::initPipelineVertexInputStateCreateInfo();
        VkPipelineInputAssemblyStateCreateInfo assemblyInputInfo = Initializers::initPipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        VkPipelineDepthStencilStateCreateInfo depthStencilInputInfo = Initializers::initPipelineDepthStencilStateCreateInfo(true,
                                                                        true, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkPipelineRasterizationStateCreateInfo rasterizer = Initializers::initPipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
        VkPipelineColorBlendAttachmentState colorBlendAttachment = Initializers::initPipelineColorBlendAttachmentState();
        VkPipelineMultisampleStateCreateInfo multisampling = Initializers::initPipelineMultisampleStateCreateInfo();
    };

    struct ComputePipelineDesc
    {
        // These need to be set
    };

    class Pipeline
    {
    public:
        void init(const GraphicsPipelineDesc &desc = GraphicsPipelineDesc{});
        void release();

    public:
        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_layout = VK_NULL_HANDLE;
        VkPipelineBindPoint m_bindPoint{};

        Ref<FrameBuffer> m_frameBuffer = nullptr;

        bool m_isComplete = false;
        bool m_isCompute = false;

    private:
        void generatePipelineLayoutFromShader();

//        VkPipelineVertexInputStateCreateInfo generateVertexInputStateInfoFromShader(
//            VkPipelineVertexInputStateCreateInfo descVertexInputState,
//            std::vector<VkVertexInputBindingDescription> &bindingDescriptions,
//            std::vector<VkVertexInputAttributeDescription> &attributeDescriptions);
//
//        std::vector<VkPipelineColorBlendAttachmentState> generateBlendAttachmentStateFromFrameBuffer(
//            VkPipelineColorBlendAttachmentState blendAttachmentState);
    };
}
