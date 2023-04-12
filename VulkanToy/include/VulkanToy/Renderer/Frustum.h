//
// Created by ZZK on 2023/3/3.
//

#pragma once

#include <VulkanToy/Core/Timestep.h>

#include <array>
#include <cmath>

#include <glm/glm.hpp>

namespace VT
{
    class Frustum
    {
    public:
        enum Side { LEFT = 0, RIGHT = 1, TOP = 2, BOTTOM = 3, BACK = 4, FRONT = 5 };
        std::array<glm::vec4, 6> m_Plane;

    public:
        Frustum() = default;

        void OnUpdate(Timestep ts) {}

        void Update(const glm::mat4& matrix)
        {
            m_Plane[LEFT].x = matrix[0].w + matrix[0].x;
            m_Plane[LEFT].y = matrix[1].w + matrix[1].x;
            m_Plane[LEFT].z = matrix[2].w + matrix[2].x;
            m_Plane[LEFT].w = matrix[3].w + matrix[3].x;

            m_Plane[RIGHT].x = matrix[0].w - matrix[0].x;
            m_Plane[RIGHT].y = matrix[1].w - matrix[1].x;
            m_Plane[RIGHT].z = matrix[2].w - matrix[2].x;
            m_Plane[RIGHT].w = matrix[3].w - matrix[3].x;

            m_Plane[TOP].x = matrix[0].w - matrix[0].y;
            m_Plane[TOP].y = matrix[1].w - matrix[1].y;
            m_Plane[TOP].z = matrix[2].w - matrix[2].y;
            m_Plane[TOP].w = matrix[3].w - matrix[3].y;

            m_Plane[BOTTOM].x = matrix[0].w + matrix[0].y;
            m_Plane[BOTTOM].y = matrix[1].w + matrix[1].y;
            m_Plane[BOTTOM].z = matrix[2].w + matrix[2].y;
            m_Plane[BOTTOM].w = matrix[3].w + matrix[3].y;

            m_Plane[BACK].x = matrix[0].w + matrix[0].z;
            m_Plane[BACK].y = matrix[1].w + matrix[1].z;
            m_Plane[BACK].z = matrix[2].w + matrix[2].z;
            m_Plane[BACK].w = matrix[3].w + matrix[3].z;

            m_Plane[FRONT].x = matrix[0].w - matrix[0].z;
            m_Plane[FRONT].y = matrix[1].w - matrix[1].z;
            m_Plane[FRONT].z = matrix[2].w - matrix[2].z;
            m_Plane[FRONT].w = matrix[3].w - matrix[3].z;

            for (auto i = 0; i < m_Plane.size(); ++i)
            {
                float length = std::sqrt(m_Plane[i].x * m_Plane[i].x + m_Plane[i].y * m_Plane[i].y + m_Plane[i].z * m_Plane[i].z);
                m_Plane[i] /= length;
            }
        }

        bool CheckSphere(const glm::vec3& pos, float radius)
        {
            for (auto i = 0; i < m_Plane.size(); i++)
            {
                if ((m_Plane[i].x * pos.x) + (m_Plane[i].y * pos.y) + (m_Plane[i].z * pos.z) + m_Plane[i].w <= -radius)
                {
                    return false;
                }
            }
            return true;
        }
    };
}


















































