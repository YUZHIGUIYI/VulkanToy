//
// Created by ZHIKANG on 2023/3/31.
//

#include <VulkanToy/VulkanRHI/GPUResource.h>
#include <VulkanToy/VulkanRHI/VulkanRHI.h>

namespace VT
{
    // If size <= 128 MB, use VMA, else use heap memory
    constexpr VkDeviceSize GMaxVMASize = 128 * 1024 * 1024;
    constexpr bool canUseVMA(VkDeviceSize size)
    {
        return size <= GMaxVMASize;
    }

    // For vulkan buffer
    void VulkanBuffer::setName(const std::string &newName)
    {
        if (newName != m_name)
        {
            m_name = newName;
            VulkanRHI::setResourceName(VK_OBJECT_TYPE_BUFFER, (uint64_t)m_buffer, newName.c_str());
        }
    }

    bool VulkanBuffer::innerCreate(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags,
                                    VmaAllocationCreateFlags vmaUsage, void *data)
    {
        VT_CORE_ASSERT(m_size > 0, "Must set size of buffer before creating buffer");

        VkBufferCreateInfo bufferCreateInfo{};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = m_size;
        bufferCreateInfo.usage = usageFlags;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (!m_isHeap)
        {
            VmaAllocationCreateInfo vmaAllocationInfo{};
            vmaAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
            vmaAllocationInfo.flags = vmaUsage;
            RHICheck(vmaCreateBuffer(VulkanRHI::VMA, &bufferCreateInfo, &vmaAllocationInfo,
                                        &m_buffer, &m_allocation, nullptr));
            if (data != nullptr)
            {
                void *mapped;
                vmaMapMemory(VulkanRHI::VMA, m_allocation, &mapped);
                std::memcpy(mapped, data, m_size);
                vmaUnmapMemory(VulkanRHI::VMA, m_allocation);
            }
        } else
        {
            if (vkCreateBuffer(VulkanRHI::Device, &bufferCreateInfo, nullptr, &m_buffer) != VK_SUCCESS)
            {
                VT_CORE_CRITICAL("Fail to create vulkan buffer");
            }

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(VulkanRHI::Device, m_buffer, &memRequirements);

            VkMemoryAllocateInfo memAllocateInfo;
            memAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memAllocateInfo.allocationSize = memRequirements.size;
            memAllocateInfo.memoryTypeIndex = VulkanRHI::get()->findMemoryType(memRequirements.memoryTypeBits, memoryPropertyFlags);

            if (vkAllocateMemory(VulkanRHI::Device, &memAllocateInfo, nullptr, &m_memory) != VK_SUCCESS)
            {
                VT_CORE_CRITICAL("Fail to allocate vulkan buffer memory");
            }

            m_size = memRequirements.size;

            if (data != nullptr)
            {
                void *mapped;
                vkMapMemory(VulkanRHI::Device, m_memory, 0, m_size, 0, &mapped);
                std::memcpy(mapped, data, m_size);
                vkUnmapMemory(VulkanRHI::Device, m_memory);
            }

            vkBindBufferMemory(VulkanRHI::Device, m_buffer, m_memory, 0);
        }

        // Initialize a default descriptor that covers the whole buffer size
        setupDescriptorBufferInfo();

        VulkanRHI::setResourceName(VK_OBJECT_TYPE_BUFFER, (uint64_t)m_buffer, m_name.c_str());
        VulkanRHI::addGPUResourceMemoryUsed(m_size);

        return true;
    }

    void VulkanBuffer::release()
    {
        if (!m_isHeap)
        {
            vmaDestroyBuffer(VulkanRHI::VMA, m_buffer, m_allocation);
        } else
        {
            if (m_buffer != VK_NULL_HANDLE)
            {
                vkDestroyBuffer(VulkanRHI::Device, m_buffer, nullptr);
            }
            if (m_memory != VK_NULL_HANDLE)
            {
                vkFreeMemory(VulkanRHI::Device, m_memory, nullptr);
            }
        }
    }

