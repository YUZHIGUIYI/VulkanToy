//
// Created by ZZK on 2023/3/1.
//

#pragma once

#include <VulkanToy/Core/Layer.h>
#include <VulkanToy/VulkanRHI/VulkanDevice.h>
#include <VulkanToy/Events/Event.h>
#include <VulkanToy/Events/MouseEvent.h>
#include <VulkanToy/Events/ApplicationEvent.h>

namespace VT
{
    class ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer() override;

        void ImGuiCleanup();

        void OnAttach() override;
        void OnDetach() override;
        void OnEvent(Event& event) override;

        void Begin();
        void End();

        void ImGuiFrameRender(VkCommandBuffer commandBuffer,  VkPipeline pipeline);
        void ImGuiFrameResize();

        void BlockEvents(bool block) { m_BlockEvents = block; }

        void SetDarkThemeColors();

    private:
        bool m_BlockEvents = true;
    };
}
