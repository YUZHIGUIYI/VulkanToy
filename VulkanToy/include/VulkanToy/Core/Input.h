//
// Created by ZZK on 2023/3/1.
//

#pragma once

#include <glm/glm.hpp>
#include <VulkanToy/Core/KeyCodes.h>
#include <VulkanToy/Core/MouseCodes.h>

namespace VT
{
    class Input
    {
    public:
        static bool IsKeyPressed(KeyCode key);

        static bool IsMouseButtonPressed(MouseCode button);
        static glm::vec2 GetMousePosition();
        static float GetMouseX();
        static float GetMouseY();
    };
}
