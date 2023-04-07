//
// Created by ZZK on 2023/3/1.
//

#pragma once

#include <VulkanToy/Windows/Window.h>

namespace VT
{
    class WindowsWindow : public Window
    {
    public:
        WindowsWindow(const WindowProps& props);
        virtual ~WindowsWindow();

        void tick() override;

        uint32_t getWidth() override { return m_windowData.width; }
        uint32_t getHeight() override { return m_windowData.height; }

        // Window attributes
        void setEventCallback(const EventCallbackFn& callback) override { m_windowData.eventCallback = callback; }
        void setVSync(bool enabled) override;
        [[nodiscard]] bool isVSync() const override;

        virtual void* getNativeWindow() const override { return m_window; }

    private:
        void init(const WindowProps& props);
        void shutdown();

    private:
        GLFWwindow* m_window;

        struct WindowData
        {
            std::string title{};
            uint32_t width, height;
            bool vsync = true;
            bool isRun = true;

            EventCallbackFn eventCallback;
        };

        WindowData m_windowData;
    };
}
