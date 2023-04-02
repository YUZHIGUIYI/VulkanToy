//
// Created by ZHIKANG on 2023/4/2.
//

#pragma once

#include <VulkanToy/Core/Base.h>

namespace VT
{
    // Collect of command buffer
    struct CommandBufferBase
    {
        VkCommandBuffer cmd;
        VkCommandPool pool;
        uint32_t queueFamily;
    };
}
