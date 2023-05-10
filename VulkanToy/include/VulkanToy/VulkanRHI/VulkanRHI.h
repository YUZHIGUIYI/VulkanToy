//
// Created by ZHIKANG on 2023/3/27.
//

#pragma once

#include <VulkanToy/VulkanRHI/VulkanRHICommon.h>
#include <VulkanToy/VulkanRHI/VulkanDevice.h>
#include <VulkanToy/VulkanRHI/VulkanSwapChain.h>
#include <VulkanToy/VulkanRHI/VulkanDescriptor.h>
#include <VulkanToy/VulkanRHI/Shader.h>
#include <VulkanToy/VulkanRHI/VulkanDescriptor.h>
#include <VulkanToy/VulkanRHI/Sampler.h>
#include <VulkanToy/Events/EventDelegate.h>

namespace VT
{

    class VulkanContext final : public DisableCopy
    {
    public:
        VkFormat findSupportedFormat(std::vector<VkFormat> const &candidates, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);
        int32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memoryPropertyFlags);
        SwapChainSupportDetails querySwapChainSupportDetail();

    private:
        GLFWwindow* m_window = nullptr;

        VkInstance m_instance = VK_NULL_HANDLE;
        VkDebugReportCallbackEXT m_debugReportHandle = VK_NULL_HANDLE;
        VulkanDevice m_device{};
        VulkanSwapChain m_swapChain{};
        VkSurfaceKHR  m_surface = VK_NULL_HANDLE;

        VmaAllocator m_vmaAllocator{};
        DescriptorPoolCache m_descriptorPoolCache{};

        struct PresentContext
        {
            bool isSwapChainChange = false;
            uint32_t imageIndex;
            uint32_t currentFrame;
            std::vector<VkSemaphore> semaphoresImageAvailable;
            std::vector<VkSemaphore> semaphoresRenderFinished;
            std::vector<VkFence> inFlightFences;
            std::vector<VkFence> imagesInFlight;

            void init();
            void release();
        } m_presentContext;

        ShaderCache m_shaderCache;

        SamplerCache m_samplerCache;

        std::vector<VkCommandBuffer> m_drawCmdBuffers;

    private:
        int currentWidth;
        int currentHeight;
        int lastWidth  = ~0;
        int lastHeight = ~0;

    private:
        bool isSwapChainRebuilt();

        void initInstance(std::vector<char const *> const &requiredExtensions, std::vector<char const *> const &requiredLayers);
        void releaseInstance();

        void initDevice(VkPhysicalDeviceFeatures features, std::vector<char const *> const &requestedExtensions, void *nextChain = nullptr);
        void releaseDevice();

        void initVMA();
        void releaseVMA();

        void createDrawCommandBuffers();

    public:
        GLFWwindow* getWindow() { return m_window; };
        [[nodiscard]] VkSurfaceKHR getSurface() const { return m_surface; }

        [[nodiscard]] VkFormat getSupportDepthStencilFormat() const { return m_device.cacheSupportDepthStencilFormat; }
        [[nodiscard]] VkFormat getSupportDepthOnlyFormat() const { return m_device.cacheSupportDepthOnlyFormat; }

        [[nodiscard]] uint32_t getMaxMemoryAllocationCount() const { return m_device.properties.limits.maxMemoryAllocationCount; }

    public:
        void init(GLFWwindow* window);
        void release();
        void rebuildSwapChain();

    public:
        uint32_t acquireNextPresentImage();
        void present();
        void submit(uint32_t count, VkSubmitInfo *infos);
        void submitWithoutFence(uint32_t count, VkSubmitInfo* infos);
        void resetFence();

        Events::EventDelegate<> onAfterSwapChainRebuild;

    public:
        // Major graphics queue used for present and ui render. priority 1.0.
        VkCommandBuffer createMajorGraphicsCommandBuffer();
        [[nodiscard]] VkQueue getMajorGraphicsQueue() const { return m_device.majorGraphicsPool.queue; }
        [[nodiscard]] VkCommandPool getMajorGraphicsCommandPool() const { return m_device.majorGraphicsPool.pool; }

        // Major compute queue. priority 0.8.
        [[nodiscard]] VkQueue getMajorComputeQueue() const { return m_device.majorComputePool.queue; }
        [[nodiscard]] VkCommandPool getMajorComputeCommandPool() const { return m_device.majorComputePool.pool; }

        [[nodiscard]] VkQueue getSecondMajorGraphicsQueue() const { return m_device.secondMajorGraphicsPool.queue; }
        [[nodiscard]] VkCommandPool getSecondMajorGraphicsCommandPool() const { return m_device.secondMajorGraphicsPool.pool; }

