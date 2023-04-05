//
// Created by ZZK on 2023/3/1.
//

#pragma once

#include <VulkanToy/Core/Window.h>
#include <GLFW/glfw3.h>

namespace VT
{
    class WindowsWindow : public Window
    {
    public:
        WindowsWindow(const WindowProps& props);
        virtual ~WindowsWindow();

        void OnUpdate() override;

        uint32_t& GetWidth() override { return m_Data.Width; }
        uint32_t& GetHeight() override { return m_Data.Height; }

        // Window attributes
        void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
        void SetVSync(bool enabled) override;
        [[nodiscard]] bool IsVSync() const override;

        virtual void* GetNativeWindow() const override { return m_Window; }

    private:
        void Init(const WindowProps& props);
        void Shutdown();

    private:
        GLFWwindow* m_Window;

        struct WindowData
        {
            std::string Title;
            uint32_t Width, Height;
            bool VSync = true;
            bool IsRun = true;

            EventCallbackFn EventCallback;
        };

        WindowData m_Data;
    };
}
