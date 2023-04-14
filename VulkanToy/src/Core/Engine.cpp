//
// Created by ZZK on 2023/3/1.
//

#include <VulkanToy/Core/Engine.h>
#include <VulkanToy/Core/Input.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>
#include <VulkanToy/Renderer/Renderer.h>
#include <VulkanToy/AssetSystem/AssetSystem.h>
#include <VulkanToy/Scene/Scene.h>
#include <VulkanToy/Renderer/SceneCamera.h>

namespace VT
{
    Engine* GEngine = Singleton<Engine>::Get();

    Engine::Engine()
    {

    }

    void Engine::init()
    {
        WindowProps props{ "VulkanToy", 1600, 900 };
        m_window = Window::Create(props);
        m_window->setEventCallback(VT_BIND_EVENT_FN(Engine::onEvent));

        SceneCameraHandle::Get()->init();
        VulkanRHI::get()->init(static_cast<GLFWwindow *>(m_window->getNativeWindow()));
        AssetSystemHandle::Get()->init();
        RendererHandle::Get()->init();
        SceneHandle::Get()->init();

        m_isInitialized = true;
    }

    void Engine::release()
    {
        SceneHandle::Get()->release();
        AssetSystemHandle::Get()->release();
        RendererHandle::Get()->release();
        VulkanRHI::get()->release();
    }

    void Engine::run()
    {
        while (m_isRunning)
        {
            RuntimeModuleTickData tickData{};
            auto time = static_cast<float>(glfwGetTime());
            tickData.deltaTime = time - m_lastTime;
            m_lastTime = time;
            tickData.isFocus = true;
            tickData.isMinimized = false;
            tickData.windowWidth = m_window->getWidth();
            tickData.windowHeight = m_window->getHeight();

            for (auto& layer : m_layerStack)
            {
                layer->tick(tickData);
            }
            SceneCameraHandle::Get()->tick(tickData);
            SceneHandle::Get()->tick(tickData);
            AssetSystemHandle::Get()->tick(tickData);
            RendererHandle::Get()->tick(tickData);

            m_window->tick();
        }

        vkDeviceWaitIdle(VulkanRHI::Device);
    }

    void Engine::onEvent(Event &event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<WindowCloseEvent>(VT_BIND_EVENT_FN(Engine::onWindowClose));

        // TODO: separate handle scene camera more
        SceneCameraHandle::Get()->onEvent(event);

        // TODO: separate handle events
        for (auto it = m_layerStack.rbegin(); it != m_layerStack.rend(); ++it)
        {
            if (event.Handled)
                break;
            (*it)->onEvent(event);
        }
    }

    bool Engine::onWindowClose(WindowCloseEvent &event)
    {
        VT_CORE_INFO("Close window");
        m_isRunning = false;
        return true;
    }

    void Engine::pushLayer(Layer *layer)
    {
        m_layerStack.pushLayer(layer);
        layer->onAttach();
    }

    void Engine::pushOverlay(Layer *layer)
    {
        m_layerStack.pushOverlay(layer);
        layer->onAttach();
    }

}


