//
// Created by ZZK on 2023/3/1.
//

#include <VulkanToy/Core/Engine.h>
#include <VulkanToy/Core/Input.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>
#include <VulkanToy/Renderer/Renderer.h>
#include <VulkanToy/AssetSystem/AssetSystem.h>

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

        VulkanRHI::get()->init(static_cast<GLFWwindow *>(m_window->GetNativeWindow()));
        AssetSystemHandle::Get()->init();
        RendererHandle::Get()->init();

        m_isInitialized = true;
    }

    void Engine::release()
    {
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
            tickData.windowWidth = m_window->GetWidth();
            tickData.windowHeight = m_window->GetHeight();

            for (auto layer : m_layerStack)
            {
                layer->OnUpdate(tickData);
            }

            AssetSystemHandle::Get()->tick(tickData);
            RendererHandle::Get()->tick(tickData);

            m_window->OnUpdate();
        }

        vkDeviceWaitIdle(VulkanRHI::Device);
    }

    void Engine::onEvent(Event &event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<WindowCloseEvent>(VT_BIND_EVENT_FN(Engine::onWindowClose));

        // TODO: Handle events
        for (auto it = m_layerStack.rbegin(); it != m_layerStack.rend(); ++it)
        {
            if (event.Handled)
                break;
            (*it)->OnEvent(event);
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
        m_layerStack.PushLayer(layer);
        layer->OnAttach();
    }

    void Engine::pushOverlay(Layer *layer)
    {
        m_layerStack.PushOverlay(layer);
        layer->OnAttach();
    }

}


