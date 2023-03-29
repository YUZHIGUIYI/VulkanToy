//
// Created by ZHIKANG on 2023/3/29.
//

#pragma once

#include <VulkanToy/Core/Base.h>

namespace VT
{
    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR        capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;
    };
}
