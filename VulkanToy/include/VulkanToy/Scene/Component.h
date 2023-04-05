//
// Created by ZHIKANG on 2023/4/3.
//

#pragma once

#include <VulkanToy/Core/RuntimeModule.h>

namespace VT
{
    struct Component
    {
        virtual void init() {}
        virtual void tick(const RuntimeModuleTickData &tickData) {};
        virtual void onRenderTick(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout) {}
        virtual void release() {}
    };
}
