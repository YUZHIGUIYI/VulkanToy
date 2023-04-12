//
// Created by ZZK on 2023/3/1.
//

#include <VulkanToy/Windows/Window.h>
#include <VulkanToy/Windows/WindowsWindow.h>

namespace VT
{
    Scope<Window> Window::Create(const WindowProps &props)
    {
        return CreateScope<WindowsWindow>(props);
    }
}