//
// Created by ZHIKANG on 2023/4/4.
//

#include <VulkanToy/Scene/Scene.h>
#include <VulkanToy/Scene/StaticMeshComponent.h>
#include <VulkanToy/Renderer/SceneCamera.h>

namespace VT
{
    void Scene::updateUniformBuffer()
    {
        m_cameraParas.projection = SceneCameraHandle::Get()->getProjection();
        m_cameraParas.view = SceneCameraHandle::Get()->getViewMatrix();
        m_cameraParas.position = SceneCameraHandle::Get()->getPosition();

        m_uniformBuffer->copyData(&m_cameraParas, sizeof(CameraParameters));
    }

    void Scene::init()
    {
        // TODO: initialize uniform buffer of camera parameters
        m_uniformBuffer = VulkanBuffer::create2(
                "CameraUniformBuffer",
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
                sizeof(CameraParameters));
        RHICheck(m_uniformBuffer->map());   // Map persistent
        updateUniformBuffer();                  // Update uniform buffers

        // TODO: Just for test
        Ref<StaticMeshComponent> sample = CreateRef<StaticMeshComponent>(EngineMeshes::GCerberusUUID);
        addComponent(sample);
        sample = CreateRef<StaticMeshComponent>(EngineMeshes::GBoxUUID);
        sample->scale = glm::vec3{ 5.0f, 5.0f, 5.0f };
        sample->translation = glm::vec3{ 20.0f, -10.0f, 0.0f };
        addComponent(sample);

        // SkyBox
        m_skybox = CreateRef<Skybox>();
        m_skybox->init();
    }

    void Scene::release()
    {
        // Uniform buffer must unmap
        m_uniformBuffer->unmap();
        m_uniformBuffer->release();
        m_uniformBuffer.reset();

        m_skybox->release();

        for (auto& component : m_components)
        {
            component->release();
        }
    }

    void Scene::addComponent(const Ref<Component> &component)
    {
        m_components.push_back(component);
    }

    void Scene::tick(const RuntimeModuleTickData &tickData)
    {
        updateUniformBuffer();

        m_skybox->tick(tickData);

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

    void Scene::onRenderTickSkybox(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout)
    {
        m_skybox->onRenderTick(cmd, pipelineLayout);
    }
}