        // Other queues, priority 0.5.
        [[nodiscard]] const auto& getAsyncCopyCommandPools() const { return m_device.copyPools; }
        [[nodiscard]] const auto& getAsyncComputeCommandPools() const { return m_device.computePools; }
        [[nodiscard]] const auto& getAsyncGraphicsCommandPools() const { return m_device.graphicsPools; }

        [[nodiscard]] VkInstance getInstance() const { return m_instance; }

        [[nodiscard]] const VulkanDevice::QueuesInfo& getGPUQueuesInfo() const { return m_device.queueInfos; }
        [[nodiscard]] uint32_t getGraphicsFamily() const { return m_device.queueInfos.graphicsFamily; }
        [[nodiscard]] uint32_t getComputeFamily() const { return m_device.queueInfos.computeFamily; }
        [[nodiscard]] uint32_t getCopyFamily() const { return m_device.queueInfos.copyFamily; }

        // Descriptor
        DescriptorPoolCache& getDescriptorPoolCache() { return m_descriptorPoolCache; }

        DescriptorFactory descriptorFactoryBegin();

        void updateDescriptorSet(VkDescriptorSet &dstDescriptorSet, uint32_t dstBinding, VkDescriptorType descriptorType, const std::vector<VkDescriptorImageInfo> &descriptors) const;

        void updateDescriptorSet(VkDescriptorSet &dstDescriptorSet, uint32_t dstBinding, VkDescriptorType descriptorType, const std::vector<VkDescriptorBufferInfo> &descriptors) const;

        // Other
        VkPipelineLayout createPipelineLayout(const VkPipelineLayoutCreateInfo& info);

        VkCommandBuffer& getDrawCommandBuffer(uint32_t index) { return m_drawCmdBuffers.at(index); }

        // TODO: fix confusing function
        [[nodiscard]] uint32_t getCurrentFrameIndex() const { return m_presentContext.imageIndex; }

        VulkanSwapChain& getSwapChain() { return m_swapChain; }
        std::vector<VkImageView>& getSwapChainImageViews() { return m_swapChain.swapChainImageViews; }
        std::vector<VkImage>& getSwapChainImages() { return m_swapChain.swapChainImages; }

        [[nodiscard]] VkFormat getSwapChainFormat() const { return m_swapChain.colorFormat; }
        [[nodiscard]] VkExtent2D getSwapChainExtent() const { return m_swapChain.swapChainExtent; }

        [[nodiscard]] VkPhysicalDeviceProperties getPhysicalDeviceProperties() const { return m_device.properties; }

        [[nodiscard]] VkSemaphore getCurrentFrameWaitSemaphore() const { return m_presentContext.semaphoresImageAvailable[m_presentContext.currentFrame]; }
        [[nodiscard]] VkSemaphore getCurrentFrameFinishSemaphore() const { return m_presentContext.semaphoresRenderFinished[m_presentContext.currentFrame]; }
    };

    namespace VulkanRHI
    {
        extern size_t MaxSwapChainCount;

        extern VkPhysicalDevice GPU;
        extern VkDevice Device;
        extern VmaAllocator VMA;
        extern ShaderCache* ShaderManager;
        extern SamplerCache* SamplerManager;

        enum class DisplayMode
        {
            DISPLAYMODE_SDR,
            DISPLAYMODE_HDR
        };
        extern DisplayMode eDisplayMode;
        extern bool isSupportHDR;
        extern VkHdrMetadataEXT HdrMetadataEXT;
        extern std::atomic<size_t> GPUResourceMemoryUsed;

        // Initialize Vulkan context and get this pointer
        inline constexpr auto get = [](){ return Singleton<VulkanContext>::Get(); };

        extern void setResourceName(VkObjectType objectType, uint64_t handle, char const *name);

        extern void setPerfMarkerBegin(VkCommandBuffer cmdBuf, char const *name, glm::vec4 const &color);
        extern void setPerfMarkerEnd(VkCommandBuffer cmdBuf);

        // Record command buffer
        extern void executeImmediately(VkCommandPool commandPool, VkQueue queue, std::function<void(VkCommandBuffer commandBuffer)>&& func);
        extern void executeImmediatelyMajorGraphics(std::function<void(VkCommandBuffer commandBuffer)>&& func);

        // Use to compute RHI resource used
        extern void addGPUResourceMemoryUsed(size_t in);
        extern void minusGPUResourceMemoryUsed(size_t in);
        extern size_t getGPUResourceMemoryUsed();

        // Push descriptor set functions
        extern PFN_vkCmdPushDescriptorSetKHR PushDescriptorSetKHR;
        extern PFN_vkCmdPushDescriptorSetWithTemplateKHR PushDescriptorSetWithTemplateKHR;

        // HDR functions
        extern PFN_vkSetHdrMetadataEXT SetHdrMetadataEXT;
    }
}
























