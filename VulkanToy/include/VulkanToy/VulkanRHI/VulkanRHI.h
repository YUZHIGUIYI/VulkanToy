//
// Created by ZHIKANG on 2023/3/27.
//

#pragma once

#include <VulkanToy/Core/Base.h>
#include <VulkanToy/VulkanRHI/VulkanDevice.h>
#include <VulkanToy/VulkanRHI/VulkanSwapChain.h>
#include <VulkanToy/VulkanRHI/VulkanDescriptor.h>

namespace VT
{

    class VulkanContext final : public DisableCopy
    {
    public:
        bool isPhysicalDeviceSuitable(std::vector<char const *> const &requestExtensions);
        void pickupSuitableGPU(std::vector<char const *> const &requestExtensions);
        // SwapchainSupportDetails querySwapchainSupportDetail();

        VkFormat findSupportedFormat(std::vector<VkFormat> const &candidates, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);
        int32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memoryPropertyFlags);

    private:
        GLFWwindow* m_window;

        VkInstance m_instance = VK_NULL_HANDLE;
        VulkanDevice m_device{};
        VulkanSwapChain m_swapChain{};
        VkSurfaceKHR  m_surface = VK_NULL_HANDLE;

        VmaAllocator m_vmaAllocator{};
        DescriptorAllocator m_descriptorAllocator{};
        DescriptorLayoutCache m_descriptorLayoutCache{};

        VkFormat m_cacheSupportDepthStencilFormat;
        VkFormat m_cacheSupportDepthOnlyFormat;

        struct CommandPool
        {
            VkQueue queue = VK_NULL_HANDLE;
            VkCommandPool pool = VK_NULL_HANDLE;
        };

        /** @brief Major graphics queue with priority 1.0f */
        CommandPool m_majorGraphicsPool;
        /** @brief Major compute queue with priority 0.8f. Use for AsyncScheduler */
        CommandPool m_majorComputePool;
        /** @brief Second major queue with priority 0.8f. Use fir Async Scheduler */
        CommandPool m_secondMajorGraphicsPool;
        /** @brief Other command pool with priority 0.5f. */
        std::vector<CommandPool> m_graphicsPools;
        std::vector<CommandPool> m_computePools;
        /** @brief Copy pool used for async uploader. */
        std::vector<CommandPool> m_copyPools;

        struct PresentContext
        {
            bool swapChainChange = false;
            uint32_t imageIndex;
            uint32_t currentFrame;
            std::vector<VkSemaphore> semaphoresImageAvailable;
            std::vector<VkSemaphore> semaphoresRenderFinished;
            std::vector<VkFence> inFlightFences;
            std::vector<VkFence> imagesInFlight;

            void init();
            void release();
        } m_presentContext;

    private:
        int currentWidth;
        int currentHeight;
        int lastWidth  = ~0;
        int lastHeight = ~0;

        bool isSwapChainRebuilt();

    private:
        void initInstance(std::vector<char const *> const &requiredExtensions, std::vector<char const *> const &requiredLayers);
        void releaseInstance();

        void initDevice(VkPhysicalDeviceFeatures features, std::vector<char const *> const &requestExtens, void *nextChain = nullptr);
        void releaseDevice();

        void initVMA();
        void releaseVMA();

        void initCommandPool();
        void releaseCommandPool();

    public:
        GLFWwindow* getWindow() { return m_window; };
        [[nodiscard]] VkSurfaceKHR getSurface() const { return m_swapChain; }

        [[nodiscard]] VkFormat getSupportDepthStencilFormat() const { return m_cacheSupportDepthStencilFormat; }
        [[nodiscard]] VkFormat getSupportDepthOnlyFormat() const { return m_cacheSupportDepthOnlyFormat; }

        [[nodiscard]] uint32_t getMaxMemoryAllocationCount() const { return m_device.properties.limits.maxMemoryAllocationCount; }

    public:
        void init(GLFWwindow* window);
        void release();
        void rebuildSwapChain();

    public:
        uint32_t acquireNextPresentImage();
        void present();
        void submit(uint32_t count, VkSubmitInfo *infos);
        void resetFence();

    public:
        // Major graphics queue used for present and ui render. priority 1.0.
        [[nodiscard]] VkQueue getMajorGraphicsQueue() const { return m_majorGraphicsPool.queue; }
        [[nodiscard]] VkCommandPool getMajorGraphicsCommandPool() const { return m_majorGraphicsPool.pool; }
        [[nodiscard]] VkCommandBuffer createMajorGraphicsCommandBuffer();

        // Major compute queue. priority 0.8.
        [[nodiscard]] VkQueue getMajorComputeQueue() const { return m_majorComputePool.queue; }
        [[nodiscard]] VkCommandPool getMajorComputeCommandPool() const { return m_majorComputePool.pool; }

        [[nodiscard]] VkQueue getSecondMajorGraphicsQueue() const { return m_secondMajorGraphicsPool.queue; }
        [[nodiscard]] VkCommandPool getSecondMajorGraphicsCommandPool() const { return m_secondMajorGraphicsPool.pool; }

        // Other queues, priority 0.5.
        [[nodiscard]] const auto& getAsyncCopyCommandPools() const { return m_copyPools; }
        [[nodiscard]] const auto& getAsyncComputeCommandPools() const { return m_computePools; }
        [[nodiscard]] const auto& getAsyncGraphicsCommandPools() const { return m_graphicsPools; }

        [[nodiscard]] VkInstance getInstance() const { return m_instance; }

        [[nodiscard]] const VulkanDevice::QueuesInfo& getGPUQueuesInfo() const { return m_device.m_queueInfos; }
        [[nodiscard]] uint32_t getGraphicsFamily() const { return m_device.m_queueInfos.graphicsFamily; }
        [[nodiscard]] uint32_t getComputeFamily() const { return m_device.m_queueInfos.computeFamily; }
        [[nodiscard]] uint32_t getCopyFamily() const { return m_device.m_queueInfos.copyFamily; }

        // Add descriptor


        // Other
        VkPipelineLayout createPipelineLayout(const VkPipelineLayoutCreateInfo& info);

        [[nodiscard]] const uint32_t getCurrentFrameIndex() const { return m_presentContext.currentFrame; }

        VulkanSwapChain& getSwapChain() { return m_swapChain; }
        std::vector<SwapChainBuffer>& getSwapChainBuffers() { return m_swapChain.buffers; }

        [[nodiscard]] VkFormat getSwapChainFormat() const { return m_swapChain.colorFormat; }
        [[nodiscard]] VkExtent2D getSwapChainExtent() const { return m_swapChain.extent2D; }

        [[nodiscard]] VkPhysicalDeviceProperties getPhysicalDeviceProperties() const { return m_device.properties; }

        [[nodiscard]] VkSemaphore getCurrentFrameWaitSemaphore() const { return m_presentContext.semaphoresImageAvailable[m_presentContext.currentFrame]; }
        [[nodiscard]] VkSemaphore getCurrentFrameFinishSemaphore() const { return m_presentContext.semaphoresRenderFinished[m_presentContext.currentFrame]; }

    };

    namespace VulkanRHI
    {
        // Initialize Vulkan context and get this pointer
        inline constexpr auto get = [](){ return Singleton<VulkanContext>::Get(); };
    }
}
























