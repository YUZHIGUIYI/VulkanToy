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
        setupDescriptor();

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
            result = vkMapMemory(VulkanRHI::Device, m_memory, offset, size, 0, &m_mapped);
        }
        //VT_CORE_ASSERT(m_mapped != nullptr, "Fail to map buffer memory");
        return result;
    }

    void VulkanBuffer::copyTo(const void *data, VkDeviceSize size)
    {
        VT_CORE_ASSERT(m_mapped != nullptr, "Must map buffer before copying");
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

    void VulkanBuffer::setupDescriptor(VkDeviceSize size, VkDeviceSize offset)
    {
        m_bufferInfo.offset = offset;
        m_bufferInfo.buffer = m_buffer;
        m_bufferInfo.range = size;
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

    void VulkanBuffer::stageCopyFrom(VkBuffer inBuffer, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset)
    {
        VulkanRHI::executeImmediatelyMajorGraphics([this, &inBuffer, size, srcOffset, dstOffset](VkCommandBuffer cmb){
            VkBufferCopy copyRegion{};
            copyRegion.srcOffset = srcOffset;
            copyRegion.dstOffset = dstOffset;
            copyRegion.size = size;
            vkCmdCopyBuffer(cmb, inBuffer, m_buffer, 1, &copyRegion);
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

    void VulkanImage::setName(const std::string &newName)
    {

    }

    bool VulkanImage::innerCreate(VkMemoryPropertyFlags propertyFlags)
    {
        RHICheck(vkCreateImage(VulkanRHI::Device, &m_createInfo, nullptr, &m_image));

        m_layouts.resize(m_createInfo.arrayLayers * m_createInfo.mipLevels);
        m_ownerQueueFamilies.resize(m_layouts.size());
        for (size_t i = 0; i < m_layouts.size(); ++i)
        {
            m_layouts[i] = VK_IMAGE_LAYOUT_UNDEFINED;
            m_ownerQueueFamilies[i] = VK_QUEUE_FAMILY_IGNORED;
        }

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

            VkMemoryPropertyFlags properties = propertyFlags;
            allocateInfo.memoryTypeIndex = VulkanRHI::get()->findMemoryType(memRequirements.memoryTypeBits, properties);
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

        for (auto& pair : m_cacheImageViews)
        {
            VT_CORE_ASSERT(pair.second != VK_NULL_HANDLE);
            vkDestroyImageView(VulkanRHI::Device, pair.second, nullptr);
        }
        m_cacheImageViews.clear();
        VulkanRHI::minusGPUResourceMemoryUsed(m_size);
        VT_CORE_INFO("Image {0} has been released", m_name);
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

    // Try to get view and create if it is exist
    VkImageView VulkanImage::getView(VkImageSubresourceRange range, VkImageViewType viewType)
    {
        VkImageViewCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image = m_image;
        info.subresourceRange = range;
        info.format = m_createInfo.format;
        info.viewType = viewType;

        // TODO: hash fix and unordered_map
        VkImageView imageView;
        RHICheck(vkCreateImageView(VulkanRHI::Device, &info, nullptr, &imageView));

        m_imageView = imageView;

        return imageView;
    }

    void VulkanImage::transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout, VkImageSubresourceRange range)
    {
        transitionLayout(commandBuffer, VulkanRHI::get()->getGraphicsFamily(), newLayout, range);
    }

    void VulkanImage::transitionLayout(VkCommandBuffer commandBuffer, uint32_t newQueueFamily,
                                        VkImageLayout newLayout, VkImageSubresourceRange range)
    {
        std::vector<VkImageMemoryBarrier> barriers;

        VkDependencyFlags dependencyFlags{};
        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        uint32_t maxLayer = glm::min(range.baseArrayLayer + range.layerCount, m_createInfo.arrayLayers);
        for (uint32_t layerIndex = range.baseArrayLayer; layerIndex < maxLayer; ++layerIndex)
        {
            uint32_t maxMip = glm::min(range.baseMipLevel + range.levelCount, m_createInfo.mipLevels);
            for (uint32_t mipIndex = range.baseMipLevel; mipIndex < maxMip; ++mipIndex)
            {
                size_t flatId = getSubresourceIndex(layerIndex, mipIndex);

                VkImageLayout oldLayout = m_layouts.at(flatId);
                uint32_t oldFamily = m_ownerQueueFamilies.at(flatId);

                if ((newLayout == oldLayout) && (oldFamily == newQueueFamily))
                {
                    continue;
                }

                m_layouts[flatId] = newLayout;
                m_ownerQueueFamilies[flatId] = newQueueFamily;

                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = oldLayout;
                barrier.newLayout = newLayout;
                barrier.srcQueueFamilyIndex = (oldFamily == VK_QUEUE_FAMILY_IGNORED) ? newQueueFamily : oldFamily;
                barrier.dstQueueFamilyIndex = newQueueFamily;
                barrier.image = m_image;

                VkImageSubresourceRange rangSpecial{
                    .aspectMask = range.aspectMask,
                    .baseMipLevel = mipIndex,
                    .levelCount = 1,
                    .baseArrayLayer = layerIndex,
                    .layerCount = 1,
                };
                barrier.subresourceRange = rangSpecial;

                VkAccessFlags srcMask{};
                VkAccessFlags dstMask{};

                switch (oldLayout)
                {
                    case VK_IMAGE_LAYOUT_UNDEFINED:
                        srcMask = 0;
                        break;
                    case VK_IMAGE_LAYOUT_PREINITIALIZED:
                        srcMask = VK_ACCESS_HOST_WRITE_BIT;
                        break;
                    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                        srcMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                        break;
                    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                        srcMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                        break;
                    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                        srcMask = VK_ACCESS_TRANSFER_READ_BIT;
                        break;
                    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                        srcMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                        break;
                    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                        srcMask = VK_ACCESS_SHADER_READ_BIT;
                        break;
                    case VK_IMAGE_LAYOUT_GENERAL:
                        srcMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                        break;
                    default:
                        VT_CORE_CRITICAL("Image layout transition does not support");
                        srcMask = ~0;
                        break;
                }

                switch (newLayout)
                {
                    case VK_IMAGE_LAYOUT_GENERAL:
                        dstMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                        break;
                    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                        dstMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                        break;
                    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                        dstMask = VK_ACCESS_TRANSFER_READ_BIT;
                        break;
                    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                        dstMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                        break;
                    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                        dstMask = dstMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                        break;
                    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                        if (srcMask == 0)
                        {
                            srcMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                        }
                        dstMask = VK_ACCESS_SHADER_READ_BIT;
                        break;
                    default:
                        VT_CORE_CRITICAL("Image layout transition does no support");
                        dstMask = ~0;
                        break;
                }

                barrier.srcAccessMask = srcMask;
                barrier.dstAccessMask = dstMask;
                barriers.push_back(barrier);
            }
        }

        if (barriers.empty())
        {
            return;
        }

        vkCmdPipelineBarrier(
                commandBuffer,
                srcStageMask,
                dstStageMask,
                dependencyFlags,
                0,
                nullptr,
                0,
                nullptr,
                static_cast<uint32_t>(barriers.size()),
                barriers.data());
    }

    void VulkanImage::transitionLayoutImmediately(VkImageLayout newLayout, VkImageSubresourceRange range)
    {
        VulkanRHI::executeImmediatelyMajorGraphics([&newLayout, range, this] (VkCommandBuffer cmb) {
            transitionLayout(cmb, VulkanRHI::get()->getGraphicsFamily(), newLayout, range);
        });
    }
}