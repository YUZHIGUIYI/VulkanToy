//
// Created by ZZK on 2023/3/3.
//

#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace VT
{
    class Camera
    {
    public:
        Camera() = default;
        explicit Camera(const glm::mat4& projection) : m_projection(projection) {}

        virtual ~Camera() = default;

        [[nodiscard]] const glm::mat4& getProjection() const { return m_projection; }

    protected:
        glm::mat4 m_projection = glm::mat4(1.0f);
    };
}





























