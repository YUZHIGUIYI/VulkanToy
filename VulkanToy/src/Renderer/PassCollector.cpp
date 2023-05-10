//
// Created by ZHIKANG on 2023/4/13.
//

#include <VulkanToy/Renderer/PassCollector.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>
#include <VulkanToy/Scene/Scene.h>
#include <VulkanToy/AssetSystem/AssetCommon.h>

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
                VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

        VkPipelineColorBlendAttachmentState blendAttachmentState = Initializers::initPipelineColorBlendAttachmentState(
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);   // 0xf

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
        // TODO: multiple subpass handle
        pipelineCI.subpass = 0;

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
        // Draw skybox
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
        // TODO: multiple subpass handle
        pipelineCI.subpass = 0;

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
        // Draw PBR model
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pbrPipeline);
        SceneHandle::Get()->onRenderTick(cmd, pbrPipelineLayout);
    }

    // ---------------------------------------------- Tonemap ----------------------------------------------
    void TonemapPass::setupDescriptor(const std::vector<VkDescriptorImageInfo> &descriptors)
    {
        // Generate descriptor set layout for tone mapping
        const std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings{
            { 0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr }
        };

        VkDescriptorSetLayoutCreateInfo descriptorLayout = Initializers::initDescriptorSetLayoutCreateInfo(descriptorSetLayoutBindings);

        RHICheck(vkCreateDescriptorSetLayout(VulkanRHI::Device, &descriptorLayout, nullptr, &tonemapDescriptorSetLayout));

        // Allocate and update descriptor sets for tone mapping input - pre-frame
        auto numFrames = VulkanRHI::get()->getSwapChain().imageCount;
        tonemapDescriptorSets.resize(numFrames);
        for (uint32_t i = 0; i < numFrames; ++i)
        {
            tonemapDescriptorSets[i] = VulkanRHI::get()->getDescriptorPoolCache().allocateSet(tonemapDescriptorSetLayout);
            VulkanRHI::get()->updateDescriptorSet(tonemapDescriptorSets[i], 0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, { descriptors[i] });
        }
    }

    void TonemapPass::setupPipeline(VkRenderPass renderPass)
    {
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = Initializers::initPipelineInputAssemblyState(
                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

        VkPipelineRasterizationStateCreateInfo rasterizationState = Initializers::initPipelineRasterizationState(
                VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

        VkPipelineColorBlendAttachmentState blendAttachmentState = Initializers::initPipelineColorBlendAttachmentState(
                0xf, VK_FALSE);

        VkPipelineColorBlendStateCreateInfo colorBlendState = Initializers::initPipelineColorBlendState(
                1, &blendAttachmentState);

        VkPipelineViewportStateCreateInfo viewportState = Initializers::initPipelineViewportState(
                1, 1);

        VkPipelineMultisampleStateCreateInfo multisampleState = Initializers::initPipelineMultisampleState(
                VK_SAMPLE_COUNT_1_BIT);

        std::vector<VkDynamicState> dynamicStateEnables{
                VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState = Initializers::initPipelineDynamicState(dynamicStateEnables);

        VkPipelineVertexInputStateCreateInfo vertexInputState{ .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = Initializers::initPipelineLayout(&tonemapDescriptorSetLayout, 1);
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
        RHICheck(vkCreatePipelineLayout(VulkanRHI::Device, &pipelineLayoutCreateInfo, nullptr, &tonemapPipelineLayout));

        // Create pipeline
        VkGraphicsPipelineCreateInfo pipelineCI = Initializers::initPipeline(tonemapPipelineLayout, renderPass);
        pipelineCI.pInputAssemblyState = &inputAssemblyState;
        pipelineCI.pRasterizationState = &rasterizationState;
        pipelineCI.pColorBlendState = &colorBlendState;
        pipelineCI.pMultisampleState = &multisampleState;
        pipelineCI.pViewportState = &viewportState;
        pipelineCI.pDepthStencilState = nullptr;
        pipelineCI.pDynamicState = &dynamicState;
        pipelineCI.pVertexInputState = &vertexInputState;
        // TODO: multiple subpass handle
        pipelineCI.subpass = 1;

        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
        auto shaderModuleVert = VulkanRHI::ShaderManager->getShader("Tonemap.vert.spv");
        auto shaderModuleFrag = VulkanRHI::ShaderManager->getShader("Tonemap.frag.spv");
        shaderStages[0] = Initializers::initPipelineShaderStage(shaderModuleVert, VK_SHADER_STAGE_VERTEX_BIT);
        shaderStages[1] = Initializers::initPipelineShaderStage(shaderModuleFrag, VK_SHADER_STAGE_FRAGMENT_BIT);
        pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineCI.pStages = shaderStages.data();

        RHICheck(vkCreateGraphicsPipelines(VulkanRHI::Device, VK_NULL_HANDLE, 1,
                                            &pipelineCI, nullptr, &tonemapPipeline));
        VT_CORE_INFO("Create Tone-mapping pipeline successfully");
    }

    void TonemapPass::init(VkRenderPass renderPass, const std::vector<VkDescriptorImageInfo> &descriptors)
    {
        setupDescriptor(descriptors);
        setupPipeline(renderPass);
    }

    void TonemapPass::release()
    {
        if (tonemapDescriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(VulkanRHI::Device, tonemapDescriptorSetLayout, nullptr);
        }
        if (tonemapPipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(VulkanRHI::Device, tonemapPipelineLayout, nullptr);
        }
        if (tonemapPipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(VulkanRHI::Device, tonemapPipeline, nullptr);
        }
    }

    void TonemapPass::updateDescriptorSets(const std::vector<VkDescriptorImageInfo> &descriptors)
    {
        // Update descriptor sets for tone mapping input - pre-frame
        auto numFrames = tonemapDescriptorSets.size();
        for (uint32_t i = 0; i < numFrames; ++i)
        {
            VulkanRHI::get()->updateDescriptorSet(tonemapDescriptorSets[i], 0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, { descriptors[i] });
        }
    }

    void TonemapPass::onRenderTick(VkCommandBuffer cmd)
    {
        // Transition to tone mapping subpass
        vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);

        // Draw a full screen triangle for post-processing/tone mapping
        auto frameIndex = VulkanRHI::get()->getCurrentFrameIndex();
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, tonemapPipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, tonemapPipelineLayout, 0,
                                1, &tonemapDescriptorSets[frameIndex], 0,nullptr);
        vkCmdDraw(cmd, 3, 1, 0, 0);
    }
}