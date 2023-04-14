//
// Created by ZHIKANG on 2023/4/14.
//

#pragma once

#include <VulkanToy/Core/RuntimeModule.h>
#include <VulkanToy/AssetSystem/MeshManager.h>

namespace VT
{
    struct Skybox
    {
        // Cache gpu mesh asset.
        Ref<GPUMeshAsset> cacheGPUMeshAsset = nullptr;
        // Cube map
        Ref<VulkanImage> cacheCubeMapAsset = nullptr;
        // Descriptor set
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        // Asset uuid
        UUID staticMeshUUID{};

        // Mesh load ready
        bool isMeshReady = false;

        // Mesh already replace
        bool isMeshReplace = true;

        void init();
        void release();
        void tick(const RuntimeModuleTickData &tickData);
        void onRenderTick(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout);

        void setupDescriptors();
    };
}
