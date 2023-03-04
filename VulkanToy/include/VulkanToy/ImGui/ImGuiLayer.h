//
// Created by ZZK on 2023/3/1.
//

#pragma once

#include <VulkanToy/Core/Layer.h>
#include <VulkanToy/Core/VulkanDevice.h>
#include <VulkanToy/Events/Event.h>
#include <VulkanToy/Events/MouseEvent.h>
#include <VulkanToy/Events/ApplicationEvent.h>

namespace VT
{
    class Application;

    // Options and values to display/toggle from the UI
    struct UISettings
    {
        bool displayModels = true;
        bool displayLogos = true;
        bool displayBackground = true;
        bool animateLight = false;
        float lightSpeed = 0.25f;
        std::array<float, 50> frameTimes{};
        float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;
        float lightTimer = 0.0f;
    };

    class ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer() override;

        void OnAttach() override;
        void OnDetach() override;
        void OnEvent(Event& event) override;

        void Begin();
        void End();

        void ImGuiFrameRender(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void ImGuiFramePresent(VkSwapchainKHR swapChain);

        void BlockEvents(bool block) { m_BlockEvents = block; }

        void SetDarkThemeColors();

    private:
        bool m_BlockEvents = true;

    private:
        // Vulkan resources for rendering the UI
        VkSampler m_Sampler;
        Buffer m_VertexBuffer;
        Buffer m_IndexBuffer;
        int32_t m_VertexCount = 0;
        int32_t m_IndexCount = 0;
        VkDeviceMemory m_FontMemory = VK_NULL_HANDLE;
        VkImage m_FontImage = VK_NULL_HANDLE;
        VkImageView m_FontView = VK_NULL_HANDLE;
        VkPipelineCache m_PipelineCache;
        VkPipelineLayout m_PipelineLayout;
        VkPipeline m_Pipeline;
        VkDescriptorPool m_DescriptorPool;
        VkDescriptorSetLayout m_DescriptorSetLayout;
        VkDescriptorSet m_DescriptorSet;
        VulkanDevice *m_Device;
        VkPhysicalDeviceDriverProperties m_DriverProperties{};
        Application *m_AppHandle;
    };
}
