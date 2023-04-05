//
// Created by ZZK on 2023/3/3.
//

#include <VulkanToy/Renderer/SceneCamera.h>
#include <VulkanToy/Core/KeyCodes.h>
#include <VulkanToy/Core/MouseCodes.h>
#include <VulkanToy/Core/Input.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace VT
{
    SceneCamera::SceneCamera()
    : m_FOV(45.0f), m_AspectRatio(1.778f), m_NearClip(0.1f), m_FarClip(1000.0f)
    {
        Camera(glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip));
        m_Projection[1][1] *= -1.0f;
        UpdateView();
    }

    SceneCamera::SceneCamera(float fov, float aspectRatio, float nearClip, float farClip)
        : m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip),
            Camera(glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip))
    {
        m_Projection[1][1] *= -1.0f;
        UpdateView();
    }

    void SceneCamera::UpdateProjection()
    {
        m_AspectRatio = m_ViewportWidth / m_ViewportHeight;
        m_Projection = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
        m_Projection[1][1] *= -1.0f;    // In Vulkan implementation
    }

    void SceneCamera::UpdateView()
    {
        // m_Yaw = m_Pitch = 0.0f; // Lock the camera's rotation
        m_Position = CalculatePosition();

        glm::quat orientation = GetOrientation();
        m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
        m_ViewMatrix = glm::inverse(m_ViewMatrix);
    }

    glm::vec2 SceneCamera::PanSpeed() const
    {
        // Trick - magical numbers
        float x = std::min(m_ViewportWidth / 1000.0f, 2.4f); // max = 2.4f
        float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

        float y = std::min(m_ViewportHeight / 1000.0f, 2.4f); // max = 2.4f
        float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

        return glm::vec2{ xFactor, yFactor };
    }

    float SceneCamera::RotationSpeed() const
    {
        return 0.8f;
    }

    float SceneCamera::ZoomSpeed() const
    {
        float distance = m_Distance * 0.2f;
        distance = std::max(distance, 0.0f);
        float speed = distance * distance;
        speed = std::min(speed, 100.0f);    // maximum speed = 100.0f
        return speed;
    }

    void SceneCamera::OnUpdate(Timestep ts)
    {
        if (Input::IsKeyPressed(Key::LeftAlt))
        {
            const glm::vec2& mouse{ Input::GetMouseX(), Input::GetMouseY() };
            glm::vec2 delta = (mouse - m_InitialMousePosition) * 0.003f;
            m_InitialMousePosition = mouse;

            if (Input::IsMouseButtonPressed(Mouse::ButtonMiddle))
                MousePan(delta);
            else if (Input::IsMouseButtonPressed(Mouse::ButtonLeft))
                MouseRotate(delta);
            else if (Input::IsMouseButtonPressed(Mouse::ButtonRight))
                MouseZoom(delta.y);
        }

        UpdateView();
    }

    void SceneCamera::OnEVent(Event &event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<MouseScrolledEvent>(VT_BIND_EVENT_FN(SceneCamera::OnMouseScroll));
    }

    bool SceneCamera::OnMouseScroll(MouseScrolledEvent &event)
    {
        float delta = event.GetYOffset() * 0.1f;
        MouseZoom(delta);
        UpdateView();
        return false;
    }

    void SceneCamera::MousePan(const glm::vec2 &delta)
    {
        glm::vec2 speed = PanSpeed();
        m_FocalPoint += -GetRightDirection() * delta.x * speed.x * m_Distance;
        m_FocalPoint += GetUpDirection() * delta.y * speed.y * m_Distance;
    }

    void SceneCamera::MouseRotate(const glm::vec2 &delta)
    {
        float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
        m_Yaw += yawSign * delta.x * RotationSpeed();
        m_Pitch += delta.y * RotationSpeed();
    }

    void SceneCamera::MouseZoom(float delta)
    {
        m_Distance -= delta * ZoomSpeed();
        if (m_Distance < 1.0f)
        {
            m_FocalPoint += GetForwardDirection();
            m_Distance = 1.0f;
        }
    }

    glm::vec3 SceneCamera::GetUpDirection() const
    {
        return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::vec3 SceneCamera::GetRightDirection() const
    {
        return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
    }

    glm::vec3 SceneCamera::GetForwardDirection() const
    {
        return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
    }

    glm::vec3 SceneCamera::CalculatePosition() const
    {
        return m_FocalPoint - GetForwardDirection() * m_Distance;
    }

    glm::quat SceneCamera::GetOrientation() const
    {
        return glm::quat{ glm::vec3(-m_Pitch, -m_Yaw, 0.0f) };
    }
}





































