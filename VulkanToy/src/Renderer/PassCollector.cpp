//
// Created by ZHIKANG on 2023/4/13.
//

#include <VulkanToy/Renderer/PassCollector.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>
#include <VulkanToy/Scene/Scene.h>

namespace VT
{
    // ----------------------------------------------- Skybox -----------------------------------------------
    void SkyboxPass::setupDescriptorLayout()
    {
        // TODO: cube map and uniform parameters
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{
            Initializers::initDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
            Initializers::initDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
        };

        VkDescriptorSetLayoutCreateInfo descriptorLayout = Initializers::initDescriptorSetLayoutCreateInfo(setLayoutBindings);
        RHICheck(vkCreateDescriptorSetLayout(VulkanRHI::Device, &descriptorLayout, nullptr, &skyboxDescriptorSetLayout));
    }

    void SkyboxPass::setupPipeline(VkRenderPass renderPass)
    {
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = Initializers::initPipelineInputAssemblyState(
                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

        VkPipelineRasterizationStateCreateInfo rasterizationState = Initializers::initPipelineRasterizationState(
                VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

        VkPipelineColorBlendAttachmentState blendAttachmentState = Initializers::initPipelineColorBlendAttachmentState(
                0xf, VK_FALSE);

        VkPipelineColorBlendStateCreateInfo colorBlendState = Initializers::initPipelineColorBlendState(
                1, &blendAttachmentState);

        VkPipelineDepthStencilStateCreateInfo depthStencilState = Initializers::initPipelineDepthStencilState(
                VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);

        VkPipelineViewportStateCreateInfo viewportState = Initializers::initPipelineViewportState(
                1, 1);

        VkPipelineMultisampleStateCreateInfo multisampleState = Initializers::initPipelineMultisampleState(
                VK_SAMPLE_COUNT_1_BIT);

        std::vector<VkDynamicState> dynamicStateEnables{
                VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState = Initializers::initPipelineDynamicState(dynamicStateEnables);

        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = Initializers::initPipelineLayout(&skyboxDescriptorSetLayout, 1);

        std::vector<VkPushConstantRange> pushConstantRanges{
                Initializers::initPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0)
        };
        pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRanges.size();
        pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
        RHICheck(vkCreatePipelineLayout(VulkanRHI::Device, &pipelineLayoutCreateInfo, nullptr, &skyboxPipelineLayout));

        // Create graphics pipeline
        VkGraphicsPipelineCreateInfo pipelineCI = Initializers::initPipeline(skyboxPipelineLayout, renderPass);
        pipelineCI.pInputAssemblyState = &inputAssemblyState;
        pipelineCI.pRasterizationState = &rasterizationState;
        pipelineCI.pColorBlendState = &colorBlendState;
        pipelineCI.pMultisampleState = &multisampleState;
        pipelineCI.pViewportState = &viewportState;
        pipelineCI.pDepthStencilState = &depthStencilState;
        pipelineCI.pDynamicState = &dynamicState;
        pipelineCI.pVertexInputState = StaticMeshVertex::getPipelineVertexInputState(
                { VertexComponent::Position, VertexComponent::Normal, VertexComponent::UV });

        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
        auto shaderModuleVert = VulkanRHI::ShaderManager->getShader("SkyBox.vert.spv");
        auto shaderModuleFrag = VulkanRHI::ShaderManager->getShader("SkyBox.frag.spv");
        shaderStages[0] = Initializers::initPipelineShaderStage(shaderModuleVert, VK_SHADER_STAGE_VERTEX_BIT);
        shaderStages[1] = Initializers::initPipelineShaderStage(shaderModuleFrag, VK_SHADER_STAGE_FRAGMENT_BIT);
        pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineCI.pStages = shaderStages.data();

        RHICheck(vkCreateGraphicsPipelines(VulkanRHI::Device, VK_NULL_HANDLE, 1,
                                            &pipelineCI, nullptr, &skyboxPipeline));
        VT_CORE_INFO("Create skybox pipeline successfully");
    }

    void SkyboxPass::init(VkRenderPass renderPass)
    {
        setupDescriptorLayout();
        setupPipeline(renderPass);
    }

    void SkyboxPass::release()
    {
        if (skyboxDescriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(VulkanRHI::Device, skyboxDescriptorSetLayout, nullptr);
        }
        if (skyboxPipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(VulkanRHI::Device, skyboxPipelineLayout, nullptr);
        }
        if (skyboxPipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(VulkanRHI::Device, skyboxPipeline, nullptr);
        }
    }

    void SkyboxPass::onRenderTick(VkCommandBuffer cmd)
    {
        // TODO: add function
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline);
        SceneHandle::Get()->onRenderTickSkybox(cmd, skyboxPipelineLayout);
    }

    // ------------------------------------------------ PBR ------------------------------------------------
    void PBRPass::setupDescriptorLayout()
    {
        // TODO: add other uniforms and samplers
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{
            Initializers::initDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
            Initializers::initDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
            Initializers::initDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
            Initializers::initDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
            Initializers::initDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
            Initializers::initDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5),
            Initializers::initDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 6),
            Initializers::initDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 7)
        };

        VkDescriptorSetLayoutCreateInfo descriptorLayout = Initializers::initDescriptorSetLayoutCreateInfo(setLayoutBindings);
        RHICheck(vkCreateDescriptorSetLayout(VulkanRHI::Device, &descriptorLayout, nullptr, &pbrDescriptorSetLayout));
    }

    void PBRPass::setupPipeline(VkRenderPass renderPass)
    {
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = Initializers::initPipelineInputAssemblyState(
                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

        VkPipelineRasterizationStateCreateInfo rasterizationState = Initializers::initPipelineRasterizationState(
                VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

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
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = Initializers::initPipelineLayout(&pbrDescriptorSetLayout, 1);

        std::vector<VkPushConstantRange> pushConstantRanges{
                Initializers::initPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0),
                Initializers::initPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(int32_t), sizeof(glm::mat4))
        };
        pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRanges.size();
        pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
        RHICheck(vkCreatePipelineLayout(VulkanRHI::Device, &pipelineLayoutCreateInfo, nullptr, &pbrPipelineLayout));

        // Create pipeline
        VkGraphicsPipelineCreateInfo pipelineCI = Initializers::initPipeline(pbrPipelineLayout, renderPass);
        pipelineCI.pInputAssemblyState = &inputAssemblyState;
        pipelineCI.pRasterizationState = &rasterizationState;
        pipelineCI.pColorBlendState = &colorBlendState;
        pipelineCI.pMultisampleState = &multisampleState;
        pipelineCI.pViewportState = &viewportState;
        pipelineCI.pDepthStencilState = &depthStencilState;
        pipelineCI.pDynamicState = &dynamicState;
        pipelineCI.pVertexInputState = StaticMeshVertex::getPipelineVertexInputState(
                { VertexComponent::Position, VertexComponent::Normal, VertexComponent::UV, VertexComponent::Tangent, VertexComponent::Bitangent });

        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
        auto shaderModuleVert = VulkanRHI::ShaderManager->getShader("PBRTexture.vert.spv");
        auto shaderModuleFrag = VulkanRHI::ShaderManager->getShader("PBRTexture.frag.spv");
        shaderStages[0] = Initializers::initPipelineShaderStage(shaderModuleVert, VK_SHADER_STAGE_VERTEX_BIT);
        shaderStages[1] = Initializers::initPipelineShaderStage(shaderModuleFrag, VK_SHADER_STAGE_FRAGMENT_BIT);
        pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineCI.pStages = shaderStages.data();

        RHICheck(vkCreateGraphicsPipelines(VulkanRHI::Device, VK_NULL_HANDLE, 1,
                                            &pipelineCI, nullptr, &pbrPipeline));
        VT_CORE_INFO("Create PBR pipeline successfully");
    }

    void PBRPass::init(VkRenderPass renderPass)
    {
        setupDescriptorLayout();
        setupPipeline(renderPass);
    }

    void PBRPass::release()
    {
        if (pbrDescriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(VulkanRHI::Device, pbrDescriptorSetLayout, nullptr);
        }
        if (pbrPipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(VulkanRHI::Device, pbrPipelineLayout, nullptr);
        }
        if (pbrPipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(VulkanRHI::Device, pbrPipeline, nullptr);
        }
    }

    void PBRPass::onRenderTick(VkCommandBuffer cmd)
    {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pbrPipeline);
        SceneHandle::Get()->onRenderTick(cmd, pbrPipelineLayout);
    }
}