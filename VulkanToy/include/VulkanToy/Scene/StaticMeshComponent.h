//
// Created by ZHIKANG on 2023/4/3.
//

#pragma once

#include <VulkanToy/Scene/Component.h>
#include <VulkanToy/AssetSystem/MaterialManager.h>
#include <VulkanToy/AssetSystem/MeshManager.h>

namespace VT
{
    struct StaticMeshComponent : Component
    {
        // Cache gpu mesh asset.
        Ref<GPUMeshAsset> cacheGPUMeshAsset = nullptr;
        // Material is optional, if no exist, store mesh info in GPU mesh asset directly
        Ref<StandardPBRMaterial> cacheMaterialAsset = nullptr;

        // Asset uuid
        UUID staticMeshUUID{};

        glm::mat4 translation;
        glm::mat4 rotation;
        glm::mat4 scale;

        // Mesh load ready
        bool isMeshReady = false;

        // Mesh already replace
        bool isMeshReplace = true;

        StaticMeshComponent() = default;
        StaticMeshComponent(const UUID &in);

        void setMeshUUID(const UUID &in);

        void loadAssetByUUID();

        void tick(const RuntimeModuleTickData &tickData) override;

        void onRenderTick(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout) override;

        void updateObjectCollectInfo(const RuntimeModuleTickData &tickData);

        void release() override;
    };
}