    uint64_t VulkanBuffer::getDeviceAddress()
    {
        if (m_deviceAddress == 0)
        {
            VkBufferDeviceAddressInfo bufferDeviceAddressInfo{};
            bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            bufferDeviceAddressInfo.buffer = m_buffer;
            m_deviceAddress = vkGetBufferDeviceAddress(VulkanRHI::Device, &bufferDeviceAddressInfo);
        }
        return m_deviceAddress;
    }

    VkResult VulkanBuffer::map(VkDeviceSize size, VkDeviceSize offset)
    {
        VkResult result;
        if (!m_isHeap)
        {
            result = vmaMapMemory(VulkanRHI::VMA, m_allocation, &m_mapped);
        } else
        {
            result = vkMapMemory(VulkanRHI::Device, m_memory, offset, m_size, 0, &m_mapped);    // TODO: fix
        }
        return result;
    }

    void VulkanBuffer::copyData(const void *data, size_t size)
    {
        VT_CORE_ASSERT(m_mapped != nullptr, "Must map buffer before copying data");
        std::memcpy(m_mapped, data, size);
    }

    void VulkanBuffer::unmap()
    {
        VT_CORE_ASSERT(m_mapped != nullptr, "Must call unmap after calling map");

        if (!m_isHeap)
        {
            vmaUnmapMemory(VulkanRHI::VMA, m_allocation);
            m_mapped = nullptr;
        } else
        {
            vkUnmapMemory(VulkanRHI::Device, m_memory);
            m_mapped = nullptr;
        }
    }

    void VulkanBuffer::setupDescriptorBufferInfo(VkDeviceSize size, VkDeviceSize offset)
    {
        m_bufferInfo.offset = offset;
        m_bufferInfo.buffer = m_buffer;
        m_bufferInfo.range = size;
    }

    VkDescriptorBufferInfo& VulkanBuffer::getDescriptorBufferInfo()
    {
        if (m_buffer != VK_NULL_HANDLE && m_bufferInfo.buffer == VK_NULL_HANDLE)
        {
            setupDescriptorBufferInfo(m_size, 0);
        }
        return m_bufferInfo;
    }

    VkResult VulkanBuffer::bind(VkDeviceSize offset)
    {
        if (!m_isHeap)
        {
            return vmaBindBufferMemory2(VulkanRHI::VMA, m_allocation, offset, m_buffer, nullptr);
        } else
        {
            return vkBindBufferMemory(VulkanRHI::Device, m_buffer, m_memory, offset);
        }
    }

    VkResult VulkanBuffer::flush(VkDeviceSize size, VkDeviceSize offset)
    {
        VkResult result;
        if (!m_isHeap)
        {
            result = vmaFlushAllocation(VulkanRHI::VMA, m_allocation, offset, size);
        } else
        {
            VkMappedMemoryRange mappedRange{};
            mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            mappedRange.memory = m_memory;
            mappedRange.offset = offset;
            mappedRange.size = size;
            result = vkFlushMappedMemoryRanges(VulkanRHI::Device, 1, &mappedRange);
        }
        return result;
    }

    VkResult VulkanBuffer::invalidate(VkDeviceSize size, VkDeviceSize offset)
    {
        VkResult result;
        if (!m_isHeap)
        {
            result = vmaInvalidateAllocation(VulkanRHI::VMA, m_allocation, offset, size);
        } else
        {
            VkMappedMemoryRange mappedRange{};
            mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            mappedRange.memory = m_memory;
            mappedRange.offset = offset;
            mappedRange.size = size;
            result = vkInvalidateMappedMemoryRanges(VulkanRHI::Device, 1, &mappedRange);
        }
        return result;
    }

