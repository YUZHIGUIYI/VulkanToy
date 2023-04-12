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
    : Camera{}
    {

    }

    SceneCamera::SceneCamera(float fov, float aspectRatio, float nearClip, float farClip)
        : m_fov(fov), m_aspectRatio(aspectRatio), m_nearClip(nearClip), m_farClip(farClip),
            Camera(glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip))
    {
        m_projection[1][1] *= -1.0f;
        updateView();
    }

    void SceneCamera::init()
    {
        m_projection = glm::perspective(glm::radians(m_fov), m_aspectRatio, m_nearClip, m_farClip);
        m_projection[1][1] *= -1.0f;
        updateView();
    }

    void SceneCamera::updateProjection()
    {
        m_aspectRatio = m_viewportWidth / m_viewportHeight;
        m_projection = glm::perspective(glm::radians(m_fov), m_aspectRatio, m_nearClip, m_farClip);
        m_projection[1][1] *= -1.0f;    // In Vulkan implementation
    }

    void SceneCamera::updateView()
    {
        // m_Yaw = m_Pitch = 0.0f; // Lock the camera's rotation
        m_position = calculatePosition();

        glm::quat orientation = getOrientation();
        m_viewMatrix = glm::translate(glm::mat4(1.0f), m_position) * glm::toMat4(orientation);
        m_viewMatrix = glm::inverse(m_viewMatrix);
    }

    glm::vec2 SceneCamera::panSpeed() const
    {
        // Trick - magical numbers
        float x = std::min(m_viewportWidth / 1000.0f, 2.4f); // max = 2.4f
        float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

        float y = std::min(m_viewportHeight / 1000.0f, 2.4f); // max = 2.4f
        float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

        return glm::vec2{ xFactor, yFactor };
    }

    float SceneCamera::rotationSpeed() const
    {
        return 0.8f;
    }

    float SceneCamera::zoomSpeed() const
    {
        float distance = m_distance * 0.2f;
        distance = std::max(distance, 0.0f);
        float speed = distance * distance;
        speed = std::min(speed, 100.0f);    // maximum speed = 100.0f
        return speed;
    }

    void SceneCamera::tick(const RuntimeModuleTickData &tickData)
    {
        setViewportSize(static_cast<float>(tickData.windowWidth), static_cast<float>(tickData.windowHeight));

        if (Input::IsKeyPressed(Key::LeftAlt))
        {
            const glm::vec2& mouse{ Input::GetMouseX(), Input::GetMouseY() };
            glm::vec2 delta = (mouse - m_initialMousePosition) * 0.003f;
            m_initialMousePosition = mouse;

            if (Input::IsMouseButtonPressed(Mouse::ButtonMiddle))
                mousePan(delta);
            else if (Input::IsMouseButtonPressed(Mouse::ButtonLeft))
                mouseRotate(delta);
            else if (Input::IsMouseButtonPressed(Mouse::ButtonRight))
                mouseZoom(delta.y);
        }

        updateView();
    }

    void SceneCamera::onEvent(Event &event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<MouseScrolledEvent>(VT_BIND_EVENT_FN(SceneCamera::onMouseScroll));
    }

    bool SceneCamera::onMouseScroll(MouseScrolledEvent &event)
    {
        float delta = event.GetYOffset() * 0.1f;
        mouseZoom(delta);
        updateView();
        return false;
    }

    void SceneCamera::mousePan(const glm::vec2 &delta)
    {
        glm::vec2 speed = panSpeed();
        m_focalPoint += -getRightDirection() * delta.x * speed.x * m_distance;
        m_focalPoint += getUpDirection() * delta.y * speed.y * m_distance;
    }

    void SceneCamera::mouseRotate(const glm::vec2 &delta)
    {
        float yawSign = getUpDirection().y < 0 ? -1.0f : 1.0f;
        m_yaw += yawSign * delta.x * rotationSpeed();
        m_pitch += delta.y * rotationSpeed();
    }

    void SceneCamera::mouseZoom(float delta)
    {
        m_distance -= delta * zoomSpeed();
        if (m_distance < 1.0f)
        {
            m_focalPoint += getForwardDirection();
            m_distance = 1.0f;
        }
    }

    glm::vec3 SceneCamera::getUpDirection() const
    {
        return glm::rotate(getOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::vec3 SceneCamera::getRightDirection() const
    {
        return glm::rotate(getOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
    }

    glm::vec3 SceneCamera::getForwardDirection() const
    {
        return glm::rotate(getOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
    }

    glm::vec3 SceneCamera::calculatePosition() const
    {
        return m_focalPoint - getForwardDirection() * m_distance;
    }

    glm::quat SceneCamera::getOrientation() const
    {
        return glm::quat{ glm::vec3(-m_pitch, -m_yaw, 0.0f) };
    }
}





































