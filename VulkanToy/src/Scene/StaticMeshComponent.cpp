//
// Created by ZHIKANG on 2023/4/3.
//

#include <VulkanToy/Scene/StaticMeshComponent.h>
#include <VulkanToy/AssetSystem/MeshManager.h>
#include <VulkanToy/AssetSystem/TextureManager.h>

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
        cacheGPUMeshAsset = MeshManager::Get()->getMesh(staticMeshUUID);
        auto& texturePathMap = cacheGPUMeshAsset->getTexturePaths();
        for (auto& path : texturePathMap)
        {
            // TODO: check
            auto tmpUUId = buildUUID();
            if (path.first == TextureType::DIFFUSE)
            {
                TextureRawDataLoadTask::buildFromPath(path.second, tmpUUId, VK_FORMAT_R8G8B8A8_UNORM);
                cacheMaterialAsset->diffuseTexture = TextureManager::Get()->getImage(tmpUUId)->getVulkanImage();
            } else if (path.first == TextureType::HEIGHT)
            {

            }
        }

        isMeshReplace = true;
        isMeshReady = cacheGPUMeshAsset->isAssetReady();
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
        if (cacheGPUMeshAsset)
        {
            glm::mat4 model = glm::mat4{ 1.0f };

            vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);
            vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), sizeof(int32_t), &id);

            const VkDeviceSize offsets[1] = {0};
            vkCmdBindVertexBuffers(cmd, 0, 1, &cacheGPUMeshAsset->getVertexBuffer().m_buffer, offsets);
            vkCmdBindIndexBuffer(cmd, cacheGPUMeshAsset->getIndexBuffer().m_buffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexed(cmd, cacheGPUMeshAsset->getIndicesCount(), 1, 0, 0, 0);
        }
    }

    void StaticMeshComponent::release()
    {
        // TODO
    }
}