    void VulkanBuffer::copyFromStagingBuffer(VkBuffer srcBuffer, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset)
    {
        VulkanRHI::executeImmediatelyMajorGraphics([this, srcBuffer, size, srcOffset, dstOffset](VkCommandBuffer cmb){
            VkBufferCopy copyRegion{};
            copyRegion.srcOffset = srcOffset;
            copyRegion.dstOffset = dstOffset;
            copyRegion.size = size;
            vkCmdCopyBuffer(cmb, srcBuffer, m_buffer, 1, &copyRegion);
        });
    }

    Ref<VulkanBuffer> VulkanBuffer::create(const char *name, VkBufferUsageFlags usageFlags,
                                            VkMemoryPropertyFlags memoryPropertyFlags, VMAUsageFlags vmaFlags,
                                            VkDeviceSize size, void *data)
    {
        auto vulkanBuffer = VT::CreateRef<VulkanBuffer>();

        vulkanBuffer->m_isHeap = !canUseVMA(size);
        vulkanBuffer->m_size = size;
        vulkanBuffer->m_name = name;

        VmaAllocationCreateFlags vmaUsage{};
        if (vmaFlags == VMAUsageFlags::StageCopyForUpload)
        {
            vmaUsage = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        } else if (vmaFlags == VMAUsageFlags::ReadBack)
        {
            vmaUsage = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        }

        vulkanBuffer->innerCreate(usageFlags, memoryPropertyFlags, vmaUsage, data);
        return vulkanBuffer;
    }

    Ref<VulkanBuffer> VulkanBuffer::create2(const char *name, VkBufferUsageFlags usageFlags,
                                            VkMemoryPropertyFlags memoryPropertyFlags, VmaAllocationCreateFlags vmaFlags,
                                            VkDeviceSize size, void *data)
    {
        auto vulkanBuffer = VT::CreateRef<VulkanBuffer>();

        vulkanBuffer->m_isHeap = !canUseVMA(size);
        vulkanBuffer->m_size = size;
        vulkanBuffer->m_name = name;
        vulkanBuffer->innerCreate(usageFlags, memoryPropertyFlags, vmaFlags, data);

        return vulkanBuffer;
    }

    Ref<VulkanBuffer> VulkanBuffer::createRTScratchBuffer(const char *name, VkDeviceSize size)
    {
        return create(name, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMAUsageFlags::GPUOnly, size, nullptr);
    }

