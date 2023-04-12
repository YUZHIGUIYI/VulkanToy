//
// Created by ZZK on 2023/3/1.
//

#pragma once

#include <VulkanToy/Core/Base.h>
#include <VulkanToy/Core/RuntimeModule.h>
#include <VulkanToy/Events/Event.h>

namespace VT
{
    struct WindowProps
    {
        std::string title{};
        uint32_t width{};
        uint32_t height{};

        WindowProps(const std::string &title = "VulkanToy", uint32_t width = 1600, uint32_t height = 900)
            : title(title), width(width), height(height) {}
    };

    class Window
    {
    public:
        using EventCallbackFn = std::function<void(Event&)>;

        virtual ~Window() = default;

        virtual void tick() = 0;

        virtual uint32_t getWidth() = 0;
        virtual uint32_t getHeight() = 0;

        // Window attributes
        virtual void setEventCallback(const EventCallbackFn& callback) = 0;
        virtual void setVSync(bool enabled) = 0;
        virtual bool isVSync() const = 0;
        virtual void* getNativeWindow() const = 0;

        static Scope<Window> Create(const WindowProps& props = WindowProps{});
    };
}































