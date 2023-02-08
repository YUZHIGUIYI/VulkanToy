//
// Created by ZZK on 2023/2/6.
//

#pragma once

#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

int main(int argc, char** argv);

class HelloTriangleApplication
{
public:
    HelloTriangleApplication() = default;
    HelloTriangleApplication(const std::string& name = "Vulkan");
    void run();

private:
    void initWindow();

    virtual void initVulkan();

    virtual void mainLoop();

    virtual void cleanup();

    friend int ::main(int argc, char** argv);

private:
    void createInstance();

    void setupDebugCallback();

    void createSurface();

    void pickPhysicalDevice();

    void createLogicalDevice();

    void createSwapChain();

    void createImageViews();

    void createRenderPass();

    void createGraphicsPipeline();

    void createFramebuffers();

    void createCommandPool();

    void createCommandBuffers();

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void createSyncObjects();

    void drawFrame();

private:
    VkInstance m_Instance;
#ifdef TOY_DEBUG
    VkDebugUtilsMessengerEXT m_DebugMessenger;
#endif
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface;
    VkDevice m_Device;
    VkQueue m_GraphicsQueue;
    VkQueue m_PresentQueue;

    VkSwapchainKHR m_SwapChain;
    std::vector<VkImage> m_SwapChainImages;
    VkFormat m_SwapChainImageFormat;
    VkExtent2D m_SwapChainExtent;

    std::vector<VkImageView> m_SwapChainImageViews;

    VkRenderPass m_RenderPass;

    VkPipelineLayout m_PipelineLayout;

    VkPipeline m_GraphicsPipeline;

    std::vector<VkFramebuffer> m_SwapChainFramebuffers;

    VkCommandPool m_CommandPool;
    std::vector<VkCommandBuffer> m_CommandBuffers;

    std::vector<VkSemaphore> m_ImageAvailableSemaphore;
    std::vector<VkSemaphore> m_RenderFinishedSemaphore;
    std::vector<VkFence> m_InFlightFences;
    size_t m_CurrentFrame = 0;

    GLFWwindow* m_Window;

    std::string m_Name{"Vulkan"};
};





























