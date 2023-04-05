//
// Created by ZHIKANG on 2023/4/4.
//

#pragma once

#include <VulkanToy/Scene/Component.h>

namespace VT
{
    class Scene
    {
    private:
        std::vector<Ref<Component>> m_components;

    public:
        Scene() = default;

        void init();

        void addComponent(const Ref<Component> &component);

        void tick(const RuntimeModuleTickData &tickData);

        void onRenderTick(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout);

        std::vector<Ref<Component>>& getComponents()
        {
            return m_components;
        }
    };

    using SceneHandle = Singleton<Scene>;
}
