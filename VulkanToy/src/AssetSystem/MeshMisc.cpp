//
// Created by ZHIKANG on 2023/4/2.
//

#include <VulkanToy/AssetSystem/MeshMisc.h>

namespace VT
{
    VkVertexInputBindingDescription StaticMeshVertex::vertexInputBindingDescription;
    std::vector<VkVertexInputAttributeDescription> StaticMeshVertex::vertexInputAttributeDescriptions;
    VkPipelineVertexInputStateCreateInfo StaticMeshVertex::pipelineVertexInputStateCreateInfo;

    VkVertexInputBindingDescription StaticMeshVertex::getInputBindingDescription(uint32_t binding)
    {
        return VkVertexInputBindingDescription{ .binding = binding,
                .stride = sizeof(StaticMeshVertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX };
    }

    VkVertexInputAttributeDescription StaticMeshVertex::getInputAttributeDescription(uint32_t binding,
                                        uint32_t location, VertexComponent component)
    {
        switch (component)
        {
            case VertexComponent::Position:
                return VkVertexInputAttributeDescription{ .location = location, .binding = binding,
                    .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = static_cast<uint32_t>(offsetof(StaticMeshVertex, position)) };
            case VertexComponent::Normal:
                return VkVertexInputAttributeDescription{ .location = location, .binding = binding,
                        .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = static_cast<uint32_t>(offsetof(StaticMeshVertex, normal)) };
            case VertexComponent::UV:
                return VkVertexInputAttributeDescription{ .location = location, .binding = binding,
                        .format = VK_FORMAT_R32G32_SFLOAT, .offset = static_cast<uint32_t>(offsetof(StaticMeshVertex, uv)) };
            case VertexComponent::Tangent:
                return VkVertexInputAttributeDescription{ .location = location, .binding = binding,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = static_cast<uint32_t>(offsetof(StaticMeshVertex, tangent)) };
            case VertexComponent::Joint0:
                return VkVertexInputAttributeDescription{ .location = location, .binding = binding,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = static_cast<uint32_t>(offsetof(StaticMeshVertex, joint0)) };
            case VertexComponent::Weight0:
                return VkVertexInputAttributeDescription{ .location = location, .binding = binding,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = static_cast<uint32_t>(offsetof(StaticMeshVertex, weight0)) };
            default:
                return VkVertexInputAttributeDescription{};
        }
    }

    std::vector<VkVertexInputAttributeDescription> StaticMeshVertex::getInputAttributeDescriptions(uint32_t binding,
                                                        const std::vector<VertexComponent> &components)
    {
        uint32_t location = 0;
        std::vector<VkVertexInputAttributeDescription> result;
        result.reserve(components.size());
        for (VertexComponent vertexComponent : components)
        {
            result.push_back(StaticMeshVertex::getInputAttributeDescription(binding, location, vertexComponent));
            ++location;
        }
        return result;
    }

    VkPipelineVertexInputStateCreateInfo* StaticMeshVertex::getPipelineVertexInputState(
            const std::vector<VertexComponent> &components, uint32_t binding)
    {
        // TODO: consider binding
        vertexInputBindingDescription = StaticMeshVertex::getInputBindingDescription(0);
        vertexInputAttributeDescriptions = StaticMeshVertex::getInputAttributeDescriptions(0, components);

        pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
        pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
        pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributeDescriptions.size());
        pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();

        return &pipelineVertexInputStateCreateInfo;
    }

    void StaticMeshRenderBound::toExtents(const StaticMeshRenderBound &inBound, float &zmin, float &zmax, float &ymin,
                                            float &ymax, float &xmin, float &xmax, float scale)
    {
        xmax = inBound.origin.x + inBound.extent.x * scale;
        xmin = inBound.origin.x - inBound.extent.x * scale;
        ymax = inBound.origin.y + inBound.extent.y * scale;
        ymin = inBound.origin.y - inBound.extent.y * scale;
        zmax = inBound.origin.z + inBound.extent.z * scale;
        zmin = inBound.origin.z - inBound.extent.z * scale;
    }

    StaticMeshRenderBound StaticMeshRenderBound::combine(const StaticMeshRenderBound &bound1,
                                                            const StaticMeshRenderBound &bound2)
    {
        float zmin1, zmin2, zmax1, zmax2, ymin1, ymin2, ymax1, ymax2, xmin1, xmin2, xmax1, xmax2;

        toExtents(bound1, zmin1, zmax1, ymin1, ymax1, xmin1, xmax1);
        toExtents(bound2, zmin2, zmax2, ymin2, ymax2, xmin2, xmax2);

        float xmin = std::min(xmin1, xmin2);
        float ymin = std::min(ymin1, ymin2);
        float zmin = std::min(zmin1, zmin2);
        float xmax = std::max(xmax1, xmax2);
        float ymax = std::max(ymax1, ymax2);
        float zmax = std::max(zmax1, zmax2);

        StaticMeshRenderBound ret{};
        ret.origin.x = (xmin + xmax) * 0.5f;
        ret.origin.y = (ymin + ymax) * 0.5f;
        ret.origin.z = (zmin + zmax) * 0.5f;

        ret.extent.x = (xmax - xmin) * 0.5f;
        ret.extent.y = (ymax - ymin) * 0.5f;
        ret.extent.z = (zmax - zmin) * 0.5f;

        ret.radius = std::sqrt(ret.extent.x * ret.extent.x + ret.extent.y * ret.extent.y + ret.extent.z * ret.extent.z);

        return ret;
    }
}





















