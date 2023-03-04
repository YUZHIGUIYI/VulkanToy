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
        bool IsVSync() const override;

        virtual void* GetNativeWindow() const { return m_Window; }

    private:
        virtual void Init(const WindowProps& props);
        virtual void Shutdown();

    private:
        GLFWwindow* m_Window;
        // TODO: GraphicsContext of Vulkan?
        //Scope<GraphicsContext> m_Context;

        struct WindowData
        {
            std::string Title;
            uint32_t Width, Height;
            bool VSync;

            EventCallbackFn EventCallback;
        };

        WindowData m_Data;
    };
}