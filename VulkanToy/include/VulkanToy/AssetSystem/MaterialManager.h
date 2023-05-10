//
// Created by ZHIKANG on 2023/4/3.
//

#pragma once

#include <VulkanToy/Core/Base.h>
#include <VulkanToy/VulkanRHI/GPUResource.h>

namespace VT
{
    enum class MaterialType
    {
        StandardPBR, StandardSkybox
    };

    enum class AlphaMode
    {
        AlphaOpaque, AlphaMask, AlphaBlend
    };

    struct MaterialInterface
    {
        std::string materialName;
        UUID uuid;
        MaterialType materialType;

        MaterialInterface(const UUID &uuid, const std::string &name, MaterialType matType)
        : uuid(uuid), materialName(name), materialType(matType)
        {

        }
    };

    struct StandardPBRMaterial
    {
//        AlphaMode alphaMode = AlphaMode::AlphaOpaque;

        float alphaCutOff = 1.0f;
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;

        glm::vec4 baseColorFactor{ 1.0f };

        Ref<VulkanImage> albedoTexture = nullptr;
        Ref<VulkanImage> normalTexture = nullptr;
        Ref<VulkanImage> aoTexture = nullptr;
        Ref<VulkanImage> metallicTexture = nullptr;
        Ref<VulkanImage> roughnessTexture = nullptr;

        inline static Ref<VulkanImage> irradianceTexture = nullptr;
        inline static Ref<VulkanImage> BRDFLUT = nullptr;
        inline static Ref<VulkanImage> prefilteredMapTexture = nullptr;

//        StandardPBRMaterial(const UUID &uuid, const std::string &name);
    };

    struct SkyboxMaterial
    {
        inline static Ref<VulkanImage> skyboxTexture = nullptr;
    };
}
