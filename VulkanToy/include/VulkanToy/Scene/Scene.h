//
// Created by ZHIKANG on 2023/4/4.
//

#pragma once

#include <VulkanToy/Scene/Component.h>
#include <VulkanToy/Scene/Skybox.h>
#include <VulkanToy/VulkanRHI/GPUResource.h>

namespace VT
{
    struct CameraParameters
    {
        alignas(16) glm::mat4 projection{};
        alignas(16) glm::mat4 view{};
        alignas(16) glm::vec3 position{};
    };

    class Scene
    {
    private:
        Ref<Skybox> m_skybox;
        std::vector<Ref<Component>> m_components;
        Ref<VulkanBuffer> m_uniformBuffer;

        CameraParameters m_cameraParas{};

    private:
        void updateUniformBuffer();

    public:
        Scene() = default;

        void init();

        void release();

        void addComponent(const Ref<Component> &component);

        void tick(const RuntimeModuleTickData &tickData);

        void onRenderTick(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout);

        void onRenderTickSkybox(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout);

        std::vector<Ref<Component>>& getComponents()
        {
            return m_components;
        }

        [[nodiscard]] Ref<VulkanBuffer> getUniformBuffer() const
        {
            return m_uniformBuffer;
        }
    };

    using SceneHandle = Singleton<Scene>;
}
