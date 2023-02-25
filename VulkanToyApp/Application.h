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

    void createDescriptorSetLayout();

    void createGraphicsPipeline();

    void createFramebuffers();

    void createCommandPool();

    void createDepthResources();

    void createTextureImage();

    void createTextureImageView();

    void createTextureSampler();

    void loadModel();

    void createVertexBuffer();

    void createIndexBuffer();

    void createUniformBuffers();

    void createDescriptorPool();

    void createDescriptorSets();

    void createCommandBuffers();

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void createSyncObjects();

    void drawFrame();

    void recreateSwapChain();

    void cleanupSwapChain();

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void updateUniformBuffer(uint32_t currentImage);

    void createImage(uint32_t width, uint32_t height, uint32_t  mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                        VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1);

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    VkCommandBuffer beginSingleTimeCommands();

    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT, uint32_t mipLevels = 1);

    VkFormat findDepthFormat();

    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

    void createColorResources();

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

    VkSampleCountFlagBits m_MsaaSamples{VK_SAMPLE_COUNT_1_BIT};

    VkRenderPass m_RenderPass;

    VkDescriptorSetLayout m_DescriptorSetLayout;

    VkPipelineLayout m_PipelineLayout;

    VkPipeline m_GraphicsPipeline;

    std::vector<VkFramebuffer> m_SwapChainFramebuffers;

    VkCommandPool m_CommandPool;
    std::vector<VkCommandBuffer> m_CommandBuffers;

    // For Msaa
    VkImage m_ColorImage;
    VkDeviceMemory m_ColorImageMemory;
    VkImageView m_ColorImageView;

    // For Depth
    VkImage m_DepthImage;
    VkDeviceMemory m_DepthImageMemory;
    VkImageView m_DepthImageView;

    uint32_t  m_MipLevels;
    VkImage m_TextureImage;
    VkDeviceMemory m_TextureImageMemory;

    VkImageView m_TextureImageView;
    VkSampler m_TextureSampler;

    VkBuffer m_VertexBuffer;
    VkDeviceMemory m_VertexBufferMemory;
    VkBuffer m_IndexBuffer;
    VkDeviceMemory m_IndexBufferMemory;

    std::vector<VkBuffer> m_UniformBuffers;
    std::vector<VkDeviceMemory> m_UniformBuffersMemory;
    std::vector<void*> m_UniformBuffersMapped;

    VkDescriptorPool m_DescriptorPool;
    std::vector<VkDescriptorSet> m_DescriptorSets;

    std::vector<VkSemaphore> m_ImageAvailableSemaphore;
    std::vector<VkSemaphore> m_RenderFinishedSemaphore;
    std::vector<VkFence> m_InFlightFences;
    uint32_t m_CurrentFrame = 0;

    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;

    GLFWwindow* m_Window;

    std::string m_Name{"Vulkan"};

public:
    bool m_FramebufferResized = false;
};





























