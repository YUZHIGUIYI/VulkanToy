//
// Created by ZHIKANG on 2023/4/2.
//

#pragma once

#include <VulkanToy/Core/Base.h>

namespace VT
{
    enum class VertexComponent
    {
        Position, Normal, UV, Tangent, Bitangent
    };

    // Standard index type
    using VertexIndexType = uint32_t;

    // Standard static mesh vertex
    struct StaticMeshVertex
    {
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };
        glm::vec3 normal{ 0.0f, 1.0f, 0.0f };   // normal to up as default.
        glm::vec2 uv{ 0.0f, 0.0f };
        glm::vec3 tangent{ 1.0f, 0.0f, 0.0f };
        glm::vec3 bitangent{ 1.0f, 0.0f, 0.0f };

        static VkVertexInputBindingDescription vertexInputBindingDescription;
        static std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
        static VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;

        static VkVertexInputBindingDescription getInputBindingDescription(uint32_t binding);
        static VkVertexInputAttributeDescription getInputAttributeDescription(uint32_t binding, uint32_t location, VertexComponent component);
        static std::vector<VkVertexInputAttributeDescription> getInputAttributeDescriptions(uint32_t binding, const std::vector<VertexComponent> &components);

        // Return the default pipeline vertex input state create info structure for the requested vertex components
        static VkPipelineVertexInputStateCreateInfo* getPipelineVertexInputState(const std::vector<VertexComponent> &components, uint32_t binding = 0);
    };

    // Explicit assert struct size avoid some glm macro pad memory to float4
    static_assert(sizeof(StaticMeshVertex) == (3 + 3 + 2 + 3 + 3) * sizeof(float));

    // Render bounds of sub-mesh
    struct StaticMeshRenderBound
    {
        glm::vec3 origin{ 0.0f, 0.0f, 0.0f };
        glm::vec3 extent{ 1.0f, 1.0f, 1.0f };
        float radius{ 1.0f };

        static void toExtents(const StaticMeshRenderBound &inBound,
            float &zmin, float &zmax,
            float &ymin, float &ymax,
            float &xmin, float &xmax, float scale = 1.5f);
        static StaticMeshRenderBound combine(const StaticMeshRenderBound &bound1, const StaticMeshRenderBound &bound2);
    };

    struct StaticMeshSubMesh
    {
        StaticMeshRenderBound renderBound{};
        uint32_t indexStartPosition = 0;
        uint32_t indexCount = 0;
        std::string material{};
    };
}


















