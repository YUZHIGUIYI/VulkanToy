//
// Created by ZHIKANG on 2023/4/3.
//

#include <VulkanToy/Scene/StaticMeshComponent.h>
#include <VulkanToy/AssetSystem/MeshManager.h>
#include <VulkanToy/AssetSystem/TextureManager.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>
#include <VulkanToy/Scene/Scene.h>

namespace VT
{
    StaticMeshComponent::StaticMeshComponent(const UUID &in)
    : staticMeshUUID(in)
    {
        loadAssetByUUID();
    }

    void StaticMeshComponent::setMeshUUID(const UUID &in)
    {
        if (staticMeshUUID.empty())
        {
            staticMeshUUID = in;
            loadAssetByUUID();
        }
    }

    void StaticMeshComponent::loadAssetByUUID()
    {
        // Vertex data
        cacheGPUMeshAsset = MeshManager::Get()->getMesh(staticMeshUUID);
        VT_CORE_INFO("Loading mesh asset successfully");
        // TODO: fix texture data - only ao now
        auto imageAssetTemp = TextureManager::Get()->getImage(EngineImages::GAoImageUUID);
        cacheMaterialAsset = CreateRef<StandardPBRMaterial>(EngineImages::GAoImageUUID, "TestMaterial");
        cacheMaterialAsset->albedoTexture = imageAssetTemp->getVulkanImage();

        isMeshReplace = true;
        isMeshReady = true;

        setupDescriptors();
    }

    void StaticMeshComponent::setupDescriptors()
    {
        auto uniformBuffer = SceneHandle::Get()->getUniformBuffer();
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = cacheMaterialAsset->albedoTexture->getView();
        imageInfo.sampler = VulkanRHI::SamplerManager->getSampler(static_cast<uint8_t>(TextureType::Albedo));

        bool result = VulkanRHI::get()->descriptorFactoryBegin()
            .bindBuffers(0, 1, &uniformBuffer->getDescriptorBufferInfo(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
            .bindImages(1, 1, &imageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build(descriptorSet);
        VT_CORE_ASSERT(result, "Fail to set up vulkan descriptor set");
    }

    void StaticMeshComponent::tick(const RuntimeModuleTickData &tickData)
    {
        if (cacheGPUMeshAsset == nullptr)
        {
            return;
        } else
        {
            updateObjectCollectInfo(tickData);
        }
    }

    void StaticMeshComponent::updateObjectCollectInfo(const RuntimeModuleTickData &tickData)
    {
        // TODO: update materials
        //
    }

    void StaticMeshComponent::onRenderTick(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout)
    {
        const int32_t id = 0;
        if (isMeshReady)
        {
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                                &descriptorSet, 0, nullptr);

            glm::mat4 model{ 1.0f };
            model = glm::translate(model, translation);
            model = glm::scale(model, scale);

            vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);
            vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(int32_t), &id);

            const VkDeviceSize offsets[1] = {0};
            vkCmdBindVertexBuffers(cmd, 0, 1, &cacheGPUMeshAsset->getVertexBuffer(), offsets);
            vkCmdBindIndexBuffer(cmd, cacheGPUMeshAsset->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexed(cmd, cacheGPUMeshAsset->getIndicesCount(), 1, 0, 0, 0);
        }
    }

    void StaticMeshComponent::release()
    {
        // Currently use one descriptor set
        vkFreeDescriptorSets(VulkanRHI::Device, VulkanRHI::get()->getDescriptorPoolCache().getPool(),
                                1, &descriptorSet);
    }
}