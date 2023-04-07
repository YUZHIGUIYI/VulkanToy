//
// Created by ZZK on 2023/3/1.
//

#include <VulkanToy/Windows/WindowsWindow.h>
#include <VulkanToy/Core/Input.h>
#include <VulkanToy/Events/ApplicationEvent.h>
#include <VulkanToy/Events/KeyEvent.h>
#include <VulkanToy/Events/MouseEvent.h>

namespace VT
{
    static uint8_t s_GLFWWindowCount = 0;

    WindowsWindow::WindowsWindow(const WindowProps &props)
    {
        init(props);
    }

    WindowsWindow::~WindowsWindow()
    {
        shutdown();
    }

    void WindowsWindow::init(const WindowProps &props)
    {
        m_windowData.title = props.title;
        m_windowData.width = props.width;
        m_windowData.height = props.height;

        VT_CORE_INFO("Creating window {0} ({1}, {2})", props.title, props.width, props.height);

        // Init glfw and window
        if (s_GLFWWindowCount == 0)
        {
            VT_CORE_INFO("Initializing GLFW");
            int success = glfwInit();
            VT_CORE_ASSERT(success, "Could not initializing GLFW!");
            glfwSetErrorCallback([](int error, const char* description) {
                VT_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
            });

            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

            m_window = glfwCreateWindow((int)props.width, (int)props.height, m_windowData.title.c_str(), nullptr, nullptr);
        }

        // User pointer
        glfwSetWindowUserPointer(m_window, &m_windowData);
        //glfwSetFramebufferSizeCallback(m_Window, framebufferResizeCallback);
        setVSync(true);

        // Set GLFW callbacks
        glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            data.width = width;
            data.height = height;

            WindowResizeEvent event(width, height);
            data.eventCallback(event);
        });

        glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            data.isRun = false;
            WindowCloseEvent event;
            data.eventCallback(event);
        });

        glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

            switch (action)
            {
                case GLFW_PRESS:
                {
                    KeyPressedEvent event(key, 0);
                    data.eventCallback(event);
                    break;
                }
                case GLFW_RELEASE:
                {
                    KeyReleasedEvent event(key);
                    data.eventCallback(event);
                    break;
                }
                case GLFW_REPEAT:
                {
                    KeyPressedEvent event(key, true);
                    data.eventCallback(event);
                    break;
                }
                default:
                    break;
            }
        });

        glfwSetCharCallback(m_window, [](GLFWwindow* window, unsigned int keycode)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

            KeyTypedEvent event(keycode);
            data.eventCallback(event);
        });

        glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

            switch (action)
            {
                case GLFW_PRESS:
                {
                    MouseButtonPressedEvent event(button);
                    data.eventCallback(event);
                    break;
                }
                case GLFW_RELEASE:
                {
                    MouseButtonReleasedEvent event(button);
                    data.eventCallback(event);
                    break;
                }
                default:
                    break;
            }
        });

        glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xOffset, double yOffset)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

            MouseScrolledEvent event((float)xOffset, (float)yOffset);
            data.eventCallback(event);
        });

        glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xPos, double yPos)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

            MouseMovedEvent event((float)xPos, (float)yPos);
            data.eventCallback(event);
        });
    }

    void WindowsWindow::shutdown()
    {
        glfwDestroyWindow(m_window);
        --s_GLFWWindowCount;

        if (s_GLFWWindowCount == 0)
        {
            VT_CORE_INFO("Terminating GLFW");
            glfwTerminate();
        }
    }

    void WindowsWindow::tick()
    {
        glfwPollEvents();
    }

    void WindowsWindow::setVSync(bool enabled)
    {
        if (enabled)
            glfwSwapInterval(1);
        else
            glfwSwapInterval(0);

        m_windowData.vsync = enabled;
    }

    bool WindowsWindow::isVSync() const
    {
        return m_windowData.vsync;
    }
}










