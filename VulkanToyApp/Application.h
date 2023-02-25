//
// Created by ZZK on 2023/2/6.
//

#pragma once

#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "RendererData.h"

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

    void createComputeDescriptorSetLayout();

    void createGraphicsPipeline();

    void createComputePipeline();

    void createFramebuffers();

    void createCommandPool();

    void createShaderStorageBuffers();

    void createUniformBuffers();

    void createDescriptorPool();

    void createComputeDescriptorSets();

    void createCommandBuffers();

    void createComputeCommandBuffers();

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void recordComputeCommandBuffer(VkCommandBuffer commandBuffer);

    void createSyncObjects();

    void updateUniformBuffer(uint32_t currentImage);

    void drawFrame();

    void recreateSwapChain();

    void cleanupSwapChain();

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    VkCommandBuffer beginSingleTimeCommands();

    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT, uint32_t mipLevels = 1);

private:
    VkInstance m_Instance;
#ifdef TOY_DEBUG
    VkDebugUtilsMessengerEXT m_DebugMessenger;
#endif
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface;
    VkDevice m_Device;
    VkQueue m_GraphicsQueue;
    VkQueue m_ComputeQueue;
    VkQueue m_PresentQueue;

    VkSwapchainKHR m_SwapChain;
    std::vector<VkImage> m_SwapChainImages;
    VkFormat m_SwapChainImageFormat;
    VkExtent2D m_SwapChainExtent;
    std::vector<VkImageView> m_SwapChainImageViews;

    VkRenderPass m_RenderPass;

    VkPipelineLayout m_PipelineLayout;
    VkPipeline m_GraphicsPipeline;

    VkDescriptorSetLayout m_ComputeDescriptorSetLayout;
    VkPipelineLayout m_ComputePipelineLayout;
    VkPipeline m_ComputePipeline;

    std::vector<VkFramebuffer> m_SwapChainFramebuffers;

    VkCommandPool m_CommandPool;
    std::vector<VkCommandBuffer> m_CommandBuffers;
    std::vector<VkCommandBuffer> m_ComputeCommandBuffers;

    std::vector<VkBuffer> m_UniformBuffers;
    std::vector<VkDeviceMemory> m_UniformBuffersMemory;
    std::vector<void*> m_UniformBuffersMapped;

    std::vector<VkBuffer> m_ShaderStorageBuffers;
    std::vector<VkDeviceMemory> m_ShaderStorageBuffersMemory;

    VkDescriptorPool m_DescriptorPool;
    std::vector<VkDescriptorSet> m_ComputeDescriptorSets;

    std::vector<VkSemaphore> m_ImageAvailableSemaphore;
    std::vector<VkSemaphore> m_RenderFinishedSemaphore;
    std::vector<VkSemaphore> m_ComputeFinishedSemaphores;
    std::vector<VkFence> m_InFlightFences;
    std::vector<VkFence> m_ComputeInFlightFences;
    uint32_t m_CurrentFrame = 0;

    float m_LastFrameTime = 0.0f;
    double m_LastTime = 0.0;

    GLFWwindow* m_Window;

    std::string m_Name{"Vulkan"};

public:
    bool m_FramebufferResized = false;
};





























