//
// Created by ZHIKANG on 2023/4/14.
//

#include <VulkanToy/Scene/Skybox.h>
#include <VulkanToy/AssetSystem/TextureManager.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>
#include <VulkanToy/Scene/Scene.h>
#include <VulkanToy/Renderer/SceneCamera.h>
#include <VulkanToy/Renderer/PreprocessPass.h>

namespace VT
{
    void Skybox::init()
    {
        // TODO: add cube map
        if (EngineMeshes::GSkyBoxRef.lock())
        {
            cacheGPUMeshAsset = EngineMeshes::GSkyBoxRef.lock();
            VT_CORE_INFO("Loading skybox mesh asset successfully");

            cacheCubeMapAsset = PreprocessPassHandle::Get()->getEnvironmentCube();
            // cacheCubeMapAsset = PreprocessPassHandle::Get()->getIrradianceCube();

            setupDescriptors();

            isMeshReady = true;
            isMeshReplace = true;
        }
    }

    void Skybox::setupDescriptors()
    {
        auto uniformBuffer = SceneHandle::Get()->getUniformBuffer();
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = cacheCubeMapAsset->getView();
        imageInfo.sampler = VulkanRHI::SamplerManager->getSampler(static_cast<uint8_t>(TextureType::Cube));
        // imageInfo.sampler = VulkanRHI::SamplerManager->getSampler(static_cast<uint8_t>(TextureType::Irradiance));

        bool result = VulkanRHI::get()->descriptorFactoryBegin()
            .bindBuffers(0, 1, &uniformBuffer->getDescriptorBufferInfo(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
            .bindImages(1, 1, &imageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build(descriptorSet);
        VT_CORE_ASSERT(result, "Fail to set up vulkan descriptor set");
    }

    void Skybox::release()
    {
        // Currently use one descriptor set
        vkFreeDescriptorSets(VulkanRHI::Device, VulkanRHI::get()->getDescriptorPoolCache().getPool(),
                                1, &descriptorSet);
    }

    void Skybox::tick(const RuntimeModuleTickData &tickData)
    {
        // TODO: add implementation
    }

    void Skybox::onRenderTick(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout)
    {
        if (isMeshReady)
        {
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                                    &descriptorSet, 0, nullptr);

            glm::mat4 model = glm::mat4{ glm::mat3{ SceneCameraHandle::Get()->getViewMatrix() } };
            vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);

            const VkDeviceSize offsets[1] = {0};
            vkCmdBindVertexBuffers(cmd, 0, 1, &cacheGPUMeshAsset->getVertexBuffer(), offsets);
            vkCmdBindIndexBuffer(cmd, cacheGPUMeshAsset->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexed(cmd, cacheGPUMeshAsset->getIndicesCount(), 1, 0, 0, 0);
        }
    }
}