    // For Vulkan image
    bool VulkanImage::innerCreate(VkMemoryPropertyFlags propertyFlags)
    {
        RHICheck(vkCreateImage(VulkanRHI::Device, &m_createInfo, nullptr, &m_image));

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(VulkanRHI::Device, m_image, &memRequirements);
        m_size = memRequirements.size;
        m_isHeap = !canUseVMA(m_size);

        vkDestroyImage(VulkanRHI::Device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;

        if (!m_isHeap)
        {
            VmaAllocationCreateInfo imageAllocateInfo{};
            imageAllocateInfo.usage = VMA_MEMORY_USAGE_AUTO;
            imageAllocateInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
            imageAllocateInfo.pUserData = (void *)m_name.c_str();
            VmaAllocationInfo gpuImageAllocateInfo{};

            RHICheck(vmaCreateImage(VulkanRHI::VMA, &m_createInfo, &imageAllocateInfo, &m_image, &m_allocation, &gpuImageAllocateInfo));
        } else
        {
            RHICheck(vkCreateImage(VulkanRHI::Device, &m_createInfo, nullptr, &m_image));

            VkMemoryAllocateInfo allocateInfo{};
            allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocateInfo.allocationSize = m_size;
            allocateInfo.memoryTypeIndex = VulkanRHI::get()->findMemoryType(memRequirements.memoryTypeBits, propertyFlags);
            RHICheck(vkAllocateMemory(VulkanRHI::Device, &allocateInfo, nullptr, &m_memory));

            vkBindImageMemory(VulkanRHI::Device, m_image, m_memory, 0);
        }
        VulkanRHI::setResourceName(VK_OBJECT_TYPE_IMAGE, (uint64_t)m_image, m_name.c_str());
        VulkanRHI::addGPUResourceMemoryUsed(m_size);
        VT_CORE_INFO("Image {0} has been created", m_name);

        return true;
    }

    void VulkanImage::release()
    {
        VT_CORE_ASSERT(m_image != VK_NULL_HANDLE, "Image is null handle");
        if (m_allocation != nullptr)
        {
            vmaDestroyImage(VulkanRHI::VMA, m_image, m_allocation);
            m_image = VK_NULL_HANDLE;
        } else
        {
            vkDestroyImage(VulkanRHI::Device, m_image, nullptr);
            m_image = VK_NULL_HANDLE;
            VT_CORE_ASSERT(m_memory != VK_NULL_HANDLE, "Image memory is null handle");
            vkFreeMemory(VulkanRHI::Device, m_memory, nullptr);
            m_memory = VK_NULL_HANDLE;
        }

        if (m_imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(VulkanRHI::Device, m_imageView, nullptr);
        }

        VulkanRHI::minusGPUResourceMemoryUsed(m_size);
        VT_CORE_INFO("Image {0} has been released", m_name);
    }

    void VulkanImage::setName(const std::string &newName)
    {
        // TODO
    }

    Ref<VulkanImage> VulkanImage::create(const char *name, const VkImageCreateInfo &createInfo,
                                            VkMemoryPropertyFlags propertyFlags)
    {
        auto vulkanImage = VT::CreateRef<VulkanImage>();

        vulkanImage->m_name = name;
        vulkanImage->m_createInfo = createInfo;
        vulkanImage->innerCreate(propertyFlags);

        return vulkanImage;
    }

    void VulkanImage::createView(VkImageSubresourceRange range, VkImageViewType viewType)
    {
        VkImageViewCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image = m_image;
        info.subresourceRange = range;
        info.format = m_createInfo.format;
        info.viewType = viewType;

        RHICheck(vkCreateImageView(VulkanRHI::Device, &info, nullptr, &m_imageView));
    }

    // Try to get view and create if it is exist
    VkImageView VulkanImage::getView(VkImageSubresourceRange range, VkImageViewType viewType)
    {
        if (m_imageView == VK_NULL_HANDLE)
        {
            createView(range, viewType);
        }
        return m_imageView;
    }

    void VulkanImage::transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange range)
    {
        transitionLayout(commandBuffer, VulkanRHI::get()->getGraphicsFamily(), oldLayout, newLayout, range);
    }

    void VulkanImage::transitionLayout(VkCommandBuffer commandBuffer, uint32_t newQueueFamily,
                                        VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange range)
    {
        // new
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_image;

        barrier.subresourceRange = range;

        VkPipelineStageFlags srcStage;
        VkPipelineStageFlags dstStage;
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        } else
        {
            VT_CORE_CRITICAL("Unsupported image layout transition");
        }

