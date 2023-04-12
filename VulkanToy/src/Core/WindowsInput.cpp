//
// Created by ZZK on 2023/3/1.
//

#include <VulkanToy/Core/Input.h>
#include <VulkanToy/Core/Engine.h>
#include <GLFW/glfw3.h>

namespace VT
{
    bool Input::IsKeyPressed(const KeyCode key)
    {
        auto* window = static_cast<GLFWwindow*>(GEngine->getWindow().getNativeWindow());
        auto state = glfwGetKey(window, static_cast<int32_t>(key));
        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    bool Input::IsMouseButtonPressed(const MouseCode button)
    {
        auto* window = static_cast<GLFWwindow*>(GEngine->getWindow().getNativeWindow());
        auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
        return state == GLFW_PRESS;
    }

    glm::vec2 Input::GetMousePosition()
    {
        auto* window = static_cast<GLFWwindow*>(GEngine->getWindow().getNativeWindow());
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        return { (float)xpos, (float)ypos };
    }

    float Input::GetMouseX()
    {
        return GetMousePosition().x;
    }

    float Input::GetMouseY()
    {
        return GetMousePosition().y;
    }
}
