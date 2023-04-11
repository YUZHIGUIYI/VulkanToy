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
        // Descriptor set
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

        // Asset uuid
        UUID staticMeshUUID{};

        glm::vec3 translation{ 0.0f, 0.0f, 0.0f };
        glm::vec3 rotation{ 0.0f };
        glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

        // Mesh load ready
        bool isMeshReady = false;

        // Mesh already replace
        bool isMeshReplace = true;

        StaticMeshComponent() = default;
        StaticMeshComponent(const UUID &in);

        void setMeshUUID(const UUID &in);

        void loadAssetByUUID();

        void setupDescriptors() override;

        void tick(const RuntimeModuleTickData &tickData) override;

        void onRenderTick(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout) override;

        void updateObjectCollectInfo(const RuntimeModuleTickData &tickData);

        void release() override;
    };
}