        vkCmdPipelineBarrier(
                commandBuffer,
                srcStage,
                dstStage,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &barrier);
    }

    void VulkanImage::transitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccessMask,
                                        VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStageMask,
                                        VkPipelineStageFlags dstStageMask, VkImageSubresourceRange range)
    {
        VulkanRHI::executeImmediatelyMajorGraphics([oldLayout, newLayout, srcAccessMask, dstAccessMask, srcStageMask, dstStageMask, range, this] (VkCommandBuffer cmd) {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = m_image;
            barrier.subresourceRange = range;
            barrier.srcAccessMask = srcAccessMask;
            barrier.dstAccessMask = dstAccessMask;

            vkCmdPipelineBarrier(
                    cmd,
                    srcStageMask,
                    dstStageMask,
                    0,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &barrier);
        });
    }

    // TODO: combine two functions
    void VulkanImage::transitionLayoutImmediately(VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange range)
    {
        VulkanRHI::executeImmediatelyMajorGraphics([oldLayout, newLayout, range, this] (VkCommandBuffer cmd) {
            transitionLayout(cmd, VulkanRHI::get()->getGraphicsFamily(), oldLayout, newLayout, range);
        });
    }

    void VulkanImage::transitionImageLayout(const ImageMemoryBarrier &imageMemoryBarrier, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
    {
        VulkanRHI::executeImmediatelyMajorGraphics([&imageMemoryBarrier, srcStageMask, dstStageMask] (VkCommandBuffer cmd)
        {
            vkCmdPipelineBarrier(cmd, srcStageMask, dstStageMask, 0, 0, nullptr,
                                    0, nullptr, 1, &imageMemoryBarrier.barrier);
        });
    }

    void VulkanImage::copyFromStagingBuffer(VkBuffer stagingBuffer, uint32_t width, uint32_t height)
    {
        VulkanRHI::executeImmediatelyMajorGraphics([stagingBuffer, width, height, this] (VkCommandBuffer cmd) {
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = { width, height, 1 };

            vkCmdCopyBufferToImage(cmd,
                stagingBuffer,
                m_image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region);
        });
    }

    // Generate mipmaps
    void VulkanImage::generateMipmaps()
    {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(VulkanRHI::GPU, m_createInfo.format, &formatProperties);
        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        {
            VT_CORE_CRITICAL("Image format does not support linear blitting");
        }

        VulkanRHI::executeImmediatelyMajorGraphics([this] (VkCommandBuffer cmd) {
            auto mipWidth = static_cast<int32_t>(m_createInfo.extent.width);
            auto mipHeight = static_cast<int32_t>(m_createInfo.extent.height);
            for (uint32_t level = 1; level < m_createInfo.mipLevels; ++level, mipWidth /= 2, mipHeight /= 2)
            {
                const auto preBlitBarrier = ImageMemoryBarrier(m_image, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
                                                                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL).mipLevels(level, 1);
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
                                        0, nullptr, 1, &preBlitBarrier.barrier);

                VkImageBlit region{};
                region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, level - 1, 0, m_createInfo.arrayLayers };
                region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, level, 0, m_createInfo.arrayLayers };
                region.srcOffsets[1] = { mipWidth, mipHeight, 1 };
                region.dstOffsets[1] = { int32_t(mipWidth / 2), int32_t(mipHeight / 2), 1 };
                vkCmdBlitImage(cmd, m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                1, &region, VK_FILTER_LINEAR);

                const auto postBlitBarrier = ImageMemoryBarrier(m_image, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                                                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL).mipLevels(level, 1);
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
                                        0, nullptr, 1, &postBlitBarrier.barrier);
            }
            // Transition whole mip chain to shader-read-only-layout
            const auto finalBarrier = ImageMemoryBarrier(m_image, VK_ACCESS_TRANSFER_WRITE_BIT, 0,
                                                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr,
                                    0, nullptr, 1, &finalBarrier.barrier);
        });
    }

    ImageMemoryBarrier::ImageMemoryBarrier(VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                                            VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = srcAccessMask;
        barrier.dstAccessMask = dstAccessMask;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    }

    ImageMemoryBarrier& ImageMemoryBarrier::aspectMask(VkImageAspectFlags aspectMask)
    {
        barrier.subresourceRange.aspectMask = aspectMask;
        return *this;
    }

    ImageMemoryBarrier& ImageMemoryBarrier::mipLevels(uint32_t baseMipLevel, uint32_t levelCount)
    {
        barrier.subresourceRange.baseMipLevel = baseMipLevel;
        barrier.subresourceRange.levelCount = levelCount;
        return *this;
    }

    ImageMemoryBarrier& ImageMemoryBarrier::arrayLayers(uint32_t baseArrayLayer, uint32_t layerCount)
    {
        barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
        barrier.subresourceRange.layerCount = layerCount;
        return *this;
    }
}









































