//
// Created by ZZK on 2023/3/3.
//

#pragma once

#include <VulkanToy/Core/RuntimeModule.h>
#include <VulkanToy/Renderer/Camera.h>
#include <VulkanToy/Events/Event.h>
#include <VulkanToy/Events/MouseEvent.h>

namespace VT
{
    class SceneCamera : public Camera
    {
    public:
        SceneCamera();
        SceneCamera(float fov, float aspectRatio, float nearClip, float farClip);

        void init();

        void tick(const RuntimeModuleTickData &tickData);
        void onEvent(Event& event);

        [[nodiscard]] float getDistance() const { return m_distance; }
        void setDistance(float distance) { m_distance = distance; }

        void setViewportSize(float width, float height) { m_viewportWidth = width; m_viewportHeight = height; updateProjection(); }

        [[nodiscard]] const glm::mat4& getViewMatrix() const { return m_viewMatrix; }
        [[nodiscard]] glm::mat4 getViewProjection() const { return m_projection * m_viewMatrix; }

        [[nodiscard]] glm::vec3 getUpDirection() const;
        [[nodiscard]] glm::vec3 getRightDirection() const;
        [[nodiscard]] glm::vec3 getForwardDirection() const;
        [[nodiscard]] const glm::vec3& getPosition() const { return m_position; }
        [[nodiscard]] glm::quat getOrientation() const;

        [[nodiscard]] float getPitch() const { return m_pitch; }
        [[nodiscard]] float getYaw() const { return m_yaw; }

    private:
        void updateProjection();
        void updateView();

        bool onMouseScroll(MouseScrolledEvent& event);

        void mousePan(const glm::vec2& delta);
        void mouseRotate(const glm::vec2& delta);
        void mouseZoom(float delta);

        [[nodiscard]] glm::vec3 calculatePosition() const;

        [[nodiscard]] glm::vec2 panSpeed() const;
        [[nodiscard]] float rotationSpeed() const;
        [[nodiscard]] float zoomSpeed() const;

    private:
        float m_fov = 45.0f, m_aspectRatio = 1.778f, m_nearClip = 0.1f, m_farClip = 1000.0f;
        float m_distance = 10.0f;
        float m_pitch = 0.0f, m_yaw = 0.0f;

        float m_viewportWidth = 1600.0f, m_viewportHeight = 900.0f;

        glm::mat4 m_viewMatrix;

        glm::vec3 m_position{ 0.0f, 0.0f, 0.0f };
        glm::vec3 m_focalPoint{ 0.0f, 0.0f, 0.0f };
        glm::vec2 m_initialMousePosition{ 0.0f, 0.0f };
    };

    using SceneCameraHandle = Singleton<SceneCamera>;
}



























