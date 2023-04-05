//
// Created by ZHIKANG on 2023/4/3.
//

#pragma once

#include <VulkanToy/VulkanRHI/CommandBuffer.h>
#include <VulkanToy/VulkanRHI/GPUResource.h>


namespace VT
{
    // Static uploader: allocate static stage buffer and never release
    // Dynamic uploader: allocate dynamic stage buffer when needed, and release when no task
    constexpr size_t GStaticUploaderMaxSize = 64 * 1024 * 1024;     // 64 MB
    constexpr size_t GDynamicUploaderMinSize = 32 * 1024 * 1024;    // 32 MB

    struct AssetLoadTask
    {
        // When load task finish call
        virtual void finishCallback() = 0;

        // Load task need stage buffer size
        virtual uint32_t getUploadSize() const = 0;

        // Upload main body function
        virtual void uploadDevice(uint32_t stageBufferOffset,
                        void *mapped,
                        CommandBufferBase &commandBuffer,
                        VulkanBuffer &stageBuffer) = 0;
    };
}
