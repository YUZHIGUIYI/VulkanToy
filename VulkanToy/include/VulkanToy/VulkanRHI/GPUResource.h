//
// Created by ZHIKANG on 2023/3/31.
//

#pragma once

#include <VulkanToy/Core/Base.h>
#include <VulkanToy/VulkanRHI/VulkanRHICommon.h>

namespace VT
{
    // Buffer
    struct VulkanBuffer
    {
    private:
        std::string m_name{};
        VkBuffer m_buffer = VK_NULL_HANDLE;
        VmaAllocation m_allocation = nullptr;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;
        VkDeviceSize m_size = 0;
        void *m_mapped = nullptr;

        uint64_t m_deviceAddress = 0;

        VkDescriptorBufferInfo m_bufferInfo{};

        bool m_isHeap = false;

    public:
        [[nodiscard]] const VkBuffer& getBuffer() const { return m_buffer; }
        [[nodiscard]] VkDeviceMemory getMemory() const { return m_memory; }
        [[nodiscard]] VkDeviceSize getMemorySize() const { return m_size; }
        [[nodiscard]] bool isHeap() const { return m_isHeap; }
        [[nodiscard]] const char* getName() const { return m_name.c_str();}

        void setName(const std::string &newName);

        bool innerCreate(VkBufferUsageFlags usageFlags,
            VkMemoryPropertyFlags memoryPropertyFlags,
            VmaAllocationCreateFlags vmaUsage,
            void *data);

        void release();

        uint64_t getDeviceAddress();
        VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void copyData(const void *data, size_t size);
        void unmap();

        void setupDescriptorBufferInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        VkDescriptorBufferInfo& getDescriptorBufferInfo();

        VkResult bind(VkDeviceSize offset = 0);

        VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void copyFromStagingBuffer(VkBuffer srcBuffer, VkDeviceSize size, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);

        static Ref<VulkanBuffer> create(const char *name,
            VkBufferUsageFlags usageFlags,
            VkMemoryPropertyFlags memoryPropertyFlags,
            VMAUsageFlags vmaFlags,
            VkDeviceSize size,
            void *data = nullptr);  // Copy data

        static Ref<VulkanBuffer> create2(const char *name,
            VkBufferUsageFlags usageFlags,
            VkMemoryPropertyFlags memoryPropertyFlags,
            VmaAllocationCreateFlags vmaFlags,
            VkDeviceSize size,
            void *data = nullptr);  // Copy data

        static Ref<VulkanBuffer> createRTScratchBuffer(const char *name, VkDeviceSize size);
    };

    // Image
    struct VulkanImage
    {
    private:
        std::string m_name{};
        VkImage m_image = VK_NULL_HANDLE;
        VkImageView m_imageView = VK_NULL_HANDLE;
        VmaAllocation m_allocation = nullptr;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;
        VkDeviceSize m_size = 0;
        VkImageCreateInfo m_createInfo{};

        std::vector<uint32_t> m_ownerQueueFamilies;
        std::vector<VkImageLayout> m_layouts;
        std::unordered_map<size_t, VkImageView> m_cacheImageViews{};

        bool m_isHeap = false;

    public:
        [[nodiscard]] const VkImage& getImage() const { return m_image; }
        [[nodiscard]] VkFormat getFormat() const { return m_createInfo.format; }
        [[nodiscard]] VkExtent3D getExtent() const { return m_createInfo.extent; }
        [[nodiscard]] const VkImageCreateInfo& getInfo() const { return m_createInfo; }
        [[nodiscard]] VkDeviceSize getMemorySize() const { return m_size; }
        [[nodiscard]] bool isHeap() const { return m_isHeap; }

        bool innerCreate(VkMemoryPropertyFlags propertyFlags);

        void release();

        [[nodiscard]] size_t getSubresourceIndex(uint32_t layerIndex, uint32_t mipLevel) const
        {
            VT_CORE_ASSERT((layerIndex < m_createInfo.arrayLayers) && (mipLevel < m_createInfo.mipLevels), "Fail to get subresource index");
            return layerIndex * m_createInfo.mipLevels + mipLevel;
        }

        [[nodiscard]] VkImageLayout getCurrentLayout(uint32_t layerIndex, uint32_t mipLevel) const
        {
            return m_layouts.at(getSubresourceIndex(layerIndex, mipLevel));
        }

        void setName(const std::string &newName);

        // Get view, and try to create if no exist
        void createView(VkImageSubresourceRange range, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);
        VkImageView getView(VkImageSubresourceRange range = VkImageSubresourceRange{}, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);

        // Use graphics queue family
        void transitionLayout(VkCommandBuffer commandBuffer,
            VkImageLayout oldLayout,
            VkImageLayout newLayout,
            VkImageSubresourceRange range);
        
        void transitionLayout(VkCommandBuffer commandBuffer,
            uint32_t newQueueFamily,
            VkImageLayout oldLayout,
            VkImageLayout newLayout,
            VkImageSubresourceRange range);

        // Transition on major graphics
        void transitionLayoutImmediately(VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange range);

        // Copy buffer to image
        void copyFromStagingBuffer(VkBuffer stagingBuffer, uint32_t width, uint32_t height);

        // Generate mipmap
        void generateMipmaps();

        static Ref<VulkanImage> create(const char *name,
            const VkImageCreateInfo &createInfo,
            VkMemoryPropertyFlags propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    };
}


















