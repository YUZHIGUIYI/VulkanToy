//
// Created by ZHIKANG on 2023/4/4.
//

#include <VulkanToy/Scene/Scene.h>
#include <VulkanToy/Scene/StaticMeshComponent.h>

namespace VT
{

    void Scene::init()
    {
        // Just for test
        Ref<StaticMeshComponent> sample = CreateRef<StaticMeshComponent>(EngineMeshes::GBoxUUID);

        addComponent(sample);
    }

    void Scene::addComponent(const Ref<Component> &component)
    {
        m_components.push_back(component);
    }

    void Scene::tick(const RuntimeModuleTickData &tickData)
    {
        for (auto& component : m_components)
        {
            component->tick(tickData);
        }
    }

    void Scene::onRenderTick(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout)
    {
        for (auto& component : m_components)
        {
            component->onRenderTick(cmd, pipelineLayout);
        }
    }
}