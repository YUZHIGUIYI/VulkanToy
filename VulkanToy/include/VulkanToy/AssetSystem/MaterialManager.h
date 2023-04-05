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
        StandardPBR,
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

    struct StandardPBRMaterial : MaterialInterface
    {
        AlphaMode alphaMode = AlphaMode::AlphaOpaque;

        float alphaCutOff = 1.0f;
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;

        glm::vec4 baseColorFactor{ 1.0f };

        Ref<VulkanImage> baseColorTexture = nullptr;
        Ref<VulkanImage> metallicRoughnessTexture = nullptr;
        Ref<VulkanImage> normalTexture = nullptr;
        Ref<VulkanImage> occlusionTexture = nullptr;
        Ref<VulkanImage> emissiveTexture = nullptr;

        Ref<VulkanImage> specularGlossinessTexture = nullptr;
        Ref<VulkanImage> diffuseTexture = nullptr;

        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

        explicit StandardPBRMaterial(const UUID &uuid, const std::string &name);
    };
}