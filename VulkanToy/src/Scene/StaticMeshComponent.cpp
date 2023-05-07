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

        cacheMaterialAsset = CreateRef<StandardPBRMaterial>();
        cacheMaterialAsset->albedoTexture = TextureManager::Get()->getImage(EngineImages::GAlbedoImageUUID)->getVulkanImage();
        cacheMaterialAsset->normalTexture = TextureManager::Get()->getImage(EngineImages::GNormalImageUUID)->getVulkanImage();
        cacheMaterialAsset->roughnessTexture = TextureManager::Get()->getImage(EngineImages::GRoughnessImageUUID)->getVulkanImage();
        cacheMaterialAsset->metallicTexture = TextureManager::Get()->getImage(EngineImages::GMetallicImageUUID)->getVulkanImage();

        isMeshReplace = true;
        isMeshReady = true;

        setupDescriptors();
    }

    void StaticMeshComponent::setupDescriptors()
    {
        auto uniformBuffer = SceneHandle::Get()->getUniformBuffer();
        VkDescriptorImageInfo albedoImageInfo{};
        albedoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        albedoImageInfo.imageView = cacheMaterialAsset->albedoTexture->getView();
        albedoImageInfo.sampler = VulkanRHI::SamplerManager->getSampler(static_cast<uint8_t>(TextureType::Albedo));

        VkDescriptorImageInfo normalImageInfo{};
        normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        normalImageInfo.imageView = cacheMaterialAsset->normalTexture->getView();
        normalImageInfo.sampler = VulkanRHI::SamplerManager->getSampler(static_cast<uint8_t>(TextureType::Normal));

        VkDescriptorImageInfo roughnessImageInfo{};
        roughnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        roughnessImageInfo.imageView = cacheMaterialAsset->roughnessTexture->getView();
        roughnessImageInfo.sampler = VulkanRHI::SamplerManager->getSampler(static_cast<uint8_t>(TextureType::Roughness));

        VkDescriptorImageInfo metallicImageInfo{};
        metallicImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        metallicImageInfo.imageView = cacheMaterialAsset->metallicTexture->getView();
        metallicImageInfo.sampler = VulkanRHI::SamplerManager->getSampler(static_cast<uint8_t>(TextureType::Metallic));

        VkDescriptorImageInfo irradianceImageInfo{};
        irradianceImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        irradianceImageInfo.imageView = StandardPBRMaterial::irradianceTexture->getView();
        irradianceImageInfo.sampler = VulkanRHI::SamplerManager->getSampler(static_cast<uint8_t>(TextureType::Irradiance));

        VkDescriptorImageInfo prefilteredImageInfo{};
        prefilteredImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        prefilteredImageInfo.imageView = StandardPBRMaterial::prefilteredMapTexture->getView();
        prefilteredImageInfo.sampler = VulkanRHI::SamplerManager->getSampler(static_cast<uint8_t>(TextureType::Prefiltered));

        VkDescriptorImageInfo BRDFLUTImageInfo{};
        BRDFLUTImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        BRDFLUTImageInfo.imageView = StandardPBRMaterial::BRDFLUT->getView();
        BRDFLUTImageInfo.sampler = VulkanRHI::SamplerManager->getSampler(static_cast<uint8_t>(TextureType::BRDFLUT));

        bool result = VulkanRHI::get()->descriptorFactoryBegin()
            .bindBuffers(0, 1, &uniformBuffer->getDescriptorBufferInfo(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
            .bindImages(1, 1, &albedoImageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .bindImages(2, 1, &normalImageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .bindImages(3, 1, &roughnessImageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .bindImages(4, 1, &metallicImageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .bindImages(5, 1, &irradianceImageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .bindImages(6, 1, &prefilteredImageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .bindImages(7, 1, &BRDFLUTImageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
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