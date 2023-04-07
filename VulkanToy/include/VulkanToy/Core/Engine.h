//
// Created by ZZK on 2023/3/1.
//

#pragma once

#include <VulkanToy/Windows/Window.h>
#include <VulkanToy/Core/LayerStack.h>
#include <VulkanToy/Events/Event.h>
#include <VulkanToy/Events/ApplicationEvent.h>

namespace VT
{
    class Engine final
    {
    private:
        Scope<Window> m_window;

        LayerStack m_layerStack;

        float m_lastTime = 0.0f;

        bool m_isRunning = true;

        bool m_isInitialized = false;

    private:
        void onEvent(Event &event);

    public:
        Engine();
        ~Engine() = default;

        Window& getWindow() { return *m_window; }
        bool isEngineInitialized() const { return m_isInitialized; }

        void pushLayer(Layer *layer);
        void pushOverlay(Layer *layer);

        void init();
        void run();
        void release();

        bool onWindowClose(WindowCloseEvent &event);
    };

    extern Engine *GEngine;
}


























