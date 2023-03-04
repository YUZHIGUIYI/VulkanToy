//
// Created by ZZK on 2023/3/1.
//

#include <VulkanToy/Core/Window.h>
#include <Platform/Windows/WindowsWindow.h>

namespace VT
{
    Scope<Window> Window::Create(const WindowProps &props)
    {
        return CreateScope<WindowsWindow>(props);
    }
}